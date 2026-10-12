# Lab 1 — STM32F429 Discovery

Một project cho toàn bộ bài 1–4. Mở thư mục `Lab1_20261` trong VS Code.
Chọn bài bằng preset; mã ứng dụng và thư viện dùng chung.

| Bài | Preset | Chức năng |
| --- | --- | --- |
| 1 / mục 3.1 | LED | 4 hiệu ứng LED, 200 ms/bước, mỗi hiệu ứng 2 chu kỳ |
| 2 / mục 3.2 | Debug | LED 7 thanh hiện 25; B1 tăng số; LED đơn chạy nhanh |
| 3 / mục 3.3 | IR | Nhận NEC trên PG5, xem mã trên PuTTY qua ST-LINK |
| 4 / mục 4 | Remote | Hiện byte mã nút, nút 1–4 chọn hiệu ứng, Play/Pause đảo LED4 |
| 2 tối ưu | Release | Chức năng bài 2, tối ưu kích thước |

Build từ thư mục này, thay `Remote` bằng preset cần dùng:

```sh
cmake --preset Remote
cmake --build --preset Remote
```

Nạp/chạy bài 3 hoặc 4 và mở PuTTY trên Linux:

```sh
python3 tools/ir_monitor.py --exercise IR --putty
# Hoặc bài 4:
python3 tools/ir_monitor.py --exercise Remote --putty
```

Chỉ chạy một monitor tại một thời điểm. Ctrl+C dừng phiên đang chạy trước
khi đổi bài. PuTTY dùng **Raw → 127.0.0.1 → 9000**; monitor chuyển dữ liệu
qua cáp ST-LINK. Không cần USB–UART. Với bài 1/2, chọn preset trong CMake,
Configure/Build, rồi F5 bằng cấu hình `Lab1_20261 - ST-LINK`.

```text
App/lab_config.h              Chọn bài, timing, mã remote đã đo
App/lab_app.c                 Điều phối main loop, timer và callback EXTI
Libraries/leds/               4 hiệu ứng LED, tự chuyển hoặc giữ mode
Libraries/seven_segment/     Quét 2 LED cathode chung, hiển thị 00–99
Libraries/button/            Chống dội nút B1
Libraries/infrared/          Nhận khoảng xung và giải mã NEC từ mã mẫu
Core/                        Clock, TIM6, GPIO, các handler gọi HAL
tools/ir_monitor.py          Nhận mã và chuyển sang PuTTY
tools/stlink.py              Kết nối debugger dùng chung
Tests/                       Kiểm tra logic trên máy tính
```

Đọc [tổng hợp toàn bài và nối mạch](docs/00_tong_hop.md).
Chi tiết: [LED đơn](docs/01_led_don.md), [LED 7 thanh](docs/02_led_7_thanh.md),
[IR](docs/03_ir_remote.md), [remote điều khiển LED](docs/04_remote_led.md).
Chức năng [LED theo âm thanh](docs/audio_led.md) vẫn có ở bài 1/2.

Kiểm tra logic: `bash Tests/run.sh`. Kết quả firmware nằm tại
`build/<preset>/Lab1_20261.elf`; không sửa trực tiếp thư mục build.
