#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "logo.h"
JsonObject countries;
// Define TFT pins
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   4
const int buttonPinA = 12; 
const int buttonPinB = 14;

int lastButtonStateA = HIGH;
int lastButtonStateB = HIGH;

// Replace with your WiFi credentials
const char* ssid = "Pradex";
const char* password = "prdy5359";

// API keys
const char* ipinfoApiKey = "bdc3cb7e8d43c9";
const char* openWeatherMapApiKey = "403e1de5dd6b90cb7e64a7869033cd0d";

// Weather information variables
String city = "";
String country = "";
String coordinates = "";
String temperature = "";
String pressure = "";
String humidity = "";
String windSpeed = "";
String winddeg = "";
String weatherMain;
String weatherDescription;

String userCity = "London";
String userCountry = "GB";

// ILI9341 object
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

unsigned long lastFetchTime = 0;
const unsigned long fetchInterval = 10000; // 10 seconds
void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  // Connect to Wi-Fi
  connectToWiFi();

  // Initialize ILI9341 display
  initializeDisplay();
  displayLogo();
  pinMode(buttonPinA, INPUT_PULLUP);
  pinMode(buttonPinB, INPUT_PULLUP);
  lastButtonStateA = digitalRead(buttonPinA);
  lastButtonStateB = digitalRead(buttonPinB);
}

void loop() {
  int buttonStateA = digitalRead(buttonPinA);
  int buttonStateB = digitalRead(buttonPinB);
  // Check if button A is pressed
  if (buttonStateA == LOW && lastButtonStateA == HIGH) {
    fetchAndDisplayInfo();
  }

  // Check if button B is pressed
  if (buttonStateB == LOW && lastButtonStateB == HIGH) {
    // Get user input for city and country
    getUserInput();
    fetchAndDisplayInfo_Globle();
  }

  lastButtonStateA = buttonStateA;
  lastButtonStateB = buttonStateB;
}
void displayLogo() {
  tft.fillScreen(ILI9341_BLACK);
  tft.drawRGBBitmap(0, 0, logo, 320, 240);
  delay(3000);  // Adjust the delay time as needed
}
void getUserInput() {
   Serial.println("Enter Country:");
  while (!Serial.available()) {
    delay(100);
  }
  userCountry = Serial.readStringUntil('\n');

  Serial.println("Enter City:");
  while (!Serial.available()) {
    delay(100);
  }
  userCity = Serial.readStringUntil('\n');

  // Display entered values (optional)
  Serial.print("Entered Country: ");
  Serial.println(userCountry);
  Serial.print("Entered City: ");
  Serial.println(userCity);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void initializeDisplay() {
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
}

void fetchAndDisplayInfo() {
  // Get IPinfo data
  String ipinfoData = getIPinfoData();

  // Parse IPinfo JSON data
  parseIPinfoData(ipinfoData);

  // Get weather data
  String weatherData = getWeatherData();

  // Parse weather JSON data
  parseWeatherData(weatherData);

  // Display information on ILI9341
  displayInfo();
}

void fetchAndDisplayInfo_Globle() {
  // Update city and country based on user input
  city = userCity;
  country = userCountry;

  // Get weather data
  String weatherData = getWeatherData();

  // Parse weather JSON data
  parseWeatherData(weatherData);

  // Display information on ILI9341
  displayInfo_Globle();
}

void displayInfo_Globle() {
  tft.fillScreen(ILI9341_BLACK); // Clear the screen

  displayText("Country", country, 10);
  displayText("City", city, 30);
  displayText("Temperature", temperature + " C", 50);
  displayText("Pressure", pressure + " hPa", 70);
  displayText("Humidity", humidity + " %", 90);
  displayText("Wind", windSpeed + " m/s, " + winddeg, 110);
  displayText("Weather", "\n " + weatherMain + ", " + weatherDescription, 130);
}

String getIPinfoData() {
  String url = "http://ipinfo.io/json?token=" + String(ipinfoApiKey);
  return fetchData(url, "IPinfo data");
}

void parseIPinfoData(String json) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, json);

  city = doc["city"].as<String>();
  country = doc["region"].as<String>();
  coordinates = doc["loc"].as<String>();
}

String getWeatherData() {
  String weatherUrl = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + country + "&units=metric&APPID=" + openWeatherMapApiKey;
  return fetchData(weatherUrl, "weather data");
}

void parseWeatherData(String json) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, json);

  temperature = doc["main"]["temp"].as<String>();
  pressure = doc["main"]["pressure"].as<String>();
  humidity = doc["main"]["humidity"].as<String>();
  windSpeed = doc["wind"]["speed"].as<String>();
  winddeg = doc["wind"]["deg"].as<String>();
  weatherMain = doc["weather"][0]["main"].as<String>();
  weatherDescription = doc["weather"][0]["description"].as<String>();

  // Remove double quotes around numerical values
  if (winddeg >= "337" || winddeg < "23") {
      winddeg = "N";  // North
  } else if (winddeg >= "23" && winddeg < "68") {
      winddeg = "NE";  // Northeast
  } else if (winddeg >= "68" && winddeg < "113") {
      winddeg = "E";  // East
  } else if (winddeg >= "113" && winddeg < "158") {
      winddeg = "SE";  // Southeast
  } else if (winddeg >= "158" && winddeg < "203") {
      winddeg = "S";  // South
  } else if (winddeg >= "203" && winddeg < "248") {
      winddeg = "SW";  // Southwest
  } else if (winddeg >= "248" && winddeg < "293") {
      winddeg = "W";  // West
  } else if (winddeg >= "293" && winddeg < "337") {
      winddeg = "NW";  // Northwest
  } else {
      winddeg = "Unknown";  // Invalid wind angle
  }
}

String fetchData(String url, const char* dataType) {
  HTTPClient http;

  Serial.print("Fetching ");
  Serial.print(dataType);
  Serial.print("...");

  http.begin(url);
  int httpCode = http.GET();
  String payload = "{}";  // Default to empty JSON in case of error

  if (httpCode == HTTP_CODE_OK) {
    payload = http.getString();
    Serial.println("Done");
  } else {
    Serial.println("Failed");
  }

  http.end();
  return payload;
}

void displayInfo() {
  tft.fillScreen(ILI9341_BLACK); // Clear the screen

  displayText("Country", country, 10);
  displayText("City", city, 30);
  displayText("lng,lat", coordinates, 50);
  displayText("Temperature", temperature + " C", 70);
  displayText("Pressure", pressure + " hPa", 90);
  displayText("Humidity", humidity + " %", 110);
  displayText("Wind", windSpeed  + " m/s , " + winddeg, 130);
  displayText("Weather", "\n "+ weatherMain + ", " + weatherDescription, 150);
}

void displayText(const char* label, String value, int yPos) {
  tft.setCursor(10, yPos);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2); // Adjust the text size
  tft.print(label + String(": ") + value);
}