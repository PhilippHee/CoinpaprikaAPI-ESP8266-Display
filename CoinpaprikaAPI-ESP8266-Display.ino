/*
CoinpaprikaAPI-ESP8266-Display

Copyright (C) 2021 PhilippHee

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

This project is based on the sample project/sketch "HTTPSRequest" by Ivan
Grokhotkov from the ESP8266WiFi library and on the repository/library
"arduino-coinmarketcap-api" by Brian Lough
(https://github.com/witnessmenow/arduino-coinmarketcap-api).

Date:
2021-04-17
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

// Customize your time zone, UTC +/- X
const float timeZone = +2.0;

const byte numberOfCoins = 5;
const String coins[] = {"btc-bitcoin",
                        "eth-ethereum",
                        "ada-cardano",
                        "dot-polkadot",
                        "bnb-binance-coin"};
                        
// The number of coins must be equal to the number of coin strings.                      
// A coin string always consists of the ticker symbol and the name
// of the coin in lower case, separated by a minus sign,
// e.g. "btc-bitcoin", "bnb-binance-coin".
// See also: https://coinpaprika.com/

// *****************************************************************


// API of Coinpaprika
const String host = "api.coinpaprika.com";
const int httpsPort = 443;

// Use a web browser to view and copy SHA1 fingerprint of the certificate
// The fingerprint could change several times a year!
// Or use client.setInsecure() (see below)
const char fingerprint[] PROGMEM = "76c035e4e77e0eab61cfe9cd5a3e3f689bc89fcb";

unsigned long timeNextCoinUpdate = 0;
unsigned long timeNextCoinPrint = 0;
byte i = 0;
bool firstLoop = true;

// Structure for the coin information
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
} ticker[numberOfCoins];

// This project uses a 1.3" OLED display (128x64 pixels) with a SH1106 driver
// For other displays see here: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


// Convert Unix timestamp (epoch) to readable format (calendar)
String convertEpoch2Calendar(unsigned long epoch) {
  time_t e = (time_t)(epoch + (int)(timeZone * 3600));
  tm* timeinfo = localtime(&e);
  
  int second = timeinfo->tm_sec;
  int minute = timeinfo->tm_min;
  int hour = timeinfo->tm_hour;
  int day = timeinfo->tm_mday;
  int month = timeinfo->tm_mon + 1;
  int year = timeinfo->tm_year + 1900;

  char timeArr[30];
  snprintf(timeArr, sizeof(timeArr), "%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
  String calendarString = timeArr;

  return calendarString;
}


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

    String url = "/v1/ticker/" + coins[i];
    //Serial.print("requesting URL: " + url);
  
    client.println("GET " + url + " HTTP/1.1");
    client.println("Host: " + host);
    client.println(F("User-Agent: arduino/1.0.0"));
    client.println();

    Serial.print(String(i + 1) + " ");
    delay(20);
  
    String body = "";
    body.reserve(500);
    bool finishedHeaders = false;
    bool currentLineIsBlank = true;
    bool avail = false;
    unsigned long now = millis();
  
    while (millis() - now < 1500) {
      while (client.available()) {
        char c = client.read();
  
        if(!finishedHeaders) {
          if (currentLineIsBlank && c == '\n') {
            finishedHeaders = true;
          }
        } else {
          body = body + c;
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
  
    // Parse ticker response
    StaticJsonDocument<768> doc;
    DeserializationError error = deserializeJson(doc, body);
  
    if (error) {
      responseData.error = coins[i] + " - deserializeJson() failed";
    }
  
    responseData.id = doc["id"].as<String>();
    responseData.name = doc["name"].as<String>();
    responseData.symbol = doc["symbol"].as<String>();
    responseData.rank = doc["rank"].as<unsigned int>();
    responseData.price_usd = doc["price_usd"].as<double>();
    responseData.price_btc = doc["price_btc"].as<double>();
    responseData.volume_24h_usd = doc["volume_24h_usd"].as<double>();
    responseData.market_cap_usd = doc["market_cap_usd"].as<double>();
    responseData.circulating_supply = doc["circulating_supply"].as<double>();
    responseData.total_supply = doc["total_supply"].as<double>();
    responseData.max_supply = doc["max_supply"].as<double>();
    responseData.percent_change_1h = doc["percent_change_1h"].as<double>();
    responseData.percent_change_24h = doc["percent_change_24h"].as<double>();
    responseData.percent_change_7d = doc["percent_change_7d"].as<double>();
    responseData.last_updated = doc["last_updated"].as<unsigned long>();
    responseData.last_updated_string = convertEpoch2Calendar(responseData.last_updated);
    responseData.error = "";
  
    if (responseData.last_updated == 0) {
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
    Serial.print("Price in USD: ");
    Serial.println(t.price_usd, 6);
    Serial.print("Price in BTC: ");
    Serial.println(t.price_btc, 6);
    Serial.print("24h Volume USD: ");
    Serial.println(t.volume_24h_usd, 2);
    Serial.print("Market Cap USD: ");
    Serial.println(t.market_cap_usd, 2);
    Serial.print("Circulating Supply: ");
    Serial.println(t.circulating_supply, 2);
    Serial.print("Total Supply: ");
    Serial.println(t.total_supply, 2);
    Serial.print("Max Supply: ");
    Serial.println(t.max_supply, 2);
    Serial.print("Percent Change 1h: ");
    Serial.println(t.percent_change_1h, 2);
    Serial.print("Percent Change 24h: ");
    Serial.println(t.percent_change_24h, 2);
    Serial.print("Percent Change 7d: ");
    Serial.println(t.percent_change_7d, 2);
    Serial.print("Last Updated (Unix): ");
    Serial.println(t.last_updated);
    Serial.print("Last Updated: ");
    Serial.println(t.last_updated_string);

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

      if (t.price_usd < 10) {
        snprintf(cArr, sizeof(cArr), "Price...: %.3f $", t.price_usd);
      } else {
        snprintf(cArr, sizeof(cArr), "Price...: %.2f $", t.price_usd);
      }
      u8g2.drawStr(0, 27, cArr);
  
      if (t.percent_change_1h >= 0) {
        snprintf(cArr, sizeof(cArr), "1h......: +%.2f %%", t.percent_change_1h);
      } else {
        snprintf(cArr, sizeof(cArr), "1h......: %.2f %%", t.percent_change_1h);
      }
      u8g2.drawStr(0, 39, cArr);
  
      if (t.percent_change_24h >= 0) {
        snprintf(cArr, sizeof(cArr), "24h.....: +%.2f %%", t.percent_change_24h);
      } else {
        snprintf(cArr, sizeof(cArr), "24h.....: %.2f %%", t.percent_change_24h);
      }
      u8g2.drawStr(0, 51, cArr);
  
      if (t.percent_change_7d >= 0) {
        snprintf(cArr, sizeof(cArr), "7d......: +%.2f %%", t.percent_change_7d);
      } else {
        snprintf(cArr, sizeof(cArr), "7d......: %.2f %%", t.percent_change_7d);
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
