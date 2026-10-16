# Hệ nhúng — 20261

Repository thực hành hệ nhúng trên kit **STM32F429 Discovery**.
Project Lab 1 nằm trong [`lab1/`](lab1/), gồm GPIO, LED đơn, LED 7 thanh,
nút B1 và giải mã remote hồng ngoại NEC.

## Chuẩn bị trên Linux

- Kit STM32F429 Discovery và cáp USB cắm vào cổng **ST-LINK**.
- CMake, Ninja, Python 3, PuTTY và trình biên dịch C trên máy tính (`cc`).
- Arm GNU Toolchain: `arm-none-eabi-gcc`, thư viện C/newlib và
  `arm-none-eabi-gdb` có trong `PATH`.
- Bộ công cụ STM32Cube: **ST-LINK GDB server** và **STM32CubeProgrammer**.
- Nếu dùng VS Code: extension **STM32CubeIDE for Visual Studio Code** và
  **CMake Tools**, cùng các tool bundle trong `lab1/.settings/`.

Cài công cụ bằng trình quản lý gói của bản phân phối hoặc bộ cài toolchain,
rồi kiểm tra các lệnh:

```sh
command -v cmake ninja python3 putty cc arm-none-eabi-gcc arm-none-eabi-gdb
```

Monitor hiện tìm công cụ ST trong các thư mục sau:

```text
~/.local/share/stm32cube/bundles/stlink-gdbserver/*/bin/ST-LINK_gdbserver
~/.local/share/stm32cube/bundles/programmer/*/bin/STM32_Programmer_CLI
```

Nếu cài ở vị trí khác, sửa mẫu đường dẫn `latest_tool(...)` trong
[`lab1/tools/ir_monitor.py`](lab1/tools/ir_monitor.py).
Python chỉ dùng thư viện chuẩn, không cần `pip install`.

## Chạy nhanh bài 4

Từ thư mục chứa repository:

```sh
cd 20261_embedsys/lab1
cmake --preset Remote
cmake --build --preset Remote
python3 tools/ir_monitor.py --exercise Remote --putty
```

Nếu terminal đang ở gốc repository `20261_embedsys`, chỉ cần `cd lab1`.
Cắm kit và nối mạch trước khi chạy monitor. Script tự nạp firmware qua
ST-LINK, chạy chương trình và mở PuTTY. Bấm remote sau dòng `READY`.

Các lệnh bên dưới đều chạy từ thư mục **`lab1/`**.

## Chọn bài và build

| Bài / mục trong PDF | Preset | Chức năng |
| --- | --- | --- |
| 1 / 3.1 | `LED` | Bốn hiệu ứng LED, 200 ms/bước; mỗi hiệu ứng lặp hai chu kỳ |
| 2 / 3.2 | `Debug` | Hiện 25; B1 tăng số; LED đơn chạy mỗi 5 ms |
| 3 / 3.3 | `IR` | Nhận NEC và xem bốn byte mã lệnh trên PuTTY |
| 4 | `Remote` | Hiện mã nút, chọn hiệu ứng LED và bật/tắt LED4 |
| 2, tối ưu kích thước | `Release` | Chức năng bài 2 với tối ưu Release |

Ví dụ build bài 1:

```sh
cmake --preset LED
cmake --build --preset LED
```

Thay `LED` bằng preset cần dùng. Firmware được tạo tại
`build/<preset>/Lab1_20261.elf`. Tên firmware vẫn là `Lab1_20261`, dù
thư mục project đã đổi thành `lab1`. Build và cache không đưa lên Git.

## Nạp bài 1–2 bằng VS Code

1. Mở **thư mục `lab1/`** trong VS Code: `code .`.
2. Chọn **CMake: Select Configure Preset** → `LED`, `Debug` hoặc `Release`.
3. Chạy **CMake: Configure**, sau đó **CMake: Build**.
4. Trong Run and Debug, chọn **Lab1_20261 - ST-LINK**, nhấn **F5**.
5. Nếu debugger dừng tại `main()`, nhấn **F5** lần nữa để chạy.

Nút B1 tăng số một lần sau mỗi lần nhấn hợp lệ ở bài 2; giữ nút không
đếm thêm. Giá trị hiển thị quay vòng từ 99 về 00.

## Nhận mã remote và xem trên PuTTY

Build và chạy bài 3:

```sh
cmake --preset IR
cmake --build --preset IR
python3 tools/ir_monitor.py --exercise IR --putty
```

Ví dụ đầu ra khi bấm OK trên remote đã đo:

```text
#1  NEC: 00 FF 1C E3  | key = 0x1C (28)
```

Bốn byte lần lượt là địa chỉ, byte địa chỉ thứ hai, mã nút và byte đảo mã
nút. `key` chính là byte thứ ba. Mã có thể khác khi dùng remote khác.

PuTTY dùng **Raw TCP**, với **Host Name `127.0.0.1`**, **Port `9000`**.
Monitor chuyển dữ liệu đọc qua ST-LINK sang kết nối này; không dùng cổng
Serial `/dev/ttyUSB*` và không cần USB–UART cho cách chạy hiện tại.

Để tự mở PuTTY, chạy monitor không có `--putty`, rồi kết nối Raw như trên:

```sh
python3 tools/ir_monitor.py --exercise IR
```

Nhấn **Ctrl+C trong terminal chạy script** để dừng. Dừng monitor cũ
trước khi đổi bài; đóng cửa sổ PuTTY không tự dừng script.
Chỉ một phiên debugger hoặc monitor được dùng ST-LINK tại một thời điểm.

## Điều khiển LED bằng remote — bài 4

```sh
cmake --preset Remote
cmake --build --preset Remote
python3 tools/ir_monitor.py --exercise Remote --putty
```

| Nút | Mã đã đo | LED 7 thanh | Hành động |
| --- | --- | --- | --- |
| OK | `0x1C` | `28` | Bật/tắt LED4 |
| 1 | `0x45` | `69` | Chấm chạy từ phải sang trái |
| 2 | `0x46` | `70` | Chấm chạy từ trái sang phải |
| 3 | `0x47` | `71` | Nháy cả tám LED |
| 4 | `0x44` | `68` | Hai LED đối xứng chạy vào/ra |

LED 7 thanh hiện **byte mã nút ở dạng thập phân**, không phải số in trên
remote. Thư viện hiện hai chữ số cuối nếu mã lớn hơn 99. Hiệu ứng giữ
đến khi chọn nút khác; khung repeat NEC không đảo LED4. Khi đổi remote,
đo lại mã bằng bài 3 rồi sửa [`lab1/App/lab_config.h`](lab1/App/lab_config.h).

## Nối mạch

| Thiết bị | Chân STM32 | Kết nối |
| --- | --- | --- |
| HS0038 S/OUT | PG5 | Nhận IR bằng ngắt cạnh xuống |
| HS0038 + / − | 3V / GND | Cấp nguồn, dùng GND chung |
| Tám LED đơn | PD8–PD15 | Mỗi chân qua 330 Ω đến anode; cathode về GND |
| Thanh a–g, dp | PE8–PE15 | Qua tám điện trở 330 Ω; nối các thanh tương ứng của hai LED |
| Chọn digit đơn vị, bên phải | PG2 | Qua 10 kΩ đến base transistor NPN Q1 |
| Chọn digit hàng chục, bên trái | PG3 | Qua 10 kΩ đến base transistor NPN Q2 |
| Cathode chung mỗi digit | Collector Q1/Q2 | Emitter về GND |
| LED4 và B1 | PG14 và PA0 | Đã có sẵn trên kit |

Dùng **LED 7 thanh cathode chung**. Đối chiếu pinout LED và chân B/C/E
của transistor trước khi nối. Xếp PD15 bên trái, PD8 bên phải để hiệu ứng
đúng chiều. Chi tiết: [sơ đồ và tổng hợp toàn bài](lab1/docs/00_tong_hop.md).

## Kiểm tra logic

Không cần kit để chạy:

```sh
bash Tests/run.sh
```

Ba bộ kiểm tra dùng HAL giả lập: nút B1/LED 7 thanh, giải mã NEC, và
remote điều khiển LED. Các bài kiểm tra không xác nhận dây nối thực tế.

## Khi không chạy được

| Hiện tượng | Kiểm tra |
| --- | --- |
| `Build first...` | Configure và build đúng preset `IR` hoặc `Remote` trước khi chạy monitor |
| `STM32Cube tool not found` | Kiểm tra hai đường dẫn tool bundle ở phần chuẩn bị |
| Không tìm thấy `arm-none-eabi-*` | Bổ sung thư mục `bin` của Arm toolchain vào `PATH` |
| `No ST-LINK found` | Kiểm tra cáp có truyền dữ liệu, cổng ST-LINK và thiết bị bằng `lsusb`; kiểm tra quyền USB/udev của bộ cài ST |
| ST-LINK đang bận | Dừng phiên debug VS Code, monitor hoặc script âm thanh đang dùng kit |
| Cổng 9000 đang được dùng | Dừng monitor cũ hoặc chạy với `--tcp-port 9001 --putty` |
| Không có mã khi bấm remote | Kiểm tra nguồn HS0038, OUT→PG5, GND chung và pin remote |
| LED hiện ngược hàng chục/đơn vị | Kiểm tra PG2 chọn digit bên phải, PG3 chọn digit bên trái |
| CMake báo đường dẫn cũ sau khi chuyển thư mục | Xóa thư mục build của preset bị lỗi rồi configure lại |

## Cấu trúc mã nguồn

```text
lab1/
├── App/          Cấu hình bài, mã remote và điều phối chương trình
├── Core/         Clock, GPIO, TIM6 và handler ngắt
├── Drivers/      CMSIS, STM32 HAL
├── Libraries/    LED đơn, LED 7 thanh, B1 và IR
├── cmake/        Toolchain và cấu hình build
├── tools/        Monitor ST-LINK/PuTTY và tiện ích âm thanh
├── Tests/        Kiểm tra logic trên máy tính
└── docs/         Giải thích bài làm và nối mạch
```

Tài liệu từng phần: [LED đơn](lab1/docs/01_led_don.md),
[LED 7 thanh](lab1/docs/02_led_7_thanh.md),
[IR](lab1/docs/03_ir_remote.md), [bài 4](lab1/docs/04_remote_led.md).
