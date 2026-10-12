# Bài 4 — remote điều khiển LED

Preset `Remote` dùng HS0038 trên PG5 và mạch LED của bài 1, 2.

| Nút | Byte mã lệnh | Hiện thập phân trên LED 7 thanh | Chức năng |
| --- | --- | --- | --- |
| 1 | 0C | 12 | Running spot L |
| 2 | 18 | 24 | Running spot R |
| 3 | 5E | 94 | Flash |
| 4 | 08 | 08 | Hai LED đối xứng |
| Play/Pause | 44 | 68 | Bật/tắt LED4 PG14, thay OK |

Remote không có nút OK nên dùng Play/Pause thay thế. Mã 44 được xác nhận
bằng lần bấm Play/Pause rồi 1, 2, 3, 4 trên PuTTY. Khi đổi remote,
đổi giá trị khởi tạo `Remote_ToggleKey` trong `App/lab_app.c` nếu cần.
Mỗi nút khác vẫn cập nhật LED 7 thanh bằng byte thứ ba, giữ nguyên hiệu ứng.
Các mã đã đo trên remote này đều nằm trong 00–99; thư viện hiện tại lấy
hai chữ số cuối cho giá trị lớn hơn 99.

```sh
cmake --preset Remote
cmake --build --preset Remote
python3 tools/ir_monitor.py --exercise Remote --putty
```

Dừng phiên monitor bài 3 trước bằng Ctrl+C để giải phóng ST-LINK và cổng
9000. Monitor bài 4 tự nạp firmware, in mã nút, giá trị LED 7 thanh,
mode hiệu ứng và trạng thái LED4 lên PuTTY.

Hiệu ứng được giữ đến khi chọn nút khác, mỗi bước 200 ms. Timer quét LED
7 thanh mỗi 5 ms. B1 không thay đổi LED4 ở preset này. Khung repeat NEC
không đảo LED4; firmware vẫn hoạt động độc lập khi không mở monitor.
