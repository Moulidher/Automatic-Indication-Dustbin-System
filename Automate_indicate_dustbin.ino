#include <SoftwareSerial.h>

// Pins for GSM Module
SoftwareSerial mySerial(7, 8); // RX, TX for SIM900A
// Pins for Ultrasonic Sensor
const int trigPin = 9;
const int echoPin = 10;

// Pins for LEDs
const int ledLowLevelPin = 11; // LED for low level
const int ledHighLevelPin = 12; // LED for high level

// Threshold distance in cm
const long THRESHOLD_HIGH = 5; // Distance for full level (0 cm to 5 cm)
const int FULL_BIN_CHECKS = 5; // Number of consecutive checks before sending SMS

// Variables for message and state
bool messageSent = false;
int fullBinCounter = 0; // Counter for consecutive full readings

// Pins for GPS Module
SoftwareSerial gpsSerial(4, 3); // RX, TX for GPS

// Function declarations
String getGPSLocation();
String parseLatitude(String nmea);
String parseLongitude(String nmea);

void setup() {
  // Start communication with GSM module
  mySerial.begin(9600);
  Serial.begin(9600); // Start serial communication with Arduino IDE monitor
  
  // Setup LED pins
  pinMode(ledLowLevelPin, OUTPUT);
  pinMode(ledHighLevelPin, OUTPUT);
  
  // Setup Ultrasonic Sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Setup GPS module
  gpsSerial.begin(9600);
  
  delay(1000); // Allow GSM module to register on the network
  Serial.println("Smart Dustbin System Initialized.");
}

void loop() {
  long duration, distance;

  // Clear the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Trigger the ultrasonic sensor
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the echoPin
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Check bin level based on distance
  if (distance >= 0 && distance <= THRESHOLD_HIGH) {
    fullBinCounter++;
    
    if (fullBinCounter >= FULL_BIN_CHECKS && !messageSent) {
      String gpsData = getGPSLocation();
      
      if (gpsData != "N/A") {
        String message = "Alert: The dustbin is full! Location: " + gpsData; // GPS link here
        sendSMS("+916369667904", message);
        messageSent = true; // Mark message as sent
      } else {
        Serial.println("GPS data not available yet.");
      }
    }

    digitalWrite(ledHighLevelPin, HIGH); // Turn on high-level LED
    digitalWrite(ledLowLevelPin, LOW);   // Turn off low-level LED
  } else {
    digitalWrite(ledLowLevelPin, HIGH); // Turn on low-level LED
    digitalWrite(ledHighLevelPin, LOW); // Turn off high-level LED
    fullBinCounter = 0; // Reset full bin counter
    messageSent = false; // Reset message sent flag
  }
  
  delay(1000); // Delay for stability
}

void sendSMS(String phoneNumber, String message) {
  mySerial.println("AT+CMGF=1"); // Set SMS to text mode
  delay(1000);
  
  mySerial.print("AT+CMGS=\"");
  mySerial.print(phoneNumber);
  mySerial.println("\"");
  delay(1000);

  mySerial.println(message);
  delay(1000);

  mySerial.write(26); // End the message with Ctrl+Z
  delay(1000);
  
  Serial.println("Message sent successfully.");
}

String getGPSLocation() {
  String data = "";
  String latitude = "N/A";
  String longitude = "N/A";

  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    Serial.print(c); // Print raw GPS data for debugging
    if (c == '\n') {
      if (data.startsWith("$GPGGA")) {
        latitude = parseLatitude(data);
        longitude = parseLongitude(data);
      }
      data = ""; // Reset for the next NMEA sentence
    } else {
      data += c; // Append the character to form the NMEA sentence
    }
  }

  // Format the Google Maps link
  if (latitude != "N/A" && longitude != "N/A") {
    String googleMapsLink = "https://www.google.com/maps?q=" + latitude + "," + longitude;
    return googleMapsLink; // Return the link directly
  }
  
  return "N/A"; // Return N/A if no GPS data
}

String parseLatitude(String nmea) {
  int commaIndex1 = nmea.indexOf(',', 18);
  int commaIndex2 = nmea.indexOf(',', commaIndex1 + 1);
  return nmea.substring(commaIndex1 + 1, commaIndex2);
}

String parseLongitude(String nmea) {
  int commaIndex2 = nmea.indexOf(',', 18);
  int commaIndex3 = nmea.indexOf(',', commaIndex2 + 1);
  int commaIndex4 = nmea.indexOf(',', commaIndex3 + 1);
  return nmea.substring(commaIndex3 + 1, commaIndex4);
}