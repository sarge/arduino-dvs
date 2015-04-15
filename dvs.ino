// io
const int temperaturePin = 0; // analog
const int buttonDownPin = 11;
const int buttonUpPin   = 12;
const int fanSpeenPin   = 9;

// modes
const int TURNONTEMP = 0;
const int FANSPEED = 1;
int editMode = 0;             // 0 = temp, 1 = fan speed
int setting[]    = {19, 40};   // inital temp, inital fan speed
int settingMin[] = {15, 0};
int settingMax[] = {30, 150};
int settingInc[] = {1, 20};

// temperature
int currentTemp = 0;
int lastTemp = 0;
int lastTempCount = 0;

// history
const int DATAPOINTS = 48;
const unsigned long INTERVAL = 5 * 60000; // five minute intervals 
unsigned int histTemp[DATAPOINTS];
unsigned long nextRecordMilli = 0;
unsigned int nextRecordIndex = 0;

// fan control
int currentFanSpeed = 0;

// gfx
#include "U8glib.h"
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI 

void u8g_prepare(void) {
  u8g.setFont(u8g_font_6x10);
  u8g.setFontRefHeightExtendedText();
  u8g.setDefaultForegroundColor();
  u8g.setFontPosTop();
}

void drawStatus() {
  u8g_prepare();
    
  if (editMode == 0)
    u8g.drawStr( 0, 0, "edit mode: temp");
  else if (editMode == 1)
    u8g.drawStr( 0, 0, "edit mode: fan");
  
  u8g.drawStr( 0, 10, ("current temp: " + String(currentTemp)).c_str()); 
  u8g.drawStr( 0, 20,  ("fan temp: " + String(setting[0])).c_str());
  u8g.drawStr( 0, 30,  ("fan speed: " + String(setting[1])).c_str());
  u8g.drawStr( 0, 40,  ("fan curr speed: " + String(currentFanSpeed)).c_str());
}

void setup() {

  Serial.begin(9600);
  
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
  
  if (buttonUp_val == HIGH && buttonDown_val == HIGH){
    editMode = !editMode;
    
    Serial.print("mode: ");
    Serial.println(editMode);
  }
  else if (buttonUp_val == HIGH && setting[editMode] < settingMax[editMode]){
    Serial.println("up");
    setting[editMode] = setting[editMode] + settingInc[editMode];
  }
  else if (buttonDown_val == HIGH && setting[editMode] > settingMin[editMode]){
    Serial.println("down");
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
  
  if (nextRecordMilli < millis()){
    histTemp[nextRecordIndex] = temp;
    nextRecordIndex ++;
    
    if (nextRecordIndex == DATAPOINTS)
      nextRecordIndex = 0;
    
    nextRecordMilli = millis() + INTERVAL;
    
    printHistory();
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
    drawStatus();
  } while(u8g.nextPage()); 
 
  delay(200);
}
