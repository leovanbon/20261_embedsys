# LED theo am thanh may tinh

Dung day LED PD8..PD15 da lap theo bai 3.1, chi can USB ST-LINK.
Hien thi mot thanh muc am luong 0..8 LED, cap nhat theo am thanh dang phat.
Day la thanh RMS, khong phai 8 cot pho tan nhu CAVA tren man hinh.

1. Dung debug VS Code bang Shift+F5.
2. Mo terminal trong thu muc project Lab1_20261.
3. Chay `python3 tools/audio_led.py`.
4. Mo nhac/video tren may. Ctrl+C de dung.

Script tu nap build/Debug/Lab1_20261.elf. Khong bam F5 khi script dang chay,
vi hai chuong trinh khong the cung chiem ST-LINK.

Tang do nhay: `python3 tools/audio_led.py --gain 3`.
Chi xem muc am thanh: `python3 tools/audio_led.py --preview`.
Kiem tra GPIO 0..8 LED: `python3 tools/audio_led.py --test-leds`.
Chon nguon am thanh khac: `python3 tools/audio_led.py --device TEN_MONITOR`.

Can Python 3, parec (PulseAudio/PipeWire), arm-none-eabi-gdb va cac bundle
ST-LINK GDB server / STM32CubeProgrammer da cai boi extension STM32.
Khong can pip hay USB-UART.

Firmware giu che do LED chay neu chua bat AudioLED_Enabled. Script bat che do
audio, gui mask va sequence qua SWD. Mat cap nhat tren 500 ms thi LED tat.
Script tam dung CPU trong luc ghi mailbox roi tiep tuc chay; phu hop demo
LED nay, khong dung cung luc voi do timing/ngat hong ngoai cua bai khac.

Da build thanh cong; da kiem tra 9 muc GPIO tren chip va doc duoc monitor
am thanh PipeWire. Anh sang thuc te con phu thuoc mach LED da noi.
