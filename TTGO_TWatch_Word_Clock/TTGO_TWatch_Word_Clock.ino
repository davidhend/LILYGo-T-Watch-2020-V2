#include "config.h"

// New background colour
#define TFT_BROWN 0x38E0

// Pause in milliseconds between screens, change to 0 to time font rendering
#define WAIT 500

TTGOClass *ttgo;
TFT_eSPI *tft = nullptr;
TinyGPSPlus *gps = nullptr;
AXP20X_Class *power;

uint32_t last = 0;
bool irq = false;
char buf[128];

int Hour = 0;
int Minute = 0;
int Second = 0;

int Month = 0;
int Day = 0;
int Year = 0;

int previousMinute = 0;

void setup(void)
{
  Serial.begin(115200);
  
  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  ttgo->openBL();

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

void loop()
{

  ttgo->gpsHandler();

  if (gps->date.isUpdated()) {

    Day = gps->date.day();
    Month = gps->date.month(); 
    Year = gps->date.year() - 2000;
    
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

    if (previousMinute != Minute){
      displayWordClock();
    }
    
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

  previousMinute = Minute;

}

void displayWordClock()
{

  Serial.print("Hour: ");
  Serial.println(Hour);
  Serial.print("Minute: ");
  Serial.println(Minute);  
  
  ttgo->tft->fillScreen(TFT_BLACK);

  tft->setCursor(0, 0);
  ttgo->tft->setTextColor(TFT_YELLOW);
  tft->print(power->getBattPercentage());
  tft->println(" %");

  ttgo->tft->setTextColor(TFT_MAGENTA);   
  ttgo->tft->drawString("ITIISLTENHALF", 35, 26, 4);
  ttgo->tft->drawString("QUARTERTWENTY", 10, 52, 4);
  ttgo->tft->drawString("FIVEOMINUTESV", 25, 78, 4);
  ttgo->tft->drawString("PASTTOEONETWO", 10, 104, 4);
  ttgo->tft->drawString("THREEFOURFIVE", 20, 130, 4);
  ttgo->tft->drawString("SIXSEVENEIGHT", 25, 156, 4);
  ttgo->tft->drawString("NINETENELEVEN", 20, 182, 4);
  ttgo->tft->drawString("TWELVEUOCLOCK", 10, 208, 4);

  /* Parse time values to light corresponding pixels */
  ttgo->tft->setTextColor(TFT_YELLOW);
  ttgo->tft->drawString("IT", 35, 26, 4); //"IT"
  ttgo->tft->drawString("IS", 61, 26, 4); //"IS"

  
  /* Minutes between 0-5 - Light "O CLOCK" */
  if ((Minute>=0 && Minute<5)){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("OCLOCK", 125, 208, 4);
  }
  
  /* Minutes between 5-10 or 55-60 - Light "FIVE," "MINUTES" */
  if ((Minute>=5 && Minute<10) || (Minute>=55 && Minute<60)){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("FIVE", 25, 78, 4); //"FIVE"
    ttgo->tft->drawString("MINUTES", 96, 78, 4); //"MINUTES"
  }
  
  /* Minutes between 10-15 or 50-55 - Light "TEN," "MINUTES" */
  if ((Minute>=10 && Minute<15) || (Minute>=50 && Minute<55)){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("TEN", 96, 26, 4); //"TEN"
    ttgo->tft->drawString("MINUTES", 96, 78, 4); //"MINUTES"
  }


  /* Minutes between 15-20 or 45-50 - Light "QUARTER" */
  if ((Minute>=15 && Minute<20) || (Minute>=45 && Minute<50)){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("QUARTER", 10, 52, 4); //"QUARTER"
  }
  
  /* Minutes between 20-25 or 40-45 - Light "TWENTY," "MINUTES" */
  if ((Minute>=20 && Minute<25) || (Minute>=40 && Minute<45)){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("TWENTY", 127, 52, 4); //"TWENTY"
    ttgo->tft->drawString("MINUTES", 96, 78, 4); //"MINUTES"
  }  

  /* Minutes between 25-30 or 35-40 - Light "TWENTY," "FIVE," "MINUTES" */
  if ((Minute>=25 && Minute<30) || (Minute>=35 && Minute<40)){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("TWENTY", 127, 52, 4); //"TWENTY"
    ttgo->tft->drawString("FIVE", 25, 78, 4); //"FIVE"
    ttgo->tft->drawString("MINUTES", 96, 78, 4); //"MINUTES"
  }

  /* Minutes between 30-35 - Light "HALF" */
  if ((Minute>=30 && Minute<35)){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("HALF", 144, 26, 4); //"HALF"
  }
  
  /* Minutes between 5-35 - Light "PAST" */
  if ((Minute>=5) && (Minute<35)){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("PAST", 10, 104, 4); //"PAST"
  }

  /* Minutes between 35-60 - Light "TO" ADD 1 TO HOUR TO ACCOUNT FOR BEGIN ALMOST 6 OCLOCK*/
  if (Minute>=35){
    Hour++;
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("TO", 71, 104, 4); //"TO"
  }

  /* Hour=1 - Light "ONE" */
  if (Hour==1){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("ONE", 121, 104, 4); //"ONE"   
  }
  
  /* Hour=2 - Light "TWO" */
  if (Hour==2){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("TWO", 174, 104, 4); //"TWO"    
  }
  
  /* Hour=3 - Light "THREE" */
  if (Hour==3){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("THREE", 20, 130, 4); //"THREE"
  }
  
  /* Hour=4 - Light "FOUR" */
  if (Hour==4){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("FOUR", 101, 130, 4); //"FOUR"
  }
  
  /* Hour=5 - Light "FIVE" */
  if (Hour==5){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("FIVE", 170, 130, 4); //"FIVE"
  }
  
  /* Hour=6 - Light "SIX" */
  if (Hour==6){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("SIX", 25, 156, 4); //"SIX"    
  }
  
  /* Hour=7 - Light "SEVEN" */
  if (Hour==7){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("SEVEN", 62, 156, 4); //"SEVEN"
  }
  
  /* Hour=8 - Light "EIGHT" */
  if (Hour==8){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("EIGHT", 143, 156, 4); //"EIGHT"
  }
  
  /* Hour=9 - Light "NINE" */
  if (Hour==9){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("NINE", 20, 182, 4); //"NINE"
  }
  
  /* Hour=10 - Light "TEN" */
  if (Hour==10){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("TEN", 78, 182, 4); //"TEN"
  }
  
  /* Hour=11 - Light "ELEVEN" */
  if (Hour==11){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("ELEVEN", 126, 182, 4); //"ELEVEN"
  }
  
  /* Hour=12 - Light "TWELVE" */
  if (Hour==12){
    ttgo->tft->setTextColor(TFT_YELLOW);
    ttgo->tft->drawString("TWELVE", 10, 208, 4); //"TWELVE"
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
