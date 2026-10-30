#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>

// ================= [ ส่วนตั้งค่าข้อมูลเครือข่าย ] =================
// ค่า Wi-Fi, MAC และ IP ปลายทางอยู่ใน secrets.h (ไม่ขึ้น Git)
// คัดลอก secrets.example.h เป็น secrets.h แล้วแก้ค่าให้ตรงกับเครือข่ายของคุณ
#include "secrets.h"

const char* WIFI_SSID     = SECRET_WIFI_SSID;
const char* WIFI_PASSWORD = SECRET_WIFI_PASSWORD;

// MAC Address ของคอมพิวเตอร์ (คั่นด้วย :)
const char* TARGET_PC_MAC = SECRET_TARGET_PC_MAC;

// ส่ง Magic Packet ไปที่ router (ฝั่ง WAN) แล้ว router จะ forward ต่อไปที่เครื่องปลายทาง
const IPAddress WOL_RELAY_IP(SECRET_WOL_RELAY_IP);
const uint16_t  WOL_PORT = 9;

// ขาไฟ LED บนบอร์ด ESP32-S3 Super Mini
#define LED_PIN 48
// ==========================================================

WiFiUDP UDP;
WebServer server(80); // เปิดพอร์ต 80 สำหรับ Web Server

// โค้ด HTML สำหรับหน้าเว็บ (ออกแบบให้รองรับมือถือและมีปุ่มกดขนาดใหญ่)
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Wake-on-LAN</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #121212; color: white; margin-top: 50px; }
    h1 { color: #4CAF50; }
    .btn { background-color: #4CAF50; color: white; padding: 20px 40px; font-size: 24px; border: none; border-radius: 10px; cursor: pointer; box-shadow: 0 4px 6px rgba(0,0,0,0.3); transition: 0.2s; }
    .btn:active { background-color: #3e8e41; transform: scale(0.95); }
    .status { margin-top: 20px; font-size: 18px; color: #ffeb3b; }
  </style>
</head>
<body>
  <h1>PC Remote Start</h1>
  <p>กดปุ่มด้านล่างเพื่อสั่งเปิดคอมพิวเตอร์</p>
  <button class="btn" onclick="wakePC()">🚀 TURN ON PC</button>
  <div class="status" id="statusText"></div>

  <script>
    function wakePC() {
      document.getElementById('statusText').innerText = 'กำลังส่งคำสั่งเปิดเครื่อง...';
      fetch('/wake').then(response => response.text()).then(data => {
        document.getElementById('statusText').innerText = '✅ ส่งคำสั่งสำเร็จแล้ว!';
        setTimeout(() => { document.getElementById('statusText').innerText = ''; }, 3000);
      }).catch(() => {
        document.getElementById('statusText').innerText = '❌ ติดต่อ ESP32 ไม่ได้';
      });
    }
  </script>
</body>
</html>
)rawliteral";

// สร้างและส่ง Magic Packet: FF x6 + MAC x16 (ยิงซ้ำ 3 รอบ ห่างกัน 100ms)
void sendMagicPacket(const char* macStr) {
  uint8_t mac[6];
  sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
         &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

  uint8_t packet[102];
  memset(packet, 0xFF, 6);
  for (int i = 0; i < 16; i++) {
    memcpy(&packet[6 + i * 6], mac, 6);
  }

  for (int r = 0; r < 3; r++) {
    UDP.beginPacket(WOL_RELAY_IP, WOL_PORT);
    UDP.write(packet, sizeof(packet));
    UDP.endPacket();
    delay(100);
  }
}

// ฟังก์ชันเมื่อมีคนเข้าหน้าเว็บหลัก (IP Address)
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// ฟังก์ชันเมื่อกดปุ่มบนหน้าเว็บ
void handleWake() {
  Serial.println("\n[WEB] ได้รับคำสั่งเปิดคอมพิวเตอร์จากหน้าเว็บ!");

  // กะพริบไฟ LED รัวๆ แสดงสถานะกำลังยิงคำสั่ง
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
  }
  digitalWrite(LED_PIN, HIGH); // กลับมาติดค้าง

  // สั่งเปิดคอมพิวเตอร์ (ส่งผ่าน MikroTik)
  sendMagicPacket(TARGET_PC_MAC);
  Serial.printf("-> ส่ง Magic Packet ไปที่ %s:%d เรียบร้อยแล้ว!\n",
                WOL_RELAY_IP.toString().c_str(), WOL_PORT);

  // ตอบกลับหน้าเว็บ
  server.send(200, "text/plain", "WAKE_SUCCESS");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // ปิดไฟระหว่างเชื่อมต่อ

  delay(1000);
  Serial.println("\n==========================================");
  Serial.println("   ESP32-S3 Web Server Wake-on-LAN");
  Serial.println("==========================================");

  // 1. เชื่อมต่อ Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("กำลังเชื่อมต่อ Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // กะพริบไฟรอเน็ต
    delay(400);
    Serial.print(".");
  }

  digitalWrite(LED_PIN, HIGH); // ไฟติดค้าง แสดงว่าพร้อมทำงาน
  Serial.println("\nเชื่อมต่อ Wi-Fi สำเร็จ!");

  // 2. เริ่ม UDP สำหรับส่ง Wake-on-LAN
  UDP.begin(WOL_PORT);

  // 3. เริ่มต้น Web Server
  server.on("/", handleRoot);
  server.on("/wake", handleWake);
  server.begin();

  Serial.println("\n==========================================");
  Serial.printf(" Web Server ทำงานแล้ว! เข้าไปที่ลิงก์นี้:\n");
  Serial.printf(" http://%s\n", WiFi.localIP().toString().c_str());
  Serial.printf(" WoL relay: %s:%d -> PC %s\n",
                WOL_RELAY_IP.toString().c_str(), WOL_PORT, TARGET_PC_MAC);
  Serial.println("==========================================");
}

void loop() {
  // ให้ Web Server รอรับคำสั่งอย่างต่อเนื่อง
  server.handleClient();
}
