#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>	// https://github.com/Imroy/pubsubclient
#include <RGBConverter.h>


// #define PWMRANGE 255
// #define PWM_VALUE 31
//int gamma_table[PWM_VALUE+1] = {
//    0, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 8, 10, 11, 13, 16, 19, 23,
//    27, 32, 38, 45, 54, 64, 76, 91, 108, 128, 152, 181, 215, 255
//};

 #define PWM_VALUE 255
//int gamma_table[PWM_VALUE+1] = {
//    0, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10,
//    11, 12, 13, 15, 17, 19, 21, 23, 26, 29, 32, 36, 40, 44, 49, 55,
//    61, 68, 76, 85, 94, 105, 117, 131, 146, 162, 181, 202, 225, 250,
//    279, 311, 346, 386, 430, 479, 534, 595, 663, 739, 824, 918, 1023
//};

int gamma_table[PWM_VALUE+1] = {
    0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 8, 9, 10,
    11, 12, 13, 15, 17, 19, 21, 23, 26, 29, 32, 36, 40, 44, 49, 55,
    61, 68, 76, 85, 94, 105, 117, 131, 146, 162, 181, 202, 225, 250,
    279, 311, 346, 386, 430, 479, 534, 595, 663, 739, 824, 918, 1023
};



// RGB FET
#define redPIN    13
#define greenPIN  15
#define bluePIN   12


// W FET
#define w1PIN     14
#define w2PIN     4

// onbaord green LED D1
#define LEDPIN    5
// onbaord red LED D2
#define LED2PIN   1

// note
// TX GPIO2 @Serial1 (Serial ONE)
// RX GPIO3 @Serial


#define LEDoff digitalWrite(LEDPIN,HIGH)
#define LEDon digitalWrite(LEDPIN,LOW)

#define LED2off digitalWrite(LED2PIN,HIGH)
#define LED2on digitalWrite(LED2PIN,LOW)

//const char *ssid =  "*************";  //  your network SSID (name)
//const char *pass =  "********";       // your network password

const char *ssid =  "HomeWiFi";    // cannot be longer than 32 characters!
const char *pass =  "navi3com";    //

int currRed, currGreen, currBlue;
double currH360, currSPer, currLPer;

// Update these with values suitable for your network.
IPAddress server(192, 168, 1, 7);
// RGBConverter converter;

void readEEPROM(int startAdr, int maxLength, char* dest) {
  EEPROM.begin(512);
  delay(10);
  for (int i = 0; i < maxLength; i++){
    dest[i] = char(EEPROM.read(startAdr + i));
  }
  EEPROM.end();
  Serial1.print("ready reading EEPROM: ");
  Serial1.println(dest);
}

void writeEEPROM(int startAdr, int length, char* writeString){
  EEPROM.begin(512);
  yield();
  Serial1.println();
  Serial1.print("writing EEPROM: ");
  for (int i = 0; i < length; i++){
    EEPROM.write(startAdr + i, writeString[i]);
    Serial1.print(writeString[i]);
  }
  EEPROM.commit();
  EEPROM.end();
}

void colorFader(double reqH, double reqS, double reqL, double currH360, double currSPer, double currLPer, int numSteps)
{
  double reqH360, reqSper, reqLper;

  Serial1.println("In colorfader");

  RGBConverter::hslIntervalZeroOneToDegAndPercentage(reqH, reqS, reqL, &reqH360, &reqSper, &reqLper);

  char reqH_s[9];
  char currH360_s[9];
  dtostrf(reqH, 8,4, reqH_s);
  dtostrf(currH360, 8,4, currH360_s);

  Serial1.print("reqH => ");
  Serial1.println(reqH_s);
  Serial1.print("currH => ");
  Serial1.println(currH360_s);


  double hueStep = (reqH360 - currH360) / (double)numSteps;
  double satStep = (reqSper - currSPer) / numSteps;
  double levelStep = (reqLper - currLPer) / numSteps;

  unsigned int tRed, tGreen, tBlue;
  double tH, tS, tL;
  double tH360, tSPer, tLPer;
  double _r, _g, _b;

  char hs[9], ss[9], ls[9];
  char rs[9], gs[9], bs[9];
  char huestep_s[9];
  char satstep_s[9];
  char levstep_s[9];

  dtostrf(hueStep, 8,4, huestep_s);
  dtostrf(satStep, 8,4, satstep_s);
  dtostrf(levelStep, 8,4, levstep_s);

  Serial1.print("huestep => ");
  Serial1.println(huestep_s);
  Serial1.print("satstep => ");
  Serial1.println(satstep_s);
  Serial1.print("levelstep => ");
  Serial1.println(levstep_s);

  dtostrf(reqH360, 8, 1, hs);
  dtostrf(reqSper, 8, 4, ss);
  dtostrf(reqLper, 8, 4, ls);

  writeEEPROM(0, 9, hs);
  writeEEPROM(9, 9, ss);
  writeEEPROM(18, 9, ls);


  for(int i=1; i<=numSteps; i++)
  {
    tH360 = currH360 + (hueStep * (double)i);
    tSPer = currSPer + (satStep * (double)i);
    tLPer = currLPer + (levelStep * (double)i);

    RGBConverter::hslDegAndPercentageToIntervalZeroOne(tH360, tSPer, tLPer, &tH, &tS, &tL);
    RGBConverter::hslToRgb(tH, tS, tL, &_r, &_g, &_b);
    RGBConverter::rgbDoubleToESPInt(_r, _g, _b, &tRed, &tGreen, &tBlue);

    dtostrf(tH360, 8, 1, hs);
    dtostrf(tSPer, 8, 4, ss);
    dtostrf(tLPer, 8, 4, ls);
    dtostrf(tRed, 8, 1, rs);
    dtostrf(tGreen, 8, 4, gs);
    dtostrf(tBlue, 8, 4, bs);


    Serial1.print("(h, s, v) => step ");
    Serial1.print(i);
    Serial1.print(" (");
    Serial1.print(hs);
    Serial1.print(", ");
    Serial1.print(ss);
    Serial1.print(", ");
    Serial1.print(ls);
    Serial1.print("); (r, g, b) => (");
    Serial1.print(rs);
    Serial1.print(", ");
    Serial1.print(gs);
    Serial1.print(", ");
    Serial1.print(bs);

    Serial1.println(")");

    delay(10);
    analogWrite(redPIN, tRed);
    analogWrite(greenPIN, tGreen);
    analogWrite(bluePIN, tBlue);

  }
  currH360 = tH360;
  currSPer = tSPer;
  currLPer = tLPer;
}


void callback(const MQTT::Publish& pub) {
  Serial1.print(pub.topic());
  Serial1.print(" => ");
  Serial1.println(pub.payload_string());

  String payload = pub.payload_string();

  if(String(pub.topic()) == "/openHAB/RGB_2/Color"){
    char h360_s[9];
    char sper_s[9];
    char lper_s[9];


    readEEPROM(0, 9, h360_s);
    readEEPROM(9, 9, sper_s);
    readEEPROM(18, 9, lper_s);

    currH360 = atof(h360_s);
    currSPer = atof(sper_s);
    currLPer = atof(lper_s);

    int c1 = payload.indexOf(';');
    int c2 = payload.indexOf(';',c1+1);

    // Serial1.print("curr_h => ");
    // Serial1.println(h360_s);
    // Serial1.print("curr_s => ");
    // Serial1.println(sper_s);
    // Serial1.print("curr_l => ");
    // Serial1.println(lper_s);
    //
    //
    //
    unsigned int red = map(payload.toInt(),0,255,0,PWM_VALUE);
    red = constrain(red,0,PWM_VALUE);
    unsigned int green = map(payload.substring(c1+1,c2).toInt(),0,255,0,PWM_VALUE);
    green = constrain(green, 0, PWM_VALUE);
    unsigned int blue = map(payload.substring(c2+1).toInt(),0,255,0,PWM_VALUE);
    blue = constrain(blue,0,PWM_VALUE);

    // red = gamma_table[red];
    // green = gamma_table[green];
    // blue = gamma_table[blue];
    double _r, _g, _b;
    double h, s, l;

    h = 0.0;
    s = 0.0;
    l = 0.0;

    RGBConverter::rgbIntToDouble(red, green, blue, &_r, &_g, &_b);
    RGBConverter::rgbToHsl(_r, _g, _b, &h, &s, &l);
    // RGBConverter::hslIntervalZeroOneToDegAndPercentage(h, s, l, &currH360, &currSPer, &currLPer);

    colorFader(h, s, l, currH360, currSPer, currLPer, 32);

    //
    // char rs[9];
    // char gs[9];
    // char bs[9];
    //
    // dtostrf(_r, 8, 4, rs);
    // dtostrf(_g, 8, 4, gs);
    // dtostrf(_b, 8, 4, bs);
    //
    // Serial1.print("_r => ");
    // Serial1.println(rs);
    // Serial1.print("_g => ");
    // Serial1.println(gs);
    // Serial1.print("_b => ");
    // Serial1.println(bs);

    // RGBConverter::hslToRgb(h, s, l, &_r, &_g, &_b);
    //
    // unsigned int scaled_r, scaled_g, scaled_b;
    //
    // RGBConverter::rgbDoubleToESPInt(_r, _g, _b, &scaled_r, &scaled_g, &scaled_b);
    //
    // analogWrite(redPIN, scaled_r);
    // analogWrite(greenPIN, scaled_g);
    // analogWrite(bluePIN, scaled_b);
    //
    // currRed = red;
    // currGreen = green;
    // currBlue = blue;

    //   Serial1.println(red);
    //   Serial1.println(green);
    //   Serial1.println(blue);
  }
  else if(String(pub.topic()) == "/openHAB/RGB_2/SW1"){
    int value = map(payload.toInt(),0,100,0,PWM_VALUE);
    value = constrain(value,0,PWM_VALUE);
    value = gamma_table[value];
    analogWrite(w1PIN, value);
    //Serial1.println(value);
  }
  else if(String(pub.topic()) == "/openHAB/RGB_2/SW2"){
    int value = map(payload.toInt(),0,100,0,PWM_VALUE);
    value = constrain(value,0,PWM_VALUE);
    value = gamma_table[value];
    analogWrite(w2PIN, value);
    Serial1.println(value);
  }
}

// PubSubClient client(server);
WiFiClient wclient;
PubSubClient client(wclient, server);


void setup()
{
  pinMode(LEDPIN, OUTPUT);
  pinMode(LED2PIN, OUTPUT);

  pinMode(redPIN, OUTPUT);
  pinMode(greenPIN, OUTPUT);
  pinMode(bluePIN, OUTPUT);
  pinMode(w1PIN, OUTPUT);
  pinMode(w2PIN, OUTPUT);

  // Setup console
  Serial1.begin(115200);
  delay(10);
  Serial1.println();
  Serial1.println();

  client.set_callback(callback);

  WiFi.begin(ssid, pass);

  LEDon;

  while (WiFi.status() != WL_CONNECTED) {

    LED2off;
    delay(500);
    Serial1.print(".");
    LED2on;
  }
  char currH360_s[9], currSPer_s[9], currLPer_s[9];

  readEEPROM(0, 9, currH360_s);
  readEEPROM(9, 9, currSPer_s);
  readEEPROM(18, 9, currLPer_s);

  currH360 = atof(currH360_s);
  currSPer = atof(currSPer_s);
  currLPer = atof(currLPer_s);

  double h, s, l;
  RGBConverter::hslDegAndPercentageToIntervalZeroOne(currH360, currSPer, currLPer, &h, &s, &l);

  colorFader(h, s, l, 0, 0, 0, 32);

  Serial1.println("");

  Serial1.println("WiFi connected");
  Serial1.println("IP address: ");
  Serial1.println(WiFi.localIP());


  if (client.connect("WiFi_RGB_2")) {
    client.subscribe("/openHAB/RGB_2/Color");
    client.subscribe("/openHAB/RGB_2/SW1");
    client.subscribe("/openHAB/RGB_2/SW2");
    Serial1.println("MQTT connected");
  }

  Serial1.println("");
}


void loop()
{
  client.loop();
}
