# Bài 3.2 — Hai LED 7 thanh và nút B1

Theo trang 3–4 của `Lab 1/Thuc hanh 1.pdf`, dùng hai LED **cathode chung**,
8 điện trở 330 Ω cho các thanh và 2 transistor NPN chọn chữ số.
Sơ đồ gốc dùng loại 5161AS; nếu dùng linh kiện khác hãy đối chiếu chân
theo datasheet, ký hiệu a–g và dp mới là phần cần nối tương ứng.

| STM32 | Thanh | Chân trên 5161AS trong hình 2 |
| --- | --- | --- |
| PE8 | a | 7 |
| PE9 | b | 6 |
| PE10 | c | 4 |
| PE11 | d | 2 |
| PE12 | e | 1 |
| PE13 | f | 9 |
| PE14 | g | 10 |
| PE15 | dp | 5 |

Mỗi đường PE8–PE15 đi qua điện trở 330 Ω rồi nối song song thanh tương
ứng của cả hai LED 7 thanh. Dấu chấm dp mặc định tắt.

| Chân chọn | Nối transistor | Vị trí hiển thị |
| --- | --- | --- |
| PG2 | Qua 10 kΩ vào base Q1 | Hàng đơn vị: 5 lúc khởi động |
| PG3 | Qua 10 kΩ vào base Q2 | Hàng chục: 2 lúc khởi động |

Collector nối hai chân cathode chung (3 và 8) của LED tương ứng;
emitter nối GND. Dùng GND chung với kit. Tra đúng chân B/C/E của
transistor đang dùng. Nếu hiện thành 52 thì đổi vị trí hai module hoặc
đổi dây chọn PG2 và PG3.

Chọn preset **Debug**, build và nạp `build/Debug/Lab1_20261.elf`.

Kết quả cần thấy:

- Ban đầu hiển thị **25**.
- Nhấn rồi nhả nút xanh **B1**: 26, 27, 28…
- Giữ nút không tăng liên tục; mỗi lần nhấn hợp lệ chỉ tăng một lần.
- Đến **99**, lần nhấn tiếp theo trở về **00**.
- LED đơn PD8–PD15 chạy nhanh, mỗi bước 5 ms theo yêu cầu bài 3.2.

`SevenSeg_Init()` cấu hình PE8–PE15, PG2, PG3 và đặt giá trị 25.
`Set7SegDisplayValue()` chuẩn bị mã hai chữ số trong một biến 16 bit.
`Run7SegDisplay()` tắt cả hai module trước khi đổi các thanh, rồi bật một
module. Cách này tránh hiện bóng chữ số trước.

Timer 6 sinh ngắt mỗi 100 µs. Callback đếm 50 ngắt rồi quét một chữ số,
tức 5 ms/chữ số, một vòng hai chữ số 10 ms (100 Hz). Vì quét trong ngắt,
việc chống dội nút và chạy hiệu ứng LED không làm màn hình nhấp nháy.

`Button_Pressed()` chỉ chấp nhận mức PA0 ổn định trong 30 ms, và phát
một sự kiện ở cạnh nhấn sau chống dội. Việc tăng số thực hiện trong
main loop. Callback B1 của ứng dụng vẫn giữ thao tác bấm đúp đổi LED3/LED4
ở bài 1–3; bài 4 dành LED4 cho remote.

Đã build hai preset Debug và LED, kiểm tra chống dội, giữ nút,
99 → 00 và mã quét chữ số bằng HAL giả lập. Chưa nạp/thử bài 3.2 trên
mạch LED 7 thanh thực tế trong lần cập nhật này.
