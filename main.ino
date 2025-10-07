#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "secrets.h"


#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define BL_PIN   32

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

const String baseURL  = "https://finnhub.io/api/v1/quote?symbol=";

String symbols[]       = {"AAPL", "TSLA", "GOOGL", "MSFT", "LMT", "BLM"};
float  lastPrices[6]   = {0, 0, 0, 0, 0, 0};

int scrollX       = 0;
int tickerHeight  = 40;
String fullTicker = "";

void setup() {
  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, HIGH);

  Serial.begin(115200);
  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextWrap(false);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  tft.fillRect(0, 0, 320, tickerHeight, ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("WiFi Connected!");
  delay(1000);

  tft.fillRect(0, 0, 320, tickerHeight, ST77XX_BLACK);
  fetchStockData();
}

void fetchStockData() {
  fullTicker = "";

  for (uint8_t i = 0; i < sizeof(symbols) / sizeof(symbols[0]); i++) {
    String url = baseURL + symbols[i] + "&token=" + API_KEY;
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      JSONVar json = JSON.parse(http.getString());
      if (json.hasOwnProperty("c") && json.hasOwnProperty("d")) {
        float price = double(json["c"]);
        float delta = double(json["d"]);
        lastPrices[i] = price;

        // Format symbol, price, and change indicator
        fullTicker += symbols[i] + ":";
        fullTicker += String(price, 2);
        fullTicker += (delta >= 0) ? "+ " : "- ";
      } else {
        fullTicker += symbols[i] + ": err ";
      }
    } else {
      fullTicker += symbols[i] + ": --- ";
    }

    http.end();
  }

  Serial.println("Ticker: " + fullTicker);
}

void loop() {
  tft.fillRect(0, 0, 320, tickerHeight, ST77XX_BLACK);

  int x = scrollX;

  for (uint16_t i = 0; i < fullTicker.length(); i++) {
    char c = fullTicker.charAt(i);

    // Set color based on up/down symbols
    if (c == '+') {
      tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    } else if (c == '-') {
      tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    } else {
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    }

    tft.setCursor(x, 12); // vertical center of 40px band
    tft.write(c);
    x += 12;  // 12 pixels per character with setTextSize(2)
  }

  scrollX -= 2;
  if (scrollX < (-fullTicker.length()*12) + 320) {
    scrollX = 320;
    fetchStockData();
  }

  delay(50);
}