#include "config.h"

TTGOClass *ttgo = nullptr;
TFT_eSPI *tft = nullptr;
TinyGPSPlus *gps = nullptr;
AXP20X_Class *power;

uint32_t last = 0;
uint32_t updateTimeout = 0;

boolean dateUpdated = false;
boolean wakeFromBoot = true;

int Hour = 0;
int Minute = 0;
int Second = 0;

int Month = 0;
int Day = 0;
int Year = 0;

int previousSecond = 0;
int previousMinute = 0;
int previousHour = 0;

int FiveHourCount = 0;
int RemainingHours = 0;
int FiveMinuteCount = 0;
int RemainingMinutes = 0;

int rectHeight = 40;
int rectLength = 50;

char buf[128];
bool irq = false;

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

  power = ttgo->power;

  // ADC monitoring must be enabled to use the AXP202 monitoring function
  power->adc1Enable(
    AXP202_VBUS_VOL_ADC1 |
    AXP202_VBUS_CUR_ADC1 |
    AXP202_BATT_CUR_ADC1 |
    AXP202_BATT_VOL_ADC1,
    true);

  pinMode(AXP202_INT, INPUT_PULLUP);
  attachInterrupt(AXP202_INT, [] {
    irq = true;
  }, FALLING);
  //!Clear IRQ unprocessed  first
  ttgo->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
  ttgo->power->clearIRQ();

  pinMode(TOUCH_INT, INPUT);

  last = millis();

}


void loop(void)
{
  ttgo->gpsHandler();

  if (gps->date.isUpdated()) {

    Day = gps->date.day();
    Month = gps->date.month();
    Year = gps->date.year() - 2000;

    tft->setCursor(170, 0);
    if (Month <= 9) {
      tft->print("0");
      tft->print(Month);
    } else {
      tft->print(Month);
    }
    tft->print("/");
    if (Day <= 9) {
      tft->print("0");
      tft->print(Day);
    } else {
      tft->print(Day);
    }
    tft->print("/");
    tft->print(Year);

    dateUpdated = true;

  } else {
    if (Month != 0 && Day != 0 && Year != 0 && wakeFromBoot == true)
    {
      tft->setCursor(170, 0);
      if (Month <= 9) {
        tft->print("0");
        tft->print(Month);
      } else {
        tft->print(Month);
      }
      tft->print("/");
      if (Day <= 9) {
        tft->print("0");
        tft->print(Day);
      } else {
        tft->print(Day);
      }
      tft->print("/");
      tft->print(Year);

      dateUpdated = true;
      wakeFromBoot = false;

      updateTimeOnScreen("all");

    }
  }



  if (gps->time.isUpdated()) {

    Hour = gps->time.hour();
    Minute = gps->time.minute();
    Second = gps->time.second();

    // Determine if DST is in effect for the given date
    int dst_in_effect = is_dst_in_effect(Year, Month, Day);

    // Current UTC hour
    int utc_hours = Hour;

    // Convert UTC to CST with DST
    int cst_hours;
    utc_to_cst_with_dst(utc_hours, dst_in_effect, &cst_hours);
    Hour = cst_hours; 

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

    if (Minute != previousMinute && dateUpdated != false) {
      updateTimeOnScreen("minutes");
    }

    if (Hour != previousHour && dateUpdated != false) {
      updateTimeOnScreen("hours");
    }

    previousSecond = Second;
    previousMinute = Minute;
    previousHour = Hour;

  }

  int16_t x, y;
  if (digitalRead(TOUCH_INT) == LOW) {
    Serial.println("PRESSED");
    if (ttgo->getTouch(x, y)) {
      sprintf(buf, "x:%03d  y:%03d", x, y);
      //ttgo->tft->drawString(buf, 80, 118);
    }
  }

  if (irq) {
    irq = false;
    ttgo->power->readIRQ();
    if (ttgo->power->isPEKShortPressIRQ()) {
      // Clean power chip irq status
      ttgo->power->clearIRQ();

      // Set  touchscreen sleep
      ttgo->displaySleep();

      /*
        In TWatch2019/ Twatch2020V1, touch reset is not connected to ESP32,
        so it cannot be used. Set the touch to sleep,
        otherwise it will not be able to wake up.
        Only by turning off the power and powering on the touch again will the touch be working mode
        // ttgo->displayOff();
      */

      ttgo->powerOff();

      //Set all channel power off
      ttgo->power->setPowerOutPut(AXP202_LDO3, false);
      ttgo->power->setPowerOutPut(AXP202_LDO4, false);
      ttgo->power->setPowerOutPut(AXP202_LDO2, false);
      ttgo->power->setPowerOutPut(AXP202_EXTEN, false);
      ttgo->power->setPowerOutPut(AXP202_DCDC2, false);

      // TOUCH SCREEN  Wakeup source
      //esp_sleep_enable_ext1_wakeup(GPIO_SEL_38, ESP_EXT1_WAKEUP_ALL_LOW);
      // PEK KEY  Wakeup source
      esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
      esp_deep_sleep_start();
    }
    ttgo->power->clearIRQ();
  }

}


// Function to determine if DST is in effect
int is_dst_in_effect(int currentYear, int currentMonth, int currentDay) {
    // Determine if the given date falls within the DST period (from 2nd Sunday in March to 1st Sunday in November)
    if ((currentMonth > 3 && currentMonth < 11) ||
        (currentMonth == 3 && (currentDay > 7 || (currentDay == 7 && day_of_week(currentYear, currentMonth, currentDay) == 0))) || // 2nd Sunday in March
        (currentMonth == 11 && (currentDay < 7 || (currentDay == 7 && day_of_week(currentYear, currentMonth, currentDay) != 0)))) // 1st Sunday in November
    {
        return 1; // DST is in effect
    }
    return 0; // DST is not in effect
}

// Function to calculate the day of the week (0 = Sunday, 1 = Monday, ..., 6 = Saturday)
int day_of_week(int currentYear, int currentMonth, int currentDay) {
    int a = (14 - currentMonth) / 12;
    int y = currentYear - a;
    int m = currentMonth + 12 * a - 2;
    return (currentDay + y + y / 4 - y / 100 + y / 400 + (31 * m) / 12) % 7;
}

// Function to convert UTC to CST with DST
void utc_to_cst_with_dst(int utc_hours, int is_dst, int *cst_hours) {
    // Calculate the offset for CST (UTC-6 without DST, UTC-5 with DST)
    int cst_offset = is_dst ? -5 : -6;

    // Perform the conversion
    *cst_hours = (utc_hours + cst_offset) % 24;
}


void updateTimeOnScreen(String whatToClear) {

  if (whatToClear == "minutes")
  {
    ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(5, 200, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(62, 200, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(122, 200, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(180, 200, rectLength, rectHeight, TFT_BLACK);
  }

  if (whatToClear == "hours")
  {
    ttgo->tft->fillRect(5, 50, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(62, 50, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(122, 50, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(180, 50, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(5, 100, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(62, 100, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(122, 100, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(180, 100, rectLength, rectHeight, TFT_BLACK);
  }

  if (whatToClear == "all")
  {
    ttgo->tft->fillRect(5, 50, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(62, 50, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(122, 50, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(180, 50, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(5, 100, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(62, 100, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(122, 100, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(180, 100, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(1, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(22, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(44, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(66, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(88, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(110, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(132, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(154, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(176, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(198, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(220, 150, (rectLength / 2) - 5, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(5, 200, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(62, 200, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(122, 200, rectLength, rectHeight, TFT_BLACK);
    ttgo->tft->fillRect(180, 200, rectLength, rectHeight, TFT_BLACK);
  }


  tft->setTextFont(2);
  tft->setCursor(0, 0);
  tft->print(power->getBattPercentage());
  tft->println(" %");

  Serial.print("5 Hour Count: ");
  Serial.println(FiveHourCount);

  Serial.print("Remaining Hours: ");
  Serial.println(RemainingHours);

  switch (FiveHourCount) {
    case 0:
      ttgo->tft->drawRect(5, 50, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(62, 50, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(122, 50, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(180, 50, rectLength, rectHeight, TFT_WHITE);
      break;
    case 1:
      ttgo->tft->fillRect(5, 50, rectLength, rectHeight, TFT_RED);
      ttgo->tft->drawRect(62, 50, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(122, 50, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(180, 50, rectLength, rectHeight, TFT_WHITE);
      break;
    case 2:
      ttgo->tft->fillRect(5, 50, rectLength, rectHeight, TFT_RED);
      ttgo->tft->fillRect(62, 50, rectLength, rectHeight, TFT_RED);
      ttgo->tft->drawRect(122, 50, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(180, 50, rectLength, rectHeight, TFT_WHITE);
      break;
    case 3:
      ttgo->tft->fillRect(5, 50, rectLength, rectHeight, TFT_RED);
      ttgo->tft->fillRect(62, 50, rectLength, rectHeight, TFT_RED);
      ttgo->tft->fillRect(122, 50, rectLength, rectHeight, TFT_RED);
      ttgo->tft->drawRect(180, 50, rectLength, rectHeight, TFT_WHITE);
      break;
    case 4:
      ttgo->tft->fillRect(5, 50, rectLength, rectHeight, TFT_RED);
      ttgo->tft->fillRect(62, 50, rectLength, rectHeight, TFT_RED);
      ttgo->tft->fillRect(122, 50, rectLength, rectHeight, TFT_RED);
      ttgo->tft->fillRect(180, 50, rectLength, rectHeight, TFT_RED);
      break;
  }

  switch (RemainingHours) {
    case 0:
      ttgo->tft->drawRect(5, 100, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(62, 100, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(122, 100, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(180, 100, rectLength, rectHeight, TFT_WHITE);
      break;
    case 1:
      ttgo->tft->fillRect(5, 100, rectLength, rectHeight, TFT_RED);
      ttgo->tft->drawRect(62, 100, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(122, 100, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(180, 100, rectLength, rectHeight, TFT_WHITE);
      break;
    case 2:
      ttgo->tft->fillRect(5, 100, rectLength, rectHeight, TFT_RED);
      ttgo->tft->fillRect(62, 100, rectLength, rectHeight, TFT_RED);
      ttgo->tft->drawRect(122, 100, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(180, 100, rectLength, rectHeight, TFT_WHITE);
      break;
    case 3:
      ttgo->tft->fillRect(5, 100, rectLength, rectHeight, TFT_RED);
      ttgo->tft->fillRect(62, 100, rectLength, rectHeight, TFT_RED);
      ttgo->tft->fillRect(122, 100, rectLength, rectHeight, TFT_RED);
      ttgo->tft->drawRect(180, 100, rectLength, rectHeight, TFT_WHITE);
      break;
    case 4:
      ttgo->tft->fillRect(5, 100, rectLength, rectHeight, TFT_RED);
      ttgo->tft->fillRect(62, 100, rectLength, rectHeight, TFT_RED);
      ttgo->tft->fillRect(122, 100, rectLength, rectHeight, TFT_RED);
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
      ttgo->tft->drawRect(62, 200, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(122, 200, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(180, 200, rectLength, rectHeight, TFT_WHITE);
      break;
    case 1:
      ttgo->tft->fillRect(5, 200, rectLength, rectHeight, TFT_ORANGE);
      ttgo->tft->drawRect(62, 200, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(122, 200, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(180, 200, rectLength, rectHeight, TFT_WHITE);
      break;
    case 2:
      ttgo->tft->fillRect(5, 200, rectLength, rectHeight, TFT_ORANGE);
      ttgo->tft->fillRect(62, 200, rectLength, rectHeight, TFT_ORANGE);
      ttgo->tft->drawRect(122, 200, rectLength, rectHeight, TFT_WHITE);
      ttgo->tft->drawRect(180, 200, rectLength, rectHeight, TFT_WHITE);
      break;
    case 3:
      ttgo->tft->fillRect(5, 200, rectLength, rectHeight, TFT_ORANGE);
      ttgo->tft->fillRect(62, 200, rectLength, rectHeight, TFT_ORANGE);
      ttgo->tft->fillRect(122, 200, rectLength, rectHeight, TFT_ORANGE);
      ttgo->tft->drawRect(180, 200, rectLength, rectHeight, TFT_WHITE);
      break;
    case 4:
      ttgo->tft->fillRect(5, 200, rectLength, rectHeight, TFT_ORANGE);
      ttgo->tft->fillRect(62, 200, rectLength, rectHeight, TFT_ORANGE);
      ttgo->tft->fillRect(122, 200, rectLength, rectHeight, TFT_ORANGE);
      ttgo->tft->fillRect(180, 200, rectLength, rectHeight, TFT_ORANGE);
      break;
  }

}
