# STM32F446RE TFT Display & Touch Tally Counter

Proyek ini adalah program berbasis mikrokontroler STM32F446RE yang mengimplementasikan antarmuka layar sentuh (Touchscreen) menggunakan layar TFT ILI9488 dan kontroler sentuh XPT2046. Aplikasi yang dibuat berupa "Tally Counter" (Penghitung) sederhana.

## Penjelasan Program (`main.c`)

File `Core/Src/main.c` berisi logika utama dari aplikasi ini. Berikut adalah penjelasan mengenai alur kerja dan fitur utama dalam program:

### 1. Inisialisasi Periferal
- **SPI (hspi1, hspi2, hspi3)**: Digunakan untuk komunikasi dengan layar TFT ILI9488 dan modul layar sentuh XPT2046.
- **TIM3 (Timer 3)**: Digunakan untuk menghasilkan sinyal PWM pada *channel* 3 guna mengontrol kecerahan (*backlight*) layar LCD (di-set ke 50%).
- **USB Device** dan periferal standar lainnya juga disiapkan oleh program.

### 2. Inisialisasi Layar (ILI9488) dan Touchscreen (XPT2046)
- **`ILI9488_Init()`**: Melakukan inisialisasi pada driver layar LCD.
- **`XPT2046_Init()`**: Melakukan inisialisasi pada driver *touchscreen*.

### 3. Menggambar User Interface (UI)
Sebelum masuk ke *infinite loop*, program menggambar antarmuka awal ke layar:
- `ILI9488_FillScreen(ILI9488_BLACK)`: Membersihkan layar dengan warna hitam.
- **Judul**: Menulis teks "TALLY COUNTER".
- **Tombol Minus [-]**: Menggambar kotak berwarna merah di sebelah kiri dan teks '-' di tengahnya.
- **Tombol Plus [+]**: Menggambar kotak berwarna hijau di sebelah kanan dan teks '+' di tengahnya.
- **Nilai Counter**: Menampilkan nilai awal counter (`0000`) dengan warna kuning di bagian tengah layar.

### 4. Logika Tally Counter dan Touchscreen
Di dalam *infinite loop* (`while (1)`), program secara terus-menerus mengecek input sentuhan:
- `XPT2046_GetTouch(&touch_x, &touch_y)`: Mengambil koordinat X dan Y jika ada sentuhan.
- **Mendeteksi Tombol Minus [-]**: Jika koordinat sentuhan berada dalam area kotak merah (X: 20-120, Y: 110-210), nilai `counter` akan dikurangi (batas bawah 0) dan teks angka di layar akan di-update.
- **Mendeteksi Tombol Plus [+]**: Jika koordinat sentuhan berada dalam area kotak hijau (X: 360-460, Y: 110-210), nilai `counter` akan ditambah (batas atas 9999) dan teks angka di layar akan di-update.
- **Debounce**: Terdapat `HAL_Delay(200)` dan variabel status `touched` untuk mencegah angka bertambah/berkurang berkali-kali dalam satu sentuhan cepat.
- **Debugging**: Program juga mencetak koordinat X dan Y aktual yang terbaca dari sensor sentuh ke layar bagian bawah untuk memudahkan kalibrasi.
