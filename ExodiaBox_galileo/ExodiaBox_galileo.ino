#include <DHT.h>
#include<pt.h>
#include <LiquidCrystal.h>
#include <Servo.h>

//ProtoThread
#define PT_DELAY(pt, ms, ts) \
ts = millis(); \
PT_WAIT_WHILE(pt, millis()-ts < (ms));

//GPIO pin definition
#define LED1 A5
#define DHTIN 12 
#define DHTOUT 11
#define DHTTYPE DHT11 
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
const int TRIG_PIN = 2;
const int ECHO_PIN = 3;
// Define the sensor type (DHT 11)
DHT dht(DHTIN,DHTOUT, DHTTYPE);

//machineStatus
int height;
int humid;
int uv;
int temperature;
int top;
String toServer ;

struct pt pt_serverSendSerial;
struct pt pt_localTaskHumind;
struct pt pt_localTaskSound;
struct pt pt_localTaskUv;
struct pt pt_localTaskUltra;
struct pt pt_localTaskRoof;
struct pt pt_localTaskWater;
struct pt pt_taskTopServo;
struct pt pt_taskSideServo;

//valiable
String label = "";
//initualize Ultra
int maxCm = 22; //heigh of box - soil       change
int duration, distanceCm;
//initialize on/off
    /////////////////////////////
    ////variables description//// 
    ////  -> '0' == turn off ////
    ////  -> '1' == turn on  ////
    /////////////////////////////
int watering = 0;
int roof = 0;
int tempCheck[] = {0,0};
//initialize sound
int soundSensorPin = A0;
int soundReading = 0;
int soundThreshold = 500;
int counter = 0;
//initualize Uv
int value;
int sound;
//initialize humid
float h;
float t;
int ha;
int temp;
Servo myServo1; 
Servo myServo2; 

void setup() {
  // set serial of board and nodeMCU
  Serial.begin(9600);//galileo
  Serial1.begin(115200);//server
  lcd.begin(16, 2);
  dht.begin();

  // set pinMode
  pinMode(TRIG_PIN,OUTPUT);
  pinMode(ECHO_PIN,INPUT);
  pinMode(A0, INPUT);
  pinMode(LED1, OUTPUT);
  pinMode(soundSensorPin, INPUT);
  pinMode(A1, INPUT);

  myServo1.attach(10); 
  myServo2.attach(13);
  
  PT_INIT(&pt_serverSendSerial);
  PT_INIT(&pt_localTaskHumind);
  PT_INIT(&pt_localTaskSound);
  PT_INIT(&pt_localTaskUv);
  PT_INIT(&pt_localTaskUltra);
  PT_INIT(&pt_localTaskRoof);
  PT_INIT(&pt_localTaskWater);
  PT_INIT(&pt_taskTopServo);
  PT_INIT(&pt_taskSideServo);
  
}

void serialEvent(){   // resieve data from nodeMCU
  if(Serial1.available()){
    String str = Serial1.readStringUntil('\r');
    Serial.println(str);
    if(str.substring(0,2) == "00"){
      watering = int(str.charAt(3))-'0';
      roof = int(str.charAt(5))-'0';
      if(str.charAt(7) != '$' && str.substring(7).length() <= 16){
        label = str.substring(7);
      }
    }
    lcd.clear();
    lcd.print(label);
  }
  
}

PT_THREAD(serverSendSerial(struct pt* pt)){    // send data to nodeMCU
  static uint32_t ts;
  PT_BEGIN(pt);
  while(1){
    toServer = "00_";
    toServer += String(height);
    toServer += "_";
    toServer += String(humid);
    toServer += "_";
    toServer += String(uv);
    toServer += "_";
    toServer += String(temperature);
    toServer += "_";
    toServer += String(roof);
    Serial1.println(toServer);
    Serial.println(toServer);
    
    PT_DELAY(pt, 1000, ts);
  }
  PT_END(pt);
  
}

PT_THREAD(localTaskHumind(struct pt* pt)){
  static uint32_t ts;
  PT_BEGIN(pt);
  while(1) {
    PT_DELAY(pt,2000,ts);
    h = dht.readHumidity();
    t = dht.readTemperature();
    if (!(isnan(h) || isnan(t))) {
      ha = (int)h;
      temp = (int)t;
      Serial.print("Humidity: ");
      Serial.print(ha);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.print(temp);
      Serial.println(" *C ");
      humid = ha;
      temperature = temp;
    }
    PT_YIELD(pt);
  }
  PT_END(pt);
  
}

PT_THREAD(localTaskSound(struct pt* pt)){
  static uint32_t ts;
  PT_BEGIN(pt);
  while(1){
    soundReading = analogRead(soundSensorPin);
    if(soundReading > soundThreshold){
      if(counter % 2 == 0){
         digitalWrite(LED1, HIGH);
         delay(100);
         Serial.println(counter);
      }
      if(counter % 2 == 1){
        digitalWrite(LED1, LOW);
        delay(100);
        Serial.println(counter);
      }
      counter++;
    }
    PT_YIELD(pt);
  }
  PT_END(pt);
  
}

PT_THREAD(localTaskUv(struct pt* pt)){
  static uint32_t ts;
  PT_BEGIN(pt);
  while(1) {
   value = analogRead(A1);
   sound = (value * 100) / 240;
   uv = sound;
   PT_DELAY(pt, 1000, ts);
  }
  PT_END(pt);
  
}

PT_THREAD(localTaskUltra(struct pt* pt)){
  static uint32_t ts;
  PT_BEGIN(pt);
  while(1){
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN,HIGH);
    distanceCm = duration / 29.1 / 2 ;
    Serial.println(maxCm - distanceCm);
    height = maxCm - distanceCm;
    PT_DELAY(pt, 1000, ts);
  }
  PT_END(pt);
  
}

PT_THREAD(localTaskRoof(struct pt* pt)){
  static uint32_t ts;
  PT_BEGIN(pt);
  while(1){
    if(roof == 1){
      if(tempCheck[1] != roof){
        Serial.println("open roof");
        top = 1;
        tempCheck[1] = 1;
      }
    }else{
      if(tempCheck[1] != roof){
        Serial.println("close roof");
        top = 0;
        tempCheck[1] = 0;
      }
    }
    PT_DELAY(pt, 1000, ts);
  }
  PT_END(pt);
  
}

PT_THREAD(localTaskWater(struct pt* pt)){
  static uint32_t ts;
  PT_BEGIN(pt);
  while(1){
    if(watering == 1){
      if(tempCheck[0] != watering){
        Serial.println("open watering");
        tempCheck[0] = 1;
      }
   
    }else{
      if(tempCheck[0] != watering){
        Serial.println("close watering");
        tempCheck[0] = 0;
      }
    }
    PT_DELAY(pt, 1000, ts);
  }
  PT_END(pt);
  
}

PT_THREAD(taskTopServo(struct pt* pt)){
  static uint32_t ts;
  static int x = 10;
  PT_BEGIN(pt);
  while(1){
    PT_DELAY(pt,10,ts)
      if (roof == 1){
          if(x < 100){
            x += 1;
            myServo1.write(x) ;
          }
      }
      if (roof == 0){
        if(x > 10){
          x -= 1;
        myServo1.write(x) ;
      }
    }
  }
  PT_END(pt);
  
}

PT_THREAD(taskSideServo(struct pt* pt)){
  static uint32_t ts;
  static int x = 10;
  PT_BEGIN(pt);
  while(1){
    PT_DELAY(pt,10,ts)
      if (watering == 1){
          if(x < 90){
            x += 1;
            myServo2.write(x) ;
          }
      }
      if (watering == 0){
        if(x > 10){
          x -= 1;
        myServo2.write(x) ;
      }
    }
  }
  PT_END(pt);
  
}

void loop() {
  serialEvent();
  serverSendSerial(&pt_serverSendSerial);
  localTaskHumind(&pt_localTaskHumind);
  localTaskSound(&pt_localTaskSound);
  localTaskUv(&pt_localTaskUv);
  localTaskUltra(&pt_localTaskUltra);
  localTaskRoof(&pt_localTaskRoof);
  localTaskWater(&pt_localTaskWater);
  taskTopServo(&pt_taskTopServo);
  taskSideServo(&pt_taskSideServo);

}
