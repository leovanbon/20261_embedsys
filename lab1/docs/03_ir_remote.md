# Bài 3 — nhận remote hồng ngoại qua ST-LINK

Nguồn gốc: `Lab 1/IrRemoteControl.zip`. Hai hàm `IrGetBitTime()` và
`IrDecode()` được ghép vào project trong `Libraries/infrared/irda.c`.
Bản nguyên gốc được giữ trong `Libraries/infrared/sample/` để đối chiếu.

Nối HS0038: S/OUT → PG5, + → 3V, − → GND. Giữ cáp USB ST-LINK.

Từ thư mục `Lab1_20261`:

```sh
cmake --preset IR
cmake --build --preset IR
python3 tools/ir_monitor.py
```

Để mở PuTTY và xem mã trong cửa sổ PuTTY:

```sh
python3 tools/ir_monitor.py --putty
```

Nếu tự mở PuTTY, chọn **Raw**, Host Name **127.0.0.1**, Port **9000**,
rồi Open khi script monitor đang chạy. Monitor chuyển mã đọc từ ST-LINK
thành luồng TCP trên máy tính. Cổng 9000 là TCP, không phải cổng serial.

Dừng phiên debug VS Code hoặc script âm thanh trước khi chạy để giải phóng
ST-LINK. Script tự nạp firmware bài 3. Bấm remote hướng vào cảm biến;
terminal in bốn byte theo thứ tự truyền và byte mã nút, ví dụ minh họa:

```text
#1  NEC: 00 FF AD 52  | key = 0xAD (173)
```

Đây là ví dụ định dạng, không phải mã mặc định của remote đang dùng.
Ctrl+C dừng monitor. Có thể thử có thời hạn:
`python3 tools/ir_monitor.py --seconds 20`; cuối lượt sẽ in số cạnh PG5,
số khung hợp lệ và khoảng đo cuối. Không có cạnh khi đã bấm remote thì
kiểm tra nguồn, chân OUT, GND chung và pin remote.

Firmware dùng TIM6 tick 100 µs và ngắt cạnh xuống PG5. Khoảng đo
120–150 tick mở đầu khung, 8–15 tick là bit 0, 18–27 tick là bit 1.
Sau start và 32 bit, main giải mã LSB trước; kiểm tra byte mã nút và
byte đảo có XOR = FF. Repeat không tạo sự kiện mã nút mới.
Bộ đếm IR riêng tránh bị nút B1 đặt lại.

Monitor đặt breakpoint ở `IR_FrameReady()` sau khi hoàn tất khung,
đọc biến qua debugger rồi tiếp tục ngay. Nó không dừng giữa khung để
thăm dò định kỳ. Việc in vẫn tạm dừng CPU trong một khoảng ngắn,
nên đây là cách quan sát từng lần bấm, không phải bộ ghi tín hiệu liên tục.
PuTTY dùng kết nối Raw tới monitor; firmware không dùng UART ở bản này.

Watch trong debugger: `irda_cmd`, `IR_LastKey`, `IR_LastFrame`,
`IR_FrameCount`, `IR_EdgeCount`, `IR_LastInterval`, `bit_time`.
Chọn preset IR khi debug bài này; Debug/LED vẫn là bài 2/bài 1.
