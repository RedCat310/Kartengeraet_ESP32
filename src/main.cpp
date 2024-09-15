#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PubSubClient.h>
#include <Keypad.h>
#include <Keypad_I2C.h>
#include "Adafruit_Thermal.h"

#define NOTE_A5  880

#define SS_PIN  5  // ESP32 pin GPIO5 
#define RST_PIN 27 // ESP32 pin GPIO27 
#define MSG_BUFFER_SIZE	(50)
#define I2CADDR  0x20
#define RXD2 16
#define TXD2 17

char msg[MSG_BUFFER_SIZE];
int value = 0;
float price;
int step = 0;
float wifiw = 0;
unsigned long lastMsg = 0;
const char* mqtt_server = "test.mosquitto.org";
bool scan = false;
const int buzzer = 4;

String tags[] = {"036d7994", "4375d194", "437abe94", "", "3172ce2d"};

const char* ssid = "Einigkeit und Recht und Wifi";
const char* password = "Dilssw09dnjh##**++";
// const char* ssid = "BotNet4.0";
// const char* password = "ZXLZfq+n$xQvru!@?#xB";
char customKey;
int numbers = 0;
int user;
bool correct;



const char PINLENGTH = 4;
char keyBuffer[PINLENGTH+1] = {'-','-','-','-'};
char pinCode0[PINLENGTH+1] = {'0','2','2','6'};
char pinCode1[PINLENGTH+1] = {'7','2','6','9'};
char pinCode2[PINLENGTH+1] = {'2','8','1','4'};
char pinCode3[PINLENGTH+1] = {'1','4','4','7'};
char pinCode4[PINLENGTH+1] = {'8','4','7','6'};


Adafruit_Thermal printer(&Serial2);
LiquidCrystal_I2C lcd(0x27, 20, 4);
WiFiClient espClient;
MFRC522 rfid(SS_PIN, RST_PIN);
PubSubClient client(espClient);
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'D','#','0','*'},
  {'C','9','8','7'},
  {'B','6','5','4'},
  {'A','3','2','1'}
};
byte rowPins[ROWS] = {3, 2, 1, 0}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {7, 6, 5, 4}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad_I2C customKeypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR); 


String readHex(byte *buffer, byte bufferSize) {
  String result = "";
  for (byte i = 0; i < bufferSize; i++) {
    String* hex[] = {};
    result = result + String(buffer[i] < 0x10 ? " 0" : "");
    result = result + String(buffer[i], HEX);
  }
  return result;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32+jajhfglkjhdslkjghaldfkjvklJSDH$ZU§U";
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("kartengeraet2", "connected");
      // ... and resubscribe
      client.subscribe("kartengeraet");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setupWifi() {
  pinMode(26, OUTPUT);
  // Serial.println("scan start");

  // // WiFi.scanNetworks will return the number of networks found
  // int n = WiFi.scanNetworks();
  // Serial.println("scan done");
  // if (n == 0) {
  //     Serial.println("no networks found");
  // } else {
  //   Serial.print(n);
  //   Serial.println(" networks found");
  //   for (int i = 0; i < n; ++i) {
  //     // Print SSID and RSSI for each network found
  //     Serial.print(i + 1);
  //     Serial.print(": ");
  //     Serial.print(WiFi.SSID(i));
  //     Serial.print(" (");
  //     Serial.print(WiFi.RSSI(i));
  //     Serial.print(")");
  //     Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
  //     delay(10);
  //   }
  // }
  WiFi.disconnect();
  Serial.println("");
  lcd.clear();
  lcd.print("Zum WLAN verbinden:");
  lcd.setCursor(0,1);
  // We start by connecting to a WiFi network
  lcd.print(ssid);
  Serial.println();
  Serial.println("Connecting to ");
  Serial.print(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lcd.setCursor(0,2);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
  }

  lcd.clear();
  lcd.print("WLAN verbunden");
  lcd.setCursor(0,1);
  lcd.print("IP-Adresse: ");
  lcd.setCursor(0,2);
  lcd.println(WiFi.localIP());
  delay(2000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println();
  Serial.println(message);
  if(step == 0 && message == "open"){
    digitalWrite(26, HIGH);
    delay(1000);
    digitalWrite(26, LOW);
  }
  if(step == 0 && message == "new"){
    step = 1;
    lcd.clear();
    lcd.print("Neuer Auftrag");
    lcd.setCursor(0,1);
    lcd.print("Betrag: ");
  }else if(step == 1){
    price = message.toFloat();
    lcd.print(price);
    step = 2;
    scan = true;
    delay(1000);
    lcd.setCursor(0, 2);
    lcd.print("Auf Karte Warten...");
    lcd.setCursor(0, 1);
  }else if(step == 0 && message == "print"){
    step = 10;
    printer.setSize('L');
    printer.println("Kleiner Laden");
    printer.setSize('S');
    printer.feed(1);
  }else if(step == 10){
    step = 11;
    printer.print("Es bedient sie: ");
    printer.println(message);
    printer.feed(2);
  }else if(step == 11 && message == "next"){
    printer.feed(1);
    printer.print("Gesamtbetrag:");
    printer.tab();
    step = 13;
  }else if(step == 11){
    printer.print(message);
    printer.tab();
    step = 12;
  }else if(step == 12){
    step = 11;
    printer.println(message);
  }else if(step == 13){
    printer.println(message);
    step = 14;
  }else if(step == 14 && message == "bar"){
    printer.feed(1);
    printer.println("Mit bargeld Bezahlt:");
    printer.print("Gegeben:");
    printer.tab();
    step = 15;
  } else if(step == 14){
    printer.feed(1);
    printer.println("Mit Kreditkarte Bezahlt:");
    printer.println(message);
    printer.feed(2);
    printer.setSize('L');
    printer.println("Vielen Dank fuer ihren Einkauf");
    printer.setSize('S');
    printer.feed(4);
    step = 0;
  }else if(step == 15){
    printer.println(message);
    step = 16;
    printer.print("Rueckgeld:");
    printer.tab();
  }else if(step == 16){
    printer.println(message);
    printer.feed(2);
    printer.setSize('L');
    printer.println("Vielen Dank fuer ihren Einkauf");
    printer.setSize('S');
    printer.feed(4);
    step = 0;
  }
}

void addToKeyBuffer(char inkey) { 
  Serial.print(inkey);
  Serial.print(" > ");
  numbers++;
  // Von links nach rechts Zeichen um eins weiterkopieren = ältestes Zeichen vergessen
  for (int i=1; i<PINLENGTH; i++) {
    keyBuffer[i-1] = keyBuffer[i];
  }
  // in ganz rechter Stelle die letzte Ziffer speichern
  keyBuffer[PINLENGTH-1] = inkey;
  Serial.println(keyBuffer);
 }
bool checkKey(char pinCode[PINLENGTH +1]) {
  // Eingabe mit festgelegtem Pincode vergleichen
  if (strcmp(keyBuffer, pinCode) == 0) {
    Serial.println("OK");
    return true;
    // Aktion für richtigen Pin ausführen
  }
  else {
    Serial.println("ne");
    return false;
  }
 
  // Nach Überprüfung Eingabe leeren
  for (int i=0; i < PINLENGTH; i++) { 
    keyBuffer[i] = '-'; 
  }
 }

void setup() {
  Wire.begin( );                // GDY200622
  customKeypad.begin( ); 
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(4, 1);
  lcd.print("Kartengeraet");
  lcd.setCursor(7, 2);
  lcd.print("LKasse");
  delay(1000);
  SPI.begin();
  rfid.PCD_Init(); 
  setupWifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print("Kartengeraet");
  lcd.setCursor(7, 2);
  lcd.print("LKasse");
  lcd.setCursor(0, 3);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  for(int i = 1; i < 20; i++){
    lcd.print(".");
    delay(random(100, 200));
  }
  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print("Kartengeraet");
  lcd.setCursor(7, 2);
  lcd.print("LKasse");
  lcd.setCursor(0,3);
  lcd.print("Ready...");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if(step == 2){
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
    byte block;
    byte len;
    MFRC522::StatusCode status;
    if ( ! rfid.PICC_IsNewCardPresent()) {
      return;
    }
    if ( ! rfid.PICC_ReadCardSerial()) {
      return;
    }
    tone(buzzer, NOTE_A5, 100);
    delay(100);
    noTone(buzzer);
    Serial.println(F("**Card Detected:**"));
    String nuid = readHex(rfid.uid.uidByte, rfid.uid.size);
    rfid.PICC_DumpDetailsToSerial(&(rfid.uid));
    Serial.println(F("\n**End Reading**\n"));
    Serial.println(nuid);
    if(nuid == tags[0]){
      lcd.setCursor(0, 2);
      lcd.print("Karte 1/2 erkannt   ");
      delay(2000);
      lcd.clear();
      lcd.print("Bitte PIN von");
      lcd.setCursor(0,1);
      lcd.print("Karte 1/2 eingeben:");
      lcd.setCursor(3,2);
      lcd.print("____");
      lcd.setCursor(0,3);
      lcd.print(" C = Loeschen");
      lcd.setCursor(3,2);
      step = 3;
      user = 0;
      client.publish("kartengeraet2", "0");
    }else if(nuid == tags[1]){
      lcd.setCursor(0, 2);
      lcd.print("Karte 2/2 erkannt   ");
      delay(2000);
      lcd.clear();
      lcd.print("Bitte PIN von");
      lcd.setCursor(0,1);
      lcd.print("Karte 2/2 eingeben:");
      lcd.setCursor(3,2);
      lcd.print("____");
      lcd.setCursor(0,3);
      lcd.print(" C = Loeschen");
      lcd.setCursor(3,2);
      step = 3;
      user = 1;
      client.publish("kartengeraet2", "1");
    }else if(nuid == tags[2]){
      lcd.setCursor(0, 2);
      lcd.print("Karte 3/2 erkannt   ");
      delay(2000);
      lcd.clear();
      lcd.print("Bitte PIN von");
      lcd.setCursor(0,1);
      lcd.print("Karte 3/2 eingeben:");
      lcd.setCursor(3,2);
      lcd.print("____");
      lcd.setCursor(0,3);
      lcd.print(" C = Loeschen");
      lcd.setCursor(3,2);
      step = 3;
      user = 2;
      client.publish("kartengeraet2", "2");
    }
    // else if(nuid == tags[3]){
      
    // }
    else if(nuid == tags[4]){
      lcd.setCursor(0, 2);
      lcd.print("Karte 5/2 erkannt   ");
      delay(2000);
      lcd.clear();
      lcd.print("Bitte PIN von");
      lcd.setCursor(0,1);
      lcd.print("Karte 5/2 eingeben:");
      lcd.setCursor(3,2);
      lcd.print("____");
      lcd.setCursor(0,3);
      lcd.print(" C = Loeschen");
      lcd.setCursor(3,2);
      step = 3;
      user = 4;
      client.publish("kartengeraet2", "4");
    }else{
      lcd.setCursor(0,3);
      lcd.print("Karte abgelehnt");
      delay(1000);
    }
  }
  if(step == 3){
    char customKey = customKeypad.getKey();
    if (customKey != NO_KEY) {

      if ((int(customKey) >= 48) && (int(customKey) <= 57)){ 
        addToKeyBuffer(customKey);
        lcd.print("*");
      }else{
        Serial.println(customKey);
      }
      if(customKey == 'D'){
        switch(user){
          case 0: correct = checkKey(pinCode0); break;
          case 1: correct = checkKey(pinCode1); break;
          case 2: correct = checkKey(pinCode2); break;
          case 3: correct = checkKey(pinCode3); break;
          case 4: correct = checkKey(pinCode4); break;
        }
        if(correct){
          lcd.setCursor(3,2);
          lcd.print("PIN richtig");
          step = 4;
          client.publish("kartengeraet2", "correct");
        }else{
          client.publish("kartengeraet2", "wrong");
          lcd.setCursor(3,2);
          lcd.print("PIN Falsch, bitte");
          lcd.setCursor(0,3);
          lcd.print("erneut versuchen    ");
          delay(3000);
          lcd.setCursor(3,2);
          lcd.print("____               ");
          lcd.setCursor(0,3);
          lcd.print("C = Loeschen        ");
          lcd.setCursor(3,2);
        }
      } 
      if(customKey == 'C'){
          for (int i=0; i < PINLENGTH; i++) { 
            keyBuffer[i] = '-'; 
          }
          Serial.println("DELETE");
          lcd.setCursor(3,2);
          lcd.print("____");
          lcd.setCursor(3,2);
        }
    }
  }
  if(step == 4){
    lcd.clear();
    lcd.print("Daten Austauschen");
    lcd.setCursor(0,1);
    for(int i = 0; i<20; i++){
      lcd.print(".");
      delay(random(100, 200));
    }
    lcd.setCursor(0,2);
    lcd.print("Transaktion beenden");
    for(int i = 0; i<20; i++){
      lcd.print(".");
      delay(random(100, 200));
    }
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("   Alles Erledigt!");
    client.publish("kartengeraet2", "done");
    delay(3000);
    lcd.clear();
    lcd.setCursor(4, 1);
    lcd.print("Kartengeraet");
    lcd.setCursor(7, 2);
    lcd.print("LKasse");
    lcd.setCursor(0,3);
    lcd.print("Ready...");
    step = 0;

  }
  client.loop();
}
