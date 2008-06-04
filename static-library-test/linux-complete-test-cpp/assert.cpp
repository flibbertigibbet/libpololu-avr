#include <pololu/Orangutan.h>
#include "assert.h"

void assert(unsigned char truth)
{
  if(!truth)
  {
    OrangutanBuzzer::play("O3c");
    OrangutanPushbuttons::waitForButton(ALL_BUTTONS);
  }
}
