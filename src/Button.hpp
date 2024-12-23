#ifndef BUTTON_HPP
#define BUTTON_HPP

namespace Button
{
    const int POWER = 0x15;
    const int DISC = 0x4A;
    const int NUM_0 = 0x20;
    const int NUM_1 = 0x0;
    const int NUM_2 = 0x1;
    const int NUM_3 = 0x2;
    const int NUM_4 = 0x3;
    const int NUM_5 = 0x4;
    const int NUM_6 = 0x5;
    const int NUM_7 = 0x6;
    const int NUM_8 = 0x7;
    const int NUM_9 = 0x8;
    const int ENTER = 0xB;
    const int MEMO_INPUT = 0x69;
    const int AMS_RIGHT = 0x31;
    const int CLEAR = 0xF;
    // Time/Text/Caps are the same button but differ if alpha mode is engaged
    const int TIME_TEXT = 0x28;
    const int CAPS = 0x28;
}

#endif // BUTTON_HPP
