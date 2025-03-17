# Arduino_ESP32_LYWSD03MMC_BLE_Passive
ESP32-based Arduino project for passively scanning and decrypting temperature/humidity data from Xiaomi Bluetooth Thermometer 2 (LYWSD03MMC)

## Introduction
An Arduino project for ESP32 hardware that:
- Passively receives BLE broadcasts from Xiaomi Thermometer 2 (Model: LYWSD03MMC)
- Decrypts sensor data
- Outputs temperature/humidity values via serial port

**Key Feature**:  
Uses **BLE Passive Scanning** to prevent triggering active responses from the thermometer, minimizing battery consumption.

## Usage
### Prerequisites
1. Obtain device credentials using [Xiaomi-cloud-tokens-extractor](https://github.com/PiotrMachowski/Xiaomi-cloud-tokens-extractor):
   - MAC address
   - BLE KEY
2. Update credentials in the code

### Serial Output
Default output format with decryption diagnostics:
```text
11:28:25.825 -> BLE Passive Readout: a4:c1:38:e7:6f:7f
11:28:25.825 -> Payload: 02 01 06 1A 16 95 FE 58 58 5B 05 A9 7F 6F E7 38 C1 A4 C5 C0 E4 6E E7 01 00 00 AA CF C5 32 
11:28:25.825 -> Nonce: 7F 6F E7 38 C1 A4 5B 05 A9 01 00 00 
11:28:25.825 -> Tag: AA CF C5 32 
11:28:25.825 -> Ciphertext: C5 C0 E4 6E E7 
11:28:25.825 -> DecryptResult (HEX): 06 10 02 0C 01 
11:28:25.825 -> Temperature: -100.00  Humidity: 26.80%  Battery: 0%
```
Note: Diagnostic lines can be commented out for cleaner output.

###Important Notes
##Device Behavior
Firmware Impact:
Legacy firmware: Broadcasts every ~30 mins (1 temp + 1 humidity packet)
New firmware: Reduced broadcast interval (recommended for responsiveness)
##Technical Limitations
ESP32 BLE Library Issue:
ServiceData handling via String type truncates at 0x00 bytes
Workaround: Raw payload parsing implemented for data integrity
#Requirements
Arduino Environment:
Requires ESP32 board package v3.1.1+
Tested with ESP32-DevKitC and similar boards
