#include "serial_gasboard.h"
#include <Stepper.h>
#include <Encoder.h>

#define STEPS 200  // Number of steps per revolution of the motor
#define PERIOD 0.4
#define MOTORSPEED 150

// Rotary Encoder Pins
#define O2_ENCODER_CLK 40
#define O2_ENCODER_DT 42
#define O2_ENCODER_SW 44

#define AIR_ENCODER_CLK 41
#define AIR_ENCODER_DT 43
#define AIR_ENCODER_SW 45

Encoder O2Encoder(O2_ENCODER_CLK, O2_ENCODER_DT);
Encoder AirEncoder(AIR_ENCODER_CLK, AIR_ENCODER_DT);

// State variables for encoder
bool O2Adjustable = true;
bool AirAdjustable = true;
bool O2LastSwitchState = HIGH;
bool AirLastSwitchState = HIGH;

// Motor Pins
const int airMotorPins[] = { 47, 49, 51, 53 };
const int o2MotorPins[] = { 46, 48, 50, 52 };
Stepper airstepper(STEPS, airMotorPins[0], airMotorPins[1], airMotorPins[2], airMotorPins[3]);
Stepper o2stepper(STEPS, o2MotorPins[0], o2MotorPins[1], o2MotorPins[2], o2MotorPins[3]);

// Initialize PID constants
float input = 0.0;   // Current flowrate in sensor
float output = 0.0;  // Proportional output
float Kp = 7.5;

// Stage variables
int stage = 0;          // for overall system's stages
float measurements[4];  // Assuming the response array has a maximum of 4 values

// Variable to keep track of how much the motors moved; for resetting
int total_air_movement = 0;
int total_o2_movement = 0;

// Average concentration variables used in Stage 0 of the overall system
float average_air_concentration = 0;
float average_o2_concentration = 0;

// Setpoints according to the user's inputs; will be fetched from the other Mega
int O2setp = 21;
int Flowsetp = 2;

// Consecutive measurements buffer
const int bufferSize = 2;
float measurementBuffer[bufferSize];
int bufferIndex = 0;

void stopMotor(int motor) {
  // Stop the motor by setting all its pins to LOW
  if (motor == 1) {  // turn off airstepper
    digitalWrite(airMotorPins[0], LOW);
    digitalWrite(airMotorPins[1], LOW);
    digitalWrite(airMotorPins[2], LOW);
    digitalWrite(airMotorPins[3], LOW);
  } else if (motor == 2) {  // turn off o2stepper
    digitalWrite(o2MotorPins[0], LOW);
    digitalWrite(o2MotorPins[1], LOW);
    digitalWrite(o2MotorPins[2], LOW);
    digitalWrite(o2MotorPins[3], LOW);
  }
}

void computeProportional(float setpoint) {
  // Calculate the error
  float error = setpoint - input;

  // Calculate the proportional output
  output = Kp * error;
}

void dataCollect() {
  // Assuming functions for sending commands and reading responses are implemented in serial_gasboard.h
  sendCommand(read_com, sizeof(read_com));
  delay(10);
  float* response = readResponse();

  // Copy the response to the measurements array
  for (int i = 0; i < 2; i++) {
    measurements[i] = response[i];
  }

  // Update the measurement buffer
  measurementBuffer[bufferIndex] = measurements[1];
  bufferIndex = (bufferIndex + 1) % bufferSize;
}

bool checkConsecutiveMeasurements(float setpoint, float tolerance) {
  for (int i = 0; i < bufferSize; i++) {
    if (abs(measurementBuffer[i] - setpoint) > tolerance * setpoint) {
      return false;
    }
  }
  return true;
}

void pidMotorControl(Stepper& motor, float setpoint, int motor_num) {
  // Perform proportional control calculations
  computeProportional(setpoint);

  // Determine the step size based on the error magnitude
  int stepSize = abs(output);
  //if (stepSize < 1) stepSize = 1; // Ensure a minimum step size

  // Use the proportional output to control the stepper motor
  if (abs(setpoint - input) > 0.02 * setpoint || setpoint >= input) {
    // Set a fixed speed for motor adjustment
    motor.setSpeed(120);

    // Adjust Kp based on conditions (example given)
    if (setpoint >= 40 && input >= 40) {
      Kp = 3;
    } else {
      Kp = 10;
    }

    // Move the motor based on output direction
    if (output > 0) {
      Serial.println("opening");
      motor.step(-stepSize);  // Adjust steps as needed
      stopMotor(motor_num);

      // Keep track of total motor movement for reset
      if (motor_num == 1) {
        total_air_movement -= stepSize;
      } else {
        total_o2_movement -= stepSize;
      }

    } else {
      Serial.println("closing");
      motor.step(stepSize);  // Adjust steps as needed
      stopMotor(motor_num);

      // Keep track of total motor movement for reset
      if (motor_num == 1) {
        total_air_movement += stepSize;
      } else {
        total_o2_movement += stepSize;
      }
    }

  } else {
    // Stop the motor when within 5% error of setpoint
    Kp = 0;
    stopMotor(motor_num);
    Serial.println("Motor stopped. Input is within 2% error of the air_flow_setpoint.");
    delay(1500);  // Delay for stability after stopping
  }
  // Print flow rate and step size for debugging
  Serial.print("Flowrate for air: ");
  Serial.print(input);
  Serial.print(" Steps: ");
  Serial.println(stepSize);

  // Add a delay for stability
  delay(500);  // Adjust as necessary
}


void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  Serial2.begin(9600);
  delay(500);

  // Send the command; comment this out after the first run
  // sendCommand(baud_9600, sizeof(baud_9600));
  // Keep this uncommented; it disables the sensor from automatically sending command
  sendCommand(disable_auto, sizeof(disable_auto));

  // Serial.println("Ensure air and O2 are connected. Press enter to continue.");
  // while (!Serial.available())
  //   ;
  // while (Serial.available()) { Serial.read(); }

  // Set the speed of stepper motors
  airstepper.setSpeed(MOTORSPEED);  // Set the speed of stepper motor for air
  o2stepper.setSpeed(MOTORSPEED);   // Set the speed of stepper motor for o2

  // Initialize measurement buffer
  for (int i = 0; i < bufferSize; i++) {
    measurementBuffer[i] = 0.0;
  }
  // Allow some time for the serial monitor to initialize
  delay(100);
}

void loop() {
  switch (stage) {
    case 0:
      // Serial.println("Stage 0: Fetching concentrations of air & O2 sources");

      // Serial.println("Detecting air source concentration... ");
      // airstepper.step(-1000);
      // stopMotor(1);  // turn off airstepper's pins
      // delay(3000);   // open the air valve and allow air to flow in

      // float air_concentration_sum = 0;  // O2 concentration in the air source
      // for (int i = 0; i < 5; i++) {
      //   dataCollect();
      //   Serial.print("Air flow data: ");
      //   Serial.println(measurements[0]);
      //   air_concentration_sum += measurements[0];  // measurements[0] is the concentration
      //   delay(200);
      // }

      // average_air_concentration = air_concentration_sum / 5;

      // Serial.print("Air source concentration: ");
      // Serial.println(average_air_concentration);
      // airstepper.step(1000);  // close the air valve
      // stopMotor(1);           // turn off airstepper's pins

      // Serial.println("Detecting O2 source concentration... ");

      // o2stepper.step(-1000);
      // stopMotor(2);  // turn off o2stepper's pins
      // delay(10000);   // open the O2 valve and allow O2 to flow in; may need to increase delay for O2 to reach the sensor

      // float o2_concentration_sum = 0;  // O2 concentration in the O2 source
      // for (int i = 0; i < 5; i++) {
      //   dataCollect();
      //   Serial.print("O2 data: ");
      //   Serial.println(measurements[0]);
      //   o2_concentration_sum += measurements[0];  // measurements[0] is the concentration
      //   delay(200);
      // }

      // average_o2_concentration = o2_concentration_sum / 5;

      // Serial.print("O2 source concentration: ");
      // Serial.println(average_o2_concentration);
      // o2stepper.step(1000);  // close the O2 valve
      // stopMotor(2);          // turn off o2stepper's pins

      // Serial.println("Performing air flush... ");
      // airstepper.step(-1000);  // open the air valve to flush out the oxygen
      // stopMotor(1);            // turn off airstepper's pins
      // delay(10000);             // it may take a while before the system gets back to room air concentration (~21%)

      // dataCollect();
      // Serial.println("Air flush data: ");
      // Serial.print("Air source concentration (should be ~21%): ");
      // Serial.println(measurements[0]);

      // airstepper.step(1000);
      // stopMotor(1);  // turn off airstepper's pins

      // For testing later stages if stage 0 is skipped
      average_air_concentration = 21;
      average_o2_concentration = 100;

      Serial.println("Concentrations of air & O2 sources fetched.");
      Serial.println("Moving to stage 1...");
      stage = 1;  // Move to the next stage

    case 1:
      Serial.println("Stage 1: Inputting setpoints and calculating flow values");

      // Check for incoming setpoints via Serial2
      while (O2Adjustable || AirAdjustable) {
        if (Serial2.available() > 0) {
          String receivedData = Serial2.readStringUntil('\n');
          if (receivedData.startsWith("O2:")) {
            O2setp = receivedData.substring(3).toInt();
            Serial.print("Received O2 setpoint via Serial2: ");
            Serial.println(O2setp);
          } else if (receivedData.startsWith("Flow:")) {
            Flowsetp = receivedData.substring(5).toInt();
            Serial.print("Received Flow setpoint via Serial2: ");
            Serial.println(Flowsetp);
          } else if (receivedData.startsWith("O2 Adjustment: Disabled")) {
            O2Adjustable = false;
            Serial.println("O2 Adjustment disabled.");
          } else if (receivedData.startsWith("Air Adjustment: Disabled")) {
            AirAdjustable = false;
            Serial.println("Air Adjustment disabled.");
          }
        }
      }

      Serial.println("Both O2 & flow adjustments disabled. Setpoint fixed.");
      Serial.println("Calculating Air Flow & O2 Flow Setpoints...");

      float air_flow_setpoint = (Flowsetp * ((O2setp - average_o2_concentration) / 100)) / ((average_air_concentration - average_o2_concentration) / 100);
      float o2_flow_setpoint = Flowsetp - air_flow_setpoint;

      Serial.println("Air Flow Setpoint: ");
      Serial.println(air_flow_setpoint);
      Serial.println("O2 Flow Setpoint: ");
      Serial.println(o2_flow_setpoint);

      Serial.println("Moving to stage 2...");
      stage = 2;  // Move to the next stage

    case 2:
      Serial.println("Stage 2: Adjusting air flow to calculated air flow setpoint");
      // Transmit stage number to update the screen
      Serial2.println("Stage 2");

      while (stage == 2) {
        // Update flow value
        dataCollect();
        input = measurements[1];

        // Transmit data to the Mega with the LCD
        Serial2.print("Sensor flow rate:");
        Serial2.println(input);
        Serial2.print("Sensor O2 %:");
        Serial2.println(measurements[0]);  // measurements[0] is the sensor's concentration reading
        Serial.println("Transmitted sensor readings via Serial2");

        // Control the stepper motor using PID based on air_flow_setpoint
        pidMotorControl(airstepper, air_flow_setpoint, 1);

        // Check if the last 5 measurements are within 2% error of the setpoint
        if (checkConsecutiveMeasurements(air_flow_setpoint, 0.05)) {
          // Stop the motor if the measurements are within the error range
          Kp = 0;
          stopMotor(1);  // turn off airstepper's pins
          stage = 3;

          Serial.print("Flow rate for air: ");
          Serial.println(input);
          Serial.print("O2 %:");
          Serial.println(measurements[0]);
        }
        delay(100);
      }

    case 3:
      Serial.println("Stage 3: Adjusting o2 flow so that the total flow setpoint is reached");
      // Transmit stage number to update the screen
      Serial2.println("Stage 3");

      while (stage == 3) {
        // Update flow value
        dataCollect();
        input = measurements[1];

        // Transmit data to the Mega with the LCD
        Serial2.print("Sensor flow rate:");
        Serial2.println(input);
        Serial2.print("Sensor O2 %:");
        Serial2.println(measurements[0]);  // measurements[0] is the sensor's concentration reading
        Serial.println("Transmitted sensor readings via Serial2");

        // Turn o2stepper until the total air flow reaches Flowsetp
        pidMotorControl(o2stepper, Flowsetp, 2);

        // Check if the last 5 measurements are within 2% error of the setpoint
        if (checkConsecutiveMeasurements(Flowsetp, 0.05)) {
          // Stop the motor if the measurements are within the error range
          Kp = 0;
          stopMotor(2);  // turn off o2stepper's pins
          stage = 4;
          Serial.println("Motor stopped. 3 consecutive measurements are within 2% error of the setpoint.");
        }
        delay(100);
      }

    case 4:
      Serial.println("Stage 4: Display air flow and concentration");
      // Transmit stage number to update the screen
      Serial2.println("Stage 4");

      while (stage == 4) {
        // Update flow value
        dataCollect();
        input = measurements[1];

        // Print flow rate and step size for debugging
        Serial.print("Sensor flow rate: ");
        Serial.println(input);
        Serial.print("Sensor O2 %: ");
        Serial.println(measurements[0]);

        // Transmit data to the Mega with the LCD
        Serial2.print("Sensor flow rate:");
        Serial2.println(input);
        Serial2.print("Sensor O2 %:");
        Serial2.println(measurements[0]);  // measurements[0] is the sensor's concentration reading
        Serial.println("Transmitted sensor readings via Serial2");
        delay(500);

        // System reset: close both valves if the user presses both of the knobs once
        if (Serial2.available() > 0) {
          String receivedData = Serial2.readStringUntil('\n');
          if (receivedData.startsWith("Reset")) {
            Serial.println("Both knobs pressed. Received reset signal.");
            o2stepper.step(-total_o2_movement);  // close the o2 motor by its total number of steps
            stopMotor(2);
            Serial.println("O2 motor closed. Performing air flush for 5 seconds.");
            delay(5000); // to allow for air flush for resetting
            dataCollect();

            while (measurements[0] > 25) {
              Serial.println("Not enough air flush time. Continue with air flushing.");
              delay(10000);
              dataCollect();
            }

            airstepper.step(-total_air_movement);   // close the air motor by its total number of steps
            stopMotor(1);
            Serial.println("Air motor closed.");

            // Reset total motor movements to 0
            total_air_movement = 0;
            total_o2_movement = 0;
            
            Serial.println("Both O2 & air motors closed. System reset complete.");
            Serial2.println("Reset done");

            O2Adjustable = true;
            AirAdjustable = true;
            stage = 1; // Go back to stage 1 to allow for more user input
          }
        }
      }

    default:
      // Default case: print error message and reset to stage 0
      Serial.println("Unknown stage. Resetting to stage 0.");
      stage = 0;
        
  }
}
