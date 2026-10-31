#!/usr/bin/env python3
"""Generate editable SVG wiring diagrams; no external Python packages."""
from pathlib import Path
from html import escape

OUT = Path(__file__).resolve().parents[1] / 'docs'
INK = '#182b43'
MUTED = '#51647a'
SCL = '#007e87'
SDA = '#2462cb'
SPI = '#7755b6'
UART = '#b85916'
POWER = '#c83542'
GND = '#354357'


class SVG:
    def __init__(self, width, height, title):
        self.parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img">',
                      f'<title>{escape(title)}</title>',
                      '<style>text {font-family:"DejaVu Sans",sans-serif} .wire {fill:none;stroke-linecap:round;stroke-linejoin:round}</style>',
                      f'<rect width="{width}" height="{height}" fill="#f4f7fb"/>']

    def rect(self, x, y, w, h, fill='white', stroke='#cbd6e2', radius=12):
        self.parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" fill="{fill}" stroke="{stroke}" stroke-width="2"/>')

    def text(self, x, y, text, size=20, color=INK, weight=400, anchor='start'):
        self.parts.append(f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" font-weight="{weight}" text-anchor="{anchor}">{escape(text)}</text>')

    def line(self, points, color=INK, width=3, dash=None):
        pts = ' '.join(f'{x},{y}' for x,y in points)
        dash_attr = f' stroke-dasharray="{dash}"' if dash else ''
        self.parts.append(f'<polyline class="wire" points="{pts}" stroke="{color}" stroke-width="{width}"{dash_attr}/>')

    def dot(self, x, y, color):
        self.parts.append(f'<circle cx="{x}" cy="{y}" r="5" fill="{color}"/>')

    def block(self, x, y, w, h, title, subtitle):
        self.rect(x,y,w,h)
        self.text(x+24,y+36,title,25,INK,700)
        self.text(x+24,y+64,subtitle,17,MUTED)
        self.line([(x+18,y+80),(x+w-18,y+80)],'#dbe3ec',1)

    def pin(self,x,y,label,side,color=INK):
        # x is the module boundary; the label sits inside the module.
        offset=18 if side=='left' else -18
        self.dot(x,y,color)
        self.text(x+offset,y+7,label,20,color,500,'start' if side=='left' else 'end')

    def power(self,x,y,label,up=True):
        end=y-42 if up else y+42
        self.line([(x,y),(x,end)],POWER)
        self.line([(x-9,end+10 if up else end-10),(x,end),(x+9,end+10 if up else end-10)],POWER)
        self.text(x,end-12 if up else end+27,label,19,POWER,700,'middle')

    def ground(self,x,y):
        self.line([(x,y),(x,y+24)],GND)
        for width,dy in [(26,24),(16,31),(6,38)]:
            self.line([(x-width/2,y+dy),(x+width/2,y+dy)],GND,2)
        self.text(x,y+62,'GND',17,GND,500,'middle')

    def save(self,name):
        (OUT/name).write_text('\n'.join(self.parts+['</svg>']),encoding='utf-8')


def full_diagram():
    s=SVG(1800,1400,'Lab 2 — Sơ đồ ghép nối STM32F429, Tiny RTC, OLED, RC522 và UART')
    s.text(60,65,'LAB 2  /  GHÉP NỐI NỐI TIẾP',36,INK,700)
    s.text(60,103,'Sơ đồ ghép nối theo mục 3.4–3.9 của đề • STM32F429 Discovery • Firmware lab2',21,MUTED)

    s.block(650,220,440,820,'STM32F429I DISCOVERY','SYSCLK 180 MHz • GND chung')
    s.text(682,345,'I2C3  ·  100 kHz',22,SCL,700)
    s.pin(650,390,'PA8   / SCL','left',SCL)
    s.pin(650,430,'PC9   / SDA','left',SDA)
    s.text(900,345,'SPI4 · mode 0',20,SPI,700)
    for y,label in [(390,'PE4 / SS'),(430,'PE2 / SCK'),(470,'PE5 / MISO'),(510,'PE6 / MOSI')]:
        s.pin(1090,y,label,'right',SPI)
    s.text(682,570,'LED tích hợp trên kit',22,INK,700)
    for y,color,label,detail in [(615,'#249755','LED3 · PG13','Sáng khi phát hiện thẻ'),(700,'#df4b47','LED4 · PG14','Sáng khi thẻ được phép')]:
        s.rect(682,y-25,372,68,'#f4f7fb','#e0e7ef',8)
        s.parts.append(f'<circle cx="708" cy="{y}" r="10" fill="{color}"/>')
        s.text(730,y+6,label,22,INK,700)
        s.text(730,y+30,detail,17,MUTED)
    s.text(682,803,'USART1  ·  115200 / 8N1',22,UART,700)
    s.pin(1090,890,'PA9 / TX','right',UART)
    s.pin(1090,940,'PA10 / RX','right',UART)
    s.text(682,990,'Nạp chương trình qua cổng ST-LINK.',18,MUTED)
    s.power(760,220,'3V (≈3.3V)')
    s.power(980,220,'5V')
    s.ground(870,1040)

    # I2C common nets. A filled dot is an electrical junction.
    s.block(65,210,335,260,'TINY RTC','DS1307 + AT24C32')
    s.pin(400,310,'SCL','right',SCL)
    s.pin(400,350,'SDA','right',SDA)
    s.text(89,400,'I2C: 0x68 / 0x50 (7 bit)',18,MUTED)
    s.text(89,428,'Pin dự phòng → VBAT (*)',18,MUTED)
    s.power(200,210,'VCC = 5V')
    s.ground(100,470)
    s.text(259,493,'SQ, DS: không nối',16,MUTED)

    s.block(65,580,335,220,'OLED SH1106','128 × 64 • 1.3 inch')
    s.pin(400,680,'SCL','right',SCL)
    s.pin(400,730,'SDA','right',SDA)
    s.text(89,774,'I2C: 0x3C theo code mẫu',18,MUTED)
    s.power(200,580,'VCC = 3V')
    s.ground(140,800)
    s.line([(400,310),(490,310),(490,680),(400,680)],SCL)
    s.line([(490,390),(650,390)],SCL)
    s.dot(490,390,SCL)
    s.line([(400,350),(550,350),(550,730),(400,730)],SDA)
    s.line([(550,430),(650,430)],SDA)
    s.dot(550,430,SDA)
    s.text(470,760,'SCL',17,SCL,700)
    s.text(537,790,'SDA',17,SDA,700)

    s.block(1360,280,360,460,'RFID RC522','SPI • thẻ 13.56 MHz')
    for y,label in [(390,'SDA / SS'),(430,'SCK'),(470,'MISO'),(510,'MOSI')]:
        s.pin(1360,y,label,'left',SPI)
        s.line([(1090,y),(1360,y)],SPI)
    s.text(1118,375,'CS mức thấp',17,SPI)
    s.text(1384,560,'Chân “SDA” ở đây là SS.',19,INK,700)
    s.text(1384,590,'Không nối chân này vào I2C.',17,MUTED)
    s.pin(1720,630,'RST','right',POWER)
    s.line([(1720,630),(1750,630),(1750,565)],POWER)
    s.text(1750,550,'3V',18,POWER,700,'middle')
    s.text(1384,685,'IRQ: không nối',19,MUTED)
    s.power(1510,280,'VCC = 3V')
    s.ground(1535,740)

    s.block(1360,780,360,260,'USB–UART → PC','Hercules / PuTTY Serial')
    s.pin(1360,890,'RXD ← STM32 TX','left',UART)
    s.pin(1360,940,'TXD → STM32 RX','left',UART)
    for y in [890,940]: s.line([(1090,y),(1360,y)],UART)
    s.text(1384,985,'Mức logic 3.3V',20,UART,700)
    s.text(1384,1015,'VCC adapter: không nối',18,MUTED)
    s.ground(1535,1040)

    # Supply capacitor: named rails are electrically common throughout drawing.
    s.text(65,940,'TỤ LỌC NGUỒN',22,INK,700)
    s.power(360,975,'3V')
    s.line([(360,975),(360,991)],POWER)
    s.line([(337,991),(383,991)],INK,3)
    s.line([(337,1005),(383,1005)],INK,3)
    s.line([(360,1005),(360,1028)],GND)
    s.ground(360,1028)
    s.text(320,985,'+',24,POWER,700)
    s.text(65,1007,'C1 = 100 µF',23,INK,700)
    s.text(65,1043,'Cực + → 3V',18,POWER)
    s.text(65,1073,'Cực − → GND',18,GND)

    s.rect(60,1150,1680,190,'#fff7e9','#e3c58e')
    s.text(85,1187,'LƯU Ý MỨC ĐIỆN ÁP I2C',23,'#80500d',700)
    s.text(85,1222,'Tiny RTC trong đề có R2/R3 kéo SDA/SCL lên 5V, nhưng OLED được cấp 3V.',21,INK)
    s.text(85,1257,'Với module này, bổ sung chuyển mức I2C hai chiều: LV = 3.3V (STM32 + OLED), HV = 5V (Tiny RTC).',21,INK)
    s.text(85,1292,'(*) Giữ pin dự phòng đúng loại của module; không đấu thêm nguồn 3V song song với pin đang lắp.',19,MUTED)
    s.text(85,1321,'Xem sơ đồ i2c_level_shift.svg. Tất cả GND nối chung; chấm tròn là điểm nối, giao cắt không chấm không nối.',18,MUTED)
    s.text(60,1372,'Các chân ghi theo tên trên module, không theo vị trí hàng chân. Kiểm tra nhãn SCL/SDA trên OLED trước khi cắm.',18,MUTED)
    s.save('so_do_lab2.svg')


def level_shift():
    s=SVG(1600,850,'Lab 2 — Bổ sung bộ chuyển mức I2C hai chiều giữa Tiny RTC và bus 3.3V')
    s.text(55,65,'TÁCH MỨC ĐIỆN ÁP BUS I2C',34,INK,700)
    s.text(55,107,'Bổ sung cho Tiny RTC có điện trở kéo lên 5V • Không thay đổi firmware hoặc địa chỉ thiết bị',21,MUTED)
    s.block(60,250,400,330,'STM32 + OLED','Phía logic 3.3V')
    s.text(84,362,'PA8 + OLED SCL',23,SCL,700)
    s.text(84,422,'PC9 + OLED SDA',23,SDA,700)
    s.text(84,490,'OLED VCC → 3V của kit',20,INK)
    s.text(84,527,'I2C3: 100 kHz',20,MUTED)
    s.block(650,250,330,330,'CHUYỂN MỨC I2C','Hai chiều / open-drain')
    s.pin(650,355,'LV1','left',SCL)
    s.pin(980,355,'HV1','right',SCL)
    s.pin(650,415,'LV2','left',SDA)
    s.pin(980,415,'HV2','right',SDA)
    s.line([(460,355),(650,355)],SCL)
    s.line([(460,415),(650,415)],SDA)
    s.line([(735,355),(895,355)],SCL,2,'6 6')
    s.line([(735,415),(895,415)],SDA,2,'6 6')
    s.text(675,490,'Ví dụ: module BSS138',19,INK)
    s.text(675,523,'Có pull-up cả hai phía',18,MUTED)
    s.power(715,250,'LV → 3.3V')
    s.power(925,250,'HV → 5V')
    s.ground(815,580)
    s.block(1160,250,380,330,'TINY RTC','Phía logic 5V')
    s.pin(1160,355,'SCL','left',SCL)
    s.pin(1160,415,'SDA','left',SDA)
    s.line([(980,355),(1160,355)],SCL)
    s.line([(980,415),(1160,415)],SDA)
    s.text(1184,490,'DS1307 / AT24C32',21,INK)
    s.text(1184,527,'Giữ pull-up về 5V',20,MUTED)
    s.power(1350,250,'VCC → 5V')
    s.ground(1350,580)
    s.ground(250,580)
    s.rect(60,690,1480,110,'white','#cbd6e2')
    s.text(85,731,'GND của STM32, OLED, bộ chuyển mức và Tiny RTC phải nối chung.',22,INK,700)
    s.text(85,770,'Phía LV cần pull-up lên 3.3V; phía HV cần pull-up lên 5V. Không dùng cầu chia áp một chiều cho SDA.',20,MUTED)
    s.save('i2c_level_shift.svg')


if __name__ == '__main__':
    OUT.mkdir(parents=True,exist_ok=True)
    full_diagram()
    level_shift()
