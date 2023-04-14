// WiFi ke Antares menggunakan protokol HTTP
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#define ACCESSKEY "ISI_ACCESSKEY_ANTARES_ANDA"
#define WIFISSID "ISI_NAMA_WIFI_ANDA"
#define PASSWORD "ISI_PASSWORD_WIFI_ANDA"
#define projectName "ISI_NAMA_APPLICATIONS_ANTARES_ANDA"
#define deviceName "ISI_NAMA_DEVICE_ANTARES_ANDA"
#define serverName "http://platform.antares.id:8080/~/antares-cse/antares-id/"+String(projectName)+"/"+String(deviceName)
WiFiClient client;
HTTPClient http;
String httpRequestData;
int httpResponseCode;

// Actuator dan Sensor
#define SW420_PIN D8 // Sensor Getar
#define BUZZER_PIN D3 // Buzzer
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);
#define SOLENOID_DOORLOCK_PIN D4
#include <SPI.h>
#include <MFRC522.h>
#define RST_PIN D1
#define SDA_PIN D2
MFRC522 mfrc522(SDA_PIN, RST_PIN);

// Variabel Global
int vibration; 
String alarm_keamanan;
String content = "";
byte letter;
String doorstate;
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

// Method untuk mengatur konektivitas
void ConnectToWiFi() {
  WiFi.mode(WIFI_STA); // Membuat perangkat sebagai station
  WiFi.begin(WIFISSID, PASSWORD); // Memulai menyambungkan ke jaringan
  Serial.print("Configuring to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nTelah terhubung ke "+String(WIFISSID)+"\n\n");
}

// Method untuk inisialisasi sensor RFID
void initSensorRFID(){
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Put your card to the reader...");
}

// Method untuk baca sensor
void bacaSensorGetar(){
  vibration = digitalRead(SW420_PIN);
}
void bacaSensorRFID(){
  if(!mfrc522.PICC_IsNewCardPresent()){ return; }
  if(!mfrc522.PICC_ReadCardSerial()){ return; }

  Serial.print("UID tag :");
  for(byte i = 0; i < mfrc522.uid.size; i++){
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
}

// Method untuk mengolah nilai sensor dan mengatur otomatisasi aktuator
void Olah_Data(){
  bacaSensorGetar(); // Memanggil method bacaSensor
  bacaSensorRFID(); // Memanggil method bacaSensorRFID
  if(vibration == HIGH){
    digitalWrite(BUZZER_PIN, HIGH);
    alarm_keamanan = "ON";
    Serial.println("Status buzzer = "+String(alarm_keamanan)+" - Danger"); 
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    alarm_keamanan = "OFF";
    Serial.println("Status buzzer = "+String(alarm_keamanan)+" - Safe");
  }
  Serial.print("\nPesan RFID : "); content.toUpperCase();
  if(content.substring(1) == "D9 DF 60 B9"){
    Serial.println("Kartu cocok\n"); doorstate = "Open"; Serial.println("Status Pintu: "+doorstate); delay(1000);
  }
  else if(content.substring(1) == "79 D0 24 A3"){
    Serial.println("Kartu cocok\n"); doorstate = "Open"; Serial.println("Status Pintu: "+doorstate); delay(1000);
  }
  else{
    Serial.println("Kartu Tidak cocok"); doorstate = "Closed"; Serial.println("Status Pintu: "+doorstate); delay(1000);
  }
}

// Method untuk mengatur tampilan LCD
void LCDinit(){
  lcd.init(); // Memulai LCD
  lcd.backlight(); delay(250); lcd.noBacklight(); delay(250); lcd.backlight(); // Splash Screen
  lcd.setCursor(0,0); lcd.print("Smart GreenHouse"); delay(3000); // Menampilkan data pada LCD
  lcd.clear(); // Menghapus penampilan data pada LCD 
}
void Display_LCD(String responRFID){
  lcd.setCursor(0,0); lcd.print(responRFID); // Cetak respon RFID di baris 0, kolom 0
}

// Kirim Data ke Antares
void kirimDataAntares(){
  if ((millis() - lastTime) > timerDelay) {
    Olah_Data(); // Memanggil method Olah_Data
    if (WiFi.status() == WL_CONNECTED) {
      http.begin(client, serverName);
  
      http.addHeader("X-M2M-Origin",ACCESSKEY);
      http.addHeader("Content-Type","application/json;ty=4");
      http.addHeader("Accept","application/json");
  
      httpRequestData += "{\"m2m:cin\": { \"con\":\"{\\\"Alarm Keamanan\\\":\\\"";
      httpRequestData += String(alarm_keamanan);
      httpRequestData += "\\\",\\\"Status Pintu\\\":\\\"";
      httpRequestData += String(doorstate);
      httpRequestData += "\\\"}\"}}";
      // Serial.println(httpRequestData);
      // Serial.println();
      
      int httpResponseCode = http.POST(httpRequestData);
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.println("\n");
  
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    } 
    lastTime = millis();
  }
  delay(5000);
}

// Method ini hanya akan dikerjakan sekali setiap device dinyalakan
void setup(){
  Serial.begin(115200); // Baudrate
  pinMode(SW420_PIN,INPUT); // Inisialisasi pin sw-420 sebagai INPUT
  pinMode(BUZZER_PIN,OUTPUT); // Inisialisasi pin buzzer sebagai OUTPUT
  digitalWrite(BUZZER_PIN, LOW); // Default buzzer: OFF
  pinMode(SOLENOID_DOORLOCK_PIN,OUTPUT); // Inisialisasi pin solenoid door lock sebagai OUTPUT
  digitalWrite(SOLENOID_DOORLOCK_PIN, LOW); // Default solenoid door lock: OFF
  ConnectToWiFi(); // Memanggil method ConnectToWiFi
  LCDinit(); // Memanggil method LCDinit
  initSensorRFID(); // Memanggil method initSensorRFID
}

// Method ini akan dikerjakan berulang kali
void loop(){
  kirimDataAntares();
}

// Nama Final Project : Smart Green House (Device-2: NodeMCU)
// Nama Peserta Edspert.id : Devan Cakra Mudra Wijaya, S.Kom.
