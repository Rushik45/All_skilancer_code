#include <WiFi.h>
#include <HTTPClient.h>
#include <MFRC522.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <TinyGPS++.h>
int stateBit = 0;
#define Wifi_led 13
#define connected_led 12
const int relaypin = 27;
#define relay_pin_relay 25
const int ledpin2 = 26;
const char *ssid = "skilancer10";
const char *pass = "solarpolar10";
String GOOGLE_SCRIPT_ID ="AKfycby7oec9HQ_P5JbxQrwVQZiECb6R1WXdotgb8qxWtZqLIUGAPLbgeaXk_ZurlgXXPhJM";
const int stepsPerRevolution = 4098;
#define SS_PIN 21
#define RST_PIN 22
const int sendInterval = 50;
WiFiClientSecure client;
void write_google_sheet(String params);
void update_google_sheet();
MFRC522 mfrc522(SS_PIN, RST_PIN);
const String authorizedUIDs[] = { "D3 81 E1 11", "73 0C 5D FA" };
#define RXD2 16
#define TXD2 17
HardwareSerial neogps(1);
TinyGPSPlus gps;
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 1 * 60 * 1000;
bool isAuthorized = false;

void setup(){
    Serial.begin(115200);
    neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
    pinMode(relaypin, OUTPUT);
    pinMode(Wifi_led, OUTPUT);
    pinMode(connected_led, OUTPUT);
    Serial.print("Connecting to Wi-Fi");
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(Wifi_led, HIGH);
        delay(500);
        digitalWrite(Wifi_led, LOW);
        delay(500);
        Serial.println(".");
        Serial.println("Connecting...");
    }
    digitalWrite(connected_led, HIGH);
    Serial.println("WiFi connected");
    SPI.begin();
    mfrc522.PCD_Init();
}

void loop() {
    if (!mfrc522.PICC_IsNewCardPresent()) {
        update_google_sheet();
        return;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
        update_google_sheet();
        return;
    }
    Serial.print("UID tag :");
    String content = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    Serial.println();

    content.toUpperCase();

    isAuthorized = false;
    for (const String &uid : authorizedUIDs) {
        if (content.substring(1) == uid) {
            isAuthorized = true;
            break;
        }
    }

    if (isAuthorized && stateBit == 0) {
        digitalWrite(relaypin, HIGH);
        delay(10);
        stateBit = 1;
        Serial.println("Authorized access");
        write_google_sheet("value1=" + String("IgnitionOFF") + "&value2=" + String("Authorizedaccess") + "&value3=" + String(gps.location.lng(),6) + "&value4=" + String(gps.location.lat(),6));
        delay(10);
        lastUpdateTime = millis(); // Reset the update timer
    } else if (isAuthorized && stateBit == 1) {
        digitalWrite(relaypin, LOW);
        delay(10);
        stateBit = 0;
        Serial.println("Authorized access");
        write_google_sheet("value1=" + String("IgnitionON") + "&value2=" + String("Authorizedaccess") + "&value3=" + String(gps.location.lng(),6) + "&value4=" + String(gps.location.lat(),6));
        delay(10);
        lastUpdateTime = millis(); // Reset the update timer
    } else {
        Serial.println("Access Denied");
        write_google_sheet("value1=" + String("IgnitionOFF") + "&value2=" + String("AccessDenied"));
        delay(1000);
        // update_google_sheet();
    }
    Serial.println();

    boolean newData = false;
    for (unsigned long start = millis(); millis() - start < 1000;) {
        while (neogps.available()) {
            if (gps.encode(neogps.read())) {
                newData = true;
            }
        }
    }

    if (newData) {
        newData = false;
        Serial.println(gps.satellites.value());
        print_speed();
    }
}

void update_google_sheet() {
    if (isAuthorized && (millis() - lastUpdateTime >= updateInterval)) {
        write_google_sheet("value1=" + String(stateBit == 0 ? "IgnitionON" :"") + "&value2=" + String("Authorizedaccess") + "&value3=" + String(gps.location.lng(),6) + "&value4=" + String(gps.location.lat(),6));
        lastUpdateTime = millis();
    }
    else if(isAuthorized && (millis() - lastUpdateTime >= updateInterval)){
      write_google_sheet("value1="+ String(stateBit == 1 ? "IgnitionOFF" :"") + "&value2="+String("Authorizedaccess")+ "&value3=" + String(gps.location.lng(),6) + "&value4=" +String(gps.location.lat(),6));
      lastUpdateTime = millis();
    }
}

void write_google_sheet(String params) {
    HTTPClient http;
    String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + params;
    Serial.println(url);
    http.begin(url.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();
    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);

    String payload;
    if (httpCode > 0) {
        payload = http.getString();
        Serial.println("Payload: " + payload);
    }
    http.end();
}

void print_speed() {
    if (gps.location.isValid()==1) {
        Serial.println(gps.speed.kmph());
    } else {
        Serial.println("No Data");
    }
}
