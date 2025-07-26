#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ESP32Servo.h>

// ====== WIFI & TELEGRAM CONFIGURATION ======
const char* ssid = "Bengkel_Rumahan";
const char* password = "simulasitrash12_";
const char* botToken = "";
// ====== KONFIGURASI ID GRUP TELEGRAM ======
const String chatIDGrup = "";


WiFiClientSecure secured_client;
UniversalTelegramBot bot(botToken, secured_client);
bool wifiTersambung = false;

// ====== PIN CONFIGURATION ======
const int trigPin = 13;
const int echoPin = 12;
const int buzzerPin = 14;
const int servoPin = 27;
const int redPin = 26;
const int yellowPin = 25;
const int greenPin = 33;
const int metalPin = 21;

const int trigMetalPin = 17;    // Trigger Metal
const int echoMetalPin = 5;     // Echo Metal
const int trigNonMetalPin = 2;  // Trigger Non-Metal
const int echoNonMetalPin = 4;  // Echo Non-Metal

const int servoDefaultWrite = 80;
long duration;
int distance;
int metalDetected = 0;

Servo servo;

unsigned long detectedAt = 0;
bool isWaitingConfirmation = false;

int totalMetal = 0;
int totalNonMetal = 0;
const int JARAK_MIN = 19.5;
const int JARAK_MAX = 3.5;  // cm maksimum jarak tong (penuh)

unsigned long waktuCekPesan = 0;
const unsigned long intervalCekPesan = 3500;

bool sedangBuangSampah = false;
unsigned long waktuBuangMulai = 0;


// ====== SETUP ======
void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(trigMetalPin, OUTPUT);
  pinMode(echoMetalPin, INPUT);
  pinMode(trigNonMetalPin, OUTPUT);
  pinMode(echoNonMetalPin, INPUT);

  pinMode(buzzerPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(metalPin, INPUT);

  servo.attach(servoPin);
  servo.write(servoDefaultWrite);

  digitalWrite(buzzerPin, LOW);
  digitalWrite(redPin, LOW);
  digitalWrite(yellowPin, HIGH);  // Kuning dulu saat booting
  digitalWrite(greenPin, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Tersambung!");
    secured_client.setInsecure();
    wifiTersambung = true;
    digitalWrite(yellowPin, LOW);
    digitalWrite(greenPin, HIGH);  // Hijau saat sudah konek
    bot.sendMessage("Ahnaf", "🤖 Sistem Aktif dan Terhubung!", "");
  }
}

// ====== FUNCTIONS ======
float bacaJarak(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long durasi = pulseIn(echo, HIGH, 30000);
  return durasi * 0.034 / 2;
}

String buatBar(float jarak) {
  int persen = constrain(map(jarak, JARAK_MIN, JARAK_MAX, 0, 100), 0, 100);
  int blok = persen / 10;
  String warna = "🟩";

  if (persen > 70) warna = "🟥";
  else if (persen > 40) warna = "🟨";

  String bar = "";
  for (int i = 0; i < 10; i++) {
    bar += (i < blok) ? warna : "⬜";
  }
  return bar + " " + String(persen) + "%";
}

void cekPesanTelegram() {
  int jumlahPesan = bot.getUpdates(bot.last_message_received + 1);
  while (jumlahPesan) {
    for (int i = 0; i < jumlahPesan; i++) {
      String pesanMasuk = bot.messages[i].text;
      String idChat = String(bot.messages[i].chat_id);
      pesanMasuk.toLowerCase();

      // Hanya izinkan command jika dikirim dari grup tertentu
      if (idChat != chatIDGrup) {
        bot.sendMessage(idChat, "❌ Command ini hanya bisa digunakan di grup yang terdaftar.", "");
        continue;
      }

      if (pesanMasuk == "/reset") {
        totalMetal = 0;
        totalNonMetal = 0;
        bot.sendMessage(idChat, "🔄 Data total sampah berhasil direset!", "");
      } else if (pesanMasuk == "/status") {
        float jarakMetal = bacaJarak(trigMetalPin, echoMetalPin);
        float jarakNonMetal = bacaJarak(trigNonMetalPin, echoNonMetalPin);

        String status = "📊 Status Tempat Sampah:\n\n";
        status += "🗑️ Metal: " + buatBar(jarakMetal) + "\n";
        status += "🧴 Non-Metal: " + buatBar(jarakNonMetal) + "\n\n";
        status += "♻️ Total Metal: " + String(totalMetal) + "\n";
        status += "🧴 Total Non-Metal: " + String(totalNonMetal);
        bot.sendMessage(idChat, status, "");
      }
    }
    jumlahPesan = bot.getUpdates(bot.last_message_received + 1);
  }
}


// ====== LOOP ======
void loop() {
  if (!wifiTersambung) return;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Jarak : ");
  Serial.print(distance);
  Serial.println(" cm");

  metalDetected = digitalRead(metalPin);
  Serial.print("Metal Detected: ");
  Serial.println(metalDetected);

  if (distance < 14) {
    if (!sedangBuangSampah && !isWaitingConfirmation) {
      detectedAt = millis();
      isWaitingConfirmation = true;
      digitalWrite(redPin, LOW);
      digitalWrite(yellowPin, HIGH);
      digitalWrite(greenPin, LOW);
      digitalWrite(buzzerPin, HIGH);
    } else {
      if (isWaitingConfirmation && millis() - detectedAt >= 1500 && !sedangBuangSampah) {
        digitalWrite(redPin, HIGH);
        digitalWrite(yellowPin, LOW);
        digitalWrite(greenPin, LOW);

        if (metalDetected == HIGH) {
          servo.write(35);
          totalMetal++;
        } else {
          servo.write(125);
          totalNonMetal++;
        }

        // ---------------------------
        String notif = "📦 Sampah baru terdeteksi!\n\n";
        notif += (metalDetected == HIGH) ? "🗑️ Jenis: Metal\n" : "🧴 Jenis: Non-Metal\n";
        notif += "📊 Total Metal: " + String(totalMetal) + "\n";
        notif += "📊 Total Non-Metal: " + String(totalNonMetal) + "\n\n";

        // Tambahkan statistik volume
        float jarakMetal = bacaJarak(trigMetalPin, echoMetalPin);
        float jarakNonMetal = bacaJarak(trigNonMetalPin, echoNonMetalPin);
        notif += "📦 Volume Saat Ini:\n";
        notif += "🗑️ Metal: " + buatBar(jarakMetal) + "\n";
        notif += "🧴 Non-Metal: " + buatBar(jarakNonMetal);

        bot.sendMessage(chatIDGrup, notif, "");

        // ---------------------------

        if (sedangBuangSampah && millis() - waktuBuangMulai >= 3250) {
          servo.write(servoDefaultWrite);
          digitalWrite(buzzerPin, LOW);
          sedangBuangSampah = false;
          isWaitingConfirmation = false;
        }
      }
    }
  } else {
    isWaitingConfirmation = false;
    servo.write(servoDefaultWrite);
    digitalWrite(buzzerPin, LOW);
    digitalWrite(redPin, LOW);
    digitalWrite(yellowPin, LOW);
    digitalWrite(greenPin, HIGH);
  }

  if (wifiTersambung && millis() - waktuCekPesan > intervalCekPesan) {
    cekPesanTelegram();
    waktuCekPesan = millis();
  }
  
  delay(350);
}
