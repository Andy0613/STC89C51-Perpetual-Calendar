# 系统模块结构图（函数调用关系）

## main.c 模块

```
main()  ──────────────────────────────────────────────────────────────────────
  │  [初始化]
  ├── Timer1Init()                      // 定时器1初始化
  ├── LcdInit()          ──→ lcd.c      // LCD1602初始化
  ├── Ds1302Init()       ──→ ds1302.c   // DS1302初始化
  │    └── Ds1302Read(), Ds1302Write()
  ├── DS18B20_SetResolution() ──→ ds18b20.c
  ├── DS18B20_StartConvert()  ──→ ds18b20.c
  └── Ds1302Read()      ──→ ds1302.c    // 读取闹钟参数
  │
  │  [主循环 while(1)]
  ├── Ds1302ReadTime()  ──→ ds1302.c    // 每次循环读取时钟
  ├── TempPros()                        // 温度处理（每5次循环）
  │    ├── DS18B20_GetTemp()  ──→ ds18b20.c
  │    └── DS18B20_StartConvert()
  ├── LcdFramework()                    // 模式切换时刷新框架
  │    └── LcdWriteStr() → LcdWriteData() ──→ lcd.c
  ├── AlarmJudge()                      // 闹钟时间匹配判断
  ├── AlarmBuzPro()                     // 蜂鸣器驱动
  │    └── DelayMs()
  ├── KeyScan()                         // 按键扫描
  │    └── KeyPressed() (×5)           // 消抖检测
  │         ├── SetAlarm()              // K1: 闹钟设置
  │         │    ├── LcdWriteStr()  → lcd.c
  │         │    ├── LcdShowNumAt() → lcd.c
  │         │    ├── AdjustValue()       // 小时/分钟调节
  │         │    │    ├── KeyPressed()
  │         │    │    ├── KeySound()
  │         │    │    └── LcdShowNumAt()
  │         │    ├── KeyPressed()
  │         │    │    └── KeySound()
  │         │    └── Ds1302Write() → ds1302.c
  │         ├── KeySound() / ReturnSound()
  │         │    └── DelayMs()
  │         └── 修改秒表全局变量
  │
  └── LcdUpdate()                       // LCD数据刷新
       ├── LcdShowNumAt()  → lcd.c
       ├── ShowLcdWeek()
       │    └── LcdWriteData() → lcd.c
       ├── LcdWriteCom()   → lcd.c
       └── LcdWriteData()  → lcd.c

Timer1() interrupt 3                     // 定时器中断（10ms）
  └── sw_running == 1 → sw_csec++ → sw_sec++  // 秒表计时
```

## ds1302.c 模块

```
Ds1302Init()
  ├── Ds1302Read(0x81)          // 读秒寄存器，检测CH位
  └── Ds1302Write()  × 9       // 写入默认时间 + 关闭写保护

Ds1302ReadTime()
  └── Ds1302Read()  × 7        // 批量读取7个时间寄存器

Ds1302Write(addr, dat)          // SPI协议写
  ├── RST=1, SCLK=0
  ├── 循环8次: 逐位发送地址(LSB→MSB)
  └── 循环8次: 逐位发送数据

Ds1302Read(addr)                // SPI协议读
  ├── RST=1, SCLK=0
  ├── 循环8次: 逐位发送地址
  └── 循环8次: 逐位读取数据(MSB→LSB)
```

## ds18b20.c 模块

```
DS18B20_Reset()
  ├── DQ=0 → 延时480μs
  └── DQ=1 → 等待应答

DS18B20_WriteByte(dat)          // 单总线写
  └── 循环8次: DQ=0 → 写bit → 延时60μs → DQ=1

DS18B20_ReadByte()              // 单总线读
  └── 循环8次: DQ=0 → DQ=1 → 读bit → 延时

DS18B20_StartConvert()          // 启动温度转换
  ├── DS18B20_Reset()
  ├── DS18B20_WriteByte(0xCC)   // 跳过ROM
  └── DS18B20_WriteByte(0x44)   // 启动转换

DS18B20_SetResolution(res)      // 设置分辨率
  ├── DS18B20_Reset()
  ├── DS18B20_WriteByte(0xCC)
  ├── DS18B20_WriteByte(0x4E)   // 写暂存器
  ├── DS18B20_WriteByte(0x00)   // TH
  ├── DS18B20_WriteByte(0x00)   // TL
  ├── DS18B20_WriteByte(res)    // 配置寄存器
  ├── DS18B20_Reset()
  ├── DS18B20_WriteByte(0xCC)
  └── DS18B20_WriteByte(0x48)   // 复制到EEPROM

DS18B20_GetTemp(int *temp)      // 读取温度值
  ├── DS18B20_Reset()
  ├── DS18B20_WriteByte(0xCC)
  ├── DS18B20_WriteByte(0xBE)   // 读暂存器
  ├── DS18B20_ReadByte()        // 低字节
  └── DS18B20_ReadByte()        // 高字节
```

## lcd.c 模块

```
LcdInit()
  └── LcdWriteCom() × 5         // 初始化指令序列

LcdWriteCom(com)                // 写指令（4位模式）
  ├── RS=0, RW=0
  ├── P0=com        → 写高4位
  ├── E=1 → 延时 → E=0
  └── P0=com<<4     → 写低4位
       └── E=1 → 延时 → E=0
       └── 内部调用 Lcd1602_Delay1ms()

LcdWriteData(dat)               // 写数据（4位模式）
  ├── RS=1, RW=0
  ├── P0=dat        → 写高4位
  ├── E=1 → 延时 → E=0
  └── P0=dat<<4     → 写低4位
       └── E=1 → 延时 → E=0
       └── 内部调用 Lcd1602_Delay1ms()
```

## 函数调用总图

```
┌─────────────────────────────────────────────────────────────────────┐
│                         main() 主循环                                │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐  │
│  │ DS1302   │ │ DS18B20  │ │ LCD1602  │ │ 按键扫描 │ │ 闹钟处理 │  │
│  │ 时钟模块  │ │ 温度模块  │ │ 显示模块  │ │ KeyScan  │ │ Judge   │  │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘  │
│       │            │            │            │            │         │
│  ┌────▼────┐  ┌────▼────┐ ┌────▼────┐ ┌────▼────┐ ┌────▼────┐     │
│  │ReadTime │  │TempPros │ │LcdUpdate│ │KeyPressed│ │AlarmBuz │     │
│  │Init     │  │GetTemp  │ │Framework│ │SetAlarm  │ │Pro      │     │
│  └─────────┘  └─────────┘ └─────────┘ └─────────┘ └─────────┘     │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                   Timer1 中断 (10ms)                         │    │
│  │                   → 秒表 sw_csec / sw_sec 累加              │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
                            │
         ┌──────────────────┼──────────────────┐
         ▼                  ▼                  ▼
   ┌───────────┐     ┌───────────┐      ┌───────────┐
   │ ds1302.c  │     │ ds18b20.c │      │  lcd.c    │
   │ 底层驱动   │     │ 底层驱动   │      │ 底层驱动   │
   │ SPI协议    │     │ 单总线协议  │      │ 并行接口   │
   └───────────┘     └───────────┘      └───────────┘
         │                  │                  │
         ▼                  ▼                  ▼
   ┌───────────────────────────────────────────────┐
   │             STC89C52 硬件层                    │
   │  P1^0-2 DS1302  │  P3^7 DS18B20  │  P0/P2 LCD │
   │  P3^0-3 按键     │  P2^0 蜂鸣器    │  Timer1   │
   └───────────────────────────────────────────────┘
```

## 文件间依赖

```
main.c ──→ lcd.h      (LcdInit, LcdWriteCom, LcdWriteData)
main.c ──→ ds1302.h   (Ds1302Init, Ds1302ReadTime, Ds1302Read, Ds1302Write)
main.c ──→ ds18b20.h  (DS18B20_GetTemp, DS18B20_StartConvert, DS18B20_SetResolution)

ds1302.c ──→ reg51.h, intrins.h
ds18b20.c ──→ reg51.h, intrins.h
lcd.c ──→ reg51.h
```
