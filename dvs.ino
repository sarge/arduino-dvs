// io
const int temperaturePin = 0; // analog
const int buttonDownPin = 11;
const int buttonUpPin   = 12;
const int fanSpeenPin   = 9;

// modes
const int TURNONTEMP = 0;
const int FANSPEED = 1;
int editMode = 0;              // 0 = temp, 1 = fan speed
int setting[]    = {19, 40};   // inital temp, inital fan speed
int settingMin[] = {15, 0};
int settingMax[] = {30, 160};
int settingInc[] = {1, 20};

int last_buttonUp_val = 0;
int last_buttonDown_val = 0;

// temperature
int currentTemp = 10;
int lastTemp = 0;
int lastTempCount = 0;

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
#include "U8glib.h"
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
      u8g.setFont(u8g_font_helvB24);
      break;
  }
  u8g.setFontRefHeightExtendedText();
  u8g.setFontPosTop();
}

void drawStatus() {
  u8g_prepare();
  
  setFont(FONT_NORMAL);  
  u8g.drawStr( 0, 10, ("current temp: " + String(currentTemp)).c_str()); 
  u8g.drawStr( 0, 40,  ("fan curr speed: " + String(currentFanSpeed)).c_str()); 
  
  setFont(editMode == 0 ? FONT_BOLD : FONT_NORMAL);
  u8g.drawStr( 0, 20,  ("fan temp: " + String(setting[0])).c_str());
  
  setFont(editMode == 1 ? FONT_BOLD : FONT_NORMAL);
  u8g.drawStr( 0, 30,  ("fan speed: " + String(setting[1])).c_str());
}

void drawStatus2() {
  u8g_prepare();
  
  u8g.drawLine(44, 0, 24, 63);
  u8g.drawLine(46, 0, 26, 63);

  setFont(FONT_BIG);
  
  int width = u8g.getStrWidth(String(currentTemp).c_str());
  u8g.drawStr( 36 - width, 0, String(currentTemp).c_str());
  
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

//  Serial.begin(9600);
  
  pinMode(buttonUpPin, INPUT);
  pinMode(buttonDownPin, INPUT);
  pinMode(fanSpeenPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // gfx
  pinMode(13, OUTPUT);           
  digitalWrite(13, HIGH); 
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
    
    buttonUp_val = LOW;
    last_buttonDown_val = LOW;
  }
  else if (buttonUp_val == HIGH && setting[editMode] < settingMax[editMode]){
    setting[editMode] = setting[editMode] + settingInc[editMode];
  }
  else if (buttonDown_val == HIGH && setting[editMode] > settingMin[editMode]){
    setting[editMode] = setting[editMode] - settingInc[editMode];
  }
}

void printHistory(){
  for(int j = 0; j < DATAPOINTS; j++){
      Serial.print(histTemp[j]);
      Serial.print("|");
    }
  Serial.println("");
  Serial.print("current: ");
  Serial.println(millis());
  Serial.print("internval: ");
  Serial.println(INTERVAL);
  Serial.print("next: ");
  Serial.println(nextRecordMilli); 
}

void recordHistory(int temp){
  
//  currentTemp = temp = (millis() % 30) - 5;  //fake data
  
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
        
    //printHistory();
  }
}


int getTemperature(){
  
  int rawTemp = analogRead(temperaturePin);
  int temp = (125*rawTemp)>>8 ; // Temperature calculation formula
  
  if (lastTemp == temp){
    lastTempCount++;
  }
  else 
  {
    lastTemp = temp;
    lastTempCount = 0;
  }
  
  if (currentTemp != lastTemp && lastTempCount == 3){
    currentTemp = lastTemp;
  }
  
  return currentTemp;
}

void updateFan(int temp){
  
  int desiredFanSpeed = currentFanSpeed;
  
  if (temp > setting[TURNONTEMP])
    desiredFanSpeed = setting[FANSPEED];
  else
    desiredFanSpeed = 0;
  
  if (currentFanSpeed != desiredFanSpeed){
    Serial.print("Changing fan speed to : ");
    Serial.println(desiredFanSpeed); 

    analogWrite(fanSpeenPin, desiredFanSpeed);
    currentFanSpeed = desiredFanSpeed;
  }
  
  if (currentFanSpeed > 10)
        digitalWrite(LED_BUILTIN, HIGH);
}

void printStatus(){
  Serial.print("temp : "); 
  Serial.print(currentTemp); 
  Serial.print(" mode: "); 
  Serial.print(editMode);
  Serial.print(" temp: "); 
  Serial.print(setting[0]);
  Serial.print(" fan: "); 
  Serial.print(setting[1]);
  Serial.print(" fan speed: "); 
  Serial.print(currentFanSpeed);
  Serial.println("");
}

void loop() {
 
  int temp = getTemperature();
 
  checkButtons();
 
  updateFan(temp);
  
  recordHistory(temp);
  
//  printStatus();
  
  u8g.firstPage(); 
  do {
    drawStatus2();
  } while(u8g.nextPage()); 
 
  delay(200);
}
