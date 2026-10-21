# AgriSense V2.0 ESP32-S3 Firmware

Firmware PlatformIO hasil port sketch AgriSense ke `src/main.cpp`, dengan
portal Wi-Fi custom MAKERINDO, publikasi MQTT, task FreeRTOS untuk sensor,
watchdog, dan pemeriksaan OTA dari GitHub Releases.

## Perangkat keras dan pin

| Fungsi | GPIO / koneksi |
| --- | --- |
| LED status koneksi Wi-Fi | GPIO 4 |
| DHT22 DATA | GPIO 7 |
| ADS1115 SDA / SCL | GPIO 8 / GPIO 9 |
| MQ-4 / MQ-135 / NO2 ADS1115 input | A1 / A2 / A3 |
| GPS RX / TX ESP32 | GPIO 1 / GPIO 15 |
| RS485 RX / TX ESP32 | GPIO 18 / GPIO 17 |
| Pembaca tegangan baterai | GPIO 5 |
| ADS1115 alamat I2C | `0x48` |


## Wi-Fi custom

Saat boot, perangkat mencoba kredensial yang tersimpan di NVS. Bila belum
terhubung, portal konfigurasi MAKERINDO membuka access point
`AgriSense-Setup`; hubungkan ponsel ke AP itu dan buka `http://192.168.4.1`.
Portal berhenti saat koneksi berhasil dan tersedia lagi setelah koneksi
terputus.

Ganti `WIFI_AP_PASSWORD` di `src/main.cpp` dengan password unik sebelum
deployment. Password AP saat ini hanya nilai contoh.

## Sensor, RTOS, dan watchdog

- Pembacaan DHT22 berjalan pada task FreeRTOS setiap 2 detik.
- GPS dan anemometer RS485 berjalan pada task terpisah; akses data bersama
  dilindungi mutex.
- WDT task loop dikonfigurasi dengan timeout 60 detik.
- Waktu memakai NTP UTC+7.
- Data gas dibaca melalui ADS1115; tegangan baterai memakai ADC ESP32.

## MQTT dan payload konfigurasi global

Telemetry dikirim ke `broker.emqx.io:1883`, topic
`agrisense/iot/readings`. Interval default 900.000 ms (15 menit, mengikuti
nilai `SEND_INTERVAL_MS` di sketch sumber). Payload memuat data lokasi, gas,
lingkungan, baterai, status node, versi firmware, jumlah boot, dan interval
telemetry.

Interval dapat diubah dari topic `agrisense/iot/config` dengan payload:

```json
{"publish_interval_ms":10000}
```

Nilai yang diterima 1.000–3.600.000 ms dan disimpan di NVS. MQTT publik ini
tidak terenkripsi atau diautentikasi; gunakan hanya untuk prototipe dan jangan
kirim data sensitif atau konfigurasi penting.

## OTA GitHub Releases

Workflow GitHub Actions di `.github/workflows/release-firmware.yml` membuat
firmware dan GitHub Release secara otomatis ketika tag versi baru di-push.
Workflow mengatur versi firmware dari tag, mengisi owner/nama repo dari GitHub,
dan melampirkan binary dengan nama `firmware.bin`.

Setelah workflow sudah ada pada branch default, buat tag semver baru (misalnya
`v2.0.1`) lalu push tag tersebut:

```sh
git tag v2.0.1
git push origin v2.0.1
```

Pantau hasilnya di tab **Actions**. Jika build berhasil, release versi itu akan
muncul di **Releases**. Firmware memeriksa release terbaru setiap 6 jam saat
Wi-Fi tersambung, membandingkan versi, lalu mengunduh asset melalui HTTPS
dengan bundle CA bawaan ESP32. Untuk unduhan OTA tanpa autentikasi, repo harus
bersifat **Public**. Pastikan release berisi binary untuk board dan partition
layout yang sama.

## Build dan upload

```sh
pio run
pio run --target upload
pio device monitor
```

Target build default adalah `esp32-s3-devkitc-1`; ubah `board` di
`platformio.ini` jika menggunakan board yang berbeda.
