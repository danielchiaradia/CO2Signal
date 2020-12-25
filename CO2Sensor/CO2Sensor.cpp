#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <forcedClimate.h>

#define FASTLED_ESP8266_NODEMCU_PIN_ORDER   // map output pins to what they are called on the NodeMCU
#include "FastLED.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

bool isCalibrating = false;
bool blink = false;

byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};  // get gas command
byte cmdCal[9] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};  // calibrate command

char response[9];  // holds the recieved data

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
SoftwareSerial SerialCom (D3,D4); //D3, D4
CRGB led[1];
ForcedClimate climateSensor = ForcedClimate();

void displayText(const char *text) {
	display.clearDisplay();
	display.dim(true);

	display.setTextSize(2); // Draw 2X-scale text
	display.setTextColor(WHITE, BLACK);
	display.setCursor(0, 0);

	display.println(text);
	display.display();
}

bool isCalibrationMode() {
	digitalWrite(D7, LOW);
	return digitalRead(D7) == HIGH;
}

void updateDisplay(int co2Value) {
	String co2Text = String("CO2: ");
	co2Text.concat(co2Value);
	co2Text.concat("\nTemp:");

	char tempChar[4];
	dtostrf(climateSensor.getTemperatureCelcius(), 2, 2, tempChar);
	co2Text.concat(tempChar);

	char humChar[4];
	dtostrf(climateSensor.getRelativeHumidity(), 2, 2, humChar);
	co2Text.concat("\nHum: ");
	co2Text.concat(humChar);

	char presChar[4];
	dtostrf(climateSensor.getPressure(), 4, 0, presChar);
	co2Text.concat("\nPres: ");
	co2Text.concat(presChar);

	displayText(co2Text.c_str());

	// Some output for Serial ...
	Serial.println("##################################################");

	Serial.print("PPM : ");
	Serial.println(co2Value);

	Serial.print("Status: ");
	Serial.println(SerialCom.available());

	Serial.print("Response: ");
	Serial.println(response);

	Serial.print("Humidity: ");
	Serial.println(climateSensor.getRelativeHumidity());

	Serial.print("Temperature: ");
	Serial.println(climateSensor.getTemperatureCelcius());

	Serial.print("Pressure: ");
	Serial.println(climateSensor.getPressure());
}

void updateSignal() {
	SerialCom.begin(9600);
	SerialCom.write(cmd, 9);
	SerialCom.readBytes(response, 9);
	int resHigh = (int) response[2];
	int resLow = (int) response[3];
	int co2Value = (256 * resHigh) + resLow;

	updateDisplay(co2Value);

	// Turn on signal LED
	led[0] = CRGB::Green;

	if (co2Value > 926) {
		led[0] = CRGB::OrangeRed;
	}

	if (co2Value > 1127) {
		if (blink) {
			led[0] = CRGB::Black;
		}
		else {
			led[0] = CRGB::Red;
		}
		blink = !blink;
	}

	FastLED.show();
}

void calibrate() {
	if (!isCalibrating) {
		SerialCom.begin(9600);
		SerialCom.write(cmdCal, 9);
		isCalibrating = true;
	}
	delay(3000);
}

void setup() {
	Serial.begin(9600);
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

	pinMode(D7, INPUT);

	climateSensor.begin();

	FastLED.addLeds<WS2801, 6, 5, RBG>(led, 1);
}

void loop() {
	if (isCalibrationMode()) {
		displayText("Kalibrierung");
		calibrate();

	} else {
		isCalibrating = false;
		updateSignal();
	}
	delay(1000);
}
