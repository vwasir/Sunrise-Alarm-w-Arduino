#include <RTClib.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>

// Define pins
#define LED_PIN 6
#define LED_COUNT 80
#define BTN_HOUR 2
#define BTN_MIN 3
#define RAMP_SECS 3600  // 1 hour in seconds

// Create objects
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables
int alarmHour = 7;
int alarmMin = 0;
bool lastBtnHour = HIGH;
bool lastBtnMin = HIGH;
unsigned long startTime = 0;

void setup() {
  Serial.begin(9600);
  
  // Initialize components
  pinMode(BTN_HOUR, INPUT_PULLUP);
  pinMode(BTN_MIN, INPUT_PULLUP);
  
  strip.begin();
  strip.show();
  strip.setBrightness(40);
  
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  lcd.init();
  lcd.backlight();
  lcd.print("Sunrise Alarm");
  delay(2000);
  lcd.clear();
  
  startTime = millis();
}

void loop() {
  // Read buttons
  bool btnHour = digitalRead(BTN_HOUR);
  bool btnMin = digitalRead(BTN_MIN);
  
  // Check if hour button was pressed (transition from HIGH to LOW)
  if (btnHour == LOW && lastBtnHour == HIGH) {
    alarmHour = (alarmHour + 1) % 24;  // Increase hour, wrap at 24
    delay(200);  // Debounce delay
  }
  
  // Check if minute button was pressed (transition from HIGH to LOW)
  if (btnMin == LOW && lastBtnMin == HIGH) {
    alarmMin = (alarmMin + 1) % 60;  // Increase minute, wrap at 60
    delay(200);  // Debounce delay
  }
  
  lastBtnHour = btnHour;
  lastBtnMin = btnMin;
  
  // Get current time from RTC
  DateTime now = rtc.now();
  
  // Convert time to total minutes for easier calculation
  int nowMins = now.hour() * 60 + now.minute();
  int alarmMins = alarmHour * 60 + alarmMin;
  int rampStart = alarmMins - 60;  // Start ramp 60 minutes before alarm
  
  // Calculate progress (0.0 to 1.0) based on time until alarm
  float progress = 0.0;
  
  if (nowMins >= rampStart && nowMins < alarmMins) {
    // Within ramp period: calculate how far along we are
    progress = (float)(nowMins - rampStart) / 60.0;
  } else if (nowMins >= alarmMins) {
    // Alarm time reached: full brightness
    progress = 1.0;
  }
  
  // Update LED strip with calculated progress
  updateLED(progress);
  
  // Update LCD display with current time and alarm
  updateLCD(now);
  
  delay(1000);  // Update once per second
}

void updateLED(float p) {
  int r, g, b;
  
  // Three color stages based on progress through the hour
  if (p < 0.33) {
    // Stage 1: Deep red (first 20 minutes, 0.0-0.33 progress)
    float t = p / 0.33;
    r = (int)(150 + t * 50);
    g = (int)(t * 40);
    b = 0;
  } else if (p < 0.66) {
    // Stage 2: Orange (middle 20 minutes, 0.33-0.66 progress)
    float t = (p - 0.33) / 0.33;
    r = (int)(200 + t * 55);
    g = (int)(40 + t * 100);
    b = 0;
  } else {
    // Stage 3: Golden yellow (final 20 minutes, 0.66-1.0 progress)
    float t = (p - 0.66) / 0.34;
    r = 255;
    g = (int)(140 + t * 60);
    b = (int)(t * 40);
  }
  
  // Apply color to all 80 LEDs
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  
  strip.show();
}

int convertTo12Hour(int hour) {
  if (hour == 0) {
    return 12;  // Midnight is 12 AM
  } else if (hour > 12) {
    return hour - 12;  // Convert 13-23 to 1-11 PM
  } else {
    return hour;
  }
}

String getAMPM(int hour) {
  if (hour >= 12) {
    return "PM";
  } else {
    return "AM";
  }
}

void updateLCD(DateTime now) {
  int hour12 = convertTo12Hour(now.hour());
  int alarm12 = convertTo12Hour(alarmHour);
  
  lcd.setCursor(0, 0);
  lcd.print("Time:");
  lcd.print(printFormat(hour12));
  lcd.print(":");
  lcd.print(printFormat(now.minute()));
  lcd.print(getAMPM(now.hour()));
  
  lcd.setCursor(0, 1);
  lcd.print("Alm:");
  lcd.print(printFormat(alarm12));
  lcd.print(":");
  lcd.print(printFormat(alarmMin));
  lcd.print(getAMPM(alarmHour));
}

String printFormat(int val) {
  if (val < 10) {
    return "0" + String(val);
  }
  return String(val);
}