#include <WiFi.h>
#include <HTTPClient.h>
#include <Ticker.h>
#include <Firebase_ESP_Client.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <pitches.h>
#include <SimpleDHT.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include <stdio.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <IRsend.h>

#define WIFI_SSID "ssid Y7"
#define WIFI_PASSWORD "password"

#define SERVER_URL "Server url"

#define API_KEY "Api_Key"
#define DATABASE_URL "db url"

#define LED1_PIN 25
#define LED2_PIN 26
#define LED3_PIN 27
#define FAN_PIN 23
#define DHT11_PIN 4
#define SERVO_PIN 13
#define RED_PIN 5
#define GREEN_PIN 18
#define BLUE_PIN 19
#define FLAME_SENSOR_PIN 33
#define WATER_LEVEL_SENSOR_PIN 32  // GPIO 32 is ADC1 Channel 4
#define BUZZER_PIN 15
#define BUTTON_PIN 14 

int melody[] = {
  NOTE_E7, NOTE_C7, NOTE_D7, NOTE_G6, REST,
  NOTE_G6, NOTE_D7, NOTE_E7, NOTE_C7
};

int durations[] = {
  2, 2, 2, 2, 1,
  2, 2, 2, 2
};

const uint16_t kIrLedPin = 12;
LiquidCrystal_I2C lcd(0x27, 16, 2);
SimpleDHT11 dht11;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Servo myservo;
Ticker ticker;
IRsend irsend(kIrLedPin);


bool requestInProgress = false;
bool light2Switch = false;
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

uint64_t TV_ON_OFF = 0xE0E040BF;
uint64_t TV_VOL_UP = 0xE0E0E01F;
uint64_t TV_VOL_DOWN = 0xE0E0D02F;
uint64_t AC_OFF = 0xB27BE0;
uint64_t AC_ON = 0xB2BF40;
uint64_t AC_TEMP_UP = 0xB2BFC0;
uint64_t AC_TEMP_DOWN = 0xB2BF50;

String command = "";

void setup() {
  Serial.begin(9600);
  
  pinMode(2, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(WATER_LEVEL_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(FLAME_SENSOR_PIN, INPUT);

  // Set initial LED color to off
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, 0);

  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);    // Standard 50Hz servo
  myservo.attach(SERVO_PIN, 500, 2400);

  irsend.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

  //Connect to Wi-Fi
  connectWiFi(WIFI_SSID,WIFI_PASSWORD); 
  //setup connection to firebase db
  setupFirebase();
  
  // Schedule the check for sending HTTP request every 10 seconds
  ticker.attach(10, checkAndSendHttpRequest);
  
}

void loop() {
  //door(getSwitchValue("Door1"));  //opens and closes the door
  //ringDoor();
  //setTempHum();  //gets the temp and humididty and sends them to the app
  //light_RGB();  //controll RGB light
  checkCommand();
  if(getSwitchValue("Living Room1")){
    digitalWrite(LED1_PIN , HIGH);
  }
  else{
    digitalWrite(LED1_PIN , LOW);
  }
  if(getSwitchValue("Kitchen2")){
    digitalWrite(LED2_PIN , HIGH);
  }
  else{
    digitalWrite(LED2_PIN , LOW);
  }
  if(getSwitchValue("Bedroom3")){
    digitalWrite(LED3_PIN , HIGH);
  }
  else{
    digitalWrite(LED3_PIN , LOW);
  }
  if(getSwitchValue("Living Room Fan1")){
    digitalWrite(FAN_PIN , HIGH);
  }
  else{
    digitalWrite(FAN_PIN , LOW);
  }
  //rainDetected();
  //fireDetected();
  //IRsend("TV");
  //IRsend("AC");
}


void ringDoor(){
  int pinValue = digitalRead(BUTTON_PIN);
  if(pinValue == 0){
    myservo.detach();
    for (int note = 0; note < 9; note++) {
    int duration = 1000/durations[note];
    tone(BUZZER_PIN, melody[note], duration);
    int pauseBetweenNotes = duration * 1.00;
    delay(pauseBetweenNotes);
    noTone(BUZZER_PIN);
    }
  }
}

void setupFirebase(){
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  //Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

//function to connect to WiFi
void connectWiFi(String ssid, String password){
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to WiFi, IP address: ");
  Serial.println(WiFi.localIP());
}

//function to check the value of a switch (Light, Fans, Door, etc)     DONE
bool getSwitchValue(String Name) {
  String path = Name + "/Switch";
  if (Firebase.RTDB.getBool(&fbdo, path)) {
    return fbdo.boolData();
  } else {
    Serial.println("Failed to get value for " + Name + "/Switch");
    Serial.println("REASON: " + fbdo.errorReason());
    return false;
  }
}

//function to set the value of a light switch (it is used with vouce commands)    DONE
void setLightSwitch(String lightName, bool value) {
  String path = lightName + "/Switch";

  // Write the value
  if (Firebase.RTDB.setBool(&fbdo, path, value)) {
    Serial.println(lightName + "/Switch set successfully to " + String(value));
  } else {
    Serial.println("Failed to set " + lightName + "/Switch");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

//function to read temp and humidity and set them in the database and display them on the lcd screen     DONE
void setTempHum(){
   // read with raw sample data.
  byte temperature = 0;
  byte humidity = 0;
  byte data[40] = {0};
  if (dht11.read(DHT11_PIN, &temperature, &humidity, data)) {
    Serial.print("Read DHT11 failed");
    return;
  }
  
  Serial.print("Sample RAW Bits: ");
  for (int i = 0; i < 40; i++) {
    Serial.print((int)data[i]);
    if (i > 0 && ((i + 1) % 4) == 0) {
      Serial.print(' ');
    }
  }
  Serial.println("");
  
  Serial.print("Sample OK: ");
  Serial.print((int)temperature); Serial.print(" *C, "); 
  Serial.print((int)humidity); Serial.println(" %");
  
  if (Firebase.RTDB.setInt(&fbdo, "Temp", (int)temperature)) {
    Serial.println("Temperature set successfully");
  } else {
    Serial.println("Failed to set temperature");
    Serial.println("REASON: " + fbdo.errorReason());
  }

  if (Firebase.RTDB.setInt(&fbdo, "Hum", (int)humidity)) {
    Serial.println("Humidity set successfully");
  } else {
    Serial.println("Failed to set humidity");
    Serial.println("REASON: " + fbdo.errorReason());
  }

  lcd.clear(); // Clear the previous display
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print((int)temperature);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Humid: ");
  lcd.print((int)humidity);
  lcd.print("%");
}

// Function to convert hex color string to RGB values and set LED     DONE
void setColorFromHex(String hex) {
  // Ensure the hex string starts with #
  if (hex.startsWith("#")) {
    hex.remove(0, 1);
  }

  // Convert hex string to integer
  long number = strtol(hex.c_str(), NULL, 16);

  // Extract RGB values
  int r = (number >> 16) & 0xFF;
  int g = (number >> 8) & 0xFF;
  int b = number & 0xFF;

  // Set the RGB LED color
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

// Function to fetch the color value from Firebase and set the RGB LED     DONE
void light_RGB() {
  if (Firebase.RTDB.getString(&fbdo, "color")) {
    String colorHex = fbdo.stringData();
    Serial.println("Color fetched from Firebase: " + colorHex);
    setColorFromHex(colorHex);
  } else {
    Serial.println("Failed to fetch color from Firebase");
  }
}

//Function to handle opening and closing the door      DONE
void door(bool state){
  myservo.attach(SERVO_PIN, 500, 2400);
  if (state) {
    myservo.write(0);
  }
  else{
    myservo.write(180);
  }
}

//Finction to send notification if rain, fire is detected
void setFCMNotifier(String keyName, bool value) {
  String path = keyName; // Assuming the key in the database is named "fire"
  
  // Write the value to the specified key
  if (Firebase.RTDB.setBool(&fbdo, path, value)) {
    Serial.println("Key " + keyName + "set successfully to " + String(value));
  } else {
    Serial.println("Failed to set key " + keyName + "/fire");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

//Function do detect fire
void fireDetected(){
  int flame_state = digitalRead(FLAME_SENSOR_PIN);
  Serial.println(flame_state);
  delay(1000);

  if (flame_state == HIGH){
    Serial.println("No flame dected => The fire is NOT detected");
    digitalWrite(2,LOW);
    //setFCMNotifier("Fire" , false);
  }
  else{
    myservo.detach();
    Serial.println("Flame dected => The fire is detected");
    tone(BUZZER_PIN, 1500, 2000);
    setFCMNotifier("Fire" , true);
  }
}

//Function do detect rain
void rainDetected(){
  int sensorValue = analogRead(WATER_LEVEL_SENSOR_PIN);

  Serial.print("Sensor Value: ");
  Serial.println(sensorValue);

  if (sensorValue > 800) {
    Serial.println("Water detected!");
    setFCMNotifier("Rain" , true);
  } else {
    Serial.println("No water detected.");
    //setFCMNotifier("Rain" , false);
  }
}

//Function to send IR signal 
void checkSingaltoSend(){}

//Function to check the state of the request
void checkAndSendHttpRequest() {
  if (!requestInProgress) {
    Serial.println("sending request");
    sendHttpRequest();
  }
}

//Function to send http request for the voice commands api
void sendHttpRequest() {
  if (WiFi.status() == WL_CONNECTED) { // Check WiFi connection status
    requestInProgress = true;
    HTTPClient http;

    // Specify the URL and set timeout
    http.begin(SERVER_URL);
    http.setTimeout(2000000); // Set timeout to 10 seconds

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    // Check the returning code
    if (httpResponseCode > 0) {
      String response = http.getString();
      
      // Remove the quotation marks from the response
      response.replace("\"", "");
      
      Serial.println(httpResponseCode); // Print HTTP return code
      Serial.println(response);
      command = response;
      Serial.println(command);
    } else {
      Serial.print("Error on sending GET request: ");
      Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
    requestInProgress = false;
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void checkCommand(){
  if(command == "living room light on"){
    setLightSwitch("Living Room1", true);
    command = "";
  }
  else if(command == "living room light off"){
    setLightSwitch("Living Room1", false);
    command = "";
  }
  else if(command == "kitchen light on"){
    setLightSwitch("Kitchen2", true);
    command = "";
  } 
  else if(command == "kitchen light off"){
    setLightSwitch("Kitchen2", false);
    command = "";
  } 
  else if(command == "fan on"){
    setLightSwitch("Living Room Fan1", true);
    command = "";
  }
  else if(command == "fan off" || command == "fan of" || command == "turn off"){
    setLightSwitch("Living Room Fan1", false);
    command = "";
  }
  else if(command == "kitchen light off"){
    setLightSwitch("Kitchen2", false);
    command = "";
  } 
}

void IRsend(String device){
  String state = device + "/state";
  String up = device + "/Up";
  String down = device + "/Down";
  if (Firebase.RTDB.getBool(&fbdo, state)) {
    if(fbdo.boolData()){
      if(device == "TV"){
        irsend.sendSAMSUNG(TV_ON_OFF);
      }
      else if(device == "AC"){
        irsend.sendCOOLIX(AC_ON);
      }
      Firebase.RTDB.setBool(&fbdo, state, false);
    } 
  } 
  if (Firebase.RTDB.getBool(&fbdo, up)) {
    if(fbdo.boolData()){
      if(device == "TV"){
        irsend.sendSAMSUNG(TV_VOL_UP);
      }
      else if(device == "AC"){
        irsend.sendCOOLIX(AC_TEMP_UP);
      }
      Firebase.RTDB.setBool(&fbdo, up, false);
    } 
  } 
  if (Firebase.RTDB.getBool(&fbdo, down)) {
    if(fbdo.boolData()){
      if(device == "TV"){
        irsend.sendSAMSUNG(TV_VOL_DOWN);
      }
      else if(device == "AC"){
        irsend.sendCOOLIX(AC_TEMP_DOWN);
      }
      Firebase.RTDB.setBool(&fbdo, down, false);
    } 
  } 
}
