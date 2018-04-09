// Donald Peeke-Vout
// keypad.h
// .h file for keypad class

#include "agb.h"

#ifndef KEYPAD_H
#define KEYPAD_H

#define KEY_A 1
#define KEY_B 2
#define KEY_SELECT 4
#define KEY_START 8
#define KEY_RIGHT 16
#define KEY_LEFT 32
#define KEY_UP 64
#define KEY_DOWN 128
#define KEY_R 256
#define KEY_L 512

#define KEYS (volatile u32*)0x04000130

class keypad {
 public:
  keypad();
  u8 press(u32 key);
  u8 hold(u32 key);
  u8 release(u32 key);
  void update();

 private:
  u32 lastKey;
  u32 newKey;
};

#endif

