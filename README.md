# ESP32 Wake-on-LAN

หน้าเว็บบน ESP32-S3 มีปุ่มเดียวสำหรับสั่งเปิดคอมพิวเตอร์ด้วย Wake-on-LAN ใช้ได้แม้คอมพิวเตอร์จะอยู่หลัง router อีกตัว (คนละ subnet)

## หลักการทำงาน

```
มือถือ ──HTTP──> ESP32 (192.168.1.x) ──UDP:9──> MikroTik WAN 192.168.1.33
                                                    │ dst-nat + static ARP
                                                    └──> PC 192.168.88.254
```

Magic packet แบบ broadcast ข้าม router ไม่ได้ ESP32 จึงส่งแบบ unicast ไปที่ router แทน แล้ว router จะ forward ต่อไปยังเครื่องปลายทาง

## ตั้งค่า

1. คัดลอก `secrets.example.h` เป็น `secrets.h` แล้วแก้ค่า Wi-Fi, MAC ของเครื่องปลายทาง และ IP ของ router
2. บอร์ด: **ESP32S3 Dev Module** ตั้ง **USB CDC On Boot: Enabled**
3. อัปโหลด แล้วเปิด Serial Monitor (115200) เพื่อดู IP ของ ESP32

```sh
arduino-cli compile --upload -p COM3 --fqbn "esp32:esp32:esp32s3:CDCOnBoot=cdc" .
```

## ตั้งค่า MikroTik

```
/ip arp add address=192.168.88.254 mac-address=AA:BB:CC:DD:EE:FF interface=bridge comment="WoL target PC (static)"
/ip firewall nat add chain=dstnat in-interface=ether1 protocol=udp dst-address=192.168.1.33 dst-port=9 \
    action=dst-nat to-addresses=192.168.88.254 to-ports=9 comment="WoL from 192.168.1.x -> PC"
```

Static ARP ทำให้ router ยังส่ง packet ถึงเครื่องได้ตอนเครื่องปิดอยู่ ควรตั้ง DHCP reservation บน router หลัก ให้ IP ฝั่ง WAN ของ MikroTik ไม่เปลี่ยน

## ฝั่งคอมพิวเตอร์

- BIOS: เปิด Wake on LAN / Power On by PCI-E
- Windows: Device Manager → การ์ด LAN → Power Management → "Only allow a magic packet to wake the computer" และปิด Fast Startup
