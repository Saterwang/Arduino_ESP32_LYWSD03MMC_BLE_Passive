#include <sstream>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEBeacon.h>
#include <mbedtls/ccm.h>
extern "C" 
{
  #include <mbedtls/error.h>
}

#define SCAN_TIME  10 // 每轮扫描的周期，单位：秒
#define LED_PIN  2

// 请修改为自己的设备MAC，请使用Xiaomi-cloud-tokens-extractor联网登录小米ID获取
char ATC_MAC[18] = "a4:c1:38:e7:6f:7f"; // 小写MAC地址，

// 请修改为自己的设备BLE KEY ，请使用Xiaomi-cloud-tokens-extractor联网登录小米ID获取
uint8_t key[16] = {0xc5,0xe3,0x7a,0x3d,0xb7,0x9e,0xee,0xcb,0x64,0x4e,0x61,0x8c,0xf4,0x27,0xfb,0x8d};  // 设备BLEKEY

// AES解密需要的其它临时变量
uint8_t nonce[12] = {0};
uint8_t tag[4] = {0};
uint8_t ciphertext[5] = {0};
uint8_t add_data[1] = {0x11}; // 单字节附加数据 

// 解密明文输出缓冲区 
uint8_t plaintext[5] = {0};

// 温度计的读出值
float battery_voltage = -100;
float battery_percent = 0;
float humidity = -100;
float temperature = -100;

BLEScan* pBLEScan;
bool isScanning = false;

void loop() 
{
    // 由于使用异步扫描，在上次扫描未结束前，不要开始下一次扫描
    if(!isScanning)
    {
      isScanning = true;

      bool ret = pBLEScan->start(SCAN_TIME, ScanCompleteCB);  // 使用异步扫描
      if (ret)
      {
          digitalWrite(LED_PIN, LOW);   // Turn the LED
      }
    }
}

// 打印HEX字符串的工具函数
void PrintHEX(char* array, size_t len, bool isNewLine = true)
{
  for(size_t i=0; i<len; i++) 
  {
      if(array[i] < 0x10) Serial.print('0');  // 补零对齐 
      Serial.print(array[i],  HEX);
      Serial.print(" ");
  }
  if(isNewLine)
  {
    Serial.println();
  }
  
}
/**************************************************************************************
 *    蓝牙扫描回调 
 *    注意：在Passive模式下，ATC设备只发送ServiceData，没有DeviceName。
 *    应该先用主动模式记录下ATC设备的MAC，再切换到被动模式，以MAC作为识别依据
 **************************************************************************************/
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks 
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        /*
         * 被动搜索：按MAC找设备
         */
        if (advertisedDevice.getAddress().toString().indexOf(ATC_MAC) >=0)
        {
            int payloadLen = advertisedDevice.getPayloadLength();
            uint8_t *payload =  advertisedDevice.getPayload();

            // 温湿度报文30字节，电量报文29字节
            if(payloadLen >= 29)  
            {
              Serial.printf("BLE Passive Readout: %s\n", ATC_MAC);
              Serial.print("Payload: ");
              PrintHEX((char*)payload, payloadLen);

              //nonce
              memcpy((char*)nonce, (const char*)payload + 12, 6); // MAC:6
              memcpy((char*)(nonce + 6), (const char*)payload + 9, 3); // ProduceID:2 + FrameCounter:1
              memcpy((char*)(nonce + 9), (const char*)payload + (payloadLen-7), 3); // Random Number:3
              Serial.print("Nonce: ");
              PrintHEX((char*)nonce, sizeof(nonce));

              //tag
              memcpy((char*)(tag), (const char*)payload + (payloadLen-4), 4); // Message Integrity Check:4
              Serial.print("Tag: ");
              PrintHEX((char*)tag,sizeof(tag));

              //ciphertext
              memcpy((char*)(ciphertext), (const char*)payload + 18, payloadLen == 30 ? 5: 4); // Cipher Payload:5-Temp&Hum, 4-Batt
              Serial.print("Ciphertext: ");
              PrintHEX((char*)ciphertext,sizeof(ciphertext));              
              
              // 执行解密 
              aes128_ccm_decrypt(key, nonce, ciphertext, payloadLen == 30 ? 5: 4, tag, add_data, sizeof(add_data), plaintext);
          
              // 打印解密结果 
              Serial.print("DecryptResult (HEX): ");
              PrintHEX((char*)plaintext,payloadLen == 30 ? 5: 4);   

              // Temp
              if(plaintext[0] == 0x04 && plaintext[1] == 0x10) 
              {
                signed short value = (plaintext[4] << 8) + plaintext[3];
                temperature = (float)value / 10;
              }

              // hum
              if(plaintext[0] == 0x06 && plaintext[1] == 0x10) 
              {
                signed short value = (plaintext[4] << 8) + plaintext[3];
                humidity = (float)value / 10;
              }

              // batt
              if(plaintext[0] == 0x0A && plaintext[1] == 0x10) 
              {
                signed short value = plaintext[3];
                battery_percent = (float)value;
              }     

              printf("Temperature: %.2f  Humidity: %.2f%%  Battery: %.0f%%\n", temperature, humidity, battery_percent);         
            }
        }

    }
};

void setup() 
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);     // Initialize the LED_PIN as an output
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); 
    pBLEScan->setActiveScan(false);  // 使用被动扫描，不额外消耗目标设备的电池
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void ScanCompleteCB(BLEScanResults foundDevices) 
{ // 扫描结束回调函数
    isScanning = false;
    digitalWrite(LED_PIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

// AES128解密
void aes128_ccm_decrypt(uint8_t* key, uint8_t* nonce, uint8_t* ciphertext, size_t ciphertext_len, uint8_t* tag, uint8_t* add_data, size_t add_data_len, uint8_t* plaintext) 
{
    mbedtls_ccm_context ctx;
    mbedtls_ccm_init(&ctx);
 
    // 参数校验 
    if(mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128) != 0) 
    {
        Serial.println("Key Error");
        return;
    }
 
    // 执行CCM解密 
    int ret = mbedtls_ccm_auth_decrypt(
        &ctx,
        ciphertext_len,
        nonce, 12,           // nonce长度固定12字节 
        add_data, add_data_len,
        ciphertext,
        plaintext,
        tag, 4               // tag长度固定4字节 
    );
 
    // 错误处理 
    if(ret != 0) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, 100);
        Serial.print("Decrypt Failed: ");
        Serial.println(error_buf); 
    }
    
    mbedtls_ccm_free(&ctx);
}
 
