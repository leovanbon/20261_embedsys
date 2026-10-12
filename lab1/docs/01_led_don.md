# Phan 3.1 - Ghep noi LED don

Day LED noi PD8..PD15 qua tung dien tro 330 ohm den anode LED;
cathode LED noi GND theo Hinh 1 trong tai lieu.
Quy uoc sap xep: PD8 o ben phai, PD15 o ben trai.

GPIO duoc cau hinh output push-pull trong LED_Init() tai Libraries/leds/led_effects.c.
Bit 0 cua LED_Value dieu khien PD8, bit 7 dieu khien PD15.

DisplayLEDs(mode) phat tung trang thai cua hieu ung:

| Mode | Hieu ung | Chuoi mask hex trong mot chu ky |
| --- | --- | --- |
| 1 | Running spot L, phai sang trai | 00 01 02 04 08 10 20 40 80 00 |
| 2 | Running spot R, trai sang phai | 00 80 40 20 10 08 04 02 01 00 |
| 3 | Flash, bat/tat ca 8 LED | 00 FF 00 |
| 4 | Spot bumper, doi xung vao/ra | 00 81 42 24 18 18 24 42 81 00 |

Moi trang thai giu 200 ms. Ket thuc chuoi, LED_CycleDone bao mot chu ky
da hoan thanh. LED_RunEffects() tang LED_Cycles, du 2 chu ky thi chuyen LED_Mode.
Sau mode 4 quay lai mode 1. Khong can bam B1 de chay day LED.
Neu lap PD8 o ben trai thi chieu vat ly cua mode 1 va 2 se dao nguoc.

Chu ky day du 4 mode: (10+10+3+10)*2*200 ms = 13.2 giay.
Co trang thai tat dau/cuoi moi chu ky va giu cap LED giua hai nhip
theo chuoi minh hoa trong PDF.

Build bai LED don: chon preset LED trong VS Code, Configure va Build.
Nap: dung script am thanh truoc, F5 de nap/debug, F5 tiep neu dung o main.
Ban da nap vao board 32F429IDISCOVERY va verify Flash thanh cong.

Cho bao cao: chup anh mach, quay video du 13.2 giay de thay het 4 mode,
dua bang mask, giai thich GPIO va bien dem chu ky vao bao cao.
Tinh nang audio chi duoc bat khi chay tools/audio_led.py, doc docs/audio_led.md.
