#ifndef DISC_STORAGE_HPP
#define DISC_STORAGE_HPP

#include <EEPROM.h>
#include <Arduino.h>

// Structure to store disc information
struct DiscInfo
{
    uint16_t discNumber; // Disc number (2 bytes)
    char memo[16];       // Memo data (16 bytes, including padding for 13-char memos)
    bool isDataCD;       // Flag indicating if the disc is a data CD (1 byte)
};

class DiscStorage
{
private:
    static constexpr int MAX_DISCS = 300;                // Maximum number of discs
    static constexpr int RECORD_SIZE = sizeof(DiscInfo); // Size of each record in bytes

    // Calculate EEPROM address based on index
    int calculateAddress(int index)
    {
        return index * RECORD_SIZE;
    }

public:
    // Read a disc record from EEPROM
    DiscInfo readDisc(int index)
    {
        DiscInfo disc;
        int address = calculateAddress(index);
        EEPROM.get(address, disc); // Retrieve the record
        return disc;
    }

    // Write a disc record to EEPROM
    void writeDisc(int index, const DiscInfo &disc)
    {
        int address = calculateAddress(index);
        EEPROM.put(address, disc); // Save the record
    }

    // Write a disc record to EEPROM using the disc number
    void writeDiscWithNumber(int discNumber, String memo, bool isDataCD = false)
    {
        int discIndex = discNumber - 1;

        // Read the existing record to preserve the current flags
        DiscInfo disc = readDisc(discIndex);

        // Update the memo
        char charBuf[16];
        memo.toCharArray(charBuf, sizeof(charBuf));         // Copy the String to char array
        strncpy(disc.memo, charBuf, sizeof(disc.memo) - 1); // Ensure null-termination
        disc.memo[sizeof(disc.memo) - 1] = '\0';            // Explicit null-termination (safe)

        // Preserve the isDataCD flag unless explicitly overwritten
        disc.isDataCD = isDataCD ? isDataCD : disc.isDataCD;

        // Write the updated record back to EEPROM
        writeDisc(discIndex, disc);
    }

    // Mark a disc as a Data CD
    void setDiscAsDataCD(int discNumber, bool isDataCD)
    {
        int discIndex = discNumber - 1;
        DiscInfo disc = readDisc(discIndex); // Read the existing record
        disc.isDataCD = isDataCD;            // Update the flag
        writeDisc(discIndex, disc);          // Write it back to EEPROM
    }

    // Check if a disc is a Data CD
    bool isDataDisc(int discNumber)
    {
        int discIndex = discNumber - 1;
        DiscInfo disc = readDisc(discIndex);
        return disc.isDataCD;
    }

    // Get the maximum number of discs supported
    int getMaxDiscs() const
    {
        return MAX_DISCS;
    }

    // Initialize all EEPROM entries with default values
    void initializeEEPROM()
    {
        for (int i = 0; i < MAX_DISCS; i++)
        {
            DiscInfo disc = {static_cast<uint16_t>(i + 1), "", false};
            writeDisc(i, disc);
        }
    }
};

#endif
