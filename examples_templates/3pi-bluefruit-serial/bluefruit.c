/**
 * Use an Adafruit Bluefruit LE UART Friend module to control a 3pi.
 * https://www.adafruit.com/product/2479
 */
#include <pololu/3pi.h>
#include <ctype.h>
#include<stdio.h>

// including ctype for `isdigit`

/*
 * serial1: for the Orangutan LV, SV, SVP, X2, Baby-O and 3pi robot.
 *
 * This example listens for bytes on PD0/RXD.  Whenever it receives a byte, it
 * performs a custom action.  Whenever the user presses the middle button, it
 * transmits a greeting on PD1/TXD.
 *
 * The Baby Orangutan does not have a green LED, LCD, or pushbuttons so
 * that part of the code will not work.
 *
 * To make this example compile for the Orangutan SVP or X2, you
 * must add a first argument of UART0 to all the serial_* function calls
 * to receive on PD0/RXD0 and transmit on PD1/TXD0.
 *
 * http://www.pololu.com/docs/0J20
 * http://www.pololu.com
 * http://forum.pololu.com
 */

// receive_buffer: A ring buffer that we will use to receive bytes on PD0/RXD.
// The OrangutanSerial library will put received bytes in to
// the buffer starting at the beginning (receiveBuffer[0]).
// After the buffer has been filled, the library will automatically
// start over at the beginning.
char receive_buffer[32];

// receive_buffer_position: This variable will keep track of which bytes in the receive buffer
// we have already processed.  It is the offset (0-31) of the next byte
// in the buffer to process.
unsigned char receive_buffer_position = 0;

// send_buffer: A buffer for sending bytes on PD1/TXD.
char send_buffer[32];

// wait_for_sending_to_finish:  Waits for the bytes in the send buffer to
// finish transmitting on PD1/TXD.  We must call this before modifying
// send_buffer or trying to send more bytes, because otherwise we could
// corrupt an existing transmission.
void wait_for_sending_to_finish() {
    while (!serial_send_buffer_empty()) {}
}

// Sends the version of the slave code that is running.
// This function also shuts down the motors, so it is
// useful as an initial command.
void send_signature() {
    print("3pi");
    set_motors(0, 0);
    serial_send_blocking("3pi", 3);
}

// Sends the batter voltage in millivolts
void send_battery_millivolts() {
    int message[1];
    message[0] = read_battery_millivolts();
    serial_send_blocking((char *)message, 2);
}

// Displays data to the screen
void do_print() {
    print("TODO");
    //print_character(character);
}

// Drives m1 forward.
void m1_forward() {
    set_m1_speed(50);
}

// Drives m2 forward.
void m2_forward() {
    set_m2_speed(50);
}

// Drives m1 backward.
void m1_backward() {
    set_m1_speed(50);
}

// Drives m2 backward.
void m2_backward() {
    set_m2_speed(50);
}

int drive_command_offset = -1;
int forwards = 1;
char drive_command_buffer[2];
// process_received_byte: Responds to a byte that has been received on PD0/RXD.
void process_received_char(char byte) {
    switch (byte) {
        // If the character 'c' is received, play the note c.
        case 'c':
            play_from_program_space(PSTR("c16"));
            break;
        // If the character 'd' is received, play the note d.
        case 'd':
            play_from_program_space(PSTR("d16"));
            break;
        case 'g':
            send_signature();
            break;
        case 'b':
            send_battery_millivolts();
            break;
        case 'r':
            clear();
            break;
        case 'p':
            do_print();
            break;
        case 's':
            // stop driving
            set_motors(0, 0);
            break;
        case 'u':
            clear();
            print("back");
            forwards = 0;
            drive_command_offset = 0;
            break;
        case 'v':
            // flag to read the next two bytes as wheel speeds to set
            clear();
            print("drive");
            forwards = 1;
            drive_command_offset = 0;
            break;
        case 'w':
            m1_forward();
            break;
        case 'x':
            m1_backward();
            break;
        case 'y':
            m2_forward();
            break;
        case 'z':
            m2_backward();
            break;

        // If any other character is received, change its capitalization and
        // send it back.
        default:
            wait_for_sending_to_finish();
            send_buffer[0] = byte ^ 0x20;
            serial_send(send_buffer, 1);
            break;
    }
}

void process_received_int(int byte) {
    if (drive_command_offset > 1) {
        // index unexpectedly high
        clear();
        print("error");
        drive_command_offset = -1;
    }
    drive_command_buffer[drive_command_offset] = byte;
    if (drive_command_offset == 1) {
        // execute command and reset reading
        clear();
        print("go");
        print(" ");
        char s1[4];
        char s2[4];
        sprintf(s1, "%d", drive_command_buffer[0]);
        sprintf(s2, "%d", drive_command_buffer[1]);
        print(s1);
        print(" ");
        print(s2);
        if (forwards == 1) {
            set_motors(drive_command_buffer[0], drive_command_buffer[1]);
        } else {
            // drive backwards
            set_motors(-drive_command_buffer[0], -drive_command_buffer[1]);
        }
        drive_command_offset = -1;
    } else {
        print(" 1");
        drive_command_offset += 1;
    }
    return;
}

void check_for_new_bytes_received() {
    while (serial_get_received_bytes() != receive_buffer_position) {
        // Process the new byte that has just been received.

        if (drive_command_offset != -1) {
            process_received_int(receive_buffer[receive_buffer_position]);
        } else {
            process_received_char(receive_buffer[receive_buffer_position]);
        }

        // Increment receive_buffer_position, but wrap around when it gets to
        // the end of the buffer.
        if (receive_buffer_position == sizeof(receive_buffer) - 1) {
            receive_buffer_position = 0;
        } else {
            receive_buffer_position++;
        }
    }
}

int main() {
    // Set the baud rate to 9600 bits per second.  Each byte takes ten bit
    // times, so you can get at most 960 bytes per second at this speed.
    serial_set_baud_rate(9600);
    print("Howdy");

    // Start receiving bytes in the ring buffer.
    serial_receive_ring(receive_buffer, sizeof(receive_buffer));

    while (1) {
        // Deal with any new bytes received.
        check_for_new_bytes_received();

        // If the user presses the middle button, stop driving,
        // send "Hi there!", and wait until the user releases the button.
        if (button_is_pressed(MIDDLE_BUTTON)) {
            set_motors(0, 0);
            wait_for_sending_to_finish();
            memcpy_P(send_buffer, PSTR("Hi there!\r\n"), 11);
            serial_send(send_buffer, 11);

            // Wait for the user to release the button.  While the processor is
            // waiting, the OrangutanSerial library will take care of receiving
            // bytes using the serial reception interrupt.  But if enough bytes
            // arrive during this period to fill up the receive_buffer, then the
            // older bytes will be lost and we won't know exactly how many bytes
            // have been received.
            wait_for_button_release(MIDDLE_BUTTON);
        }
    }
}
