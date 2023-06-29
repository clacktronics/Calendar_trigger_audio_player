#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Bounce.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

#include "daily_sunset.h"
#include "daily_schedule.h"

AlarmId id;

AudioPlaySdWav           playWav1;
AudioOutputI2S           audioOutput;



AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);
//AudioControlSGTL5000     sgtl5000_1;

#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used

#define MODE_SWITCH 40
#define AUDIO_TRIG 41

Bounce mode_sw = Bounce(MODE_SWITCH,5 ); 
Bounce trig_sw = Bounce(AUDIO_TRIG,10 ); 

time_t upload_epoc = Teensy3Clock.get();
time_t until_next_performance;
time_t getTeensy3Time() {return Teensy3Clock.get();}
elapsedMillis sincePrint;

void playFile(const char *filename)
{
  Serial.print("Playing file: ");
  Serial.println(filename);

  // Start playing the file.  This sketch continues to
  // run while the file plays.
  playWav1.play(filename);

  // A brief delay for the library read WAV info
  delay(25);

  // Simply wait for the file to finish playing.
  while (playWav1.isPlaying()) {
    // uncomment these lines if you audio shield
    // has the optional volume pot soldered
    //float vol = analogRead(15);
    //vol = vol / 1024;
    // sgtl5000_1.volume(vol);
  }
}


void poll_times(){
  // Get the number of seconds today from midnight exluding epoch
  uint32_t seconds_now = (((hour()*60) + minute()) * 60) + second();
  if(mode_sw.read() == true) // SET BACK TO TRUE
  {
  // get the position in the array for current day,
  // now is seconds now since epoch, 
  // sunset_data_start_date tells us what the data starts from in daily_sunset.h
  // 86400 is seconds in 24 hours
  // so we should get number of days since the data started
  uint64_t sunset_day = (now() - sunset_data_start_date) / 86400; 
  // work out the number of seconds since midnight the performance is
  uint32_t seconds_perf = (((hour(sunsetdate[sunset_day])*60) + minute(sunsetdate[sunset_day])) * 60) + second(sunsetdate[sunset_day]);
  // If the performance is coming up (now is nammer than the perf) then set the until_next_performance to the number of seconds from now til then
  if(sunsetdate[sunset_day] > now()) {
    until_next_performance = seconds_perf - seconds_now;
  }
  // If we are past the performance then its the next days time, which is the number of seconds from now until midnight + seconds_perf
  else {
    sunset_day++;
    until_next_performance = (86400 - seconds_now) + seconds_perf;
  }
  Serial.println("SUNSET MODE");
  Serial.print("Sunset day set to ");
  Serial.println(sunset_day);
  Serial.println("so the sunset is coming in");
  print_next_countdown();
  
  }
else {
  // Iterate through the schedule until the next performance is bigger than the time now
  uint32_t length_schedule = sizeof(daily_schedule)/4;
  for(int d=0; d < length_schedule;d++){
    if(seconds_now > daily_schedule[d]) {next_perfomance_number = d+1;}
  }
    // if we got right to the last value then the next perfomance must be next day so set to first performance time
   if(next_perfomance_number == length_schedule) {next_perfomance_number = 0;}
   // Process it for the next day by getting number of seconds until midnight then add the seconds from midnight to now
   if(daily_schedule[next_perfomance_number] < seconds_now) {until_next_performance = (86400- seconds_now)+daily_schedule[next_perfomance_number];}
  // else just work out the  normal way
  else {until_next_performance = daily_schedule[next_perfomance_number] - seconds_now;}

  Serial.println("DAILY MODE");
  print_date((now() - seconds_now) + daily_schedule[next_perfomance_number]);
  Serial.println("so the next perf is coming in");
  print_next_countdown();
}  
}


void print_date(time_t t) {
  Serial.print("Date: ");
  Serial.print(day(t));
  Serial.print("/");
  Serial.print(month(t));
  Serial.print("/");
  Serial.println(year(t));

  Serial.print("Time: ");
  Serial.print(hour(t));
  Serial.print(":");
  Serial.print(minute(t));
  Serial.print(":");
  Serial.println(second(t));
}

void print_next_countdown(){
  time_t til_next_performace = until_next_performance;
  Serial.print("Next performance is:");
  Serial.print(hour(til_next_performace));
  Serial.print(":");
  Serial.print(minute(til_next_performace));
  Serial.print(":");
  Serial.println(second(til_next_performace));    
}

bool play_trigger = false;
bool sunset_mode;
void trigger_play() { play_trigger = true;}



void setup() {
  Serial.begin(115200);

    // set the Time library to use Teensy 3.0's RTC to keep time
  setSyncProvider(getTeensy3Time);
  Serial.println("I think the time is");
  print_date(now());   

  if (timeStatus()!= timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("RTC has set the system time");
  }
  
  AudioMemory(8);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(1);
  display.setCursor(0,0);
  display.println("CALENDAR");
  display.println("PLAYER");
  display.display();
  delay(1000);

//  sgtl5000_1.enable();
//  sgtl5000_1.volume(0.8);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);

  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(0,0);
    display.println("SD card fail");
    display.display();
  //while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
   //}
  }
    
  pinMode(MODE_SWITCH, INPUT_PULLUP);
  pinMode(AUDIO_TRIG, INPUT_PULLUP);
  
  
  mode_sw.update();
  poll_times();
  Alarm.timerOnce(until_next_performance, trigger_play);  
}

void renderDisplay() {
  time_t til_next_performace = until_next_performance;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(1);
  display.setCursor(0,0);

  display.print(day());
  display.print("/");
  display.print(month());
  display.print("/");
  display.print(year());
  display.print(" ");
  display.print(hour());
  display.print(":");
  display.print(minute());
  display.print(":");
  display.println(second());
  if(mode_sw.read() == true) { display.println("Sunset mode"); }  
  else {display.println("Daily Schedule mode");}

  display.print(hour(til_next_performace));
  display.print(":");
  display.print(minute(til_next_performace));
  display.print(":");
  display.println(second(til_next_performace));
  
  
  display.display();
  delay(1000);
}

void loop() {
  Alarm.delay(0); 
  mode_sw.update();
  trig_sw.update();
  if(sincePrint > 1000){
    //print_next_countdown(); 
    //Serial.println(mode_sw.read());
    poll_times();
    renderDisplay();
    sincePrint = sincePrint - 1000;}

  if(trig_sw.fallingEdge()) {
    Serial.println("Playing audio coz button");    
    trigger_play();
  }

  if(mode_sw.fallingEdge() || mode_sw.risingEdge())
  {
    poll_times();
    //Alarm.free(id);
    Alarm.timerOnce(until_next_performance, trigger_play);      
  }

  if(play_trigger){
  Serial.println("Playing Audio");
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(1);
  display.setCursor(0,0);
  display.println("PLAYING");
  display.display();
  //delay(1000);
  playFile("GONG.WAV");  // filenames are always uppercase 8.3 format
  poll_times(); // update times
  Alarm.timerOnce(until_next_performance, trigger_play); 
  play_trigger = false;
  }
}
