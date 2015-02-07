#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 10
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;
int resolution = 12;
int delayInMillis = 0;
unsigned long lastTempRequest = 0;

// If using software SPI (the default case):
#define OLED_MOSI  2
#define OLED_CLK   3
#define OLED_DC    4
#define OLED_CS    5
#define OLED_RESET 6
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define REGLERMAX 475
#define REGLERSTEP 5 // 0-95 C°

int analogPin = 0;
double a0_val = 0;

int sollTemp = 0;
int istTemp = 0;

#define RELAISE 12

void setup()   {                
  Serial.begin(9600);
  
  pinMode(RELAISE, OUTPUT);
  
  sensors.begin();
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, resolution);
  sensors.setWaitForConversion(false);
  delayInMillis = 750 / (1 << (12 - resolution));
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done

  // Clear the buffer.
  display.clearDisplay();
}

boolean heating = false;
void setHeating(){
  heating = true;
  
  showTemp();  
}
void unsetHeating(){
  heating = false;
  
  showTemp();
}

void showTemp(){
  display.clearDisplay();
   // text display tests
  display.setTextSize(2);
  if( heating ){ 
    display.setTextColor(BLACK, WHITE); // 'inverted' text
  }else{
    display.setTextColor(WHITE);
  }
  display.setCursor(0,0);
  display.print("Temp: ");
  display.print(istTemp);
  display.println("C");
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.print(sollTemp);
  display.println("C");
  display.display();
}

int reglerCount = 0;
#define MAXREGELERCOUNT 200

void readRegler(){
  if( reglerCount == MAXREGELERCOUNT ){
    reglerCount = 0;
    int res = a0_val / MAXREGELERCOUNT;
    a0_val = 0;
    if( res > REGLERMAX ){ res = REGLERMAX; }
    int temp = res / REGLERSTEP;
    if( temp != sollTemp ){
      sollTemp = temp;
      Serial.println(sollTemp);
      showTemp();
    }
  }else{
    reglerCount++;
    int val = analogRead(analogPin);
    val = val & 0xFFFC;
    a0_val += val;
  }
}

unsigned long lastSec = 0;
int secCount = 0;
void loop() {
  readRegler();
  if (millis() - lastTempRequest >= delayInMillis) // waited long enough??
  {
    lastTempRequest = millis();
    sensors.requestTemperatures();
    istTemp = sensors.getTempCByIndex(0);
    Serial.println(istTemp);
    if( istTemp < sollTemp ){
      setHeating();
    }else{
      unsetHeating();
    }
  }
  if( millis() - lastSec >= 1000 ){
     lastSec = millis();
     secCount++;
  }
  
  if( secCount < 3 ){
    digitalWrite(RELAISE, 0);
  }else if( secCount == 3 && heating ){
    digitalWrite(RELAISE, 1);
  }else if(secCount == 4){
    digitalWrite(RELAISE, 0);
    secCount = 0;
  }
}


