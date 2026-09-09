# Maker ESP32

[English](README.md) | **Bahasa Melayu**

Projek Arduino untuk **Maker ESP32 + Robo ESP32**. Sketch pertama ialah mobile
robot dua motor DC yang dikawal menggunakan controller PS4 / DualShock 4.

## Struktur projek

```text
Arduino/
  MakerESP32_MobileRobot_PS4/
    MakerESP32_MobileRobot_PS4.ino
tests/
  test_controls.cpp
  stubs/
```

## Hardware dan sambungan

| Komponen | Sambungan | GPIO |
|---|---|---|
| Motor kanan | Robo ESP32 MOTOR1 | A = 12, B = 13 |
| Motor kiri | Robo ESP32 MOTOR2 | A = 14, B = 27 |
| Maker ESP32 | Soket ESP32 pada Robo ESP32 | Ikut orientasi board |
| PS4 / DualShock 4 | Bluetooth terus ke Maker ESP32 | Tiada receiver USB |

Robo ESP32 menggunakan driver dalam mod `PWM_PWM`. Maker ESP32 menggunakan
ESP32-WROOM-32E; jangan gantikan dengan ESP32-S3/C3 untuk controller PS4 Bluetooth
Classic. Keserasian controller clone perlu diuji secara fizikal.

Untuk Robo ESP32, gunakan bateri LiPo/Li-ion **1 sel** melalui connector bateri
yang sesuai, atau **3.6–6V pada terminal VIN Robo ESP32**. Voltan motor mengikut
bekalan board. Jangan sambung bateri 2S terus. Had ini merujuk kepada Robo ESP32,
bukan pin bekalan pada Maker ESP32. Padankan voltan dan arus stall motor dengan
rating driver/bekalan. Pengguna menggunakan bateri 6V dan motor TT; model tepat
dan arus stall motor belum disahkan.

## Kawalan

Kedua-dua cara aktif tanpa butang tukar mode. **D-pad mendapat keutamaan** apabila
ditekan. Apabila dilepaskan, robot kembali mengikut kedudukan analog semasa;
jika analog masih ditolak, robot terus bergerak mengikut analog tersebut.

D-pad bermula pada kira-kira **50% PWM**. Semasa menahan arah D-pad, trigger analog
R2 menaikkan kelajuan secara berterusan dari **50% hingga 100%**: dilepaskan ialah
50%, ditekan separuh kira-kira 75%, dan ditekan penuh ialah 100%. R2 tidak
mengubah kawalan analog.

### 1. D-pad kiri: lapan arah

`+` = maju, `-` = undur, `0` = output motor berhenti. Arah dalam jadual ialah arah
robot selepas polariti setiap motor dibetulkan.

| D-pad | Gerakan | Motor kiri | Motor kanan |
|---|---|---|---|
| Atas | Maju | + | + |
| Bawah | Undur | - | - |
| Kiri | Pusing setempat kiri | - | + |
| Kanan | Pusing setempat kanan | + | - |
| Atas + kiri | Maju kiri | 0 | + |
| Atas + kanan | Maju kanan | + | 0 |
| Bawah + kiri | Undur kiri | 0 | - |
| Bawah + kanan | Undur kanan | - | 0 |

Arah diagonal menggerakkan satu roda sahaja. Kombinasi bertentangan seperti
atas + bawah dihentikan oleh kod.

### 2. Dua analog

- **Analog kiri, paksi Y:** atas = maju, bawah = undur.
- **Analog kanan, paksi X:** kiri/kanan = steering.
- Analog kiri X dan analog kanan Y tidak digunakan.
- Steering sahaja menghasilkan pusing setempat.
- Kedua-dua input digabungkan: `left = throttle + steering`,
  `right = throttle - steering`, kemudian dinormalkan kepada had PWM.
- Throttle dan steering menggunakan lengkung kuasa dua selepas deadband. Gerakan
  kecil joystick menghasilkan output lembut, manakala gerakan penuh masih 100%.
- Steering kanan sentiasa menghasilkan putaran badan ke kanan, termasuk
  semasa undur; ini ialah kawalan putaran differential-drive. D-pad diagonal
  pula memilih arah perjalanan seperti dalam jadual.

### Stop dan mula bergerak

- **Stop biasa:** lepaskan D-pad dan kembalikan analog ke neutral. Output motor
  terus menjadi sifar pada laporan input berikutnya; tidak perlu tekan Cross
  atau tunggu 300 ms untuk berhenti. Robot kekal ready.
- **Cross (×)** ialah stop tambahan yang mengatasi input lain, walaupun joystick
  masih ditolak. Ia menghentikan output motor dan membatalkan keadaan ready.
- Untuk kembali ready: lepaskan Cross, lepaskan D-pad dan neutralkan kedua-dua
  paksi analog yang digunakan selama **300 ms** dengan data controller diterima.
- Syarat neutral yang sama digunakan selepas startup, reconnect atau timeout.
- Tiada data baru daripada controller aktif selama **300 ms**: motor dihentikan.
- Controller disconnect: motor dihentikan apabila disconnect dikesan.
- Controller pertama yang bersambung mengawal robot. Callback controller tambahan
  atau touchpad DS4 diabaikan tanpa memutuskan controller aktif.
- Tidak perlu tahan R1 untuk memandu.

Stop bermaksud output PWM menjadi sifar. Roda mungkin masih bergerak kerana
inersia; ini bukan emergency stop fizikal. Software timeout memerlukan loop/CPU
masih berjalan. Gunakan suis kuasa untuk memutuskan kuasa jika perlu.

## Setup Arduino IDE

1. Tambah dua URL berikut dalam **Preferences → Additional Boards Manager URLs**:

   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json
   ```

2. Dalam Boards Manager, pasang package ESP32 rasmi mengikut panduan Bluepad32,
   serta package **ESP32 + Bluepad32 versi 4.1.0**. Versi package Bluepad32 inilah
   yang digunakan untuk build yang disahkan dalam projek ini.
3. Pilih **ESP32 Dev Module daripada menu ESP32 + Bluepad32**. Memilih board
   ESP32 biasa tidak menyediakan integrasi Bluetooth Bluepad32 yang diperlukan.
4. Dalam Library Manager, pasang **Cytron Motor Drivers Library versi 1.0.1**.
   Bluepad32 datang bersama board package; tidak perlu library PS4Controller.
5. Tetapkan **Flash Size: 8MB**, **Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)**.
   Pilih port USB Maker ESP32. Kekalkan tetapan lain pada default board package.
6. Buka `Arduino/MakerESP32_MobileRobot_PS4/MakerESP32_MobileRobot_PS4.ino`,
   kemudian Verify dan Upload. **Angkat roda daripada lantai semasa ujian awal.**
7. Buka Serial Monitor pada **115200 baud**.
8. Pada controller, tahan **SHARE + PS** sehingga light bar berkelip untuk pairing.
   Selepas mesej connected, neutralkan kawalan sehingga mesej `Ready` muncul.

Kod mengekalkan Bluetooth pairing keys semasa reset. Ia tidak memanggil
`forgetBluetoothKeys()` setiap boot. Jika controller cuba menyambung ke konsol
atau komputer lama, matikan sambungan tersebut dan cuba pairing SHARE + PS lagi.

## Tetapan dalam sketch

| Constant | Default | Kegunaan |
|---|---|---|
| `MAX_PWM` | 255 | Had PWM analog, 100% duty |
| `DPAD_PWM` | 128 | PWM minimum D-pad, kira-kira 50% |
| `R2_LIMIT` | 1023 | Nilai maksimum analog R2 untuk 100% PWM D-pad |
| `AXIS_DEADBAND` | 40 | Toleransi sekitar tengah joystick, julat paksi ±512 |
| `INPUT_TIMEOUT_MS` | 300 | Had masa tanpa data baru |
| `NEUTRAL_HOLD_MS` | 300 | Tempoh neutral sebelum ready |
| `INVERT_RIGHT` | false | Songsangkan arah motor kanan |
| `INVERT_LEFT` | false | Songsangkan arah motor kiri |

D-pad berubah secara linear dari kira-kira 50% hingga 100% PWM mengikut nilai
analog R2. Analog joystick menggunakan lengkung kuasa dua: separuh daripada julat
joystick yang boleh digunakan menghasilkan kira-kira 25% PWM, manakala gerakan
penuh menghasilkan 100%. Peratus PWM bukan jaminan peratus kelajuan fizikal. Jika
motor tidak mula berpusing pada PWM rendah, periksa bekalan, beban dan mekanikal
sebelum menambah input analog. Kod menggunakan deadband yang diskalakan semula,
tanpa minimum-PWM jump atau ramp.

## Ujian pada robot

1. Angkat roda. Pastikan tiada gerakan ketika startup, belum paired, atau pairing
   dibuat dengan joystick ditolak.
2. Selepas ready, tekan atas sebentar. Kedua-dua roda mesti memacu robot ke depan.
   Jika salah satu terbalik, ubah `INVERT_RIGHT` atau `INVERT_LEFT`, kemudian upload.
3. Semak semua lapan arah D-pad mengikut jadual dan kedua-dua analog berasingan.
4. Semak D-pad override analog dan peralihan kembali kepada analog semasa dilepas.
5. Uji Cross ketika bergerak: output berhenti, dan gerakan tidak bersambung semula
   sehingga kawalan neutral.
6. Matikan controller semasa memandu dengan roda terangkat. Semak motor berhenti
   dan reconnect dengan input ditahan tidak terus menggerakkan motor.
7. Uji perlahan di lantai; semak drift, arus/bekalan, motor panas, reset/brownout,
   respons Bluetooth dan jarak berhenti. Uji kelajuan biasa D-pad dahulu, kemudian
   tekan R2 secara beransur-ansur dan pastikan kelajuan meningkat lancar ke 100% PWM.

## Validation

Disahkan pada 9 September 2026:

- **Compile ESP32 lulus:** `esp32-bluepad32:esp32@4.1.0` dan
  `Cytron Motor Drivers Library@1.0.1`.
- FQBN: `esp32-bluepad32:esp32:esp32:FlashSize=8M,PartitionScheme=huge_app`.
- Sketch: **719281 bytes**, global RAM: **87228 bytes**.
- **Host logic tests lulus:** lapan arah D-pad dan kombinasi tidak sah, deadband,
  interpolasi kelajuan analog R2, lengkung kuasa dua analog joystick, had output,
  mixing, D-pad priority, release-to-stop tanpa Cross, neutral arming, Cross stop,
  timeout termasuk laporan baru selepas sela panjang, controller ownership dan
  `millis()` rollover.
- Pengendalian sambungan mengikut contoh rasmi Bluepad32 dengan menyimpan callback
  pertama tanpa menapis kelas device sebelum proses setup DS4 selesai.
- **Belum diuji pada hardware:** upload ke board, pairing PS4, arah motor,
  prestasi bekalan dan masa berhenti sebenar.

Compile menggunakan Arduino CLI setelah dependency dipasang:

```sh
arduino-cli compile \
  --fqbn 'esp32-bluepad32:esp32:esp32:FlashSize=8M,PartitionScheme=huge_app' \
  --build-path /tmp/maker-esp32-build-output \
  Arduino/MakerESP32_MobileRobot_PS4
```

Jalankan host tests dari root repository dengan compiler C++17:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -I tests/stubs \
  tests/test_controls.cpp -o /tmp/maker-esp32-test
/tmp/maker-esp32-test
```

Tests memasukkan sketch sebenar menggunakan stub Arduino/Bluepad32/motor.
Ia menguji logik arahan sahaja, bukan Bluetooth stack, output elektrik atau
pergerakan fizikal. Stub tidak digunakan oleh Arduino IDE.

## Rujukan rasmi

- [Maker ESP32](https://my.cytron.io/p-maker-esp32-bundle)
- [Robo ESP32 dan datasheet](https://my.cytron.io/p-robo-esp32)
- [Contoh pin motor Cytron](https://github.com/CytronTechnologies/Cytron-ROBO-ESP32/blob/main/Getting%20Started%20Guide/Arduino/DCMotor/DCMotor.ino)
- [Cytron Motor Driver Library](https://github.com/CytronTechnologies/CytronMotorDriver)
- [Panduan Arduino Bluepad32](https://bluepad32.readthedocs.io/en/latest/plat_arduino/)
- [Controller yang disokong dan pairing](https://bluepad32.readthedocs.io/en/latest/supported_gamepads/)
- [Tutorial ESP32 + PS4 Cytron](https://my.cytron.io/tutorial/esp32-ps4controller-beginner)
