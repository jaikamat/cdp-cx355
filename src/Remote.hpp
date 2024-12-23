#ifndef REMOTE_HPP
#define REMOTE_HPP

#include <Arduino.h>
#include <IRremote.hpp>
#include <map>
#include "Button.hpp"

class Remote
{
public:
    enum Mode
    {
        Default,
        Alpha
    };

    enum CapsMode
    {
        Uppercase,
        Lowercase,
        Numbers
    };

    Mode currentMode = Default;
    CapsMode currentCapsMode = Uppercase;

    void setMode(Mode mode)
    {
        currentMode = mode;
    }

    void cycleCapsMode()
    {
        switch (currentCapsMode)
        {
        case Uppercase:
            currentCapsMode = Lowercase;
            Serial.println("Caps Mode: Lowercase");
            break;
        case Lowercase:
            currentCapsMode = Numbers;
            Serial.println("Caps Mode: Numbers");
            break;
        case Numbers:
            currentCapsMode = Uppercase;
            Serial.println("Caps Mode: Uppercase");
            break;
        }
        press(Button::CAPS); // Send the CAPS button signal
        delay(300);          // Allow time for the device to register the change
    }

    // Sometimes we need to hold down a button, specifically the CLEAR button. This satisfies that for now.
    void hold(int buttonCode)
    {
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
        press(buttonCode);
    }

    void press(int buttonCode)
    {
        Serial.print("Sending IR signal for code: ");
        Serial.println(buttonCode, HEX);
        IrSender.sendSony(0x11, buttonCode, 2, 12); // Send the IR signal
    }

    void sendNumber(int number)
    {
        std::map<char, int> digitToCode = {
            {'0', Button::NUM_0},
            {'1', Button::NUM_1},
            {'2', Button::NUM_2},
            {'3', Button::NUM_3},
            {'4', Button::NUM_4},
            {'5', Button::NUM_5},
            {'6', Button::NUM_6},
            {'7', Button::NUM_7},
            {'8', Button::NUM_8},
            {'9', Button::NUM_9},
        };

        std::string numString = std::to_string(number);

        for (char digit : numString)
        {
            auto it = digitToCode.find(digit);
            if (it != digitToCode.end())
            {
                press(it->second);
                delay(500);
            }
            else
            {
                Serial.print("Error: Invalid digit '");
                Serial.print(digit);
                Serial.println("'");
            }
        }
    }

    // TODO: Support spaces
    // TODO: Support punctuation
    void sendAlpha(std::string input)
    {
        std::map<char, std::pair<int, int>> alphaUppercaseMap = {
            {'A', {Button::NUM_2, 1}},
            {'B', {Button::NUM_2, 2}},
            {'C', {Button::NUM_2, 3}},
            {'D', {Button::NUM_3, 1}},
            {'E', {Button::NUM_3, 2}},
            {'F', {Button::NUM_3, 3}},
            {'G', {Button::NUM_4, 1}},
            {'H', {Button::NUM_4, 2}},
            {'I', {Button::NUM_4, 3}},
            {'J', {Button::NUM_5, 1}},
            {'K', {Button::NUM_5, 2}},
            {'L', {Button::NUM_5, 3}},
            {'M', {Button::NUM_6, 1}},
            {'N', {Button::NUM_6, 2}},
            {'O', {Button::NUM_6, 3}},
            {'P', {Button::NUM_7, 1}},
            {'R', {Button::NUM_7, 2}},
            {'S', {Button::NUM_7, 3}},
            {'T', {Button::NUM_8, 1}},
            {'U', {Button::NUM_8, 2}},
            {'V', {Button::NUM_8, 3}},
            {'W', {Button::NUM_9, 1}},
            {'X', {Button::NUM_9, 2}},
            {'Y', {Button::NUM_9, 3}},
            {'Q', {Button::NUM_0, 1}},
            {'Z', {Button::NUM_0, 2}},
        };

        std::map<char, std::pair<int, int>> alphaLowercaseMap = {
            {'a', {Button::NUM_2, 1}},
            {'b', {Button::NUM_2, 2}},
            {'c', {Button::NUM_2, 3}},
            {'d', {Button::NUM_3, 1}},
            {'e', {Button::NUM_3, 2}},
            {'f', {Button::NUM_3, 3}},
            {'g', {Button::NUM_4, 1}},
            {'h', {Button::NUM_4, 2}},
            {'i', {Button::NUM_4, 3}},
            {'j', {Button::NUM_5, 1}},
            {'k', {Button::NUM_5, 2}},
            {'l', {Button::NUM_5, 3}},
            {'m', {Button::NUM_6, 1}},
            {'n', {Button::NUM_6, 2}},
            {'o', {Button::NUM_6, 3}},
            {'p', {Button::NUM_7, 1}},
            {'r', {Button::NUM_7, 2}},
            {'s', {Button::NUM_7, 3}},
            {'t', {Button::NUM_8, 1}},
            {'u', {Button::NUM_8, 2}},
            {'v', {Button::NUM_8, 3}},
            {'w', {Button::NUM_9, 1}},
            {'x', {Button::NUM_9, 2}},
            {'y', {Button::NUM_9, 3}},
            {'q', {Button::NUM_0, 1}},
            {'z', {Button::NUM_0, 2}},
        };

        std::map<char, std::pair<int, int>> numberMap = {
            {'0', {Button::NUM_0, 1}},
            {'1', {Button::NUM_1, 1}},
            {'2', {Button::NUM_2, 1}},
            {'3', {Button::NUM_3, 1}},
            {'4', {Button::NUM_4, 1}},
            {'5', {Button::NUM_5, 1}},
            {'6', {Button::NUM_6, 1}},
            {'7', {Button::NUM_7, 1}},
            {'8', {Button::NUM_8, 1}},
            {'9', {Button::NUM_9, 1}},
        };

        if (currentMode != Alpha)
        {
            Serial.println("Error: Remote is not in ALPHA mode.");
            return;
        }

        if (input.length() > 13)
        {
            Serial.println("Error: Input exceeds 13 characters.");
            return;
        }

        auto switchToMode = [&](CapsMode newMode)
        {
            while (currentCapsMode != newMode)
            {
                cycleCapsMode(); // Cycle until the desired mode is reached
            }
        };

        // Initialize the current mapping based on the caps mode
        std::map<char, std::pair<int, int>> *currentMap;

        switch (currentCapsMode)
        {
        case Uppercase:
            currentMap = &alphaUppercaseMap;
            break;
        case Lowercase:
            currentMap = &alphaLowercaseMap;
            break;
        case Numbers:
            currentMap = &numberMap;
            break;
        }

        // Clear the existing text by "holding down" the CLEAR button;
        hold(Button::CLEAR);

        for (char c : input)
        {
            CapsMode requiredMode;
            if (isupper(c))
            {
                requiredMode = Uppercase;
            }
            else if (islower(c))
            {
                requiredMode = Lowercase;
            }
            else if (isdigit(c))
            {
                requiredMode = Numbers;
            }
            else
            {
                Serial.print("Error: Unsupported character '");
                Serial.print(c);
                Serial.println("'");
                continue;
            }

            // Switch mode if necessary
            if (requiredMode != currentCapsMode)
            {
                switchToMode(requiredMode);
            }

            // Find the character in the appropriate map
            std::map<char, std::pair<int, int>> *currentMap;
            switch (currentCapsMode)
            {
            case Uppercase:
                currentMap = &alphaUppercaseMap;
                break;
            case Lowercase:
                currentMap = &alphaLowercaseMap;
                break;
            case Numbers:
                currentMap = &numberMap;
                break;
            }

            auto it = currentMap->find(c);
            if (it != currentMap->end())
            {
                auto [buttonCode, presses] = it->second;
                for (int i = 0; i < presses; i++)
                {
                    press(buttonCode);
                    delay(300); // Delay between presses
                }
                press(Button::AMS_RIGHT); // Move to the next character
                delay(300);
            }
        }
    }
};

#endif // REMOTE_HPP
