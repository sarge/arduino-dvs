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

void setup() {

  Serial.begin(9600);
  
  pinMode(buttonUpPin, INPUT);
  pinMode(buttonDownPin, INPUT);
  pinMode(fanSpeenPin, OUTPUT);
}

void checkButtons(){
  
  int buttonUp_val = digitalRead(buttonUpPin);
  int buttonDown_val = digitalRead(buttonDownPin);
  
  if (buttonUp_val == 1 && buttonDown_val == 1){
    editMode = !editMode;
    Serial.print("mode: ");
    Serial.println(editMode);
  }
  else if (buttonUp_val == 1 && setting[editMode] < settingMax[editMode]){
    Serial.println("up");
    setting[editMode] = setting[editMode] + settingInc[editMode];
  }
  else if (buttonDown_val == 1 && setting[editMode] > settingMin[editMode]){
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
  
  printStatus();
 
  delay(200);
}
