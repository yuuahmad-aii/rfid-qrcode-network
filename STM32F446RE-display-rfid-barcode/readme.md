# Sistem Kontrol Akses STM32F446RE dengan Display, RFID, dan Barcode Scanner

Proyek ini adalah **Sistem Kontrol Akses (Access Control System)** berbasis mikrokontroler STM32F446RE. Sistem ini mengintegrasikan Layar Sentuh TFT LCD, pembaca RFID, Pemindai Barcode/QR Code via USB, dan berkomunikasi dengan modul sekunder (seperti ESP32/ESP8266) melalui antarmuka UART untuk sinkronisasi waktu dan verifikasi akses ke server.

## 1. Fitur Utama

- **Layar TFT LCD (ILI9488):** Menyediakan antarmuka visual (UI) dengan beberapa halaman (Booting, Standby, Akses Diterima/Ditolak, Menu Admin).
- **Layar Sentuh (Touch Screen - XPT2046):** Memungkinkan interaksi pengguna dengan sistem, seperti memasukkan PIN Admin pada keypad virtual di layar.
- **Pemindai RFID (RC522):** Membaca kartu/tag RFID berjenis MIFARE (13.56 MHz) untuk otentikasi pengguna.
- **Pemindai Barcode USB (USB Host HID):** Membaca barcode atau QR code melalui pemindai USB yang terhubung langsung ke STM32 (USB OTG FS).
- **Komunikasi UART:** Mengirim data hasil pindaian (RFID/Barcode) ke modul eksternal (seperti modul Wi-Fi ESP32) dan menerima status akses (`GRANTED` atau `DENIED`) serta data sinkronisasi waktu.
- **Real-Time Clock (RTC):** Menyimpan dan menampilkan waktu saat ini (jam, menit, detik) pada layar siaga (standby).
- **Menu Admin:** Menu yang dilindungi dengan kata sandi (PIN Default: `1234`) untuk menambahkan kartu baru (simulasi) atau menyinkronkan waktu sistem dengan server.

## 2. Perangkat Keras (Hardware) yang Digunakan

1. **Mikrokontroler:** STM32F446RE (Nucleo-F446RE atau Custom Board).
2. **Layar (Display):** Modul TFT LCD ILI9488 (Antarmuka SPI).
3. **Kontroler Sentuh (Touch Controller):** XPT2046 (Antarmuka SPI).
4. **Pembaca RFID:** Modul RC522 (Antarmuka SPI).
5. **Pemindai Barcode/QR:** Pemindai USB standar yang mendukung profil HID (Human Interface Device).
6. **Modul Jaringan Eksternal (Opsional):** ESP8266, ESP32, atau PC yang dihubungkan melalui pin UART1.
7. **Modul MicroSD (Opsional):** Dihubungkan via SDIO untuk penyimpanan lokal (FATFS diinisialisasi namun belum sepenuhnya diimplementasikan di program utama).

## 3. Konfigurasi Pin (Pinout)

Berdasarkan konfigurasi `main.h`, berikut adalah alokasi pin yang digunakan pada STM32F446RE:

### a. Layar TFT LCD (ILI9488) - SPI1
- **SPI1_CS (Chip Select):** `PA4`
- **SPI1_DC (Data/Command):** `PC5`
- **SPI1_RST (Reset):** `PC4`
- **SPI1_SCK, MISO, MOSI:** Menggunakan pin default SPI1 perangkat keras.
- **Backlight (PWM):** TIM3 Channel 3.

### b. Layar Sentuh (XPT2046) - SPI2
- **SPI2_CS (Chip Select):** `PB12`
- **SPI2_IRQ (Interrupt):** `PC6`
- **SPI2_SCK, MISO, MOSI:** Menggunakan pin default SPI2.

### c. Pembaca RFID (RC522) - SPI3
- **SPI3_CS (Chip Select):** `PB6`
- **SPI3_RST (Reset):** `PB7`
- **SPI3_SCK, MISO, MOSI:** Menggunakan pin default SPI3.

### d. USB Host (Barcode Scanner)
- **USB_POWER (Enable Power):** `PC7`
- **USB_DM & USB_DP:** `PA11` & `PA12`

### e. SD Card (SDIO)
- **SDIO_DET (Card Detect):** `PA8`
- **SDIO D0-D3, CMD, CLK:** Menggunakan pin periferal SDIO standar.

### f. Komunikasi Eksternal & Indikator
- **UART1 (Komunikasi ke ESP32):** Menggunakan `USART1` (TX & RX).
- **USER_LED:** `PB2` (Digunakan sebagai indikator status akses).
- **USER_BTN:** `PC13` (Tombol bawaan Nucleo).

## 4. Cara Kerja Program

Program ini menggunakan arsitektur *State Machine* (Mesin Status) yang disebut `AppState`. Loop utama terus-menerus memeriksa status saat ini dan input periferal.

### Alur Kerja (Workflow):
1. **Inisialisasi (`STATE_BOOTING`):** STM32 melakukan inisialisasi pada HAL, Clock, SPI, UART, RTC, USB Host, dan Layar. Menampilkan pesan booting di layar.
2. **Siaga (`STATE_STANDBY`):** Layar menampilkan jam (dari RTC) dan pesan "SILAKAN SCAN KARTU ATAU QR CODE". Program membaca sentuhan (Touch), menunggu USB membaca Barcode, dan menunggu SPI3 membaca kartu RFID (RC522).
3. **Pembacaan (`STATE_READING`):**
   - Jika **Kartu RFID** ditempel, ID akan dikirim via UART dengan format `RFID:XXXXXXXX\n`.
   - Jika **Barcode** dipindai, teks akan dikirim via UART dengan format `BARCODE:text\n`.
   - Layar berubah menjadi "Sedang memproses..." dan menunggu respon dari UART.
4. **Respon Akses (`STATE_GRANTED` / `STATE_DENIED`):**
   - Jika UART menerima `GRANTED:nama\n`, layar menampilkan centang hijau, pesan selamat datang, menyalakan LED sesaat, lalu kembali ke Standby.
   - Jika UART menerima `DENIED:alasan\n`, layar menampilkan silang merah, alasan penolakan, mengedipkan LED, lalu kembali ke Standby.
   - Terdapat mekanisme *Timeout* jika server tidak membalas dalam 10 detik, yang secara otomatis akan masuk ke status `DENIED`.

### Menu Admin & Sinkronisasi Waktu
Terdapat tombol tersembunyi (area teks "ADMIN") di pojok kanan bawah pada menu Standby.
- Saat area tersebut ditekan, layar akan menampilkan keypad virtual (`STATE_MENU_PASS`).
- Pengguna harus memasukkan PIN `1234`. Jika benar, masuk ke `STATE_MENU_ADMIN`.
- Di Menu Admin, pengguna bisa menekan tombol **"TAMBAH KARTU"** untuk masuk ke mode pendaftaran (menunggu kartu ditempel lalu menekan OK).
- Pengguna juga bisa menekan tombol **"SYNC WAKTU"**. Alat akan mengirim pesan UART `CMD:SYNC_TIME\n`. Saat server membalas dengan `TIME:YY-MM-DD HH:MM:SS\n`, RTC pada STM32 akan otomatis diperbarui dan layar kembali menampilkan jam yang akurat.

## 5. Cara Menggunakan Alat

1. **Nyalakan Perangkat:** Beri daya pada papan STM32. Tunggu proses booting selesai hingga muncul tulisan "SILAKAN SCAN KARTU ATAU QR CODE".
2. **Proses Absensi / Akses:**
   - **Metode 1:** Tempelkan kartu/tag RFID MIFARE Anda pada sensor RC522.
   - **Metode 2:** Pindai Barcode/QR Code menggunakan scanner USB yang terhubung.
   - Perhatikan layar. Jika akses diterima, alat akan menampilkan nama Anda. Jika ditolak, alat akan menampilkan pesan kesalahan.
3. **Masuk ke Mode Admin:**
   - Pada layar siaga, sentuh tombol **"ADMIN"** yang terletak di pojok kanan bawah layar.
   - Ketik PIN (Bawaan: `1234`) menggunakan keypad di layar sentuh, lalu tekan tombol hijau **OK**.
4. **Menambahkan Kartu Baru (Simulasi UI):**
   - Di Menu Admin, pilih **"TAMBAH KARTU"**.
   - Tempelkan kartu RFID baru yang belum terdaftar.
   - Jika ID kartu terbaca di layar, tekan tombol **"OK"** untuk mendaftarkan.
5. **Menyinkronkan Waktu (Sync Waktu):**
   - Di Menu Admin, pilih **"SYNC WAKTU"**.
   - Alat akan meminta data waktu terbaru dari server (via ESP32). Jika berhasil, akan ada notifikasi "WAKTU BERHASIL DIUPDATE".

## 6. Pengembangan Kedepannya (Future Development)

Sistem ini memiliki dasar struktur kode yang sangat baik dan perangkat keras yang kuat (STM32F446RE). Beberapa fitur tambahan yang dapat diimplementasikan di masa depan:
- **Penyimpanan Log Akses Offline (SD Card):** Modul FatFS telah disiapkan (`fatfs.h`). Sistem dapat dimodifikasi untuk menyimpan (mencatat) setiap kali akses berhasil atau ditolak dalam file `.txt` atau `.csv` di dalam MicroSD. Jika koneksi ke server terputus, data absensi tetap tersimpan lokal dan dapat diunggah ulang ketika jaringan pulih.
- **Koneksi Jaringan Mandiri (ESP-AT Command):** Daripada hanya saling berbalas string dengan ESP32 menggunakan protokol khusus, STM32 dapat langsung mengatur ESP32 menggunakan *AT Commands* untuk melakukan koneksi HTTP/MQTT (POST/GET) langsung dari kode STM32.
- **Tampilan Antarmuka Grafis Lebih Kompleks:** Menggunakan library UI seperti **LVGL (Light and Versatile Graphics Library)** untuk memberikan animasi, font anti-aliased yang lebih halus, grafik, dan bayangan, sehingga UI terlihat lebih modern dibanding langsung menggambar bentuk geometri kasar (fill rectangle).
- **Keamanan Komunikasi (Enkripsi):** Payload string yang dikirim via UART bisa dienkripsi dengan algoritma AES dasar agar tidak dapat mudah disadap oleh *logic analyzer* atau perangkat eksternal.
- **Pembaruan Firmware Over-The-Air (OTA):** Menambahkan bootloader kustom agar firmware STM32 dapat diperbarui melalui file biner yang diunduh oleh ESP32.
- **Manajemen User (Database Lokal):** STM32 dapat menyimpan database `UID -> NAMA` kecil-kecilan di Memori Flash internal atau SD Card untuk proses otentikasi luring (offline caching) yang lebih cepat tanpa selalu harus menunggu *response* dari *server*.
