#pragma once

// คัดลอกไฟล์นี้เป็น secrets.h แล้วแก้ค่าด้านล่าง

#define SECRET_WIFI_SSID      "your-wifi-ssid"
#define SECRET_WIFI_PASSWORD  "your-wifi-password"
#define SECRET_TARGET_PC_MAC  "AA:BB:CC:DD:EE:FF"   // MAC ของเครื่องที่จะปลุก (คั่นด้วย :)
#define SECRET_WOL_RELAY_IP   192, 168, 1, 33       // IP ของ router ที่ forward UDP 9 ไปหาเครื่องปลายทาง
