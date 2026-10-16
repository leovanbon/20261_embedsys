# BÁO CÁO BÀI THỰC HÀNH 1

**Học phần:** Hệ nhúng  
**Nội dung:** Ghép nối GPIO, ngắt ngoài và timer

| Họ và tên | MSSV |
| --- | --- |
| Nguyễn Gia Khánh | 202416803 |
| Đào Huy Hoàng | 202400045 |

## 1. Bài 3.1 — Giải thích phần code viết thêm

Chương trình điều khiển tám LED qua PD8–PD15. Biến `LED_Value` chứa trạng thái các LED; bit 0 tương ứng PD8, bit 7 tương ứng PD15. Hàm `DisplayLEDs(mode)` lấy trạng thái từ bảng `effects`, sau đó dùng `HAL_GPIO_WritePin()` để bật/tắt từng LED.

| Mode | Hiệu ứng | Chuỗi trạng thái hexadecimal |
| --- | --- | --- |
| 1 | Một LED chạy từ phải sang trái | `00 01 02 04 08 10 20 40 80 00` |
| 2 | Một LED chạy từ trái sang phải | `00 80 40 20 10 08 04 02 01 00` |
| 3 | Bật/tắt đồng thời tám LED | `00 FF 00` |
| 4 | Hai LED đối xứng chạy từ ngoài vào trong rồi ngược lại | `00 81 42 24 18 18 24 42 81 00` |

Biến `step` xác định trạng thái hiện tại trong chuỗi. Khi hết chuỗi, chương trình đặt `LED_CycleDone`. Hàm `LED_RunEffects()` đếm chu kỳ bằng `LED_Cycles`; đủ hai chu kỳ thì chuyển sang hiệu ứng tiếp theo, sau mode 4 quay lại mode 1.

Mỗi bước cách nhau 200 ms, được kiểm tra bằng `HAL_GetTick()` để không chặn chương trình bằng delay dài. Phần điều phối nằm trong `Bai1_Process()` tại `App/lab_app.c`; bảng hiệu ứng và xử lý GPIO nằm trong `Libraries/leds/led_effects.c`.

## 2. Bài 3.2 — Giải thích phần code viết thêm

Hai LED 7 thanh cathode chung dùng chung các đường a–g, dp trên PE8–PE15; PG2 chọn digit đơn vị và PG3 chọn digit hàng chục qua transistor NPN.

`SevenSeg_Init()` đặt giá trị ban đầu là 25. Hàm `Set7SegDisplayValue()` tách hàng chục, hàng đơn vị, tra bảng mã segment và lưu mã của hai digit vào `segment_frame`. Giá trị được lấy modulo 100 nên sau 99 quay về 00.

```c
if (Button_Pressed(now))
  Set7SegDisplayValue(Get7SegDisplayValue() + 1);
```

`Button_Pressed()` chống dội 30 ms và chỉ báo một sự kiện khi nút B1 chuyển sang trạng thái nhấn ổn định. Vì vậy mỗi lần bấm tăng một đơn vị; giữ nút không tăng liên tục.

TIM6 tạo ngắt mỗi 100 µs. Callback đếm 50 ngắt rồi gọi `Run7SegDisplay()`, tức quét một digit mỗi 5 ms. Hàm quét tắt cả hai digit, cập nhật các thanh rồi bật digit cần hiển thị. Một vòng hai digit mất 10 ms, tương ứng tần số làm mới 100 Hz cho mỗi digit.

Trong main loop, dãy tám LED đơn vẫn chạy với bước 5 ms theo yêu cầu đề. Việc quét LED 7 thanh thực hiện trong ngắt nên độc lập với xử lý B1 và hiệu ứng LED. Code nằm trong `Bai2_Process()`, `Libraries/seven_segment/7seg.c` và `Libraries/button/button.c`.

## 3. Bài 3.3 — Trả lời các câu hỏi phần tự làm

### Câu 1. Biểu đồ tín hiệu trên PG5 khi bấm remote

OUT của HS0038 nối PG5. Khi không nhận burst hồng ngoại, OUT ở mức 1; khi nhận burst, OUT xuống mức 0. HS0038 đã giải điều chế sóng mang 38 kHz, nên PG5 nhận đường bao của tín hiệu.

```text
             Leader                  Bit 0                 Bit 1
PG5:  ──┐             ┌────────┐          ┌─────┐          ┌──────────┐
        │             │        │          │     │          │          │
        └─────────────┘        └──────────┘     └──────────┘          └─ ...
        ↓                      ↓                ↓                     ↓
        t0                     t1               t2                    t3

LOW:    9 ms                   562,5 µs          562,5 µs
HIGH:   4,5 ms                 562,5 µs          1687,5 µs

t1 − t0 = 13,5 ms; t2 − t1 = 1,125 ms; t3 − t2 = 2,25 ms.
```

Biểu đồ minh họa leader và hai bit 0, 1, không vẽ theo cùng tỷ lệ thời gian. Toàn khung gồm leader, 32 bit dữ liệu và burst stop. Với NEC chuẩn, bốn byte là địa chỉ, đảo địa chỉ, command và đảo command; mỗi byte truyền bit thấp trước (LSB first).

### Câu 2. EXTI5 dùng Rising edge hay Falling edge? Tại sao?

PG5 được cấu hình **Falling edge** (`GPIO_MODE_IT_FALLING`). Mỗi cạnh xuống đánh dấu bắt đầu một burst. Đo khoảng giữa hai cạnh xuống liên tiếp thu được tổng thời gian LOW và HIGH của thành phần trước, từ đó phân biệt leader, bit 0 và bit 1.

EXTI line 5 sử dụng vector `EXTI9_5_IRQHandler()`. Handler gọi HAL; callback chuyển sự kiện PG5 đến hàm `IrGetBitTime()`.

### Câu 3. Làm thế nào để đo thời gian các bit?

TIM6 có clock 90 MHz, prescaler 89 và auto-reload 99:

```text
f_ngắt = 90 000 000 / ((89 + 1) × (99 + 1)) = 10 000 Hz
T_ngắt = 100 µs
```

Mỗi ngắt timer, `IR_Tick()` tăng bộ đếm `ir_ticks`. Khi có cạnh xuống PG5, `IrGetBitTime()` đọc bộ đếm rồi đặt về 0. Khoảng thời gian đo xấp xỉ `ir_ticks × 100 µs`. Cạnh xuống đầu tiên tạo mốc đo; cạnh tiếp theo mới cho khoảng leader. Bộ đếm IR riêng giúp thao tác B1 không làm sai phép đo.

### Câu 4. Căn cứ vào đâu để xác định start, bit 0, bit 1?

NEC phân biệt bit bằng thời gian giữa các burst. Cửa sổ nhận trong code như sau:

| Thành phần | Thời gian danh định giữa hai cạnh xuống | Số tick chấp nhận |
| --- | --- | --- |
| Start/leader | 9 ms + 4,5 ms = 13,5 ms | 120–150 |
| Bit 0 | 562,5 µs + 562,5 µs = 1,125 ms | 8–15 |
| Bit 1 | 562,5 µs + 1687,5 µs = 2,25 ms | 18–27 |

Sau khi kiểm tra khoảng đo hợp lệ, `IrDecode()` coi khoảng lớn hơn 17 tick là bit 1, nhóm còn lại là bit 0. Dùng cửa sổ để chấp nhận sai số của phép đo và xử lý ngắt.

Phần mô tả ở trang 5 tài liệu ghi bit 0 là 1,25 ms; theo tổng độ rộng xung và phụ lục trong tài liệu, giá trị đúng là **1,125 ms**. Khung repeat NEC khi giữ nút không được giải mã như một command mới trong chương trình này.

### Câu 5. Khi nào giải mã xong? Mã lệnh được để ở đâu?

`bit_time[0]` lưu khoảng leader, `bit_time[1..32]` lưu 32 khoảng dữ liệu. Khi đủ 33 khoảng đo, `command_ok = 1`. Cạnh xuống bắt đầu burst stop hoàn tất phép đo bit dữ liệu cuối. Đây là 32 bit dữ liệu cộng một khoảng đồng bộ, không phải 33 bit dữ liệu.

Main gọi `IR_ProcessFrame()` rồi `IrDecode()` để dựng khung LSB first. Command chỉ được công nhận khi XOR với byte đảo bằng `0xFF`. Trong lúc chờ giải mã, cờ `command_ok` ngăn ngắt ghi đè bộ đệm; sau đó cờ được xóa để nhận tiếp.

Kết quả lưu tại:

- `irda_cmd[0..3]`: bốn byte theo thứ tự nhận; byte thứ ba `irda_cmd[2]` là mã nút.
- `IR_LastKey`: mã nút vừa giải mã.
- `IR_LastFrame`: toàn bộ khung, byte nhận đầu tiên nằm ở tám bit thấp.
- `IR_FrameCount`: số khung hợp lệ.

Ví dụ nút OK nhận được `00 FF 1C E3`, nên `IR_LastKey = 0x1C = 28` và `IR_LastFrame = 0xE31CFF00`. Code kiểm tra cặp command/đảo command, không bắt buộc hai byte địa chỉ đảo nhau để hỗ trợ cả NEC mở rộng.

Trong bản thực hành trên Linux, script Python đọc kết quả qua ST-LINK tại `IR_FrameReady()` và chuyển sang PuTTY bằng Raw TCP `127.0.0.1:9000`, thay cho đường UART/Hercules trong tài liệu mẫu.

## 4. Bài 4 — Giải thích chương trình kết hợp

Hàm `Bai4_Process()` nhận khung hợp lệ, xử lý mã nút rồi tiếp tục chạy hiệu ứng LED:

```c
static void Bai4_Process(void)
{
  if (IR_ProcessFrame())
  {
    Bai4_HandleKey(IR_LastKey);
    IR_FrameReady();
  }
  LED_RunSelected(HAL_GetTick(), LAB_LED_INTERVAL_MS);
}
```

Trong `Bai4_HandleKey()`, chương trình luôn cập nhật `Set7SegDisplayValue(key)` để hiển thị byte thứ ba ở dạng thập phân. Nếu là OK thì đảo `Remote_LED4State` và xuất trạng thái lên PG14. Nếu là nút 1–4 thì gọi `LED_SelectMode()` với mode tương ứng.

| Nút | Khung đo trên PuTTY | LED 7 thanh | Chức năng |
| --- | --- | --- | --- |
| 1 | `00 FF 45 BA` | 69 | Hiệu ứng 1 |
| 2 | `00 FF 46 B9` | 70 | Hiệu ứng 2 |
| 3 | `00 FF 47 B8` | 71 | Hiệu ứng 3 |
| 4 | `00 FF 44 BB` | 68 | Hiệu ứng 4 |
| OK | `00 FF 1C E3` | 28 | Bật/tắt LED4 trên kit, chân PG14 |

`LED_RunSelected()` giữ hiệu ứng đã chọn đến khi nhận nút chọn khác, mỗi bước 200 ms. Quét LED 7 thanh vẫn chạy trong ngắt timer. Ở bài 4, chức năng bấm đúp B1 đảo LED4 được tắt để tránh xung đột với remote.

Kết quả monitor sau khi cập nhật mã đã ghi nhận nút 1 chọn `effect = 1`; các lần bấm OK liên tiếp làm trạng thái LED4 đổi 1 rồi 0. Giá trị in `7seg` là giá trị phần mềm yêu cầu hiển thị; hiện tượng độ sáng không đều của LED 7 thanh còn cần kiểm tra lại trên mạch.

**Mã nguồn:** [Điều phối các bài](App/lab_app.c), [LED đơn](Libraries/leds/led_effects.c), [LED 7 thanh](Libraries/seven_segment/7seg.c), [giải mã IR](Libraries/infrared/irda.c), [cấu hình mã nút](App/lab_config.h).
