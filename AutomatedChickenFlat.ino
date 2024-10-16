#include "Wire.h"                    // I2C-Library 
#include "DS3231.h"                  // Library V1.0.1 from Rinky-DinkElectronics (http://www.rinkydinkelectronics.com)


DS3231 RTC(SDA, SCL);                // Init for the DS3231-module over the I2C-interface
Time CLOCKTIME;                      // Init time structure


// == BEGIN: Declaration of the variables and constants ==========================

// Threshold LDR
int LDR_TH_MORNING = 770;
int LDR_TH_EVENING = 50;

// Threshold time
int TIME_UP_HRS = 7;
int TIME_UP_MIN = 0;
int TIME_DOWN_HRS = 22;
int TIME_DOWN_MIN = 0;

// Pin allocation
const byte BUTTON_UP = 2;
const byte BUTTON_DOWN = 3;
const byte REED_UP = 4;
const byte REED_DOWN = 5;
const byte ENGINE_1 = 6;
const byte ENGINE_2 = 7;
const int LDR = A0;

// flags
bool REED_STATE_UP = 0;
bool REED_STATE_DOWN = 0;
bool RTC_FLAG = 0;
volatile bool ISR_FLAG_UP = 0;       // keypress up
volatile bool ISR_FLAG_DOWN = 0;     // keypress down

int LDR_VALUE = 1023;

// == END: Declaration of the variables and constants ============================


void setup()
{
  RTC.begin();                                     // Init from RTC-object
  delay(1000);
  //RTC.setTime(15,10,0);                            // Set one initiale time, as long as RTC has its own battery, the time continues automatically

  // init serial monitor
  Serial.begin(9600);

  // define arduino ports
  pinMode(ENGINE_1, OUTPUT);
  pinMode(ENGINE_2, OUTPUT);

  pinMode(REED_UP, INPUT_PULLUP);
  pinMode(REED_DOWN, INPUT_PULLUP);

  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);

  attachInterrupt(0, INTERRUPT_BUTTON_UP, LOW);
  attachInterrupt(1, INTERRUPT_BUTTON_DOWN, LOW);

  while(!RTC_FLAG)                                 // check until RTC-module is reachable
  {
    Serial.println("Search RTC-Module: ....");
    RTC.getTime();                                 // Reading time from DS3231
    Serial.println("RTC-Module found!");
    RTC_FLAG = 1;
    delay(1000);
  }  
}


void loop()
{
  // -- BEGIN: Initial read -------------------------------------------------------------------

  REED_STATE_UP = digitalRead(REED_UP);            // Read the Reed-Contact-Status for UP (1=open or 0=closed)
  REED_STATE_DOWN = digitalRead(REED_DOWN);        // Read the Reed-Contact-Status for DOWN (1=open or 0=closed)

  LDR_VALUE = analogRead(LDR);
  CLOCKTIME = RTC.getTime();
  
  // -- END: Initial read ---------------------------------------------------------------------


  PRINT_ALL_SERIAL_MONITOR();


  // -- BEGIN: ISR - keypress button  ---------------------------------------------------------

  if (ISR_FLAG_UP == 1)                            // Condition at keypress UP => the set ISR_FLAG_UP
  {
    ISR_FLAG_UP = 0;
    Serial.println("[ISR] - BUTTON_UP is pressed");
    
    while(REED_STATE_UP)                           // runs until reed-contact-up closes
    {
      ENGINE_UP();
      Serial.println("[ISR] - Hatch opens.");
      REED_STATE_UP = digitalRead(REED_UP);
    }
    
    ENGINE_STOP();
  }
  else if (ISR_FLAG_DOWN == 1)                     // Condition at keypress DOWN => set ISR_FLAG_DOWN
  {
    ISR_FLAG_DOWN = 0;
    Serial.println("[ISR] - BUTTON_DOWN is pressed");
    
    while(REED_STATE_DOWN)                         // runs until reed-contact-down closes
    {
      ENGINE_DOWN();
      Serial.println("[ISR] - Hatch closing.");
      REED_STATE_DOWN = digitalRead(REED_DOWN);
    }

    ENGINE_STOP();
  }

  // -- END: ISR - keypress button  -----------------------------------------------------------


  // -- BEGIN: condition open/close depends on LDR or RTC  ------------------------------------

  if((LDR_VALUE >= LDR_TH_MORNING)||(CLOCKTIME.hour == TIME_UP_HRS && CLOCKTIME.min == TIME_UP_MIN))
  {
    FLASH_BUILTIN_LED_ONCE();
    while(REED_STATE_UP)
    {
      ENGINE_UP();
      REED_STATE_UP = digitalRead(REED_UP);          // Reach reed-up-sensor --> value 0 --> while-loop done
      digitalWrite(LED_BUILTIN, HIGH);
    }
    ENGINE_STOP();
    digitalWrite(LED_BUILTIN, LOW);
  }

  if((LDR_VALUE <= LDR_TH_EVENING)||(CLOCKTIME.hour == TIME_DOWN_HRS && CLOCKTIME.min == TIME_DOWN_MIN))
  {
    FLASH_BUILTIN_LED_ONCE();
    while(REED_STATE_DOWN)
    {
      ENGINE_DOWN();
      REED_STATE_DOWN = digitalRead(REED_DOWN);        // Reach reed-down-sensor --> value 0 --> while-loop done
      digitalWrite(LED_BUILTIN, HIGH);
    }
    ENGINE_STOP();
    digitalWrite(LED_BUILTIN, LOW);
  }

  // -- END: condition open/close depends on LDR or RTC  --------------------------------------

  delay(1000);
}


// == BEGIN: Function definitions ================================================

void FLASH_BUILTIN_LED_ONCE()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);  
}

void INTERRUPT_BUTTON_UP()
{ 
  Serial.println("ISR button up called.");
  ISR_FLAG_UP = 1; 
}

void INTERRUPT_BUTTON_DOWN()
{  
  Serial.println("ISR button down called.");
  ISR_FLAG_DOWN = 1;
}

void ENGINE_UP()
{
  Serial.println("Hatch opening.");
  digitalWrite(ENGINE_1, HIGH);
  digitalWrite(ENGINE_2, LOW);
}

void ENGINE_DOWN()
{
  Serial.println("Hatch closing.");
  digitalWrite(ENGINE_1, LOW);
  digitalWrite(ENGINE_2, HIGH);
}

void ENGINE_STOP()
{
  Serial.println("Engine in stop mode.");
  digitalWrite(ENGINE_1, LOW);
  digitalWrite(ENGINE_2, LOW);
}

void PRINT_ALL_SERIAL_MONITOR()
{
  Serial.println("---------------");
  Serial.print("LDR: ");
  Serial.println(LDR_VALUE);

  Serial.print("ReedUp: ");
  Serial.print(REED_STATE_UP);
  Serial.print(", ReedDown: ");
  Serial.println(REED_STATE_DOWN);

  Serial.print("Time: ");
  Serial.print(CLOCKTIME.hour);
  Serial.print(":");
  Serial.println(CLOCKTIME.min);

  Serial.print("ISR_FLAG_UP = ");
  Serial.print(ISR_FLAG_UP);
  Serial.print(", ISR_FLAG_DOWN = ");
  Serial.println(ISR_FLAG_DOWN);

  Serial.println("---------------");
}

// == END: Function definitions ==================================================
