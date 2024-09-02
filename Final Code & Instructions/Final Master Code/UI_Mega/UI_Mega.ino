#include <Arduino.h>
#include <RotaryEncoder.h>
#include <LCDWIKI_GUI.h> // Core graphics library
#include <LCDWIKI_KBV.h> // Hardware-specific library

// Define the pins for the rotary encoders
#define O2_ENCODER_CLK 45
#define O2_ENCODER_DT 43
#define O2_ENCODER_SW 47

#define AIR_ENCODER_CLK 51
#define AIR_ENCODER_DT 53
#define AIR_ENCODER_SW 49

// Create encoder objects
RotaryEncoder O2Encoder(O2_ENCODER_DT, O2_ENCODER_CLK, RotaryEncoder::LatchMode::FOUR3);
RotaryEncoder AirEncoder(AIR_ENCODER_DT, AIR_ENCODER_CLK, RotaryEncoder::LatchMode::FOUR3);

// Setpoint variables
int O2Setp = 21;
int FlowSetp = 2;

// Variables for actual sensor readings
float O2Reading = 0;
float FlowReading = 0;

// State variables
bool O2Adjustable = true;
bool AirAdjustable = true;
bool O2LastSwitchState = HIGH;
bool AirLastSwitchState = HIGH;

// Flag to indicate fetching sensor data
bool fetchingSensorData = false;

// Flag to print "Reset" on the screen only once so it doesn't flicker
bool resetDisplayed = false;

// LCD object
LCDWIKI_KBV mylcd(320,480,A3,A2,A1,A0,A4); //width,height,cs,cd,wr,rd,reset

// Color definitions (if not already defined in the library)
#define CYAN   0x07FF
#define BLACK  0x0000
#define LIME   0x07E0
#define WHITE  0xFFFF
#define YELLOW 0xFFE0

void PrintInitialScreen() {
  mylcd.Set_Text_Mode(0);
  mylcd.Set_Text_Back_colour(BLACK);

  // Box 1 - Top Left - Flow Rate
  mylcd.Set_Text_colour(CYAN);
  mylcd.Set_Text_Size(3);
  mylcd.Print_String("Flow Rate", 40, 40);
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(2);
  mylcd.Print_String("L/min", 140, 80);

  // Box 2 - Top Right - O2%
  mylcd.Set_Text_Size(3);
  mylcd.Set_Text_colour(CYAN);
  mylcd.Print_String("Oxygen %", 290, 40);
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(2);
  mylcd.Print_String("% O2", 380, 80);

  // Box 3 - Bottom Left - Set Flow
  mylcd.Set_Text_colour(CYAN);
  mylcd.Set_Text_Size(3);
  mylcd.Print_String("Set Flow", 50, 200);
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(2);
  mylcd.Print_String("L/min", 140, 250);

  // Box 4 - Bottom Right - Set O2
  mylcd.Set_Text_colour(CYAN);
  mylcd.Set_Text_Size(3);
  mylcd.Print_String("Set O2", 310, 200);
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(2);
  mylcd.Print_String("% O2", 380, 250);
}

void UpdateFlowReading(float Flow_Reading) {
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(2);
  mylcd.Print_Number_Float(Flow_Reading, 2, 60, 80, '.', 0, ' ');
}

void UpdateO2Reading(float O2_Reading) {
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(2);
  mylcd.Print_Number_Float(O2_Reading, 2, 320, 80, '.', 0, ' ');
}

void UpdateFlowSetpoint(float Flow_Setpoint) {
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(2);
  mylcd.Print_Number_Float(Flow_Setpoint, 2, 70, 250, '.', 0, ' ');
}

void UpdateO2Setpoint(float O2_Setpoint) {
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(2);
  mylcd.Print_Number_Float(O2_Setpoint, 2, 310, 250, '.', 0, ' ');
}

void DisplayMiddleText(const char* text) {
  // Clear the previous text by drawing a rectangle over the text area
  mylcd.Set_Draw_color(BLACK);
  mylcd.Fill_Rectangle(190, 140, 480, 160); // Adjust the coordinates and dimensions as needed

  // Display the new text
  mylcd.Set_Text_Back_colour(BLACK);
  mylcd.Set_Text_colour(LIME); // Change the color as needed
  mylcd.Set_Text_Size(3);       // Change the size as needed
  mylcd.Print_String(text, 190, 140); // Adjust the coordinates as needed
}

void DisplayBottomLeftText(const char* text) {
  // Clear the previous text by drawing a rectangle over the text area
  mylcd.Set_Draw_color(BLACK);
  mylcd.Fill_Rectangle(70, 280, 240, 300); // Adjust the coordinates and dimensions as needed

  // Display the new text
  mylcd.Set_Text_Back_colour(BLACK);
  mylcd.Set_Text_colour(YELLOW); // Change the color as needed
  mylcd.Set_Text_Size(2);        // Change the size as needed
  mylcd.Print_String(text, 85, 280); // Adjust the coordinates as needed
}

void DisplayBottomRightText(const char* text) {
  // Clear the previous text by drawing a rectangle over the text area
  mylcd.Set_Draw_color(BLACK);
  mylcd.Fill_Rectangle(310, 280, 480, 300); // Adjust the coordinates and dimensions as needed

  // Display the new text
  mylcd.Set_Text_Back_colour(BLACK);
  mylcd.Set_Text_colour(YELLOW); // Change the color as needed
  mylcd.Set_Text_Size(2);        // Change the size as needed
  mylcd.Print_String(text, 325, 280); // Adjust the coordinates as needed
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600); // Initialize Serial1 for communication
  Serial.println("Stage 1: Inputting setpoints and calculating flow values");

  // Set the switch pins as input with internal pull-up resistors
  pinMode(O2_ENCODER_SW, INPUT_PULLUP);
  pinMode(AIR_ENCODER_SW, INPUT_PULLUP);

  // Initialize LCD
  mylcd.Init_LCD();
  mylcd.Fill_Screen(BLACK);
  mylcd.Set_Rotation(1); // Landscape Orientation

  // Print the initial screen layout
  PrintInitialScreen();

  // Initial values update
  UpdateFlowReading(0);
  UpdateO2Reading(0);
  UpdateFlowSetpoint(FlowSetp);
  UpdateO2Setpoint(O2Setp);

  DisplayBottomLeftText("Enabled");
  DisplayBottomRightText("Enabled");
  DisplayMiddleText("Stage 1");
}

void loop() {
  // Read the O2 encoder switch state
  bool O2SwitchState = digitalRead(O2_ENCODER_SW);
  if (O2SwitchState == LOW && O2LastSwitchState == HIGH) {
    // Toggle O2Adjustable state
    O2Adjustable = !O2Adjustable;
    Serial.print("O2 Adjustment: ");
    Serial.println(O2Adjustable ? "Enabled" : "Disabled");
    delay(200); // Debounce delay
    Serial1.print("O2 Adjustment: "); // Send updated O2Setp
    Serial1.println(O2Adjustable ? "Enabled" : "Disabled");

    // Update display
    DisplayBottomRightText(O2Adjustable ? "Enabled" : "Disabled");
  }
  O2LastSwitchState = O2SwitchState;

  // Read the Air encoder switch state
  bool AirSwitchState = digitalRead(AIR_ENCODER_SW);
  if (AirSwitchState == LOW && AirLastSwitchState == HIGH) {
    // Toggle AirAdjustable state
    AirAdjustable = !AirAdjustable;
    Serial.print("Air Adjustment: ");
    Serial.println(AirAdjustable ? "Enabled" : "Disabled");
    delay(200); // Debounce delay
    Serial1.print("Air Adjustment: ");
    Serial1.println(AirAdjustable ? "Enabled" : "Disabled");

    // Update display
    DisplayBottomLeftText(AirAdjustable ? "Enabled" : "Disabled");
  }
  AirLastSwitchState = AirSwitchState;

  // Update the O2 encoder state
  if (O2Adjustable) {
    static int O2_pos = 0;
    O2Encoder.tick();
    int O2_newPos = O2Encoder.getPosition();
    if (O2_pos != O2_newPos) {
      O2Setp += (O2_newPos - O2_pos);
      O2Setp = constrain(O2Setp, 21, 100); // Adjust O2 setpoint value within a range
      Serial.print("O2 Setpoint: ");
      Serial.println(O2Setp);
      UpdateO2Setpoint(O2Setp);
      Serial1.print("O2:"); // Send updated O2Setp
      Serial1.println(O2Setp);
    }
    O2_pos = O2_newPos;
  }

  // Update the Air encoder state
  if (AirAdjustable) {
    static int air_pos = 0;
    AirEncoder.tick();
    int air_newPos = AirEncoder.getPosition();
    if (air_pos != air_newPos) {
      FlowSetp += (air_newPos - air_pos);
      FlowSetp = constrain(FlowSetp, 2, 60); // Adjust air setpoint value within a range
      Serial.print("Flow Setpoint: ");
      Serial.println(FlowSetp);
      UpdateFlowSetpoint(FlowSetp);
      Serial1.print("Flow:"); // Send updated FlowSetp
      Serial1.println(FlowSetp);
    }
    air_pos = air_newPos;
  }

  // Update LCD with sensor data from the other Mega
  if (!AirAdjustable && !O2Adjustable) {
    // Set the flag to indicate fetching sensor data
    fetchingSensorData = true;

    if (Serial1.available() > 0) {
      String receivedData = Serial1.readStringUntil('\n');
      if (receivedData.startsWith("Stage")) {
        Serial.print("Received stage number via Serial1");
        DisplayMiddleText(receivedData.c_str()); // Update screen with stage number
      } else if (receivedData.startsWith("Sensor flow rate:")) {
        FlowReading = receivedData.substring(17).toFloat();
        Serial.print("Received sensor flow rate reading via Serial1: ");
        Serial.println(FlowReading);
        UpdateFlowReading(FlowReading);
      } else if (receivedData.startsWith("Sensor O2 %:")) {
        O2Reading = receivedData.substring(12).toFloat();
        Serial.print("Received sensor O2 % reading via Serial1: ");
        Serial.println(O2Reading);
        UpdateO2Reading(O2Reading);
      }
    }
  }

  // After the system reaches the desired setpoints, the system resets (i.e. both air and oxygen valves are closed) 
  // if the user enables both setpoint adjustments. To do this, the user presses each of the encoders once.
  if (fetchingSensorData && AirAdjustable && O2Adjustable) {
    if (!resetDisplayed) {
      Serial1.println("Reset"); // Send reset signal to the other Mega
      DisplayMiddleText("Reset"); // Update screen
      Serial.println("Both knobs pressed. Sending reset signal.");
      resetDisplayed = true; // Set the flag to true after displaying "Reset"
    }

    if (Serial1.available() > 0) {
      String receivedData = Serial1.readStringUntil('\n');
      if (receivedData.startsWith("Reset done")) {
        // Display 0 for the sensor readings for both flow and o2
        UpdateFlowReading(0);
        UpdateO2Reading(0);
        resetDisplayed = false; // Reset the flag after receiving "Reset done"
        DisplayMiddleText("Stage 1"); // Goes back to Stage 1
      }
    }
  } else {
    resetDisplayed = false; // Reset the flag if the condition is no longer met
  }

  delay(1); // Small delay to reduce screen flicker
}
