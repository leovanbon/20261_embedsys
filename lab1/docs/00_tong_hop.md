# Tổng hợp code Lab 1

## Chọn bài

| Preset | LAB_EXERCISE | Nội dung trong PDF |
| --- | --- | --- |
| LED | 1 | 3.1: bốn hiệu ứng trên PD8–PD15 |
| Debug / Release | 2 | 3.2: hai LED 7 thanh, B1 tăng số |
| IR | 3 | 3.3: đo xung, giải mã remote |
| Remote | 4 | 4: kết hợp remote, LED đơn, LED 7 thanh |

`CMakePresets.json` chọn `LAB_EXERCISE`; `App/lab_config.h` định nghĩa
các tính năng cần bật, thời gian chạy và mã nút. CMake chỉ biên dịch
các thư viện cần thiết cho bài đã chọn.

## Nối mạch dùng chung

| Thiết bị | Chân STM32 | Cách nối |
| --- | --- | --- |
| HS0038 OUT/S | PG5 | Ngắt cạnh xuống |
| HS0038 +, − | 3V, GND | Cấp nguồn, GND chung |
| 8 LED đơn | PD8–PD15 | Mỗi chân qua 330 Ω tới anode; cathode về GND |
| Thanh a,b,c,d,e,f,g,dp | PE8–PE15 | Qua 8 điện trở 330 Ω, chia tới hai LED |
| Chọn hàng đơn vị (phải) | PG2 | Qua 10 kΩ vào base NPN Q1 |
| Chọn hàng chục (trái) | PG3 | Qua 10 kΩ vào base NPN Q2 |
| Cathode chung mỗi LED | Collector Q1/Q2 | Emitter transistor về GND |
| LED4 trên kit | PG14 | Có sẵn, không cần LED ngoài |
| Nút B1 trên kit | PA0 | Có sẵn |

Dùng LED 7 thanh cathode chung. Với 5161AS theo PDF,
chân a,b,c,d,e,f,g,dp là 7,6,4,2,1,9,10,5; cả chân chung 3 và 8
nối collector. Loại khác phải đối chiếu pinout. Transistor cần xác định
đúng B/C/E. Sắp dãy LED đơn PD15 bên trái, PD8 bên phải.

## Vị trí code từng bài

Toàn bộ phần điều phối bài tập được gom trong `App/lab_app.c`, theo thứ tự:

1. Phần dùng chung hỗ trợ LED theo âm thanh.
2. Bài 1: `Bai1_Init()` và `Bai1_Process()`.
3. Bài 2: `Bai2_Init()` và `Bai2_Process()`.
4. Bài 3: `Bai3_Init()` và `Bai3_Process()`.
5. Bài 4: `Bai4_Init()`, `Bai4_HandleKey()` và `Bai4_Process()`.
6. Chọn bài theo preset: `LabApp_Init()` và `LabApp_Process()`.
7. Callback ngắt timer và EXTI dùng chung.

Mỗi khối có chú thích bài, preset và thư viện liên quan. Chỉ khối ứng với
preset đã chọn được biên dịch. Để sửa hành vi bài nào, bắt đầu từ khối
đó; để sửa bảng hiệu ứng, quét LED hoặc giải mã, mở thư viện tương ứng.

## Luồng chương trình

`main.c` khởi tạo HAL, clock 180 MHz, GPIO, TIM6; gọi `LabApp_Init()`;
sau đó lặp `LabApp_Process()` với nghỉ 1 ms. Hiệu ứng không dùng delay
200 ms để tránh chặn xử lý remote và nút.

TIM6 chạy 90 MHz, PSC=89 và ARR=99: ngắt mỗi 100 µs.
`TIM6_DAC_IRQHandler()` gọi HAL; callback trong `lab_app.c` tăng thời
gian IR, theo dõi khoảng bấm B1 và quét một chữ số mỗi 50 tick (5 ms).
Các handler EXTI cũng chỉ gọi HAL; callback chuyển PG5 sang bộ nhận IR.

## Bài 1: LED đơn

`DisplayLEDs(mode)` lấy mẫu từ bảng hiệu ứng, xuất bit 0–7 lên PD8–PD15.
`LED_RunEffects()` chạy mỗi 200 ms, đủ hai chu kỳ thì chuyển mode 1→2→3→4→1.
`LED_SelectMode()` bắt đầu lại hiệu ứng; `LED_RunSelected()` giữ mode
cho bài 4. Cả hai cách chạy dùng chung một hàm định thời.

## Bài 2: LED 7 thanh và B1

`SevenSeg_Init()` hiển thị 25. `Button_Pressed()` chống dội 30 ms;
mỗi lần nhấn hợp lệ tăng số một đơn vị, 99 quay về 00.
`Set7SegDisplayValue()` cập nhật đồng thời mã hai chữ số.
`Run7SegDisplay()` tắt hai digit trước khi đổi các thanh rồi bật một digit,
luân phiên đơn vị và hàng chục. LED đơn chạy mỗi 5 ms.

## Bài 3: remote hồng ngoại

Nguồn gốc hai hàm `IrGetBitTime()`/`IrDecode()` là `IrRemoteControl.zip`;
bản nguyên gốc lưu trong `Libraries/infrared/sample/`.
OUT của HS0038 bình thường ở mức 1, có burst thì xuống 0. Đo thời gian
giữa hai cạnh xuống bằng bộ đếm IR riêng, đơn vị 100 µs:

| Tín hiệu | Thời gian danh định | Khoảng chấp nhận |
| --- | --- | --- |
| Start | 13,5 ms | 120–150 tick |
| Bit 0 | 1,125 ms | 8–15 tick |
| Bit 1 | 2,25 ms | 18–27 tick |

`bit_time[0]` lưu start; 32 vị trí còn lại lưu dữ liệu. Khi nhận đủ,
`command_ok` khóa bộ đệm; main gọi `IR_ProcessFrame()` để giải mã LSB
trước, kiểm tra byte command và byte đảo XOR bằng FF, rồi trả bộ đệm
cho ngắt. Khung lỗi và repeat không tạo mã nút mới. Chấp nhận cả địa chỉ
NEC chuẩn và mở rộng, chỉ kiểm tra cặp command/đảo command.

Kết quả: `irda_cmd[0..3]` là địa chỉ, byte địa chỉ thứ hai, mã nút,
byte đảo mã nút. `IR_LastKey`, `IR_LastFrame`, `IR_FrameCount` có thể xem
trong Watch. `IR_Init()` và `IR_Tick()` quản lý trạng thái và thời gian.

## Bài 4: kết hợp

| Nút đã đo | Byte thứ ba | LED 7 thanh (thập phân) | Hành động |
| --- | --- | --- | --- |
| OK | 1C | 28 | Đảo LED4 |
| 1 | 45 | 69 | Chấm chạy phải sang trái |
| 2 | 46 | 70 | Chấm chạy trái sang phải |
| 3 | 47 | 71 | Nháy cả tám LED |
| 4 | 44 | 68 | Hai LED đối xứng vào/ra |

Các nút khác chỉ cập nhật giá trị hiển thị. Hiệu ứng đã chọn tiếp tục
đến khi chọn nút khác. B1 không đảo LED4 ở bài 4. LED 7 thanh hiện byte
mã nút, không phải nhãn số in trên remote. Thư viện hiện 00–99; giá trị
lớn hơn 99 lấy hai chữ số cuối (các mã đã đo trên remote này đều ≤99).

## Build, nạp và xem PuTTY

Từ thư mục `Lab1_20261`:

```sh
cmake --preset Remote
cmake --build --preset Remote
python3 tools/ir_monitor.py --exercise Remote --putty
```

Thay `Remote` bằng `IR` để chỉ đo mã bài 3. PuTTY nhận Raw TCP từ
`127.0.0.1:9000`; chương trình Python đọc dữ liệu từ kit qua ST-LINK.
Breakpoint đặt ở `IR_FrameReady()` sau khi đã xử lý khung, đọc biến rồi
chạy tiếp. Việc này tạm dừng CPU ngắn khi in mã; LED vẫn chạy độc lập
khi không dùng monitor. Đây là cách xem từng lần bấm, không phải bộ ghi
xung liên tục. Chỉ một debugger/monitor dùng ST-LINK tại một thời điểm.

## Kiểm tra

```sh
bash Tests/run.sh
```

Kiểm tra chống dội, giữ B1, vòng 99→00, quét LED, timeout âm thanh,
giải mã NEC LSB-first, byte đảo sai, repeat, khôi phục khung lỗi,
ánh xạ 1–4, giữ hiệu ứng và OK đảo LED4. HAL giả chỉ kiểm tra
logic; cần quan sát LED thực tế để kiểm tra dây nối.
