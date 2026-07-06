# 51 单片机万年历电子时钟

## 功能特性

- 实时显示年月日、星期、时分秒
- 自动闰年判断
- 按键调节时间
- 支持秒表功能
- 支持温度显示功能

## 硬件平台
- STC89C52RC
- DS1302 实时时钟
- LCD1602 液晶显示
- DS18B20 温度采集

## 软件设计
- 采用模块化驱动设计
- 定时器中断计时
- 按键状态机消抖

## 实物展示
![实物接线图](https://raw.githubusercontent.com/Andy0613/STC89C51-Perpetual-Calendar/main/docs/Picture.jpg)

## 仿真图展示

![万年历仿真图](https://raw.githubusercontent.com/Andy0613/STC89C51-Perpetual-Calendar/main/docs/schematic/circuit.png)

## 代码说明
- `Keil Code/Last/`：当前(最终)版本源码
- `Keil Code/Revise_X`：代码更改历史（归档）
- `Keil Code/Origin/`：初始实训版本（归档）
