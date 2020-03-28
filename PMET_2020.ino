

// code to run RTC, bmp289, and SD card.  If sd card does not ititialize pull it out and try a different one. 



// load libraries needed for communications SD card, DS3231 RTC, Soil Moisture Sensors, and T/RH sensor
#include <Wire.h>
#include <SPI.h>
#include <SD.h> // library for SD Card
#include <LowPower.h>  // lower power library to put arduino asleep during measurement intervals
#include <I2CSoilMoistureSensor.h>  // soil moisture sensor library
#include <RTClibExtended.h> //library for Real Time Clock (RTC) 
#include <avr/sleep.h>


//Define pins and addresses on the board
#define wakePin 2    //use interrupt 0 (pin 2) and run function wakeUp when pin 2 gets LOW
#define ledPin 13    //use arduino on-board led for indicating sleep or wakeup status
#define DS3231_I2C_ADDRESS 0x68  // don't change - tells arduino that we are using I2C communications with the RTC
#define rtc_power 3

RTC_DS3231 rtc;

File myFile; // don't chagne gives name for the SD card file 


//define variables
byte AlarmFlag = 1;
byte ledStatus = 1;
float loop_repeat = 2;
float loop_count = 1; 

int voltPin = A0;
int Volt;

unsigned int sleepCounter;


//_________________________________________________________________________________________________________________________________________________
//Define datalogger name
//_________________________________________________________________________________________________________________________________________________

String dataloggerID = "DL_2"; // this is the name for the datalogger - change it to something unique for each datalogger



//_________________________________________________________________________________________________________________________________________________
// Create custom functions
//_________________________________________________________________________________________________________________________________________________


void wakeUp()        // here the interrupt is handled after wakeup
{
  Serial.println("Interrupt Fired");
  sleep_disable();
  detachInterrupt(0);
}


//function that will put the atemga328 to sleep and turn off brown-out and ADC
void going_to_sleep(){
  
  Serial.println("Going to sleep for awhile...");
    
  delay(100);

  noInterrupts ();           // timed sequence follows
  sleep_enable();
  attachInterrupt(0, wakeUp, LOW);
  digitalWrite(ledPin, LOW);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  Serial.println("just woke up!");
  
}



//_________________________________________________________________________________________________________________________________________________
// SETUP LOOP 
//      - initializes SD card, moisture sensor and print warning to serial port (and blink fxn) if not working properly
//_________________________________________________________________________________________________________________________________________________

// SETUP LOOP (same for RTC, sd card, and moisture sensors)
void setup() {

  //Set pin D2 as INPUT for accepting the interrupt signal from DS3231
  pinMode(wakePin, INPUT);

  //set pin modes
  pinMode(ledPin, OUTPUT);
  pinMode(rtc_power, OUTPUT);
  pinMode(voltPin,INPUT);

  //set pin status
  digitalWrite(ledPin, HIGH);
  digitalWrite(rtc_power, HIGH);
  
  Wire.begin();
  Serial.begin(9600);
  
  // initialize sd card        
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
   pinMode(10, OUTPUT);
   
  if (!SD.begin(4)) {
    Serial.println("   initialization failed!");
    return;
  }

    // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
    Serial.println("initializing the RTC alarms...");
    rtc.armAlarm(1, false);
    rtc.clearAlarm(1);
    rtc.alarmInterrupt(1, false);
    rtc.armAlarm(2, false);
    rtc.clearAlarm(2);
    rtc.alarmInterrupt(2, false);

    //set SQW pin on DS3231 to low
    rtc.writeSqwPinMode(DS3231_OFF);

    //Set alarm1 every day at 18:33
    rtc.setAlarm(ALM1_MATCH_SECONDS, 00, 00, 0);   //set your wake-up time here
    rtc.alarmInterrupt(1, true);
  
    Serial.println("initialization done.");
  
    Serial.println("date, time, logger_ID");  // send column headings to monitor  
  
    myFile = SD.open("PMET.csv", FILE_WRITE); 
    myFile.println("date, time, logger_ID");  // save column headings to sd card  
    myFile.close();

    delay(500);
    digitalWrite(ledPin, LOW);
  
}

// END SETUP****




//___________________________________________________________________________________________________________________________________
// create a loop that will cycle through at specified time intercal to extract data from sensors, send to serial and write to SD card.  You won't need to change this
//___________________________________________________________________________________________________________________________________

void loop() {
  
  delay(100);
     
  //On first loop we enter the sleep mode
  if (AlarmFlag == 0) {
    //put MCU to sleep
    going_to_sleep();                       

    //When exiting the sleep mode - clear the alarm
    //digitalWrite(rtc_power, HIGH);
    delay(500);
    rtc.armAlarm(1, false);
    rtc.clearAlarm(1);
    rtc.alarmInterrupt(1, false);
    AlarmFlag++;
  }

  //cycles the led to indicate that we are no more in sleep mode
  if (ledStatus == 0) {
    ledStatus = 1;
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
  }
  else {
    ledStatus = 0;
    digitalWrite(ledPin, LOW);
  }

  delay(100);
  
  // the RAM on the arduino is very small... so cannot let strings get longer than ~40 characters... or else bad things happen
  // therefore, break all string variables up into lengths not exceeding 40 chars
  String fullstring1 = "";  // create string "fullString" that contains all clock and datalogger data
  String fullstring2 = "";  // create string "fullString" that contains all sensor data

  
/// recording data to SD card
  myFile = SD.open("PMET.csv", FILE_WRITE);  // if you change this file name don't make it too long or the sd card writter will stop working

  if(myFile) {

    // retrieve data from DS3231
    DateTime now = rtc.now();
    
    // call data from real-time-clock (RTC)  
    fullstring1 += now.month(); fullstring1 += "/";
    fullstring1 += now.day(); fullstring1 += "/";
    fullstring1 += now.year(); fullstring1 += ",";
    fullstring1 += now.hour(); fullstring1 += ":";
    fullstring1 += now.minute(); fullstring1 += ":";
    fullstring1 += now.second(); fullstring1 += ",";
    fullstring1 += dataloggerID;
   
    // separates sampling from sensors and writing to sd card so there is not a large spike in current
    delay(1000);
    
    // save string (with all sensor and time/date data to file "myFile"
    myFile.println(fullstring1); // print carriage return so that new data is added to one line below previous sample
      delay(50); // you will need a small delay here so the arduino can complete copying data to the sd card...  
     myFile.close();
      delay(50);  // give a little time for file to close

    Serial.println(fullstring1); // print "fullstring1" to monitor
     delay(50); // give a little time for string to print to serial
}

  //Go into sleep mode when 'loop' has run loop_repeat times
  if (loop_count == loop_repeat) {

  //reset the alarm before putting the MCU asleep at the beginning of the next loop
  AlarmFlag = 0;
  loop_count = 1;
  rtc.armAlarm(1, false);
  rtc.clearAlarm(1);
  rtc.alarmInterrupt(1, false);
  rtc.armAlarm(2, false);
  rtc.clearAlarm(2);
  rtc.alarmInterrupt(2, false);

  //set SQW pin on DS3231 to low
  rtc.writeSqwPinMode(DS3231_OFF);

  //Set alarm1 every day at 18:33
  rtc.setAlarm(ALM1_MATCH_SECONDS, 00, 00, 0);   //set your wake-up time here
  rtc.alarmInterrupt(1, true);
  //digitalWrite(rtc_power, LOW);
  
  }
  else {
      
      loop_count++;
    for (sleepCounter = 1; sleepCounter > 0; sleepCounter--) {
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
    }
    
  }

}
