// Donald Peeke-Vout
// keypad.cpp
// Implementation of keypad class

#include "agb.h"
#include "keypad.h"

keypad::keypad() {
  lastKey = *KEYS;
  newKey = *KEYS;
}

u8 keypad::press(u32 key) {
  if (((key) & lastKey) && !((key) & newKey))
    return 1;
  else
    return 0;
}

u8 keypad::hold(u32 key) {
  if (!((key) & newKey) && !((key) & lastKey))
    return 1;
  else
    return 0;
}

u8 keypad::release(u32 key) {
  if (!((key) & lastKey) && ((key) & newKey))
    return 1;
  else
    return 0;
}

void keypad::update() {
  lastKey = newKey;
  newKey = *KEYS;
}


