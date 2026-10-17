# Sistem Absensi Berbasis RFID & FreeRTOS (STM32F405RGT6)

Proyek ini merupakan implementasi sistem absensi cerdas berbasis **FreeRTOS (CMSIS-RTOS v2)** menggunakan mikrokontroler **STM32F405RGT6 (ARM Cortex-M4 @ 168 MHz)**.

Arsitektur ini dirancang untuk mengatasi masalah performa sistem *baremetal* sebelumnya (seperti lag pada layar, pembacaan RFID yang terlewat, dan blocking I/O pada MicroSD), dengan membagi seluruh beban komputasi menjadi task-task berprioritas terisolasi yang berkomunikasi via FreeRTOS Queues, Mutexes, dan Task Notifications.

---

## 1. Arsitektur FreeRTOS & Tabel Task

| Task Name | Priority | Stack Size | Entry Function | Peran & Tanggung Jawab |
| :--- | :--- | :--- | :--- | :--- |
| **`Task_RFID`** | `osPriorityHigh` (40) | 512 Words (2 KB) | `StartRfidTask` | Tidur menunggu **Task Notification** dari pin interrupt `RFID_IRQ` (`SPI3_IRQ` / PA15 atau PA8). Saat bangun: mengunci `spi2Mutex`, membaca UID kartu dari chip **NXP CLRC66303**, mengambil waktu dari RTC, membunyikan **BUZZER** (PA2) & **LED** (PB2), lalu mengirimkan data ke `uiEventQueue` dan `sdLogQueue`. |
| **`Task_GUI`** | `osPriorityAboveNormal` (32) | 1024 Words (4 KB) | `StartGuiTask` | Menjalankan siklus grafis layar **ILI9488 3.5" (480x320)** setiap 10 ms. Membaca sentuhan layar **XPT2046** saat `TOUCH_IRQ` (PC6/EXTI9_5) terpicu dengan proteksi `spi2Mutex`. Memeriksa `uiEventQueue` untuk menampilkan modal pop-up absensi, status koneksi, dan jam digital real-time. |
| **`Task_ESP32`** | `osPriorityNormal` (24) | 768 Words (3 KB) | `StartEsp32Task` | Mengelola komunikasi serial paket (**USART1**) dengan modul **ESP32-C6** (Wi-Fi & BLE). Mengambil payload absensi dari `espTxQueue`, mengirimkannya via serial, serta memproses balasan `GRANTED`, `DENIED`, status `WIFI`, dan sinkronisasi waktu NTP (`TIME:HH:MM:SS-DD/MM/YYYY`). |
| **`Task_Storage`** | `osPriorityBelowNormal` (16) | 1024 Words (4 KB) | `StartStorageTask` | Menunggu log absensi masuk pada `sdLogQueue`. Saat ada data: mengunci `sdioFatFsMutex`, membuka file log `ATTEND.CSV` pada **MicroSD (SDIO 4-bit)** via FatFs (`f_open`, `f_write`, `f_sync`, `f_close`), lalu meneruskan payload ke `espTxQueue`. Prioritas dibuat rendah agar operasi I/O disk yang lambat dapat dipotong (*preempt*) kapan saja oleh `Task_RFID` dan `Task_GUI`. |

---

## 2. Antrean Data (FreeRTOS Queues)

| Queue Name | Kapasitas | Ukuran Item | Deskripsi Aliran Data |
| :--- | :--- | :--- | :--- |
| **`uiEventQueue`** | 8 item | 32 Byte | Mengalirkan event UI dari `Task_RFID` (kartu terdeteksi) atau `Task_ESP32` (verifikasi nama karyawan / akses diterima / ditolak / status Wi-Fi) menuju `Task_GUI`. |
| **`sdLogQueue`** | 16 item | 64 Byte | Mengalirkan paket absensi lokal (UID + Timestamp RTC + Tanggal) dari `Task_RFID` menuju `Task_Storage` untuk dicatat ke file `ATTEND.CSV`. |
| **`espTxQueue`** | 16 item | 64 Byte | Mengalirkan payload absensi yang siap dikirim via Wi-Fi/Cloud dari `Task_Storage` menuju `Task_ESP32`. |

---

## 3. Sinkronisasi & Mutex (FreeRTOS Mutex & Semaphore)

- **`spi2Mutex`**: Melindungi bus SPI2 agar tidak terjadi tabrakan sinyal (*bus contention*) antara pembacaan sentuhan layar XPT2046 dan modul RFID reader.
- **`sdioFatFsMutex`**: Melindungi re-entrancy file system FatFs pada bus SDIO MicroSD jika diakses dari beberapa task berbeda.
- **`spi1TxDmaSem`**: Binary Semaphore untuk transmisi data piksel ILI9488 via DMA2 Stream 5 SPI1. Membebaskan 100% CPU saat transfer frame buffer layar berlangsung.

---

## 4. Pemetaan Pin & Periferal Hardware

| Periferal / Modul | Pin STM32F405RGT6 | Konfigurasi / Deskripsi |
| :--- | :--- | :--- |
| **TFT LCD ILI9488 (3.5")** | **SPI1** (`PA5` SCK, `PA7` MOSI, `PA6` MISO)<br>`PA4` (CS), `PC4` (RST), `PC5` (DC)<br>`PB0` (**TIM3_CH3** PWM Backlight) | Resolusi 480x320 px, 18-bit color, transmisi data grafis dipercepat dengan DMA SPI1. |
| **Touch XPT2046** | **SPI2** (`PB13` SCK, `PB14` MISO, `PB15` MOSI)<br>`PB12` (CS)<br>`PC6` (**EXTI9_5** IRQ Sentuh) | Resistive Touch Controller, dibaca dengan filtering noise & konversi koordinat kalibrasi 480x320. |
| **RFID NXP CLRC66303** | **SPI3** (`PB3` SCK, `PB4` MISO, `PB5` MOSI)<br>`PB6` (CS), `PB7` (RST / PDOWN)<br>`PA15` (**EXTI15_10** IRQ Kartu) | High-performance NFC/RFID Frontend IC, mendukung ISO14443A (Mifare Classic, Ultralight, NTAG). |
| **MicroSD Card** | **SDIO (4-bit Wide Bus)** (`PC8` D0, `PC9` D1, `PC10` D2, `PC11` D3, `PC12` CK, `PD2` CMD)<br>`PA8` (SDIO_DET) | Format FAT32 via FatFs, logging transaksi absensi `ATTEND.CSV`. |
| **ESP32-C6 (Wi-Fi/BLE)** | **USART1** (`PA9` TX, `PA10` RX @ 115200 bps) | Sinkronisasi waktu NTP, validasi ID karyawan dengan REST API / MQTT, dan upload data absensi. |
| **Internal RTC** | Kristal LSE 32.768 kHz / LSI Backup | Menjaga waktu tetap presisi (*real-time*) bahkan saat daya utama mati via baterai koin VBAT. |
| **Buzzer Audio** | `PA2` (`OUTPUT_BUZZER_Pin`) | Feedback suara beep saat tap kartu sukses atau tombol sentuh ditekan. |
| **User Status LED** | `PB2` (`USER_LED_Pin`) | Indikator visual aktivitas sistem dan deteksi kartu. |

---

## 5. Mengapa FreeRTOS Lebih Cepat & Responsif Dibanding Baremetal?

1. **Eliminasi Busy-Wait Polling**: Pada baremetal, CPU tertahan dalam loop `while(!spi_dma_complete)` dan `while(!sd_ready)`. Pada FreeRTOS, task langsung masuk ke mode `Blocked` menggunakan Semaphore atau Task Notification, sehingga CPU digunakan 100% untuk task lain.
2. **Prioritas Preemptive**: Saat MicroSD sedang melakukan penulisan blok (yang bisa memakan waktu hingga puluhan milidetik), kedatangan kartu RFID atau sentuhan layar akan **seketika memotong (*preempt*)** operasi disk tersebut tanpa jeda (*zero latency*).
3. **Pemisahan UI dan Network**: Pengiriman data ke ESP32 atau penulisan log offline tidak lagi membuat animasi layar membeku (*no freeze*).
