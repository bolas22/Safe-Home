#include <SoftwareSerial.h>
#include <Servo.h>

// GSM Module
SoftwareSerial sim(10, 11); // RX, TX pins for SoftwareSerial (GSM module)
String number = "+201026617374"; // Number to send SMS
String _buffer;
int _timeout;

// MQ-9 Gas Sensor
const int mqSensorPin = A0; // Analog pin connected to the MQ-9 sensor
Servo servo; // Create a servo object to control the servo motor
const int servoPin = 9; // Digital pin connected to the servo motor
const int fanPin = 3; // Digital pin connected to the DC fan
const float mqSensorRo = 100.0; // Sensor resistance in clean air
const float mqSensorM = 0.25; // Slope value
bool isServoActivated = false; // Flag to track servo activation status
unsigned long previousMqCheck = 0; // Variable to store the previous time for MQ-9 sensor checks
const unsigned long mqCheckInterval = 5000; // Interval to check MQ-9 sensor (5 seconds)

// Flame Sensor
#define AO_PIN A1 // Arduino's pin connected to AO pin of the flame sensor
const int waterPumpPin = 4; // Digital pin connected to the water pump relay
bool isWaterPumpActivated = false; // Flag to track water pump activation status
unsigned long previousFlameCheck = 0; // Variable to store the previous time for flame sensor checks
const unsigned long flameCheckInterval = 5000; // Interval to check flame sensor (5 seconds)

// Temperature Sensors
const int tempSensor1Pin = A2;
const int tempSensor2Pin = A3;

void setup() {
  Serial.begin(9600); // Initialize serial communication
  Serial.println("System started...");

  sim.begin(9600); // Initialize SoftwareSerial for GSM module
  delay(1000);

  servo.attach(servoPin); // Attach the servo to its pin
  servo.write(0); // Initial position of the servo (0 degrees)

  pinMode(fanPin, OUTPUT); // Set fanPin as an output
  pinMode(waterPumpPin, OUTPUT); // Set waterPumpPin as an output
  digitalWrite(waterPumpPin, LOW); // Initially turn off the water pump

  checkNetworkQuality(); // Send welcome message with network quality
}

void loop() {
  unsigned long currentMillis = millis(); // Get the current time

  // Check MQ-9 sensor every mqCheckInterval (5 seconds)
  if (currentMillis - previousMqCheck >= mqCheckInterval) {
    previousMqCheck = currentMillis; // Update the previous time

    int mqSensorValue = analogRead(mqSensorPin); // Read analog value from MQ-9 sensor
    float mqSensorPpm = convertToPPM(mqSensorValue); // Convert sensor value to PPM

    Serial.print("MQ-9 PPM: ");
    Serial.println(mqSensorPpm); // Print PPM value to serial monitor

    if (mqSensorPpm > 30 && !isServoActivated) {
      Serial.println("Gas Leak!");
      activateFanAndServo(); // Activate the fan and servo
      sendGasLeakAlert(); // Send SMS alert for gas leak
      isServoActivated = true; // Set the flag to indicate servo is activated
    } else if (mqSensorPpm <= 30 && isServoActivated) {
      deactivateFanAndServo(); // Deactivate the fan and servo
      sendSituationResolvedAlert("Gas leak resolved! Fan and servo deactivated."); // Send SMS alert for gas leak resolved
      isServoActivated = false; // Reset the flag to indicate servo is deactivated
    }
  }

  // Check flame sensor every flameCheckInterval (5 seconds)
  if (currentMillis - previousFlameCheck >= flameCheckInterval) {
    previousFlameCheck = currentMillis; // Update the previous time

    int flameValue = analogRead(AO_PIN); // Read analog value from flame sensor
    Serial.print("Flame Value: ");
    Serial.println(flameValue); // Print flame sensor value to serial monitor

    if (flameValue < 1100 && flameValue > 700 && !isWaterPumpActivated) {
      Serial.println("FIRE!!!!");
      activateWaterPump(); // Activate the water pump
      sendFireAlert(); // Send SMS alert for fire
      isWaterPumpActivated = true; // Set the flag to indicate water pump is activated
    } else if ((flameValue <= 700 || flameValue >= 1100) && isWaterPumpActivated) {
      deactivateWaterPump(); // Deactivate the water pump
      sendSituationResolvedAlert("Fire extinguished! Water pump deactivated."); // Send SMS alert for fire resolved
      isWaterPumpActivated = false; // Reset the flag to indicate water pump is deactivated
    }
  }

  // Check temperature sensors every 5 seconds
  if (currentMillis % 5000 == 0) {
    int tempSensor1Value = analogRead(tempSensor1Pin);
    int tempSensor2Value = analogRead(tempSensor2Pin);

    Serial.print("Temperature Sensor 1: ");
    Serial.println(tempSensor1Value); // Print temperature sensor 1 value to serial monitor

    Serial.print("Temperature Sensor 2: ");
    Serial.println(tempSensor2Value); // Print temperature sensor 2 value to serial monitor
  }

  // Check for user input to check network quality
  if (Serial.available() > 0) {
    char input = Serial.read();
    if (input == 'q') {
      checkNetworkQuality();
    }
  }
}

// Function to convert sensor value to PPM
float convertToPPM(int sensorValue) {
  float Rs = sensorValue; // Sensor resistance in target gas
  float ppm = powf(Rs / mqSensorRo, 1 / mqSensorM); // Use powf function from math.h library
  return ppm;
}

// Function to activate the fan and rotate the servo
void activateFanAndServo() {
  digitalWrite(fanPin, HIGH); // Turn on the fan
  servo.write(90); // Rotate servo to 90 degrees
}

// Function to deactivate the fan and reset the servo
void deactivateFanAndServo() {
  digitalWrite(fanPin, LOW); // Turn off the fan
  servo.write(0); // Reset servo position to 0 degrees
}

// Function to send SMS alert for gas leak
void sendGasLeakAlert() {
  sim.println("AT+CMGF=1"); // Sets the GSM Module in Text Mode
  delay(200);
  sim.println("AT+CMGS=\"" + number + "\""); // Number to send message
  delay(200);
  String SMS = "Gas leak detected! Fan and servo activated.";
  sim.println(SMS);
  delay(100);
  sim.write((char)26); // ASCII code of CTRL+Z
  delay(200);
  _buffer = _readSerial();
  Serial.println("SMS sent (MQ-9 Gas Sensor): " + _buffer); // Confirmation message
}

// Function to activate the water pump
void activateWaterPump() {
  digitalWrite(waterPumpPin, HIGH); // Turn on the water pump
}

// Function to deactivate the water pump
void deactivateWaterPump() {
  digitalWrite(waterPumpPin, LOW); // Turn off the water pump
}

// Function to send SMS alert for fire
void sendFireAlert() {
  sim.println("AT+CMGF=1"); // Sets the GSM Module in Text Mode
  delay(200);
  sim.println("AT+CMGS=\"" + number + "\""); // Number to send message
  delay(200);
  String SMS = "Fire detected! Water pump activated.";
  sim.println(SMS);
  delay(100);
  sim.write((char)26); // ASCII code of CTRL+Z
  delay(200);
  _buffer = _readSerial();
  Serial.println("SMS sent (Flame Sensor): " + _buffer); // Confirmation message
}

// Function to send SMS alert for situation resolved
void sendSituationResolvedAlert(String message) {
  sim.println("AT+CMGF=1"); // Sets the GSM Module in Text Mode
  delay(200);
  sim.println("AT+CMGS=\"" + number + "\""); // Number to send message
  delay(200);
  sim.println(message);
  delay(100);
  sim.write((char)26); // ASCII code of CTRL+Z
  delay(200);
  _buffer = _readSerial();
  Serial.println("SMS sent (Situation Resolved): " + _buffer); // Confirmation message
}

String _readSerial() {
  String response = "";
  while (sim.available()) {
response += (char)sim.read();
  }
  return response;
}

// Function to check network quality and send welcome message
void checkNetworkQuality() {
  sim.println("AT+CSQ"); // AT command to check signal strength
  delay(200);
  _buffer = _readSerial();
  Serial.println("Welcome! Network Quality: " + _buffer);
}