# Kiểm tra logic trên máy tính

Từ thư mục project:

```sh
bash Tests/run.sh
```

Script dùng `cc` với C11 và `-Wall -Wextra -Werror`, biên dịch vào thư
mục tạm rồi chạy ba bộ kiểm tra:

- `test_lab.c`: chống dội và giữ B1, vòng 99→00, timer quét chữ số,
  mã segment, timeout âm thanh và ngưỡng bấm đúp của project mẫu.
- `test_ir.c`: giải mã NEC LSB-first, loại repeat/byte đảo sai,
  khôi phục khi khung thiếu hoặc thời gian xung sai, bộ đếm IR.
- `test_remote.c`: nhận qua callback PG5, ánh xạ nút 1–4, giữ hiệu ứng
  sau hai chu kỳ, Play/Pause bật/tắt LED4, repeat/B1 không đảo LED4,
  giá trị hiển thị và timer quét LED 7 thanh.

HAL giả chỉ dùng cho test; firmware build bằng STM32 HAL thật.
Kiểm tra logic không thay thế việc kiểm tra dây nối và LED trên mạch.
