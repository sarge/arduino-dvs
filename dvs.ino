#include <OneWire.h>
#include "U8glib.h"

// io
const int temperaturePin = 0; // analog a0
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
OneWire  room(2);
byte ceiling_addr[8];
OneWire  ceiling(3);

// history
const int DATAPOINTS = 96;
const unsigned long INTERVAL = 900000;//(24 * 60 * 60 * 1000) / DATAPOINTS; // five minute intervals
//const unsigned long INTERVAL = 100; // testing
int histTemp[DATAPOINTS];
unsigned long nextRecordMilli = 0;
unsigned int nextRecordIndex = 0;
int temp_low = 40;
int temp_high = -40;

// fan control
int currentFanSpeed = 0;

// gfx
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI

const  int max_size = 76;

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

void drawStatus2(int roomTemp) {
  u8g_prepare();

  u8g.drawLine(44, 0, 24, 63);
  u8g.drawLine(46, 0, 26, 63);

  setFont(FONT_BIG);
  int width = u8g.getStrWidth(String(roomTemp).c_str());
  u8g.drawStr( 36 - width, 0, String(roomTemp).c_str());

  setFont(FONT_SMALL);
  u8g.drawStr( 0, 50, ("L " + String(temp_low)).c_str());
  u8g.drawStr( 0, 40, ("H " + String(temp_high)).c_str());
  u8g.drawStr( 37, -1, "o");

  setFont(FONT_NORMAL);
  u8g.drawStr(50,  0, "Fan:");
  u8g.drawStr(50, 10, "Temp:");

  u8g.drawStr(90,  0, currentFanSpeed > 10 ? "on" : "off");

  // fan temp
  setFont(editMode == 0 ? FONT_BOLD : FONT_NORMAL);
  u8g.drawStr(90, 10, String(setting[0]).c_str());

  // fan speed
  int speed_percentage = int((float(setting[1]) / float(settingMax[1])) * 100);
  int screen_size = (float(speed_percentage) / float(100)) * max_size;

  if (editMode == 1)
    u8g.drawFrame(48, 21, max_size + 4, 8);
  u8g.drawBox(50, 23, screen_size, 4);

  // print history
  for(int i = nextRecordIndex; i < DATAPOINTS + nextRecordIndex; i++){
    u8g.drawPixel(30 + i - nextRecordIndex, 60 - histTemp[i % DATAPOINTS ]);
  }
}

void setup() {

  Serial.begin(9600);
  
   // 28 C8 39 1 7 0 0 A2
  
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


void recordHistory(int temp){

  if (nextRecordMilli < millis()){
    histTemp[nextRecordIndex] = temp;
    nextRecordIndex ++;

    if (nextRecordIndex == DATAPOINTS)
      nextRecordIndex = 0;

    nextRecordMilli = millis() + INTERVAL;

    // recalculate lows and highs
    for(int i = 0; i < DATAPOINTS; i++){
      if (histTemp[i] < temp_low)
        temp_low = histTemp[i];

      if (histTemp[i] > temp_high)
        temp_high = histTemp[i];
    }
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
  
  int i;
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
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


void updateFan(int temp){

  int desiredFanSpeed = currentFanSpeed;

  if (temp > setting[TURNONTEMP])
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
  prepareTemperatureDS(ceiling, ceiling_addr);
  
  delay(1000);     // maybe 750ms is enough, maybe not
  
  int temp = getTemperatureDS(room, room_addr);
  int temp2 = getTemperatureDS(ceiling, ceiling_addr);

  checkButtons();

  updateFan(temp);

  recordHistory(temp);

  u8g.firstPage();
  do {
    drawStatus2(temp);
  } while(u8g.nextPage());
}
