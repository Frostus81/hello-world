#include "Nextion.h"
#include "RTClib.h"
#include "DHT.h"
#include "Buttons.h"
//#include "Lichtsteuerung.h"
#include <Adafruit_NeoPixel.h>

#define N_Pixels 6 //NeoPixel-Ring hat 24 LED
#define LED_PIN 6 //NeoPixel ist an diesen Pin angeschlossen
#define DHTPIN 13     // Temp- und Luftfeuchtigkeitssensor ist an diesen Pin angeschlossen
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_Pixels, LED_PIN, NEO_GRB + NEO_KHZ800);

/*
 * Globale Variabeln
 */
float temperature = 0.0f;
float humidity = 0.0f;

int saufg = 0, suntg = 0, mond = 0, tagl = 0, over = 0;
float counter1, counter2, counter3 = 255, counter4 = 80;

unsigned long interval=5000; // the time we need to wait
unsigned long previousMillis=0; // millis() returns an unsigned long.
unsigned long previousMillis2=0; // millis() returns an unsigned long.

boolean sw = true, sw1 = true;

DHT dht(DHTPIN, DHTTYPE);
RTC_DS1307 rtc;

NexButton bSoauf = NexButton(0, 2, "bSoauf");
NexButton bSontg = NexButton(0, 11, "bSontg");
NexButton bMond = NexButton(0, 12, "bMond");
NexButton bOff = NexButton(0, 3, "bOff");
NexButton bRst = NexButton(0, 13, "bRst");
NexText tTempC = NexText(0, 7, "tTempC");
NexText tState = NexText(0, 1, "tState");
NexProgressBar jHumidity = NexProgressBar(0, 6, "jHumidity");
NexText tHumidity = NexText(0, 8, "tHumidity");

NexTouch *nex_listen_list[] = {
  &bSoauf,
  &bSontg,
  &bOff,
  &bMond,
  &bRst,
  NULL
};

/*
 * Konfiguration RTC
 */
char daysOfTheWeek[7][12] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};

//Lichtstatus S1(0,0,0,0,0);

void setup () {
  dht.begin();

  strip.begin();
  strip.setBrightness(125);//Helligkeit einstellen
  strip.show();
  
  while (!Serial); // for Leonardo/Micro/Zero

  Serial.begin(9600);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2020, 4, 1, 7, 35, 10));
  }
  nexInit();
  bSoauf.attachPop(bSoaufPopCallback, &bSoauf);
  bSontg.attachPop(bSontgPopCallback, &bSontg);
  bOff.attachPop(bOffPopCallback, &bOff);
  bMond.attachPop(bMondPopCallback, &bMond);
  bRst.attachPop(bRstPopCallback, &bRst);
}

/*
 * Button für manuellen Sonnenaufgang
 */
void bSoaufPopCallback(void *ptr) 
{
  saufg = 1;
  suntg = 0; 
  mond = 0;
  over = 1;
  tagl = 0;
}

/*
 * Button für manuellen Sonnenuntergang
 */
void bSontgPopCallback(void *ptr) 
{
  saufg = 0;
  suntg = 1; 
  mond = 0;
  over = 1;
  tagl = 0;
}

/*
 * Button für manuellen Mondschein
 */
void bMondPopCallback(void *ptr) 
{
  saufg = 0;
  suntg = 0;
  mond = 1;
  over = 1;
  tagl = 0;
  Mondlicht();
}

/*
 * Button für manuell Licht aus
 */
void bOffPopCallback(void *ptr) 
{
  saufg = 0;
  suntg = 0; 
  mond = 0;
  tagl = 0;
  over = 1;
  Lichtaus();
}

/*
 * Button für Ende manuellen Sonnenuntergang
 */
void bRstPopCallback(void *ptr) 
{
  Taglicht();
  saufg = 0;
  suntg = 0; 
  mond = 0;
  over = 0;
  tagl = 1;
  counter1 = 0;
  counter2 = 0;
  counter3 = 255;
  counter4 = 80;
}

/*
 * Zeit an Nextion schicken
 */
void ZeitanNextion(){
  DateTime now = rtc.now();
  char buf1[] = "hh:mm";
  String zeit = "tZeit.txt=\""+String(now.toString(buf1))+"\"";
  Serial.print(zeit);
  endNextionCommand();
  String zeit2 = "tTime.txt=\""+String(now.toString(buf1))+"\"";
  Serial.print(zeit2);
  endNextionCommand();
}

/*
 * Sensordaten lesen und an Display senden
 */
void readSensor()
{
 humidity = dht.readHumidity();
 temperature = dht.readTemperature();
}

void Feuchtigkeit()
{
  String feuchtigkeit = "tHumidity.txt=\""+String(humidity,1)+" %\"";
  Serial.print(feuchtigkeit);
  endNextionCommand();
}

void Temperatur()
{
  String temperatur = "tTempC.txt=\""+String(temperature,1)+" C\"";
  Serial.print(temperatur);
  endNextionCommand();
}

/*
 * Lichtsteuerung
 */
 void Lichtsteuerung()
{
  DateTime now = rtc.now();

  if ((now.hour() == 6) && (now.minute() == 30) && (now.second() <= 10)){
    saufg = 1;
    }

  else if((now.hour() == 7) && (now.minute() == 0) && (over == 0)){
    saufg = 0;
    tagl = 1;
    Taglicht();
  }

  else if((now.hour() == 21) && (now.minute() == 29) && (now.second() <= 10)){
    tagl = 0;
    suntg = 1;
    }
  
  else if ((now.hour() == 22) && (now.minute() == 00) && (now.second() <= 10)){
    suntg = 0;
    mond = 1;
    Mondlicht();
    }
  
  else if ((now.hour() == 23) && (now.minute() == 59) && (now.second() <= 10)){
    mond = 0;
    Lichtaus();
    }
}

void Lichtstatus()
{
 if(tagl==1){
    String taglicht = "tState.txt=\""+String("Taglicht")+"\"";
    Serial.print(taglicht);
    endNextionCommand();
    }
 if(mond==1){
    String mond1 = "tState.txt=\""+String("Mondlicht")+"\"";
    Serial.print(mond1);
    endNextionCommand();
    }
 if(suntg==1){
    String suntg = "tState.txt=\""+String("Sonnenuntergang")+"\"";
    Serial.print(suntg);
    endNextionCommand();
    }
 if(saufg==1){
    String saufg = "tState.txt=\""+String("Sonnenaufgang")+"\"";
    Serial.print(saufg);
    endNextionCommand();
    }
 if(saufg==0 && suntg==0 && mond==0 && tagl==0){
    String aus = "tState.txt=\""+String("Licht aus")+"\"";
    Serial.print(aus);
    endNextionCommand();
    }
  }



// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
  }
}

  void Lichtaus() //Licht manuell ausschalten
{
    colorWipe (strip.Color(0, 0, 0),0);
    strip.show();
}

  void Mondlicht() //Farbe des Mondlichtes
{
    colorWipe (strip.Color(0, 0, 25),0);
    strip.show();
}

  void Taglicht() //Farbe des Taglichtes
{
    colorWipe (strip.Color(200, 150, 0),0);
    strip.show();
}

void Sonnenstand()
{
  /*
 * Fade Sonnenaufgang
 */
  unsigned long currentMillis = millis(); // grab current time

  if (saufg == 1){// check if "interval" time has passed (10 milliseconds)
  if ((unsigned long)(currentMillis - previousMillis) >= interval) {

if(sw)
    {
      counter1+=0.71;
      counter2+=0.22;
      if(counter2>=79.2){
        saufg = 0;
        counter1 = 0;
        counter2 = 0;
      }
    }
    // fade led mit counter
    colorWipe (strip.Color(counter1, counter2, 0), 0);
    strip.show();
    previousMillis = millis();
   }
  }
    /*
   * Fade Sonnenuntergang
   */
  unsigned long currentMillis2 = millis(); // grab current time

  if (suntg == 1){// check if "interval" time has passed (10 milliseconds)
  if ((unsigned long)(currentMillis2 - previousMillis2) >= interval) {

if(sw1)
    {
      counter3-=0.71;
      counter4-=0.22;
      if(counter4<=1){
          suntg = 0;
          counter3 = 255;
          counter4 = 80;
                    }
    }
    // fade led mit counter
    colorWipe (strip.Color(counter3, counter4, 0), 0);
    strip.show();
    previousMillis2 = millis();
   }
  }
}

void endNextionCommand() //Nötig um einen Befehl an das Nextion TFT zu übergeben
{
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

void Debug()
{
  Serial.print("Sonnenaufgang: ");
  Serial.print(saufg);
  Serial.print(" / Sonnenuntergang: ");
  Serial.print(suntg);
  Serial.print(" / Mondlicht: ");
  Serial.print(mond);
  Serial.print(" / Taglicht: ");
  Serial.print(tagl);
  Serial.print(" / Override: ");
  Serial.print(over);
  Serial.println();
  Serial.print("Lichtstatus: ");
  Serial.print(counter1);
  Serial.println();
  }

void loop () {
  nexLoop(nex_listen_list);
  readSensor();
  Feuchtigkeit();
  Temperatur();
  ZeitanNextion();
  Lichtsteuerung();
  Lichtstatus();
  Sonnenstand();
  Serial.print(counter1);
  Serial.println(counter2);
//Debug();
  delay(1000);
}
