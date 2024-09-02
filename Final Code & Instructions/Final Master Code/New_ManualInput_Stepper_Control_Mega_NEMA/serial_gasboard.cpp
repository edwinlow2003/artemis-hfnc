#include "serial_gasboard.h"

// Command arrays
byte baud_9600[] = {0x11, 0x02, 0x08, 0x00};
byte read_com[] = {0x11, 0x01, 0x01};

// Function to calculate checksum
byte calculateChecksum(byte *data, size_t length) {
  int sum = 0;
  for (size_t i = 0; i < length; i++) {
    sum += data[i];
  }
  return 256 - sum;
}

// Function to send command with variable length
void sendCommand(byte *command, size_t length) {
  // Calculate checksum
  byte checksum = calculateChecksum(command, length);
  // Send the command bytes
  for (size_t i = 0; i < length; i++) {
    Serial1.write(command[i]);
  }
  // Send the checksum
  Serial1.write(checksum);
}

// Function to read response and return an array of values
float* readResponse() {
  static float values[3]; // Array to hold the three return values
  const int maxResponseLength = 20; // Maximum expected response length
  byte response[maxResponseLength];
  int responseLength = 0;
  unsigned long startTime = millis(); // Start time in milliseconds

  // Initialize response array to zeros
  memset(response, 0, sizeof(response));

  // Read until a valid sequence is found or timeout occurs
  while (millis() - startTime < 10000) { // Adjust timeout as needed
    // Check if data is available to read
    if (Serial1.available() > 0) {
      // Read the response byte
      response[responseLength++] = Serial1.read();

      // Check if we have enough bytes to check the sequence
      if (responseLength >= 12) {
        // Check for the start of the sequence 0x16 0x09 0x01
        if (response[0] == 0x16 && response[1] == 0x09 && response[2] == 0x01) {
          byte calculatedChecksum = calculateChecksum(response, responseLength - 1);
          if (response[responseLength - 1] == calculatedChecksum) {
            //Serial.println("Valid response sequence found:");
            // Extract the values
            byte DF1 = response[3];
            byte DF2 = response[4];
            byte DF3 = response[5];
            byte DF4 = response[6];
            byte DF5 = response[7];
            byte DF6 = response[8];
            byte DF7 = response[9];
            byte DF8 = response[10];

            // Calculate measurements
            float o2Concentration = (DF1 * 256.0 + DF2) / 10.0;
            float gasFlowValue = (DF3 * 256.0 + DF4) / 100.0; // For Vol % (L/min)
            float gasTemperatureValue = (DF5 * 256.0 + DF6) / 10.0 - 50.0;

            // Store the values in the array
            values[0] = o2Concentration;
            values[1] = gasFlowValue;
            values[2] = gasTemperatureValue;

            return values; // Return the array once a valid sequence is found
            
          } else {
            Serial.println("Checksum validation failed. Discarding...");
            responseLength = 0; // Reset response length to start over
            memset(response, 0, sizeof(response)); // Clear response array
            values[0] = values[1] = values[2] = -1; // Indicate error with -1
          }
        } else {
          // Shift the array to discard the first byte and continue reading
          for (int i = 0; i < responseLength - 1; i++) {
            response[i] = response[i + 1];
          }
          responseLength--;
        }
      }
    }
  }
}
