# ESP32-C6 Co-MCU untuk Sistem Kontrol Akses (Display, RFID, Barcode)

Proyek ini adalah *firmware* pendamping (Co-MCU) berbasis **ESP32-C6** yang bertindak sebagai antarmuka jaringan (Wi-Fi) dan pemrosesan API untuk mikrokontroler utama (seperti STM32). ESP32 menerima data pemindaian via UART, mengirimkannya ke server melalui protokol HTTP/HTTPS (REST API), dan mengembalikan hasil validasi ke mikrokontroler utama.

## Fitur Utama

- **Konektivitas Wi-Fi:** Terhubung ke jaringan nirkabel secara otomatis dengan fitur *auto-reconnect*.
- **Komunikasi UART:** Berkomunikasi dengan mikrokontroler utama (STM32) untuk menerima data pemindaian dan mengirimkan respon akses.
- **REST API Client (HTTPS):** Mengirim data absensi/transaksi (`transaction_uid`, `timestamp`, `identifier`, `identifier_type`) ke server dalam format JSON.
- **Validasi Sertifikat SSL/TLS:** Dilengkapi dengan *Root CA* Cloudflare tertanam untuk komunikasi HTTPS yang aman ke server (contoh: `device.dsalute.id`).
- **Sinkronisasi Waktu (SNTP & API):** Mengambil waktu dari `pool.ntp.org` dan dari endpoint API khusus untuk memastikan stempel waktu (timestamp) akurat dan menyinkronkan jam RTC di STM32.
- **Indikator LED RGB (WS2812):** Menggunakan LED terintegrasi untuk memberikan *feedback* visual mengenai status sistem (Wi-Fi, Transmisi, Sukses, Gagal).
- **Tombol Test (BOOT):** Dapat digunakan untuk mengirim *request* API pengujian secara manual (berguna untuk *debugging*).

## Perangkat Keras (Hardware) & Pinout

Kode ini disesuaikan untuk *board* ESP32-C6 (seperti ESP32-C6-DevKitC) dengan pinout berikut:

- **UART (Komunikasi ke STM32):**
  - Baud Rate: `115200`
  - `TXD`: GPIO 4
  - `RXD`: GPIO 5
- **Tombol BOOT:** GPIO 9 (Tekan untuk mengirim *test request*).
- **LED RGB (WS2812 - via SPI2 DMA):** GPIO 8.

### Status Indikator LED RGB
- 🔴 **Merah:** Tidak terhubung ke Wi-Fi atau Terjadi Kesalahan (HTTP Error / Ditolak Server).
- 🔵 **Biru:** Terhubung ke Wi-Fi dan *Idle* (Siap beroperasi).
- 🟡 **Kuning:** Sedang mengirim *request* HTTP ke server.
- 🟢 **Hijau:** *Request* berhasil (Akses Diterima).

## Cara Kerja Sistem & Protokol UART

ESP32 mendengarkan input dari port UART. Ketika mikrokontroler utama (STM32) memindai kartu atau barcode, ia mengirim pesan ke ESP32.

### Pesan dari STM32 ke ESP32
1. **`RFID:<UID>\n`**
   - Memicu ESP32 untuk mengirim HTTP POST ke API dengan `identifier_type: "rfid"` dan nilai `<UID>`.
2. **`BARCODE:<DATA>\n`**
   - Memicu ESP32 untuk mengirim HTTP POST ke API dengan `identifier_type: "qr"` dan nilai `<DATA>`.
3. **`CMD:SYNC_TIME\n`**
   - Meminta ESP32 untuk memanggil endpoint sinkronisasi waktu dan mengembalikan waktunya ke STM32.

### Pesan dari ESP32 ke STM32
1. **`GRANTED:<Nama>\n`**
   - Dikirim ketika API mengembalikan `success: true`. `<Nama>` diambil dari parameter `attendance_result.name`.
2. **`DENIED:<Alasan>\n`**
   - Dikirim ketika API mengembalikan `success: false` atau koneksi gagal.
3. **`TIME:YY-MM-DD HH:MM:SS\n`**
   - Dikirim setelah berhasil mengambil waktu server.
4. **`TIME_ERR:<Alasan>\n`**
   - Dikirim jika sinkronisasi waktu gagal.

## Konfigurasi (Wajib Diubah!)

Sebelum melakukan *flash* ke ESP32-C6, Anda **WAJIB** mengubah konfigurasi kredensial Wi-Fi dan URL API di bagian atas file `main.c`:

```c
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#define API_TEST_URL "http://192.168.137.1:3000/api/test"
#define API_TIME_URL "https://device.dsalute.id/api/v1/time.php"
#define API_TRANSACTIONS_URL "https://device.dsalute.id/api/v1/transactions.php"
#define API_KEY "dev_8dfa3625a4593e78de6c02e7b03b1daf7f3b13ba9cae2d81"
```

Jika server Anda menggunakan sertifikat SSL dari CA yang berbeda (bukan Cloudflare), Anda juga harus memperbarui variabel `cloudflare_ca_pem`.

## Kompilasi & Flash (ESP-IDF)

Proyek ini dibangun menggunakan **ESP-IDF** (Espressif IoT Development Framework).

1. Buka terminal ESP-IDF.
2. Atur target ke ESP32-C6:
   ```bash
   idf.py set-target esp32c6
   ```
3. Bangun (*build*), *flash*, dan pantau (*monitor*) kode:
   ```bash
   idf.py build flash monitor
   ```

## Dependencies / Komponen
- `esp_wifi`, `esp_http_client`, `esp_netif_sntp`
- `cJSON` (Untuk pembuatan dan penguraian payload JSON).
- `led_strip` (Untuk mengontrol LED WS2812).
