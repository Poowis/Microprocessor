#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);

byte mac[] = {0x00, 0xAB, 0xCD, 0xEF, 0xAB, 0xCD};
IPAddress server(10, 30, 4, 184);
IPAddress ip(10, 30, 4, 185);

EthernetClient ethClient;
PubSubClient client(ethClient);

const int temp = A0;
const int trig = 2;
const int echo = 7;
const int pir = 4;

const int light = 9;
const int r = 3;
const int g = 6;
const int b = 5;

int Light = 0;
int brightness = 0;

void printText(int col, int row, String text)
{
  lcd.setCursor(col, row);
  if (text.charAt(0) == '0')
  {
    lcd.print(0);
  }
  lcd.print(text);
}

void ds1621()
{
  Wire.begin();
  Wire.beginTransmission(0x48); // connect to DS1621 (#0)
  Wire.write(0xAC);             // Access Config
  Wire.write(0x02);             // set for continuous conversion
  Wire.beginTransmission(0x48); // restart
  Wire.write(0xEE);             // start conversions
  Wire.endTransmission();
}

float getDSTemp()
{
  int8_t firstByte;
  int8_t secondByte;
  float temp = 0;
  delayMicroseconds(10); // give time for measurement
  Wire.beginTransmission(0x48);
  Wire.write(0xAA); // read temperature command
  Wire.endTransmission();
  Wire.requestFrom(0x48, 2); // request two bytes from DS1621 (0.5 deg. resolution)
  firstByte = Wire.read();   // get first byte
  secondByte = Wire.read();  // get second byte
  temp = firstByte;
  if (secondByte) // if there is a 0.5 deg difference
    temp += 0.5;

  return temp;
}

float getTemp()
{
  int temp = 750 / 7 * (analogRead(temp) / 1023.0 * 5 - 1) + 46;
  return temp;
}

void ultraSonic()
{
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
}

float getLength()
{
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  return pulseIn(echo, HIGH) / 29 / 2 / 100.0;
}

int getPIR()
{
  if (digitalRead(pir) == HIGH)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void rgb()
{
  pinMode(r, OUTPUT);
  pinMode(g, OUTPUT);
  pinMode(b, OUTPUT);
}

void setRGB(int red, int green, int blue)
{
  analogWrite(r, 255 - red);
  analogWrite(g, 255 - green);
  analogWrite(b, 255 - blue);
}

void setLight(char *topic, byte *payload, unsigned int length)
{
  if (strcmp(topic, "light") == 0) {
    brightness = atoi((char *)payload);
  } else  if (strcmp(topic, "Light") == 0){
    Light = atoi((char *)payload);
  } 
  if (Light == 0)
   {
    setRGB(0, 0, 0);
      
   } else {
      setRGB(brightness, brightness, brightness);
   }
   Serial.print(Light);
   Serial.print("   ");
   Serial.println(brightness);
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient"))
    {
      Serial.println("connected");
      client.subscribe("light");
      client.subscribe("Light");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool publish(char topic[], String text)
{
  return client.publish(topic, (char *)text.c_str());
}

void setup()
{
  Serial.begin(9600);

  client.setServer(server, 1883);
  client.setCallback(setLight);

  Ethernet.begin(mac);

  delay(1500);

  lcd.init();
  lcd.backlight();

  ds1621();
  ultraSonic();
  rgb();
  pinMode(pir, INPUT);
  pinMode(light, OUTPUT);

  printText(0, 0, "PIR");
  printText(8, 0, "ULT");

  printText(0, 1, "TMP");
  printText(8, 1, "DST");
}

void loop()
{
  String length = String(getLength());
  String temp = String(getTemp());
  String dsTemp = String(getDSTemp());
  String pirr = String(getPIR());

  printText(3, 0, pirr);
  printText(11, 0, length);
  printText(3, 1, temp);
  printText(11, 1, dsTemp);

  publish("length", length);
  publish("temp", temp);
  publish("dstemp", dsTemp);
  publish("pir", pirr);

  if (!client.connected())
  {
    reconnect();
  }
  
  client.loop();

  delay(100);
}
