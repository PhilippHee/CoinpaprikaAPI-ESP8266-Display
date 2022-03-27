/*
CoinpaprikaAPI-ESP8266-Display2

Copyright (C) 2022 PhilippHee

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Board:
- LOLIN(WEMOS) D1 R2 & mini (esp8266 by ESP8266 Community)
  https://github.com/esp8266/Arduino

Additionally used libraries:
- ArduinoJson by Benoit Blanchon
  https://github.com/bblanchon/ArduinoJson
- NTPClient by Fabrice Weinberg
  https://github.com/arduino-libraries/NTPClient
- U8g2 by olikraus
  https://github.com/olikraus/u8g2

Project Description:
With this project/sketch, the API of Coinpaprika (https://api.coinpaprika.com)
can be used and information of the different coins/cryptocurrencies (e.g.
Bitcoin, Ethereum) can be retrieved. The information of a coin is retrieved
in the function getTickerInfo(). The information is collected in a structure
and can be displayed in the printCoinOnSerialMonitor() function via the
serial monitor. Additionally the information of the coins can be printed on
a (small) display via the function printCoinOnDisplay().
In the upper part of the source code you have to store your WiFi connection
data and enter the coins that should be retrieved.

The following board and display are used:
Board: https://amzn.to/2QwETcy
Display: https://amzn.to/3e8u20L

Connections between board and display:
    5V <-> VDD
   GND <-> GND
Pin D1 <-> SCK
Pin D2 <-> SDA

This project is based on the sample project/sketch "HTTPSRequest" by Ivan
Grokhotkov from the ESP8266WiFi library and on the repository/library
"arduino-coinmarketcap-api" by Brian Lough
(https://github.com/witnessmenow/arduino-coinmarketcap-api).

Notes: 
1st version: The code is still under development, but it already works.
Difference from "CoinpaprikaAPI-ESP8266-Display":
Currency setting (see line 108)

Date:
2022-03-27
*/


// Libraries
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <U8g2lib.h>


// Modify the following data ***************************************

// WiFi login data
const char* wifi_ssid = "your-ssid";
const char* wifi_password = "your-password";

// The number of coins must be equal to the number of coin strings.                      
// A coin string always consists of the ticker symbol and the name
// of the coin in lower case, separated by a minus sign,
// e.g. "btc-bitcoin", "bnb-binance-coin".
// See also: https://coinpaprika.com/

const byte numberOfCoins = 5;
const String coins[] = {"btc-bitcoin",
                        "eth-ethereum",
                        "ada-cardano",
                        "dot-polkadot",
                        "bnb-binance-coin"};

// Allowed values as currency: BTC, ETH, USD, EUR, PLN, KRW, GBP,
// CAD, JPY, RUB, TRY, NZD, AUD, CHF, UAH, HKD, SGD, NGN, PHP, MXN,
// BRL, THB, CLP, CNY, CZK, DKK, HUF, IDR, ILS, INR, MYR, NOK, PKR,
// SEK, TWD, ZAR, VND, BOB, COP, PEN, ARS, ISK

const String currency = "EUR"; // Always 3 characters!
                        
// *****************************************************************


// API of Coinpaprika ("Single IP address can send less than 10 requests per second")
const String host = "api.coinpaprika.com";
const int httpsPort = 443;

// Use a web browser to view and copy the fingerprint of the certificate
// The fingerprint could change several times a year!
// Or use client.setInsecure() (see below)
const char fingerprint[] PROGMEM = "cc96f444011b5d81ce80254b80fad86a15574476";

unsigned long timeNextCoinUpdate = 0;
unsigned long timeNextCoinPrint = 0;
byte i = 0;
bool firstLoop = true;

// Structure for the coin information
// Currently only selected information is processed
struct CPTickerResponse {
  String id;
  String name;
  String symbol;
  unsigned int rank;
  String last_updated;
  double price;
  double volume_24h;
  double volume_24h_change_24h;
  double market_cap;
  double market_cap_change_24h;
  double percent_change_1h;
  double percent_change_24h;
  double percent_change_7d;
  double percent_change_30d;
  double percent_change_1y;
  String error;
} ticker[numberOfCoins];

// This project uses a 1.3" OLED display (128x64 pixels) with a SH1106 driver
// For other displays see here: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


// Get the ticker information of the coins
void getTickerInfo() {
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;

  // Use setFingerprint() or setInsecure()
  client.setFingerprint(fingerprint);
  //client.setInsecure();

  Serial.println("Coins update started");
  
  //Serial.print("Connecting to " +  host);
  if (!client.connect(host, httpsPort)) {
    //Serial.println("Connection to host failed");
    for (byte i = 0; i < numberOfCoins; i++) {
      ticker[i].error = "Connection to host failed";
    }
    return;
  }
  
  for (byte i = 0; i < numberOfCoins; i++) {
    CPTickerResponse responseData;

    //Example: https://api.coinpaprika.com/v1/tickers/btc-bitcoin?quotes=EUR
    String url = "/v1/tickers/" + coins[i] + "?quotes=" + currency;
    //Serial.print("requesting URL: " + url);
  
    client.println("GET " + url + " HTTP/1.1");
    client.println("Host: " + host);
    client.println(F("User-Agent: arduino/1.0.0"));
    client.println();

    Serial.print(String(i + 1) + " ");
    delay(20);
  
    String body = "";
    body.reserve(1000);
    String body2 = "";
    body2.reserve(500);
    
    bool finishedHeaders = false;
    bool currentLineIsBlank = true;
    bool avail = false;
    unsigned long now = millis();
    bool stringBegin = false;
  
    while (millis() - now < 1500) {
      while (client.available()) {
        char c = client.read();
  
        if (!finishedHeaders) {
          if (currentLineIsBlank && c == '\n') {
            finishedHeaders = true;
          }
        } else {
          // The String begins with '{'
          if (c == '{') {
            stringBegin = true;
          }
          if (stringBegin) {
            body = body + c;
          }
        }
  
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
        
        avail = true;
      }
      
      if (avail) {
        //Serial.println("BODY:");
        //Serial.println(body);
        //Serial.println("END");
        break;
      }
    }


    // Process total string and create 2nd string
    unsigned int z = 0;
    unsigned int q = 0;
  
    while(true) {
      if (body[q] == '{') {
        z++;
      } 
      
      if (z == 3) {
        body2 = body2 + body[q];
      }
  
      if (body[q] == '}') {
        //Serial.println("BODY2:");
        //Serial.println(body2);
        //Serial.println("END2");
        break;
      }
      
      q++;
    }
  
    // Parse ticker response
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);
    
    DynamicJsonDocument doc2(768);
    DeserializationError error2 = deserializeJson(doc2, body2);
  
    if (error || error2) {
      responseData.error = coins[i] + " - deserializeJson() failed";
    }
  
    responseData.id = doc["id"].as<String>();
    responseData.name = doc["name"].as<String>();
    responseData.symbol = doc["symbol"].as<String>();
    responseData.rank = doc["rank"].as<unsigned int>();
    responseData.last_updated = doc["last_updated"].as<String>();

    responseData.price = doc2["price"].as<double>();
    responseData.volume_24h = doc2["volume_24h"].as<double>();
    responseData.volume_24h_change_24h = doc2["volume_24h_change_24h"].as<double>();
    responseData.market_cap = doc2["market_cap"].as<double>();
    responseData.market_cap_change_24h = doc2["market_cap_change_24h"].as<double>();
       
    responseData.percent_change_1h = doc2["percent_change_1h"].as<double>();
    responseData.percent_change_24h = doc2["percent_change_24h"].as<double>();
    responseData.percent_change_7d = doc2["percent_change_7d"].as<double>();
    responseData.percent_change_30d = doc2["percent_change_30d"].as<double>();
    responseData.percent_change_1y = doc2["percent_change_1y"].as<double>();

    responseData.error = "";
  
    if (responseData.rank == 0) {
      responseData.error = coins[i] + " - Connection failed (invalid URL)";
    }

    ticker[i] = responseData;
  }

  Serial.println();
  Serial.println("Coins update finished");
  client.stop();
}


// Print the ticker information of the coin via serial monitor
void printCoinOnSerialMonitor(CPTickerResponse t) {
  Serial.println("****************************************");

  if (t.error == "") {
    Serial.print("ID: ");
    Serial.println(t.id);
    
    Serial.print("Name: ");
    Serial.println(t.name);
    
    Serial.print("Symbol: ");
    Serial.println(t.symbol);
    
    Serial.print("Rank: ");
    Serial.println(t.rank);
    
    Serial.print("Price in " + currency + ": ");
    Serial.println(t.price, 6);
    
    Serial.print("24h Volume: ");
    Serial.println(t.volume_24h, 2);

    Serial.print("24h Volume Change 24h: ");
    Serial.println(t.volume_24h_change_24h, 2);
    
    Serial.print("Market Cap: ");
    Serial.println(t.market_cap, 2);
    
    Serial.print("Market Cap Change 24h: ");
    Serial.println(t.market_cap_change_24h, 2);

    Serial.print("Percent Change 1h: ");
    Serial.println(t.percent_change_1h, 2);
    
    Serial.print("Percent Change 24h: ");
    Serial.println(t.percent_change_24h, 2);
    
    Serial.print("Percent Change 7d: ");
    Serial.println(t.percent_change_7d, 2);

    Serial.print("Percent Change 30d: ");
    Serial.println(t.percent_change_30d, 2);
    
    Serial.print("Percent Change 1y: ");
    Serial.println(t.percent_change_1y, 2);
    
    Serial.print("Last Updated: ");
    Serial.println(t.last_updated);

  } else {
    Serial.println("Error getting data:");
    Serial.println(t.error);
  }
  
  Serial.println("****************************************");
}


// Print the ticker information of the coin on external display
void printCoinOnDisplay(CPTickerResponse t) {
  String s;
  char cArr[30];

  if (t.error == "") {
    u8g2.firstPage();
    
    do {
      u8g2.setFont(u8g2_font_7x14B_tf);
      
      s = t.name + " (" + t.symbol + ")";
      s.toCharArray(cArr, 30);
      u8g2.drawStr(0, 10, cArr);
  
      u8g2.drawHLine(0, 14, 128);
      
      u8g2.setFont(u8g2_font_6x13_tf);

      if (t.price < 10) {
        snprintf(cArr, sizeof(cArr), "Price..: %.3f %c%c%c", t.price, currency[0], currency[1], currency[2]);
      } else {
        snprintf(cArr, sizeof(cArr), "Price..: %.2f %c%c%c", t.price, currency[0], currency[1], currency[2]);
      }
      u8g2.drawStr(0, 27, cArr);
  
      if (t.percent_change_1h >= 0) {
        snprintf(cArr, sizeof(cArr), "1h.....: +%.2f %%", t.percent_change_1h);
      } else {
        snprintf(cArr, sizeof(cArr), "1h.....: %.2f %%", t.percent_change_1h);
      }
      u8g2.drawStr(0, 39, cArr);
  
      if (t.percent_change_24h >= 0) {
        snprintf(cArr, sizeof(cArr), "24h....: +%.2f %%", t.percent_change_24h);
      } else {
        snprintf(cArr, sizeof(cArr), "24h....: %.2f %%", t.percent_change_24h);
      }
      u8g2.drawStr(0, 51, cArr);
  
      if (t.percent_change_7d >= 0) {
        snprintf(cArr, sizeof(cArr), "7d.....: +%.2f %%", t.percent_change_7d);
      } else {
        snprintf(cArr, sizeof(cArr), "7d.....: %.2f %%", t.percent_change_7d);
      }
      u8g2.drawStr(0, 63, cArr);
    
    } while (u8g2.nextPage());

    
  } else {
    u8g2.firstPage();
    
    do {
      u8g2.setFont(u8g2_font_6x13_tf);
      u8g2.drawStr(0, 35, "Error getting data");
    } while (u8g2.nextPage());
  }
  
}


void setup() {
  // Init external display
  u8g2.begin();
  
  Serial.begin(115200);
  Serial.println();

  // WiFi setup
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
}


void loop() {
  unsigned long timeNow = millis();

  if (numberOfCoins > 0) {
    // Update coins every minute
    if (timeNow > timeNextCoinUpdate) {
      timeNextCoinUpdate = timeNow + 60000;
      getTickerInfo();
    }

    // Print the next coin every 5 seconds
    if (timeNow > timeNextCoinPrint) {
      if (firstLoop) {
        // Only for the first loop cycle, so that updating
        // and printing do not happen at the same time
        firstLoop = false;
        timeNextCoinPrint = timeNow + 4500;
      } else {
        timeNextCoinPrint = timeNow + 5000;
      }
      
      //printCoinOnSerialMonitor(ticker[i]);
      printCoinOnDisplay(ticker[i]);      
      
      i++;
      if (i >= numberOfCoins) {
        i = 0;
      } 
    } 
  }
   
}
