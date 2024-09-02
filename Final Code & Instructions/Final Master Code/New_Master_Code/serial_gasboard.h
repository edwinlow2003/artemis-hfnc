#ifndef SERIAL_GASBOARD_H
#define SERIAL_GASBOARD_H

#include <Arduino.h>

// Command arrays
extern byte baud_9600[4];
extern byte read_com[3];
extern byte disable_auto[3];

// Function to calculate checksum
byte calculateChecksum(byte *data, size_t length);

// Function to send command with variable length
void sendCommand(byte *command, size_t length);

// Function to read response
float* readResponse();

#endif // SERIAL_GASBOARD_H
