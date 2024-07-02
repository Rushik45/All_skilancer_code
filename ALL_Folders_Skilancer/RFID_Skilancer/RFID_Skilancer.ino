#include <WiFi.h>
#include <HTTPClient.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Stepper.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
int stateBit = 0;
const int ledpin = 27;
const int ledpin2 = 26;
const char *ssid = "skilancer12";
const char *pass = "solarpolar12";
// const char *ssid = "Galaxy A10s7521";
// const char *pass = "mwvs8748";
String GOOGLE_SCRIPT_ID = "AKfycbxeT-VzORpk0Ag0kddToXruB5a3VrR8E4CHC_saXcPy7rW1tDCFAa4spK_VCLWex10J";
const int stepsPerRevolution = 4098;
#define IN1 4
#define IN2 2
#define IN3 15
#define IN4 13
#define SS_PIN 21
#define RST_PIN 22
const int sendInterval = 50;
WiFiClientSecure client;
void write_google_sheet(String params);
MFRC522 mfrc522(SS_PIN, RST_PIN);
const String authorizedUIDs[] = {"D3 81 E1 11", "73 0C 5D FA"};
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

void setup() 
{
  myStepper.setSpeed(4);
  Serial.begin(115200);
  delay(500);
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
    Serial.println("Connecting...");
  }

  
  Serial.println("WiFi connected");
  SPI.begin();
  mfrc522.PCD_Init();
}
  

 


void loop() 
{
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  // Show UID on serial monitor
  Serial.print("UID tag :");
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
    //  write_google_sheet("value1="+String("LockOpen"));
    //  delay(1000);
    //  mfrc522.PICC_HaltA();
    //  mfrc522.PCD_StopCrypto1();
  }
  Serial.println();

  content.toUpperCase();

  bool isAuthorized = false;
  for (const String& uid : authorizedUIDs)
  {
    if (content.substring(1) == uid)
    {
      isAuthorized = true;
      // write_google_sheet("value1="+String("LockOpen") + "&value2="+String("Authorizedaccess"));
      // delay(1000);
      break;
    }
  }


  if (isAuthorized && stateBit == 0) 
  {
    write_google_sheet("value1="+String("LockOpen") + "&value2="+String("Authorizedaccess"));
    digitalWrite(ledpin, HIGH);
    digitalWrite(ledpin2, LOW);
    myStepper.step(stepsPerRevolution);
    stateBit = 1;
    Serial.println("Authorized access");

  }
  else if (isAuthorized && stateBit == 1) 
  {
    digitalWrite(ledpin, HIGH);
    digitalWrite(ledpin2, LOW);
    stateBit = 0;
    Serial.println("Authorized access");
 
  }
  else 
  {
    write_google_sheet("value1="+String("LockClosed") + "&value2="+String("AccessDenied"));
    digitalWrite(ledpin2, HIGH);
    digitalWrite(ledpin, LOW);
    Serial.println("Access Denied");
   
  }
  Serial.println();
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

