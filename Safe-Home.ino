#include <Servo.h>

#define FLAME_SENSOR_PIN A1 // Arduino's pin connected to the flame sensor
const int WATER_PUMP_PIN = 5; // Digital pin connected to the water pump relay
const int sensorPin = A0; // Analog pin connected to the MQ-9 sensor
const int fanPin = 3; // Digital pin connected to the DC fan
Servo servo; // Create a servo object to control the servo motor
const int servoPin = 9; // Digital pin connected to the servo motor

// Calibration values (replace with your values)
const float Ro = 125.0; // Sensor resistance in clean air
const float m = 0.25; // Slope value

// GSM Module 
#include <SoftwareSerial.h> 
SoftwareSerial sim(10, 11); // RX, TX pins for SoftwareSerial (GSM module) 
String number = "+201026617374"; // Number to send SMS 
String _buffer; 
int _timeout; 
 
void setup() {
  Serial.begin(9600); // Initialize serial communication
  Serial.println("System ready!"); // Changed the message for clarity
  pinMode(fanPin, OUTPUT); // Set fanPin as an output
  pinMode(WATER_PUMP_PIN, OUTPUT); // Set water pump pin as output
  digitalWrite(WATER_PUMP_PIN, LOW); // Initially turn off the water pump
  servo.attach(servoPin); // Attach the servo to its pin
  servo.write(0); // Initial position of the servo (0 degrees)
  sim.begin(9600); // Start the GSM module 
}

void loop() {
  // Read analog value from flame sensor
  int flameValue = analogRead(FLAME_SENSOR_PIN);
  Serial.print("Flame Value: ");
  Serial.println(flameValue); // Print flame sensor value to serial monitor

  // Check if flame value is between 800 and 1100
  if (flameValue >= 35 && flameValue <= 650) {
    Serial.println("Flame detected! Turning on water pump...");
    digitalWrite(WATER_PUMP_PIN, HIGH);
    callNumber();
    sendFireAlert(flameValue); // Turn on the water pump
  } else {
    Serial.println("No flame detected.");
    digitalWrite(WATER_PUMP_PIN, LOW); // Turn off the water pump
  }

  // Read analog value from MQ-9 sensor
  int sensorValue = analogRead(sensorPin);
  float ppm = convertToPPM(sensorValue); // Convert sensor value to PPM
  Serial.print("CO PPM: ");
  Serial.println(ppm); // Print PPM value to serial monitor

  if (ppm > 100) {
    activateFanAndServo();
    callNumber();
    sendGasAlert(ppm); // Activate the fan and servo
  } else {
    digitalWrite(fanPin, LOW); // Turn off the fan
    servo.write(0); // Reset servo position to 0 degrees
  }

  delay(1000); // Delay for 1 second
}

// Function to convert sensor value to PPM
float convertToPPM(int sensorValue) {
  float Rs = sensorValue; // Sensor resistance in target gas
  float ppm = pow(Rs / Ro, 1/m); // Calculate PPM using the formula
  return ppm;
}

// Function to activate the fan and rotate the servo
void activateFanAndServo() {
  digitalWrite(fanPin, HIGH); // Turn on the fan
  servo.write(90); // Rotate servo to 90 degrees
}
// Function to send gas alert 
void sendGasAlert(float ppm) { 
  sim.println("AT+CMGF=1"); // Set GSM module to text mode 
  delay(200); 
  sim.println("AT+CMGS=\"" + number + "\""); // Number to send message 
  delay(200); 
  String SMS = "Gas alert! CO PPM is " + String(ppm); // Alert message with ppm value 
  sim.println(SMS); 
  delay(100); 
  sim.write((char)26); // End SMS transmission 
  delay(200); 
} 
// Function to send fire alert 
void sendFireAlert(int flameValue) { 
  sim.println("AT+CMGF=1"); // Set GSM module to text mode 
  delay(200);
  sim.println("AT+CMGS=\"" + number + "\""); // Number to send message 
  delay(200); 
  String SMS = "Fire alert! Flame detected. Flame value is " + String(flameValue); // Alert message with flame value 
  sim.println(SMS); 
  delay(100); 
  sim.write((char)26); // End SMS transmission 
  delay(200); 
} 
void callNumber() { 
  sim.print (F("ATD")); 
  sim.print (number); 
  sim.print (F(";\r\n"));  
  Serial.println(_buffer); 
}
