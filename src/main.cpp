#include<Arduino.h>
#include "DHT.h"
#include "thingProperties.h"
#include <Preferences.h>
#include <time.h>
#include <WiFiClientSecure.h>
#include <ESP_Mail_Client.h>

const char* SMTP_HOST     = "smtp.gmail.com";
const int   SMTP_PORT     = 465;
const char* EMAIL_SENDER  = "raduciurescu75@gmail.com";
const char* SENDER_PASS   = "";
const char* EMAIL_RECIPIENT = "podean.beni@gmail.com";
SMTPSession smtp;

#define DHTPIN 26      
#define DHTTYPE DHT22   

DHT dht(DHTPIN, DHTTYPE);
Preferences prefs;
Preferences humPrefs; 
// Obiecte globale pentru sesiune Åi mesaj


void smtpCallback(SMTP_Status status) {
  Serial.println(status.info());
  if (status.success()) {
    Serial.println("Email trimis cu succes.");
    smtp.sendingResult.clear();
  } else {
    Serial.println("Trimitere eÈuatÄ.");
  }
}

void sendHighHumidityEmail(float humidity) {
  smtp.debug(1);  // ActiveazÄ loguri detaliate
  smtp.callback(smtpCallback);  // SeteazÄ funcÈia de callback

  Session_Config config;
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = EMAIL_SENDER;
  config.login.password = SENDER_PASS;
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

  SMTP_Message message;
  message.sender.name = F("ESP32 Alert");
  message.sender.email = EMAIL_SENDER;
  message.subject = F("ð¨ AlertÄ umiditate ridicatÄ!");
  message.addRecipient(F("Utilizator"), EMAIL_RECIPIENT);

  String body = "AtenÈie! Umiditatea a ajuns la " 
                + String(humidity, 1) + "%\r\n"
                + "VerificÄ imediat condiÈiile din Ã®ncÄpere.";
  message.text.content = body.c_str();
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&config)) {
    Serial.printf("Eroare conectare SMTP: %d, %s\n", smtp.statusCode(), smtp.errorReason().c_str());
    return;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.printf("Eroare trimitere email: %d, %s\n", smtp.statusCode(), smtp.errorReason().c_str());
  }
}


void displayStoredHumidityEvents() {
  uint8_t nextIdx = humPrefs.getUInt("hum_index", 0);
  Serial.println(F("=== Evenimente umiditate â¥75% ==="));
  for (uint8_t i = 0; i < 10; i++) {
    uint8_t idx = (nextIdx + i) % 10;

    // cheile pentru valoare Èi timestamp
    char keyVal[12], keyTs[12];
    snprintf(keyVal, sizeof(keyVal), "humVal%u", idx);
    snprintf(keyTs,  sizeof(keyTs),  "humTs%u",  idx);

    float h = humPrefs.getFloat(keyVal, -1.0f);
    unsigned long ts = humPrefs.getUInt(keyTs, 0);

    if (ts == 0) {
      Serial.printf("%2u: <gol>\n", i + 1);
    } else {
      // converteÈte Unix timestamp Ã®n datÄ/ora localÄ
      time_t t = (time_t)ts;
      struct tm *tm_info = localtime(&t);
      char buf[26];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
      Serial.printf("%2u: %.1f%% la %s\n", i + 1, h, buf);
    }
  }
  Serial.println(F("=== sfÃ¢rÈit evenimente ==="));
}


void displayStoredMessages() {

  // ObÈine indexul de scriere (urmÄtoarea poziÈie)
  uint8_t nextIdx = prefs.getUInt("msg_index", 0);

  // Cel mai vechi este chiar la nextIdx
  for (uint8_t i = 0; i < 10; i++) {
    uint8_t idx = (nextIdx + i) % 10;
    char key[6];
    snprintf(key, sizeof(key), "msg%u", idx);
    String msg = prefs.getString(key, "<gol>");
    Serial.printf("%2u: %s\n", i + 1, msg.c_str());
  }

  Serial.println(F("=== sfÃ¢rÈit mesaje ==="));
}

// Interval de afiÅare a mesajelor (3s)
const unsigned long PRINT_INTERVAL = 3000;
unsigned long lastPrint = 0;

void setup() {
  Serial.begin(115200);
  pinMode(2,OUTPUT);
  digitalWrite(2, LOW); 
  led=false;

 // IniÅ£ializeazÄ NVS cu namespace "msg_store"
  if (!prefs.begin("msg_store", false)) {
    Serial.println("!!! Eroare la iniÅ£ializarea NVS");
  } else {
    Serial.println("NVS init OK");
  }
   if (!humPrefs.begin("hum_store", false))
    Serial.println("!!! Eroare init NVS hum_store");
  else
    Serial.println("NVS hum_store OK");

  // 3) Sincronizare NTP (pentru timestamp corect)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  Serial.println("Pornire senzor DHT22...");
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  dht.begin();
}
float humidity=0;
// alerta pt mail
bool alert90Sent = false;

void loop() {

  unsigned long now = millis();
  // Citim umiditatea Èi temperatura
  ArduinoCloud.update();
  humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Celsius

  // VerificÄm dacÄ citirea a reuÈit
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Eroare la citirea de la senzorul DHT22!");
    delay(2000);
  }

  // AfiÈÄm valorile
  umiditate=humidity;
  temperatura=temperature;
  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.print(" Â°C | Umiditate: ");
  Serial.print(humidity);
  Serial.println(" %");

  if(humidity>75)
  {
    
  uint8_t idx = humPrefs.getUInt("hum_index", 0);
  char keyVal[12], keyTs[12];
  snprintf(keyVal, sizeof(keyVal), "humVal%u", idx);
  snprintf(keyTs,  sizeof(keyTs),  "humTs%u",  idx);

  humPrefs.putFloat(keyVal, umiditate);
  unsigned long ts = (unsigned long)time(nullptr);
  humPrefs.putUInt(keyTs, ts);
  humPrefs.putUInt("hum_index", (idx + 1) % 10);
  }
  
  if (humidity > 90.0 && !alert90Sent) {
    sendHighHumidityEmail(humidity);
    alert90Sent = true;
  } else if (humidity <= 90.0) {
    alert90Sent = false;
  }


   // 3) AfiÅeazÄ stocarea mesaje la fiecare 3s
  if (now - lastPrint >= PRINT_INTERVAL) {
    displayStoredMessages();
    displayStoredHumidityEvents();
    lastPrint = now;
  }

  delay(2000); 
}
/*
  Since Messenger is READ_WRITE variable, onMessengerChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onMessengerChange()  {
   Serial.print("onMessengerChange(): â");
  Serial.print(messenger);
  Serial.println("â");

  // CiteÈte indexul curent (0â9)
  uint8_t idx = prefs.getUInt("msg_index", 0);

  // Scrie mesajul la cheia "msg0"..."msg9"
  char key[6];
  snprintf(key, sizeof(key), "msg%u", idx);
  prefs.putString(key, messenger);
  Serial.printf("â Stocat la %s: %s\n", key, messenger.c_str());

  // Increment circular
  idx = (idx + 1) % 10;
  prefs.putUInt("msg_index", idx);


  
}
/*
  Since Led is READ_WRITE variable, onLedChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onLedChange()  {
  if(led==true)
   digitalWrite(2,HIGH);
  else
    digitalWrite(2,LOW);
}
/*
  Since Pwn is READ_WRITE variable, onPwnChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onPwnChange()  {
  // Add your code here to act upon Pwn change
}
/*
  Since Umiditate is READ_WRITE variable, onUmiditateChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onUmiditateChange()  {
}
/*
  Since Temperatura is READ_WRITE variable, onTemperaturaChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onTemperaturaChange()  {
  // Add your code here to act upon Temperatura change
}