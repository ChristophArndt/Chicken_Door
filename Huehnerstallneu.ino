#include <Wire.h>
#include "ds3231.h"
#include <LiquidCrystal.h>

LiquidCrystal lcd(8,9,4,5,6,7); 
#define BUFF_MAX 128
uint8_t time[8];
char recv[BUFF_MAX];
unsigned int recv_size = 0;
unsigned long prev, interval = 1000;

int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

//Variablen Motor; Türsensor
int motorPin1=2;
int motorPin2=3; // PWM
int sensortuer = 1; 
int senlicht = A3; 
int zaehler = 0;
boolean tuerauf = true;


void setup()
{
    Serial.begin(9600);
    Wire.begin();
    DS3231_init(DS3231_INTCN);
    memset(recv, 0, BUFF_MAX);
    Serial.println("GET time");
    lcd.begin(16, 2);
    lcd.clear();

    //setup Motor/sensor
    pinMode(motorPin1,OUTPUT);
    pinMode(motorPin2,OUTPUT);
    Serial.begin(9600);
    pinMode (sensortuer, INPUT);
    pinMode (senlicht, INPUT);
    //seconds, minutes, hours, day of the week, date, month and year
    //Serial.println("Setting time");
    //parse_cmd("T105515509022018",16);
}

void motorStop(){
  digitalWrite(motorPin1,LOW);
  digitalWrite(motorPin2,LOW);
  delay(500);
}

boolean tueroeffnen(){
  if (tuerauf == true){return true;}
    else {
      while (tuerauf == false){
         digitalWrite(motorPin1,HIGH);   // Motor Vor
         digitalWrite(motorPin2,LOW);
         zaehler += 1; 
         if (analogRead (sensortuer)<= 30) {
          tuerauf = true;
          motorStop();
          Serial.println(zaehler);
          zaehler = 0;
          return true;
         }
         if (zaehler > 100){
            tuerauf = true;
            motorStop();
            zaehler = 0;
            return true;}
         delay(200);
      }
    }
  }

boolean tuerschliest(){
  if (tuerauf == false){return false;}
    else {
      while (tuerauf == true){
         digitalWrite(motorPin1,LOW);   // Motor Vor
         digitalWrite(motorPin2,HIGH);
         zaehler += 1; 
         if (analogRead (sensortuer)>= 1000) {
          tuerauf = false;
          motorStop();
          Serial.println(zaehler);
          zaehler = 0;
          return true;
         }
         if (zaehler > 100){
            tuerauf = false;
            motorStop();
            zaehler = 0;
            return true;}
         delay(200);
      }
    }
  }

int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor 
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result

 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 195)  return btnUP; 
 if (adc_key_in < 380)  return btnDOWN; 
 if (adc_key_in < 555)  return btnLEFT; 
 if (adc_key_in < 790)  return btnSELECT;   
 return btnNONE;  // when all others fail, return this...
}

void anzeigen(){
  //setup LCD und Temoeratur
    char in;
    char tempF[6]; 
    float temperature;
    char buff[BUFF_MAX];
    unsigned long now = millis();
    struct ts t;

    // show time once in a while
        DS3231_get(&t); //Get time
        parse_cmd("C",1);
        temperature = DS3231_get_treg(); //Get temperature
        dtostrf(temperature, 5, 1, tempF);
        
        lcd.clear();
        lcd.setCursor(1,0);
        
        lcd.print(t.mday);
        
        printMonth(t.mon);
        
        lcd.print(t.year);
        
        lcd.setCursor(0,1); //Go to second line of the LCD Screen
        lcd.print(t.hour);
        lcd.print(":");
        if(t.min<10)
        {
          lcd.print("0");
        }
        lcd.print(t.min);
        lcd.print(":");
        if(t.sec<10)
        {
          lcd.print("0");
        }
        lcd.print(t.sec);
        
        lcd.print(' ');
        lcd.print(tempF);
        lcd.print((char)223);
        lcd.print("C ");
        
    
}
//Bedingungen Prüfen für Tür auf (1) Tür zu (2) ERR(3)
void doorcheck (){
    struct ts t;
    DS3231_get(&t);
    int licht = analogRead(senlicht);
    //Soll die Tür geöfnet werden?(Licht/tür zu/nach 7Uhr)
    if ((licht > 22) && (tuerauf == false) && (t.hour > 7)){
      tueroeffnen();
    }
    if ((licht < 28) && (tuerauf == true) && (t.hour > 17)){
      tuerschliest();
    }
    Serial.println("Hallo");
    Serial.println(licht);
    //
    Serial.println(t.hour);
    Serial.println(analogRead(senlicht));
}

void loop()
{
    //Setup Motor; Sensortür
    lcd_key = read_LCD_buttons();  
    int licht = analogRead(senlicht);

    Serial.println(analogRead(senlicht));

    if (licht < 20){
       doorcheck();
    }

    
    if (lcd_key == 3) {
      anzeigen();
      delay(1500);
    }
    if (lcd_key == 1){
       lcd.print("tuer oeffnet");
       tueroeffnen();
       lcd.clear();
       lcd.print("tuer geoeffnet");
       delay(2000);
    }
    if (lcd_key == 2){
       lcd.print("tuer schliest");
       tuerschliest();
       lcd.clear();
       lcd.print("tuer geschlossen");
       delay(2000);
    }
    lcd.clear();
    delay(300);
}

void parse_cmd(char *cmd, int cmdsize)
{
    uint8_t i;
    uint8_t reg_val;
    char buff[BUFF_MAX];
    struct ts t;

    //snprintf(buff, BUFF_MAX, "cmd was '%s' %d\n", cmd, cmdsize);
    //Serial.print(buff);

    // TssmmhhWDDMMYYYY aka set time
    if (cmd[0] == 84 && cmdsize == 16) {
        //T355720619112011
        t.sec = inp2toi(cmd, 1);
        t.min = inp2toi(cmd, 3);
        t.hour = inp2toi(cmd, 5);
        t.wday = inp2toi(cmd, 7);
        t.mday = inp2toi(cmd, 8);
        t.mon = inp2toi(cmd, 10);
        t.year = inp2toi(cmd, 12) * 100 + inp2toi(cmd, 14);
        DS3231_set(t);
        Serial.println("OK");
    } else if (cmd[0] == 49 && cmdsize == 1) {  // "1" get alarm 1
        DS3231_get_a1(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 50 && cmdsize == 1) {  // "2" get alarm 1
        DS3231_get_a2(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 51 && cmdsize == 1) {  // "3" get aging register
        Serial.print("aging reg is ");
        Serial.println(DS3231_get_aging(), DEC);
    } else if (cmd[0] == 65 && cmdsize == 9) {  // "A" set alarm 1
        DS3231_set_creg(DS3231_INTCN | DS3231_A1IE);
        //ASSMMHHDD
        for (i = 0; i < 4; i++) {
            time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // ss, mm, hh, dd
        }
        byte flags[5] = { 0, 0, 0, 0, 0 };
        DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
        DS3231_get_a1(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 66 && cmdsize == 7) {  // "B" Set Alarm 2
        DS3231_set_creg(DS3231_INTCN | DS3231_A2IE);
        //BMMHHDD
        for (i = 0; i < 4; i++) {
            time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // mm, hh, dd
        }
        byte flags[5] = { 0, 0, 0, 0 };
        DS3231_set_a2(time[0], time[1], time[2], flags);
        DS3231_get_a2(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 67 && cmdsize == 1) {  // "C" - get temperature register
        Serial.print("temperature reg is ");
        Serial.println(DS3231_get_treg(), DEC);
    } else if (cmd[0] == 68 && cmdsize == 1) {  // "D" - reset status register alarm flags
        reg_val = DS3231_get_sreg();
        reg_val &= B11111100;
        DS3231_set_sreg(reg_val);
    } else if (cmd[0] == 70 && cmdsize == 1) {  // "F" - custom fct
        reg_val = DS3231_get_addr(0x5);
        Serial.print("orig ");
        Serial.print(reg_val,DEC);
        Serial.print("month is ");
        Serial.println(bcdtodec(reg_val & 0x1F),DEC);
    } else if (cmd[0] == 71 && cmdsize == 1) {  // "G" - set aging status register
        DS3231_set_aging(0);
    } else if (cmd[0] == 83 && cmdsize == 1) {  // "S" - get status register
        Serial.print("status reg is ");
        Serial.println(DS3231_get_sreg(), DEC);
    } else {
        Serial.print("unknown command prefix ");
        Serial.println(cmd[0]);
        Serial.println(cmd[0], DEC);
    }
}

void printMonth(int month)
{
  switch(month)
  {
    case 1: lcd.print(" January ");break;
    case 2: lcd.print(" February ");break;
    case 3: lcd.print(" March ");break;
    case 4: lcd.print(" April ");break;
    case 5: lcd.print(" May ");break;
    case 6: lcd.print(" June ");break;
    case 7: lcd.print(" July ");break;
    case 8: lcd.print(" August ");break;
    case 9: lcd.print(" September ");break;
    case 10: lcd.print(" October ");break;
    case 11: lcd.print(" November ");break;
    case 12: lcd.print(" December ");break;
    default: lcd.print(" Error ");break;
  } 
}
