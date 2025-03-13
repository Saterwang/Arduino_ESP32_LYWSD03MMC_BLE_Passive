# Arduino_ESP32_LYWSD03MMC_BLE_Passive
使用Arduino环境的ESP32开发板，被动扫描读取并解密米家蓝牙温度计2的温湿度数据
# 简介
这是一个基于ESP32硬件的Arduino工程
用于接收米家温湿度计2 （型号：LYWSD03MMC）的BLE数据，并且将其解密，通过串口输出温湿度值。
这个案例中使用了BLE Passive Scan被动扫描，不会导致温度计主动响应扫描请求而额外消耗电池。

# 使用方法
使用时请先用[Xiaomi-cloud-tokens-extractor工具](https://github.com/PiotrMachowski/Xiaomi-cloud-tokens-extractor) 通过自己的小米ID联网获取温度计的MAC地址和BLE KEY，更新到代码中

串口输出部分可以根据自己的要求修改输出格式
默认打印的格式如下
```
11:28:25.825 -> BLE Passive Readout: a4:c1:38:e7:6f:7f
11:28:25.825 -> Payload: 02 01 06 1A 16 95 FE 58 58 5B 05 A9 7F 6F E7 38 C1 A4 C5 C0 E4 6E E7 01 00 00 AA CF C5 32 
11:28:25.825 -> Nonce: 7F 6F E7 38 C1 A4 5B 05 A9 01 00 00 
11:28:25.825 -> Tag: AA CF C5 32 
11:28:25.825 -> Ciphertext: C5 C0 E4 6E E7 
11:28:25.825 -> DecryptResult (HEX): 06 10 02 0C 01 
11:28:25.825 -> Temperature: -100.00  Humidity: 26.80%  Battery: 0%
```
这里将解密的过程数据也打印了出来，供核实确认，可自行注释掉

# 注意
米家温度计2的BLE广播，旧固件大约30分钟广播1轮数据，包含1个温度报文和1个湿度报文。所以运行后完全没反应时请多等一段时间，或者将固件升级到新版，可以缩短广播间隔。

ESP32的BLE库，使用String格式输出BLE的ServiceData，这有一个BUG，当输出数据中出现0x00就会引起截断。所以本程序没有使用ServiceData，而是转而采用了直读所有Payload自己解析的方式保证数据完整性。

使用本案例请确保Arduino中的ESP32板库更新到了3.1.1以上版本
