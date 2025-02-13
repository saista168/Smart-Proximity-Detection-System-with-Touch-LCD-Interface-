# Working on Proximity Sensor with touch LCD

#include <EEPROM.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

// Initialize LCD
hd44780_I2Cexp lcd;
const int LCD_COLS = 16; // Number of columns on LCD
const int LCD_ROWS = 2;  // Number of rows on LCD

const int sensorPin = 2;  // GPIO pin connected to the proximity sensor
int metalCount = 0;       // Counter to store the number of metal detections
bool lastState = HIGH;    // Variable to store the previous state of the sensor
bool isCounting = false;  // Flag to control the counting process
unsigned long startTime = 0;  // Start time for the timer
unsigned long elapsedTime = 0;  // Total elapsed time when stopped
unsigned long currentMillis = 0;  // For stopwatch update
int countsPerUnit = 10;    // Default counts required for one distance unit
String unit = "cm";        // Default unit

void setup() {
  pinMode(sensorPin, INPUT_PULLUP); // Configure sensor pin
  Serial.begin(115200);             // Serial communication for debugging
  EEPROM.begin(512);                // Initialize EEPROM with size (e.g., 512 bytes)

  // Initialize the LCD
  if (lcd.begin(LCD_COLS, LCD_ROWS)) {
    while (1); // Halt if LCD initialization fails
  }

  lcd.print("Proximity Sensor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  delay(2000);
  lcd.clear();

  // Load calibration settings from EEPROM
  countsPerUnit = EEPROM.read(0); // Read countsPerUnit (1 byte)
  if (countsPerUnit <= 0 || countsPerUnit > 100) countsPerUnit = 10;

  char unitBuffer[3]; // To store "cm" or "m"
  EEPROM.get(1, unitBuffer); // Read unit from EEPROM
  unit = String(unitBuffer);
  if (unit != "cm" && unit != "m") unit = "cm";

  lcd.print("Unit: ");
  lcd.print(unit);
  lcd.setCursor(0, 1);
  lcd.print("Counts/Unit: ");
  lcd.print(countsPerUnit);

  delay(2000);
  lcd.clear();
}

void loop() {
  // Read commands from the Serial Monitor (optional debugging)
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equalsIgnoreCase("start")) {
      isCounting = true;
      startTime = millis();
      lcd.clear();
      lcd.print("Counting started");
      delay(1000);
    } else if (command.equalsIgnoreCase("stop")) {
      isCounting = false;
      elapsedTime += millis() - startTime;
      displayTotalDistance();
    } else if (command.equalsIgnoreCase("reset")) {
      resetAllValues();
    } else if (command.equalsIgnoreCase("set")) {
      setCalibrationSettings();
    }
  }

  // Counting logic
  if (isCounting) {
    int sensorState = digitalRead(sensorPin);
    if (lastState == HIGH && sensorState == LOW) {
      metalCount++;
      displayCurrentDistance();
    }
    lastState = sensorState;

    // Show timer
    unsigned long timerMillis = elapsedTime + (millis() - startTime);
    displayTimer(timerMillis);
  }

  delay(100); // Avoid excessive CPU usage
}

// Display the distance for the current count
void displayCurrentDistance() {
  float distanceUnits = (float)metalCount / countsPerUnit;
  lcd.setCursor(0, 0);
  lcd.print("Detected!");
  lcd.setCursor(0, 1);
  lcd.print("Distance: ");
  lcd.print(distanceUnits * (unit == "cm" ? 100 : 1));
  lcd.print(" ");
  lcd.print(unit);
}

// Show total distance after stopping
void displayTotalDistance() {
  float totalDistance = (float)metalCount / countsPerUnit;
  lcd.clear();
  lcd.print("Total Distance:");
  lcd.setCursor(0, 1);
  lcd.print(totalDistance * (unit == "cm" ? 100 : 1));
  lcd.print(" ");
  lcd.print(unit);
  delay(3000); // Allow time to read
  lcd.clear();
}

// Reset all values
void resetAllValues() {
  isCounting = false;
  metalCount = 0;
  elapsedTime = 0;
  startTime = 0;
  lcd.clear();
  lcd.print("Reset complete.");
  delay(2000);
  lcd.clear();
}

// Show a live timer
void displayTimer(unsigned long timeInMillis) {
  unsigned long minutes = timeInMillis / 60000;
  unsigned long seconds = (timeInMillis % 60000) / 1000;
  unsigned long milliseconds = timeInMillis % 1000;
  lcd.setCursor(0, 1);
  lcd.print("Timer: ");
  lcd.print(minutes);
  lcd.print(":");
  lcd.print(seconds < 10 ? "0" : "");
  lcd.print(seconds);
  lcd.print(".");
  lcd.print(milliseconds / 100); // Only show tenths of a second
}

// Set calibration settings and save to EEPROM
void setCalibrationSettings() {
  lcd.clear();
  lcd.print("Set Unit (cm/m):");
  delay(2000);

  while (Serial.available() == 0); // Wait for user input
  unit = Serial.readStringUntil('\n');
  unit.trim();

  if (unit != "cm" && unit != "m") {
    lcd.clear();
    lcd.print("Invalid unit.");
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.print("Set Counts/Unit:");
  delay(2000);

  while (Serial.available() == 0);
  countsPerUnit = Serial.readStringUntil('\n').toInt();

  if (countsPerUnit <= 0) {
    lcd.clear();
    lcd.print("Invalid count.");
    delay(2000);
    return;
  }

  EEPROM.write(0, countsPerUnit);
  char unitBuffer[3];
  unit.toCharArray(unitBuffer, 3);
  EEPROM.put(1, unitBuffer);
  EEPROM.commit();

  lcd.clear();
  lcd.print("Settings saved.");
  delay(2000);
  lcd.clear();
}
