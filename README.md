# artemis-hfnc
An automated High Flow Nasal Cannula (HFNC) system made with Arduinos. By turning two knobs, the user can input their desired oxygen concentration (21 to 100%) and total flow rate (2 to 60 LPM) setpoints. The device uses a control algorithm to automatically adjust air & oxygen flows until the desired setpoint is reached, at which point the system stabilizes. A combined air flow and oxygen concentration sensor is used. Both the air and oxygen sources are set to 50 psi to mimic hospital settings. Motor-valve assemblies (MVAs) control the air and oxygen flows. MVAs consist of a stepper motor (NEMA 17), a valve, and a coupler which connects the two components. The control algorithm turns the motor according to its calculations, and the valve is opened/closed accordingly.

Associated with the Summer Experience in Engineering Design (SEED) program at the Oshman Engineering Design Kitchen (OEDK), Rice University.

Key improvements to the project include:
- Optimized an automated High Flow Nasal Cannula (HFNC) system, a medical device that delivers oxygen-enriched, heated, and humidified air at high flow rates of up to 60 LPM.
- Reduced system response time from 5 minutes to 30 seconds by modularizing the existing control algorithm.
- Designed a reinforced motor-valve assembly housing to securely accommodate a 150-RPM motor, ensuring stability at higher speeds and preventing valves from loosening in all test runs.
- Simplified the user interface by integrating two rotary encoders and an LCD screen on the device’s enclosure, making the system intuitive to control and mountable on a hospital IV pole.

**See the "Final Code & Instructions" folder for the final master code needed to run the device, as well as setup instructions.**

**See the link below for a presentation on the project:**
https://docs.google.com/presentation/d/131RJJtJnJPLtWhvJR9N3QjBGgE00Bi876u4Z24E5AcE/edit#slide=id.p4

**See the link below for detailed setup instructions (also found in "Final Code & Instructions" folder):**
https://docs.google.com/document/d/104ywyCypigrzyAFWdaAhRMFE7SToJP98R1MAKYyCpF8/edit?usp=sharing

**See the link below for all the testings that had been done on motors, valves, and the control algorithm:**
https://docs.google.com/document/d/1b_SWOcd7VZTsdrpVrNKoZO5QYNt-mqXgGez2lnKgZk8/edit?usp=sharing
