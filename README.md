# esp-idf-analog-joystick
2-axis XY joystick using ESP-IDF.   

I bought [this](https://roboticafacil.es/datasheets/ky-023.pdf) 2-axis XY joystick module for $1 on AliExpress.   
![analog-joystick](https://user-images.githubusercontent.com/6020549/229271421-48bbf957-44ce-476f-8b74-bed132041051.JPG)

So I used ESP-IDF to read the X and Y positions.   
VRx and VRy are analog output pins.   
ESP32 has two ADCs, ADC1 and ADC2.   
This project uses ADC1.   

![web](https://user-images.githubusercontent.com/6020549/229271436-df6a5d75-3639-4d9f-9f98-2f7ade0141ca.JPG)


# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF's ADC API has changed significantly since V5.0.   

# Hardware requirements
2-axis XY joystick.   

# Installation

```Shell
git clone https://github.com/nopnop2002/esp-idf-analog-joystick
cd esp-idf-analog-joystick/pose
idf.py set-target {esp32/esp32s2/esp32s3/esp32c2/esp32c3}
idf.py menuconfig
idf.py flash
```

# Configuration

![config-top](https://user-images.githubusercontent.com/6020549/229271356-4719a0a2-0c6f-4e9c-aeee-d83b1f8e6a8e.jpg)
![config-app](https://user-images.githubusercontent.com/6020549/229271360-6064d761-3deb-4d6c-b7a0-ec462cf23031.jpg)

## WiFi Setting
Set the information of your access point.
![config-wifi](https://user-images.githubusercontent.com/6020549/229271377-937e62ea-03ac-4bc4-8f39-13bcd98d2158.jpg)

## Device Setting
Set the information of gpio for analog input and degiatl input.   
VRX and VRY are analog inputs and SW is a digital input.   
![config-device](https://user-images.githubusercontent.com/6020549/229271397-e6adde12-9131-4d92-a344-7e1d5377479e.jpg)

Analog input gpio for ESP32 is GPIO32 ~ GPIO39. 12Bits width.   
Analog input gpio for ESP32S2 is GPIO01 ~ GPIO10. 13Bits width.   
Analog input gpio for ESP32S3 is GPIO01 ~ GPIO10. 12Bits width.   
Analog input gpio for ESP32C2 is GPIO00 ~ GPIO04. 12Bits width.   
Analog input gpio for ESP32C3 is GPIO00 ~ GPIO04. 12Bits width.   

# Wireing
|JOYSTICK||ESP32|ESP32-S2/S3|ESP32-C2/C3||
|:-:|:-:|:-:|:-:|:-:|:-:|
|GND|--|GND|GND|GND||
|+5V|--|3.3V|3.3V|3.3V||
|VRx|--|GPIO32|GPIO1|GPIO0|(*1)|
|VRy|--|GPIO33|GPIO2|GPIO1|(*1)|
|SW|--|GPIO15|GPIO3|GPIO2|(*2)|

(*1)You can change it to any ADC1 using menuconfig.   

(*2)You can change it to any GPIO using menuconfig.   

# View X-Y positoion with built-in web server   
ESP32 acts as a web server.   
I used [this](https://github.com/Molorius/esp32-websocket) component.   
This component can communicate directly with the browser.   
It's a great job.   
Enter the following in the address bar of your web browser.   
```
http:://{IP of ESP32}/
or
http://esp32.local/
```

![web](https://user-images.githubusercontent.com/6020549/229271436-df6a5d75-3639-4d9f-9f98-2f7ade0141ca.JPG)
