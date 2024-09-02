#include "serial_gasboard.h"
#include <Stepper.h>
#define STEPS 200

Stepper stepper1(STEPS, 47,49,51,53); // important for ULN2003, o2
Stepper stepper2(STEPS, 46,48,50,52);
float* data;

void setup() {
  // Start the hardware serial for communication
  Serial.begin(9600);
  Serial1.begin(9600);
  stepper1.setSpeed(150);
  stepper2.setSpeed(150);
  // Allow some time for the serial monitor to initialize
  delay(100);

  // Send the command
  // Note: Comment the following line out after the first run is stable
  // sendCommand(baud_9600, sizeof(baud_9600));
}

void loop() {
  if (Serial.available()) { // Check if there is input available
    int diff1 = Serial.parseInt(); // Parse the input as an integer
    if (abs(diff1) == 1) {
      stepper1.step(diff1*STEPS);
    } else if (abs(diff1) == 11) {
      stepper2.step(diff1/11*STEPS);
    } else {
      digitalWrite(47, LOW);
      digitalWrite(49, LOW);
      digitalWrite(51, LOW);
      digitalWrite(53, LOW);
      digitalWrite(46, LOW);
      digitalWrite(50, LOW);
      digitalWrite(48, LOW);
      digitalWrite(52, LOW);
    }
  }
  sendCommand(read_com, sizeof(read_com));
  delay(100);
  data = readResponse();
  Serial.print(data[0]); // O2 concentration
  Serial.print(", ");
  Serial.print(data[1]); // Air flow
  Serial.print(", ");
  Serial.println(data[2]); // Temperature
  delay(100);
}
