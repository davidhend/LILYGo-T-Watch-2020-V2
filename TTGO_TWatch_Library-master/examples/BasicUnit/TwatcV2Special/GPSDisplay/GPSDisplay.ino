#include "config.h"

TTGOClass *ttgo = nullptr;
TFT_eSPI *tft = nullptr;
TinyGPSPlus *gps = nullptr;

uint32_t last = 0;
uint32_t updateTimeout = 0;

int Hour = 0;
int Minute = 0;
int Second = 0;
int previousSecond = 0;

int FiveHourCount = 0;
int RemainingHours = 0;
int FiveMinuteCount = 0;
int RemainingMinutes = 0;

int rectHeight = 40;
int rectLength = 50;

void setup(void)
{
  Serial.begin(115200);

  ttgo = TTGOClass::getWatch();

  ttgo->begin();

  ttgo->openBL();
  //Create a new pointer to save the display object
  tft = ttgo->tft;

  tft->fillScreen(TFT_BLACK);
  tft->setTextFont(2);
  tft->println("Begin GPS Module...");

  //Open gps power
  ttgo->trunOnGPS();

  ttgo->gps_begin();

  gps = ttgo->gps;

  tft->fillScreen(TFT_BLACK);

  last = millis();
}


void loop(void)
{
  ttgo->gpsHandler();

  if (gps->time.isUpdated()) {

    Serial.print(F("TIME Fix Age="));
    Serial.print(gps->time.age());
    Serial.print(F("ms Raw="));
    Serial.print(gps->time.value());
    Serial.print(F(" Hour="));
    Serial.print(gps->time.hour());
    Serial.print(F(" Minute="));
    Serial.print(gps->time.minute());
    Serial.print(F(" Second="));
    Serial.print(gps->time.second());
    Serial.print(F(" Hundredths="));
    Serial.println(gps->time.centisecond());

    Hour = gps->time.hour();
    Minute = gps->time.minute();
    Second = gps->time.second();

    convertUTCtoCST(Hour);

    FiveHourCount = Hour / 5;
    RemainingHours = Hour - (FiveHourCount * 5);

    FiveMinuteCount = Minute / 5;
    RemainingMinutes = Minute - (FiveMinuteCount * 5);

    if (Second != previousSecond) {
      ttgo->tft->drawCircle(ttgo->tft->width() / 2, 24, 18, TFT_WHITE);
      ttgo->tft->fillCircle(ttgo->tft->width() / 2, 24, 16, TFT_ORANGE);
    } else {
      ttgo->tft->drawCircle(ttgo->tft->width() / 2, 24, 18, TFT_WHITE);
      ttgo->tft->fillCircle(ttgo->tft->width() / 2, 24, 16, TFT_BLACK);
    }

    switch (FiveHourCount) {
      case 0:
        ttgo->tft->drawRect(5, 50, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(60, 50, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(120, 50, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(180, 50, rectLength, rectHeight, TFT_WHITE);
        break;
      case 1:
        ttgo->tft->fillRect(5, 50, rectLength, rectHeight, TFT_RED);
        ttgo->tft->drawRect(60, 50, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(120, 50, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(180, 50, rectLength, rectHeight, TFT_WHITE);
        break;
      case 2:
        ttgo->tft->fillRect(5, 50, rectLength, rectHeight, TFT_RED);
        ttgo->tft->fillRect(60, 50, rectLength, rectHeight, TFT_RED);
        ttgo->tft->drawRect(120, 50, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(180, 50, rectLength, rectHeight, TFT_WHITE);
        break;
      case 3:
        ttgo->tft->fillRect(5, 50, rectLength, rectHeight, TFT_RED);
        ttgo->tft->fillRect(60, 50, rectLength, rectHeight, TFT_RED);
        ttgo->tft->fillRect(120, 50, rectLength, rectHeight, TFT_RED);
        ttgo->tft->drawRect(180, 50, rectLength, rectHeight, TFT_WHITE);
        break;
      case 4:
        ttgo->tft->fillRect(5, 50, rectLength, rectHeight, TFT_RED);
        ttgo->tft->fillRect(60, 50, rectLength, rectHeight, TFT_RED);
        ttgo->tft->fillRect(120, 50, rectLength, rectHeight, TFT_RED);
        ttgo->tft->fillRect(180, 50, rectLength, rectHeight, TFT_RED);
        break;
    }

    switch (RemainingHours) {
      case 0:
        ttgo->tft->drawRect(5, 100, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(60, 100, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(120, 100, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(180, 100, rectLength, rectHeight, TFT_WHITE);
        break;
      case 1:
        ttgo->tft->fillRect(5, 100, rectLength, rectHeight, TFT_RED);
        ttgo->tft->drawRect(60, 100, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(120, 100, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(180, 100, rectLength, rectHeight, TFT_WHITE);
        break;
      case 2:
        ttgo->tft->fillRect(5, 100, rectLength, rectHeight, TFT_RED);
        ttgo->tft->fillRect(60, 100, rectLength, rectHeight, TFT_RED);
        ttgo->tft->drawRect(120, 100, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(180, 100, rectLength, rectHeight, TFT_WHITE);
        break;
      case 3:
        ttgo->tft->fillRect(5, 100, rectHeight, rectLength, TFT_RED);
        ttgo->tft->fillRect(60, 100, rectHeight, rectLength, TFT_RED);
        ttgo->tft->fillRect(120, 100, rectHeight, rectLength, TFT_RED);
        ttgo->tft->drawRect(180, 100, rectHeight, rectLength, TFT_WHITE);
        break;
      case 4:
        ttgo->tft->fillRect(5, 100, rectLength, rectHeight, TFT_RED);
        ttgo->tft->fillRect(60, 100, rectLength, rectHeight, TFT_RED);
        ttgo->tft->fillRect(120, 100, rectLength, rectHeight, TFT_RED);
        ttgo->tft->fillRect(180, 100, rectLength, rectHeight, TFT_RED);
        break;
    }

    switch (FiveMinuteCount) {
      case 0:
        ttgo->tft->drawRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        break;
      case 1:
        ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->drawRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        break;
      case 2:
        ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->drawRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        break;
      case 3:
        ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->drawRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        break;
      case 4:
        ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->drawRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        break;
      case 5:
        ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->drawRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        break;
      case 6:
        ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->drawRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        break;
      case 7:
        ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->drawRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        break;
      case 8:
        ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->drawRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        break;
      case 9:
        ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->drawRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        break;
      case 10:
        ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->drawRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_WHITE);
        break;
      case 11:
        ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_RED);
        ttgo->tft->fillRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        ttgo->tft->fillRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_YELLOW);
        break;
    }

    switch (RemainingMinutes) {
      case 0:
        ttgo->tft->drawRect(5, 200, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(60, 200, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(120, 200, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(180, 200, rectLength, rectHeight, TFT_WHITE);
        break;
      case 1:
        ttgo->tft->fillRect(5, 200, rectLength, rectHeight, TFT_ORANGE);
        ttgo->tft->drawRect(60, 200, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(120, 200, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(180, 200, rectLength, rectHeight, TFT_WHITE);
        break;
      case 2:
        ttgo->tft->fillRect(5, 200, rectLength, rectHeight, TFT_ORANGE);
        ttgo->tft->fillRect(60, 200, rectLength, rectHeight, TFT_ORANGE);
        ttgo->tft->drawRect(120, 200, rectLength, rectHeight, TFT_WHITE);
        ttgo->tft->drawRect(180, 200, rectLength, rectHeight, TFT_WHITE);
        break;
      case 3:
        ttgo->tft->fillRect(5, 200, rectLength, rectHeight, TFT_ORANGE);
        ttgo->tft->fillRect(60, 200, rectLength, rectHeight, TFT_ORANGE);
        ttgo->tft->fillRect(120, 200, rectLength, rectHeight, TFT_ORANGE);
        ttgo->tft->drawRect(180, 200, rectLength, rectHeight, TFT_WHITE);
        break;
      case 4:
        ttgo->tft->fillRect(5, 200, rectLength, rectHeight, TFT_ORANGE);
        ttgo->tft->fillRect(60, 200, rectLength, rectHeight, TFT_ORANGE);
        ttgo->tft->fillRect(120, 200, rectLength, rectHeight, TFT_ORANGE);
        ttgo->tft->fillRect(180, 200, rectLength, rectHeight, TFT_ORANGE);
        break;
    }

    previousSecond = Second;

  }

}

void convertUTCtoCST(int hour)
{
  switch (hour) {
    case 0:
      Hour = 19;
      break;
    case 1:
      Hour = 20;
      break;
    case 2:
      Hour = 21;
      break;
    case 3:
      Hour = 22;
      break;
    case 4:
      Hour = 23;
      break;
    case 5:
      Hour = 0;
      break;
    case 6:
      Hour = 1;
      break;
    case 7:
      Hour = 2;
      break;
    case 8:
      Hour = 3;
      break;
    case 9:
      Hour = 4;
      break;
    case 10:
      Hour = 5;
      break;
    case 11:
      Hour = 6;
      break;
    case 12:
      Hour = 7;
      break;
    case 13:
      Hour = 8;
      break;
    case 14:
      Hour = 9;
      break;
    case 15:
      Hour = 10;
      break;
    case 16:
      Hour = 11;
      break;
    case 17:
      Hour = 12;
      break;
    case 18:
      Hour = 13;
      break;
    case 19:
      Hour = 14;
      break;
    case 20:
      Hour = 15;
      break;
    case 21:
      Hour = 16;
      break;
    case 22:
      Hour = 17;
      break;
    case 23:
      Hour = 18;
      break;
  }

}
