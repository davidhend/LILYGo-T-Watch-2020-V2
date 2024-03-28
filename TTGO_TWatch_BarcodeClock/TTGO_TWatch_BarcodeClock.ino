#include "config.h"

TTGOClass *ttgo = nullptr;
TFT_eSPI *tft = nullptr;
TinyGPSPlus *gps = nullptr;
AXP20X_Class *power;

uint32_t last = 0;
uint32_t updateTimeout = 0;

int leftside[11] = {B0001101, B0011001, B0010011, B0111101, B0100011, B0110001, B0101111, B0111011, B0110111, B0001011};
int rightside[11] = {B1110010, B1100110, B1101100, B1000010, B1011100, B1001110, B1010000, B1000100, B1001000, B1110100};
int barcodeSentance[54];
byte x;

int Month = 0;
int Day = 0;
int Year = 0;

int Hour = 0;
int Minute = 0;
int Second = 0;

int previousSecond = 0;
int previousMinute = 0;

char buf[128];
bool irq = false;

int Xstart = 10;
int Ystart = 30;
int Xend = 10;
int Yend = Ystart + 165;

boolean PM = false;

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

  }

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

    if (previousMinute != Minute) {
      /*
        ttgo->tft->fillCircle(150+Xoffset, 90, 10, TFT_BLACK);
        ttgo->tft->fillCircle(120+Xoffset, 90, 10, TFT_BLACK);
        ttgo->tft->fillCircle(90+Xoffset, 90, 10, TFT_BLACK);
        ttgo->tft->fillCircle(60+Xoffset, 90, 10, TFT_BLACK);

        ttgo->tft->fillCircle(180+Xoffset, 160, 10, TFT_BLACK);
        ttgo->tft->fillCircle(150+Xoffset, 160, 10, TFT_BLACK);
        ttgo->tft->fillCircle(120+Xoffset, 160, 10, TFT_BLACK);
        ttgo->tft->fillCircle(90+Xoffset, 160, 10, TFT_BLACK);
        ttgo->tft->fillCircle(60+Xoffset, 160, 10, TFT_BLACK);
        ttgo->tft->fillCircle(30+Xoffset, 160, 10, TFT_BLACK);
      */
    }

    tft->setTextFont(2);
    tft->setCursor(0, 0);
    tft->print(power->getBattPercentage());
    tft->println(" %");

    if (Second != previousSecond) {

    }

    displayTime();

    previousSecond = Second;
    previousMinute = Minute;

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

void displayTime()
{

  if (Hour >= 12) {
    PM = true;
    if (Hour >= 13) {
      Hour = Hour - 12;
    }
  } else {
    PM = false;
  }

  //Fill barcode sentence array with 0's
  for (int i = 0; i < 53; i++) {
    barcodeSentance[i] = 0;
  }

  //Add Start/End & Mid Bits
  barcodeSentance[0] = 1;
  barcodeSentance[1] = 0;
  barcodeSentance[2] = 1;

  barcodeSentance[24] = 0;
  barcodeSentance[25] = 1;
  barcodeSentance[26] = 0;
  barcodeSentance[27] = 1;
  barcodeSentance[28] = 0;

  barcodeSentance[50] = 1;
  barcodeSentance[51] = 0;
  barcodeSentance[52] = 1;

  //Convert Hours to Char Array
  char hourChars[3];
  itoa(Hour, hourChars, 10);
  if (Hour <= 9) {
    hourChars[1] = hourChars[0];
    hourChars[0] = '0';
  }

  //Convert Minutes to Char Array
  char minuteChars[3];
  itoa(Minute, minuteChars, 10);
  if (Minute <= 9) {
    minuteChars[1] = minuteChars[0];
    minuteChars[0] = '0';
  }

  //Convert Minutes to Char Array
  char secondChars[3];
  itoa(Second, secondChars, 10);
  if (Second <= 9) {
    secondChars[1] = secondChars[0];
    secondChars[0] = '0';
  }

  //Look up the appropriate binary sequence for hours and itterate over the bits
  for (int i = 0; i < 2; i ++) {
    int index = hourChars[i] - '0';
    x = leftside[index];
    for (int j = 6; j >= 0; --j) {
      int value = bitRead(x, j);
      if (i == 0) {
        barcodeSentance[9 - j] = value;
      } else if (i == 1) {
        barcodeSentance[16 - j] = value;
      }
    }
  }

  //Look up the appropriate binary sequence for minutes and itterate over the bits
  for (int i = 0; i < 2; i ++) {
    int index = minuteChars[i] - '0';
    //Split the minutes, one digit on the left side, one on the right
    if (i == 0) {
      x = leftside[index];
    } else if (i == 1) {
      x = rightside[index];
    }
    for (int j = 6; j >= 0; --j) {
      int value = bitRead(x, j);
      if (i == 0) {
        barcodeSentance[23 - j] = value;
      } else if (i == 1) {
        barcodeSentance[35 - j] = value;
      }
    }
  }

  //Look up the appropriate binary sequence for minutes and itterate over the bits
  for (int i = 0; i < 2; i ++) {
    int index = secondChars[i] - '0';
    x = rightside[index];
    for (int j = 6; j >= 0; --j) {
      int value = bitRead(x, j);
      if (i == 0) {
        barcodeSentance[42 - j] = value;
      } else if (i == 1) {
        barcodeSentance[49 - j] = value;
      }
    }
  }

  int offset = 4;
  for (int i = 0; i < 53; i++) {
    if (barcodeSentance[i] == 1) {
      if (i == 0 || i == 1 | i == 2 || i == 24 || i == 25 || i == 26 || i == 27 || i == 28 || i == 50 || i == 51 || i == 52)
      {
        ttgo->tft->drawLine(Xstart + offset, Ystart, Xend + offset, Yend + 10, TFT_WHITE);
      } else {
        ttgo->tft->drawLine(Xstart + offset, Ystart, Xend + offset, Yend, TFT_WHITE);
      }
    } else if (barcodeSentance[i] == 0) {
      ttgo->tft->drawLine(Xstart + offset, Ystart, Xend + offset, Yend, TFT_BLACK);
    }
    offset = offset + 4;
  }

  if (PM) {
    tft->setCursor(40, 210);
    tft->print("P");
    tft->setCursor(185, 210);
    tft->print("M");
  } else {
    tft->setCursor(40, 210);
    tft->print("A");
    tft->setCursor(185, 210);
    tft->print("M");
  }

  tft->setCursor(90, 210);
  if (Hour <= 9) {
    tft->print("0");
    tft->print(Hour);
  } else {
    tft->print(Hour);
  }
  tft->print(":");
  if (Minute <= 9) {
    tft->print("0");
    tft->print(Minute);
  } else {
    tft->print(Minute);
  }
  tft->print(":");
  if (Second <= 9) {
    tft->print("0");
    tft->println(Second);
  } else {
    tft->println(Second);
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
