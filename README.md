# CoinpaprikaAPI-ESP8266-Display

**Use the ESP8266 to get information from the Coinpaprika API and print it on an external display**

With this project/sketch, the [API of Coinpaprika](https://api.coinpaprika.com) (`https://api.coinpaprika.com`) can be used and current information of the different coins/cryptocurrencies (e.g. Bitcoin, Ethereum) can be retrieved and printed on an external display.

This project is based on the sample project/sketch "HTTPSRequest" by Ivan Grokhotkov from the ESP8266WiFi library and on the repository/library "arduino-coinmarketcap-api" by Brian Lough ([GIT](https://github.com/witnessmenow/arduino-coinmarketcap-api)).

[![license](https://img.shields.io/badge/license-MIT-orange.svg)](LICENSE)

## Hardware components
The following hardware components and parts are used in this project:
- D1 Mini ESP8266 ([Link](https://amzn.to/2QwETcy))
- 1.3" OLED Display I2C 128x64 Pixels ([Link](https://amzn.to/3e8u20L))
- Power Supply 5V DC &#8805;500mA ([Link](https://amzn.to/3uZEFd8))
- DC Jack Socket Plug Female ([Link](https://amzn.to/3mUrJSJ))
- Toggle Switch ([Link](https://amzn.to/2OUV42W))
- Male and Female Headers ([Link](https://amzn.to/3tsMK9A))
- Insulated Copper Wire ([Link](https://amzn.to/3sqMl6v))
- Matrix Circuit Board ([Link](https://www.reichelt.de/lochrasterplatine-hartpapier-50x100mm-h25pr050-p8268.html))
- (3D Printed) Casing ([Link](https://www.thingiverse.com/thing:4831865))
- M3 Cylinder Head Screws ([Link](https://www.reichelt.de/zylinderkopfschrauben-schlitz-m3-8-mm-szk-m3x8-200-p65693.html))
    - 4x 8mm length (for the casing cover)
    - 2x 3.5mm length (for the display)
    - 2x 4mm length (for the circuit board)

Assembled, the "Crypto Ticker" looks like this:

![crypto_ticker](https://user-images.githubusercontent.com/81238678/115126510-cd84a100-9fcf-11eb-881d-0415081a4e98.jpg)

**Attention! Electricity is dangerous!
Reconstruction and use at your own risk!
All data without guarantee!**

## Preparation of the Arduino IDE
Before the project/source code can be used, a few preparations have to be made. This includes the configuration of the Arduino IDE and the installation of required libraries.

### Boardmanager
The Arduino IDE is not configured for the ESP8266 by default, so you have to install an additional boards manager first (see also [here](https://github.com/esp8266/Arduino)). For this you have to add the following boards manager URL in the Arduino IDE under *File > Preferences* in the *Settings* tab: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`

Then the appropriate board can be installed via *Tools > Board > Boards Manager...* by searching for *esp8266* ([**esp8266** by ESP8266 Community](https://github.com/esp8266/Arduino)) in the search bar. After that, the appropriate board must be selected from the drop-down list (*Tools > Board*). In this project a D1 Mini ESP8266 is used, the correct entry for this is *LOLIN(WEMOS) D1 R2 & mini*. Installing the board also installs many libraries that are specific to this board, such as for using the WiFi module.

To be able to use the board after connecting it to the computer, the correct COM port must also be selected in the IDE under *Tools > Port*. A manual installation of the driver of the USB interface was not necessary, but it may be.

### Board settings
This project/sketch uses the following settings in the Arduino IDE (*Tools*):
|Setting|Value|
|-|-|
|Board|LOLIN(WEMOS) D1 R2 & mini|
|Upload Speed|921600|
|CPU Frequency|160 MHz|
|Flash Size|4MB (FS:2MB OTA:~1019KB)|
|Debug port|Disabled|
|Debug Level|None|
|lwIP Variant|v1.4 Higher Bandwidth|
|VTables|Flash|
|Exceptions|Legacy (new can return nullprt)|
|Erase Flash|Only Sketch *(or: All Flash Contents)*|
|SSL Support|All SSL ciphers (most compatible)|

### Additional libraries
Additionally the following libraries are needed, which can be ...
1. searched and installed directly from the Arduino IDE (*Tools > Manage Libraries...*) or
2. downloaded from the respective GIT page and added manually to the local Arduino library directory.

|Library|Repository|
|-|-|
|**ArduinoJson** by Benoit Blanchon|[GIT](https://github.com/bblanchon/ArduinoJson)|
|**NTPClient** by Fabrice Weinberg|[GIT](https://github.com/arduino-libraries/NTPClient)|
|**U8g2** by olikraus|[GIT](https://github.com/olikraus/u8g2)|

Note: Please note the general information as well as troubleshooting tips when using the libraries!


## Source code
The source code ([**CoinpaprikaAPI-ESP8266-Display.ino**](CoinpaprikaAPI-ESP8266-Display.ino)) is roughly divided into the following areas/functionalities:
- establishing the WiFi connection with the home router
- querying the Coinpaprika API depending on the selected coins
- output of the coin information on the externel display

The information of the coins is retrieved in the function `getTickerInfo()` and collected in a structure that has the following values:
```
struct CPTickerResponse {
  String id;
  String name;
  String symbol;
  unsigned int rank;
  double price_usd;
  double price_btc;
  double volume_24h_usd;
  double market_cap_usd;
  double circulating_supply;
  double total_supply;
  double max_supply;
  double percent_change_1h;
  double percent_change_24h;
  double percent_change_7d;
  unsigned long last_updated;
  String last_updated_string;
  String error;
};
```

The function `printCoinOnDisplay()` is used to display the values on the external display. Every 60 seconds the information about the defined coins is updated. Every 5 seconds the next coin is printed on the display. In the upper part of the source code you have to store your WiFi connection data and enter the coins that should be retrieved and printed.

For more information, see the comments in the code.

# Additional Version 
The source code ([**CoinpaprikaAPI-ESP8266-Display2.ino**](CoinpaprikaAPI-ESP8266-Display2.ino)) is new. In this code you can set the preferred currency, e.g. EUR (see line 108). This code is still under development, but it already works.

## Further project
The following project is similar to this project, but without display. The information of the coins is only printed on the serial monitor of the Arduino IDE:
**CoinpaprikaAPI-ESP8266** by PhilippHee ([GIT](https://github.com/PhilippHee/CoinpaprikaAPI-ESP8266))
