#include <OneWire.h>
#include "U8glib.h"

// history
const int DATAPOINTS = 96;
const unsigned long INTERVAL = 900000;//(24 * 60 * 60 * 1000) / DATAPOINTS; // five minute intervals
//const unsigned long INTERVAL = 10; // testing

namespace History {
  typedef struct {
    int data[DATAPOINTS];
    int minimum = 40;
    int maximum = -40;
    unsigned int nextRecordIndex = 0;
    unsigned long nextRecordMilli = 0;
  } HISTORY;

  void initialiseHistory(HISTORY *h){
    for(int i = 0; i < DATAPOINTS ; i++){
      h->data[i] = -100;
    }
  };

  void record(HISTORY *h, int temp){
    
    if (h->nextRecordMilli < millis()){
      h->data[h->nextRecordIndex] = temp;
      h->nextRecordIndex ++;

      if (h->nextRecordIndex == DATAPOINTS)
        h->nextRecordIndex = 0;

      h->nextRecordMilli = millis() + INTERVAL;

      // recalculate lows and highs
      for(int i = 0; i < DATAPOINTS; i++){
        if (h->data[i] > -100){
          if (h->data[i] < h->minimum)
            h->minimum = h->data[i];

          if (h->data[i] > h->maximum)
            h->maximum = h->data[i];
        }
      }
    }
  }
  
  void printHistory(U8GLIB *u8g, HISTORY *h, int xOffset, int yOffset){
    for(int i = h->nextRecordIndex; i < DATAPOINTS + h->nextRecordIndex; i++){
      u8g->drawPixel(xOffset + i - h->nextRecordIndex, yOffset - h->data[i % DATAPOINTS ]);
    }
  }
  
  void printHistoryDash(U8GLIB *u8g, HISTORY *h, int xOffset, int yOffset){
    for(int i = h->nextRecordIndex; i < DATAPOINTS + h->nextRecordIndex; i+= 2){
      u8g->drawPixel(xOffset + i - h->nextRecordIndex, yOffset - h->data[i % DATAPOINTS ]);
    }
  }
}


// io
const int roomTempPin = 2; // 
const int ceilingTempPin = 4; // 

const int buttonDownPin = 11; //
const int buttonUpPin   = 12; //
const int fanSpeenPin   = 9;  //

// modes
const int TURNONTEMP = 0;
const int FANSPEED = 1;
int editMode = 0;              // 0 = temp, 1 = fan speed
int setting[]    = {19, 50};   // inital temp, inital fan speed
int settingMin[] = {10, 0};
int settingMax[] = {30, 250};
int settingInc[] = {1, 25};

int last_buttonUp_val = LOW;
int last_buttonDown_val = LOW;

// temperature
int lastTemp = 0;
int lastTempCount = 0;
const int TEMPDATAPOINTS = 10;
int recentTemp[TEMPDATAPOINTS];
int nextTempRecordIndex = 0;

byte room_addr[] = {0x28, 0xC8, 0x39, 0x1, 0x7, 0x0, 0x0, 0xA2};
OneWire  room(roomTempPin);
byte ceiling_addr[] = {0x28, 0x92, 0x22, 0x1, 0x7, 0x0, 0x0, 0x7}; // direct address didn't appear to work?
OneWire  ceiling(ceilingTempPin);

History::HISTORY roomHistory;
History::HISTORY ceilingHistory;
History::HISTORY fanHistory;

// fan control
int currentFanSpeed = 0;

// gfx
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI

const  int max_size = 50;

const int FONT_NORMAL = 0;
const int FONT_SMALL = 1;
const int FONT_BIG = 2;
const int FONT_BOLD = 3;

void u8g_prepare(void) {
  u8g.setFont(u8g_font_6x10);
  u8g.setFontRefHeightExtendedText();
  u8g.setDefaultForegroundColor();
  u8g.setFontPosTop();
}

void setFont(int font){
  switch(font){
    case FONT_NORMAL:
      u8g.setFont(u8g_font_6x10);
      break;
    case FONT_BOLD:
      u8g.setFont(u8g_font_helvB08);
      break;
    case FONT_SMALL:
      u8g.setFont(u8g_font_5x8);
      break;
    case FONT_BIG:
      u8g.setFont(u8g_font_fub25n);
      break;
  }
  u8g.setFontRefHeightExtendedText();
  u8g.setFontPosTop();
}

void drawStatus2(int roomTemp, int ceilingTemp) {
  u8g_prepare();

  setFont(FONT_BIG);
  int width = u8g.getStrWidth(String(roomTemp).c_str());
  u8g.drawStr( 36 - width, 0, String(roomTemp).c_str());

  setFont(FONT_SMALL);
  u8g.drawStr( 37, -1, "o");

  // ceiling
  u8g.drawStr( 0, 28, "Roof");
  u8g.drawStr( 0, 38, ("C "  + String(ceilingTemp)).c_str());
  u8g.drawStr( 0, 47, ("H " + String(ceilingHistory.maximum)).c_str());
  if (ceilingHistory.minimum > -100)
    u8g.drawStr( 0, 56, ("L " + String(ceilingHistory.minimum)).c_str());

  // room
  u8g.drawStr( 50, 0, ("H " + String(roomHistory.maximum)).c_str());
  if (ceilingHistory.minimum > -100)
    u8g.drawStr( 50, 10, ("L " + String(roomHistory.minimum)).c_str());

  u8g.drawStr(75,  0, "Fan:");
  u8g.drawStr(75, 10, "Temp:");
  u8g.drawStr(110,  0, currentFanSpeed > 10 ? "on" : "off");

  // fan temp
  setFont(editMode == 0 ? FONT_BOLD : FONT_NORMAL);
  u8g.drawStr(110, 10, String(setting[0]).c_str());

  // fan speed
  int speed_percentage = int((float(setting[1]) / float(settingMax[1])) * 100);
  int screen_size = (float(speed_percentage) / float(100)) * max_size;

  if (editMode == 1)
    u8g.drawFrame(48 + 25, 21, max_size + 4, 8);
  u8g.drawBox(50 + 25, 23, screen_size, 4);

  History::printHistoryDash(&u8g, &ceilingHistory, 30, 60);
  History::printHistory(&u8g, &roomHistory, 30, 60);
  History::printHistory(&u8g, &fanHistory, 30, 62);
}

void setup() {
  
  History::initialiseHistory(&roomHistory);  
  History::initialiseHistory(&ceilingHistory);
  History::initialiseHistory(&fanHistory);
  
  pinMode(buttonUpPin, INPUT);
  pinMode(buttonDownPin, INPUT);
  pinMode(fanSpeenPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // gfx
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  u8g.setRot180();
}

void checkButtons(){

  int buttonUp_val = digitalRead(buttonUpPin);
  int buttonDown_val = digitalRead(buttonDownPin);

  if (last_buttonUp_val != buttonUp_val)
  {
    last_buttonUp_val = buttonUp_val;
    return;
  }

  if (last_buttonDown_val != buttonDown_val)
  {
    last_buttonDown_val = buttonDown_val;
    return;
  }

  if (buttonUp_val == HIGH && buttonDown_val == HIGH){
    editMode = !editMode;

    last_buttonUp_val = LOW;
    last_buttonDown_val = LOW;
  }
  else if (buttonUp_val == HIGH && setting[editMode] < settingMax[editMode]){
    setting[editMode] = setting[editMode] + settingInc[editMode];
  }
  else if (buttonDown_val == HIGH && setting[editMode] > settingMin[editMode]){
    setting[editMode] = setting[editMode] - settingInc[editMode];
  }
}

void prepareTemperatureDS(OneWire ds, byte *addr){
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);
}

void prepareAndFindTemperatureDS(OneWire ds, byte *addr){
  if ( !ds.search(addr)) {
    ds.reset_search();
    return;
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);
}

int getTemperatureDS(OneWire ds, byte *addr){

  byte i;
  byte data[12];

  ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  byte cfg = (data[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
  return int((float)raw / 16.0);
}


void updateFan(int roomTemp, int ceilingTemp){

  int desiredFanSpeed = currentFanSpeed;
  
  // if the ceiling temp was greater than the threshold or the ceiling temp is warmer than the roomTemp
  if (ceilingTemp > setting[TURNONTEMP] || ceilingTemp > roomTemp)
    desiredFanSpeed = setting[FANSPEED];
  else
    desiredFanSpeed = 0;

  if (currentFanSpeed != desiredFanSpeed){
    analogWrite(fanSpeenPin, desiredFanSpeed);
    currentFanSpeed = desiredFanSpeed;
  }

  if (currentFanSpeed > 10)
    digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  
  prepareTemperatureDS(room, room_addr);
  prepareAndFindTemperatureDS(ceiling, ceiling_addr);
  
  delay(1000);     // maybe 750ms is enough, maybe not
  
  int roomTemp = getTemperatureDS(room, room_addr);
  int ceilingTemp = getTemperatureDS(ceiling, ceiling_addr);

  checkButtons();

  updateFan(roomTemp, ceilingTemp);

  History::record(&roomHistory, roomTemp);
  History::record(&ceilingHistory, ceilingTemp);
  History::record(&fanHistory, currentFanSpeed > 0 ? 1 : -100);
  
  u8g.firstPage();
  do {
    drawStatus2(roomTemp, ceilingTemp);    
  } while(u8g.nextPage());
}
