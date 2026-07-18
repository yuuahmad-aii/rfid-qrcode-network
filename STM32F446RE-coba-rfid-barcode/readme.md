# STM32F446RE RFID & SD Card Logger

Proyek ini adalah program berbasis mikrokontroler STM32F446RE yang berfungsi untuk membaca kartu RFID menggunakan modul RC522, menampilkan UID kartu pada modul 7-segment TM1638, dan menyimpan data log (UID dan waktu) ke dalam SD Card.

## Penjelasan Program (`main.c`)

File `Core/Src/main.c` berisi logika utama dari aplikasi ini. Berikut adalah penjelasan mengenai alur kerja dan fitur utama dalam program:

### 1. Inisialisasi Periferal
Program memulai dengan melakukan inisialisasi berbagai periferal yang dibutuhkan:
- **SPI (hspi1, hspi2, hspi3)**: Digunakan untuk komunikasi dengan modul RFID RC522 dan TM1638.
- **SDIO**: Digunakan untuk antarmuka pembacaan/penulisan SD Card.
- **FATFS**: Sistem file (FatFs) diinisialisasi untuk memanajemen file pada SD Card.
- **RTC (Real-Time Clock)**: Diinisialisasi untuk mengambil data waktu dan tanggal aktual yang akan digunakan untuk *timestamp* pada log.
- Modul lain seperti **ADC, Timers, UART, dan USB Host** juga disiapkan.

### 2. Modul RFID RC522 dan TM1638
- **`RC522_Init()`** dan **`TM1638_Init()`** dipanggil sebelum masuk ke *infinite loop* ( `while(1)` ) untuk memastikan modul siap digunakan.
- Di dalam *loop*, program membaca status tombol pada modul TM1638 (`TM1638_ReadButtons()`). Jika tombol 1 ditekan, layar TM1638 akan dibersihkan.
- Program secara terus-menerus mengecek keberadaan kartu RFID menggunakan fungsi `RC522_Check(rfid_id)`.

### 3. Pemrosesan Data RFID
Ketika kartu RFID terdeteksi:
- Program menghitung jumlah pembacaan (`rfid_read_count`).
- **Menampilkan ke TM1638**: UID kartu (4 byte) dikonversi menjadi format string heksadesimal dan ditampilkan ke modul 7-segment TM1638.
- **Mendapatkan Waktu**: Membaca tanggal dan waktu saat ini dari RTC (`HAL_RTC_GetTime` dan `HAL_RTC_GetDate`).
- **Format Data CSV**: Data UID dan waktu digabungkan ke dalam sebuah string dengan format CSV (`"YYYY-MM-DD HH:MM:SS,UID"`).

### 4. Penyimpanan ke SD Card (Logging)
- SD Card di-mount menggunakan `f_mount()`.
- File bernama `rfid_log.csv` dibuka dengan mode *append* (`FA_OPEN_APPEND | FA_WRITE`).
- Jika file masih kosong (baru dibuat), program akan menuliskan header `"Datetime,UID\n"` terlebih dahulu.
- Selanjutnya, data string CSV ditulis ke dalam file tersebut menggunakan `f_write()`.
- Terdapat sistem *error handling* sederhana yang menghitung keberhasilan (`sd_write_success_count`) dan kegagalan (`sd_write_error_count`) penulisan.
- Setelah selesai, file ditutup (`f_close()`) dan SD Card di-unmount agar data aman.
- Terdapat delay selama 1 detik (`HAL_Delay(1000)`) untuk mencegah pembacaan ganda pada kartu yang sama.
