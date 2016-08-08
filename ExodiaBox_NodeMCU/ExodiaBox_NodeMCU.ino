#include <ServerExceed.h>
#include <SPI.h>
#include <BlynkSimpleEsp8266.h>

WiFiServer server(80); // nodeMCU server : port 80
char ssid[] = "nodeMCU only";
char password[] = "";
char host[] = "10.32.176.4";
int port = 80;
String group = "eXodia"; 

ServerExceed mcu(ssid, password, host, port, group, &server);

// Setting for Blynk
char auth[] = "587f8857587d461b820d1d0dc53026d7";
char blynk_host[] = "10.32.176.4";
int blynk_port = 18442;

int watering = 0;
int roof = 0;

void setup() {
  Serial.begin(115200);
  mcu.connectServer();
  Serial.print("\n\nIP: ");
  Serial.println(WiFi.localIP());
  Blynk.config(auth, blynk_host, blynk_port);
}

String data = "";
String blynk_data = "";
String server_data = "";

BLYNK_WRITE(V1){//roof
  if(roof == 0){
    String temp = "00_";
    temp += watering;
    Serial.print(temp += "_1" );
    roof = 1;
  }else if(roof == 1) {
    String temp = "00_";
    temp += watering;
    Serial.print(temp += "_0" );
    roof = 0;
  }
}

BLYNK_WRITE(V2){//watering
  if(watering == 0){
    String temp = "00_1_";
    Serial.print(temp += String(roof) );
    watering = 1;
  }else if (watering == 1){
     String temp = "00_0_";
    Serial.print(temp += String(roof) );
    watering = 0;
  }
}

void loop() {
  if(Serial.available()) {
    data = Serial.readStringUntil('\n');
    data.replace("\r","");
    data.replace("\n","");
    Serial.flush();
      mcu.sendDataFromBoardToServer(data);

    
  }
  mcu.sendDataFromServerToBoard();
  Blynk.run();
}
