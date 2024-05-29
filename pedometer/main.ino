#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

/***********************************/
/*** GLOBAL VARIABLES DEFINITION ***/
/***********************************/
String      deviceUID;

const char* ssid =                "M";
const char* password =            "meiderato303161";

const char* ntpServer =           "asia.pool.ntp.org";
const long  gmtOffset_sec =       7 * 3600;
const int   daylightOffset_sec =  0;

const char* mongodbServer =       "https://ap-southeast-1.aws.data.mongodb-api.com/app/data-svqqfws/endpoint/data/v1/action/";
#define     DATA_SOURCE           "Funlish"
#define     DATABASE              "Pedometer"

#define SCREEN_WIDTH      128
#define SCREEN_HEIGHT     64
#define OLED_RESET        -1
#define SCREEN_ADDRESS    0x3C
#define CHAR_WIDTH        6
#define CHAR_HEIGHT       8
#define TEXT_SIZE_1       1
#define TEXT_SIZE_2       2
#define TEXT_COLOR_WHITE  SSD1306_WHITE
#define TEXT_COLOR_BLACK  SSD1306_BLACK

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;

#define RESTART_BUTTON    4
#define RESET_BUTTON      5
int     stateOn;        // track restart button
int     stateCounting;  // track reset button

struct tm     fullDateTime;
String        currentDate;
String        currentTime;
unsigned int  stepCounts;
unsigned int  totalSteps;
unsigned int  totalTime;
float         currAccVector;
float         prevAccVector;
bool          firstRun;
bool          startCounting;
String        startTime;
String        endTime;
unsigned int  idxCount;

/*****************************/
/*** FUNCTIONS DECLARATION ***/
/*****************************/

/*** PROCESS WIFI ***/
void initWiFi();
/********************/

/*** TURN ON & OFF DEVICES ***/
void turnOn();
void turnOff();
/*****************************/

/*** PROCESS CURRENT DATE & TIME WITH NTP***/
void    initNTP();
void    getDateTime();
String  getDate();
String  getTime();
String  getISODate();
unsigned int  timeDiffer(String startT, String endT);
/*******************************************/

/*** PROCESS ACCELERATION MPU6050 ***/
void  initMPU6050();
int   getTemperature();
bool  getAcceleration();
/************************************/

/*** PROCESS OLED ***/
void initOLED();
void showLogo(unsigned int duration);
void printDate();
void printTime();
void printTemperature();
void printTotalSteps();
void printStartTime();
void printEndTime();
void printCounting();
/********************/

/*** INTERACT WITH DATABASE ***/
JsonDocument  requestFindUtilityParams();
bool          requestFindDevice();
bool          requestInsertDevice();
bool          requestFindDailyIndicators();
bool          requestInsertDailyIndicators();
bool          requestUpdateTotal();
bool          requestUpdateRounds();
JsonDocument  updateDatabase(JsonDocument request, String action);
/******************************/

/*** UTILITY FUNCTIONS ***/
unsigned int  alignCenterX(String str, unsigned int textSize);
void          monitorResponse(JsonDocument res);
/*************************/

/******************************/
/*** SETUP & LOOP FUNCTIONS ***/
/******************************/

/***************************/
/********** SETUP **********/
void setup() {
  // Serial.begin(115200);
  // while (!Serial) {
  //   delay(100);
  // }

  stateOn = 0;        // 2 modes: OFF (0) - ON (1)
  stateCounting = -1; // 3 modes: BLANK (-1) - STOP (0) - START (1)
  currentDate = "";
  currentTime = "";
  stepCounts = 0;
  totalSteps = 0;
  totalTime = 0;
  currAccVector = 0.0;
  prevAccVector = 0.0;
  firstRun = true;
  startCounting = true;
  startTime = "";
  endTime = "";

  pinMode(RESET_BUTTON, INPUT);
  pinMode(RESTART_BUTTON, INPUT);
}

/**************************/
/********** LOOP **********/
void loop() {
  // detect restart button state
  int restartButtonState = digitalRead(RESTART_BUTTON);
  if (restartButtonState == HIGH) {
    // turn ON if device is OFF
    if (stateOn == 0) {
      turnOn();
      stateOn = 1;
    }
    // turn OFF if device is ON
    else {
      turnOff();
      stateOn = 0;
    }
  }
  else {
    // if device is ON
    if (stateOn == 1) {
      // update date - time & temperature
      getDateTime();
      printDate();
      printTime();
      printTemperature();

      // detect reset button state
      int resetButtonState = digitalRead(RESET_BUTTON);
      if (resetButtonState == HIGH) {

        // if state is blank -> start counting
        if (stateCounting == -1) {
          // erase previous time duration result
          oled.fillRect(1, 43, 126, 8, TEXT_COLOR_BLACK);

          printStartTime();
          
          startCounting = true;
          stateCounting = 1;
          firstRun = false;
        }

        // if state is counting -> stop counting and show result
        else if (stateCounting == 1) {
          if (!firstRun) {
            // erase start time
            oled.fillRect(1, 43, 126, 8, TEXT_COLOR_BLACK);
            
            printEndTime();
          }
          
          // only update step counts in case greater than 0
          if (stepCounts > 0) {
            // add up total steps
            totalSteps += stepCounts;
            totalTime += timeDiffer(startTime, endTime);
            // Serial.println("Time difference: " + String(timeDiffer(startTime, endTime)));

            printTotalSteps();

            // update rounds
            if (requestUpdateRounds()) {
              // update total steps
              if (!requestUpdateTotal()) {
                // Serial.println("Failed to update total steps!");
              }
            }
            else {
              // Serial.println("Failed to update rounds!");
            }

            stepCounts = 0;
          }
          
          stateCounting = 0;
        }

        // if state is showing result -> back to blank state
        else {
          // clear container
          oled.fillRect(1, 43, 126, SCREEN_HEIGHT - 44, TEXT_COLOR_BLACK);
          
          stateCounting = -1;
        }
      }
      else {
        // continue to counting if in process of counting
        if (stateCounting == 1) {
          if (startCounting) {
            printCounting();
            startCounting = false;
          }
          // process values obtained from accelerometer
          if (getAcceleration()) {
            printCounting();
          }        
        }
      }

      oled.display();
    }
  }
  delay(200);
}

/****************************/
/*** FUNCTIONS DEFINITION ***/
/****************************/

/********************/
/*** PROCESS WIFI ***/
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Serial.println("");
  // Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    // Serial.print(".");
    delay(100);
  }
  // Serial.println("");
  // Serial.println("Successfully connected!");
  // Serial.print("=> Local IP: ");
  // Serial.println(WiFi.localIP());

  deviceUID = WiFi.macAddress();
}

/*** TURN ON & OFF DEVICES ***/
void turnOn() {
  Wire.begin(8, 9);
  initWiFi();
  initNTP();
  initMPU6050();
  initOLED();

  showLogo();
  
  getDateTime();
  currentDate = getDate();
  currentTime = getTime();

  // if device not found
  if (!requestFindDevice()) {
    // insert new device
    if (requestInsertDevice()) {
      // insert new daily indicators
      if (!requestInsertDailyIndicators()) {
        // Serial.println("Failed to insert new daily indicators!");
      }
    }
    else {
      // Serial.println("Failed to insert new device!");
    }
  }
  // if device already existed
  else {
    // find daily indicators of that device for today
    if (!requestFindDailyIndicators()) {
      // if there doesn't exist data for today, then insert new daily indicators
      if (!requestInsertDailyIndicators()) {
        // Serial.println("Failed to insert new daily indicators!");
      }
    }
  }

  printWatchScreen();
}

void turnOff() {
  while (WiFi.status() == WL_CONNECTED) {
    // Serial.print("Disconnecting WiFi...");
    WiFi.disconnect();
    delay(100);
    // Serial.print(".");
  }
  // Serial.println(" Successfully terminate WiFi connection!");

  oled.clearDisplay();
  oled.display();

  stateCounting = -1;
  firstRun = true;
}

/*******************************************/
/*** PROCESS CURRENT DATE & TIME WITH NTP***/
void initNTP() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void getDateTime() {
  while (!getLocalTime(&fullDateTime)){
    // Serial.println("Failed to obtain date & time! Try again...");
    delay(100);
  }
}

String getDate() {
  char fullDate[11];
  strftime(fullDate, 11, "%d-%m-%Y", &fullDateTime);
  return fullDate;
}

String getTime() {
  char fullTime[6];
  strftime(fullTime, 6, "%H:%M", &fullDateTime);
  return fullTime;
}

String getISODate() {
  unsigned int len = sizeof "YYYY-MM-DDTHH:MM:SS";
  char ISODate[len];
  strftime(ISODate, len, "%FT%T", &fullDateTime);
  return ISODate;
}

unsigned int timeDiffer(String startT, String endT) {
  unsigned int startH = startT.substring(0, 2).toInt();
  unsigned int startM = startT.substring(3, 5).toInt();
  unsigned int endH = endT.substring(0, 2).toInt();
  unsigned int endM = endT.substring(3, 5).toInt();

  // Serial.println("");
  // Serial.println(String(startH) + " : " + String(startM) + "-" + String(endH) + " : " + String(endM));

  if (startM > endM) {
    --endH;
    endM += 60;
  }
  return (endH - startH) * 60 + endM - startM;
}

/*** PROCESS ACCELERATION MPU6050 ***/
void initMPU6050() {
  // Serial.println("");
  while (!mpu.begin()) {
    // Serial.println("Failed to find MPU6050! Try again...");
    delay(100);
  }
  // Serial.println("MPU6050 found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
}

int getTemperature() {
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  return t.temperature;
}

bool getAcceleration() {
  bool stepDetected = false;

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);

  float x = a.acceleration.x;
  float y = a.acceleration.y;
  float z = a.acceleration.z;

  // Serial.println("Acceleration: " + String(x) + " || " + String(y) + " || " + String(z));

  currAccVector = sqrt((x * x) + (y * y) + (z * z));
  // Serial.println("=> Magnitude: " + String(currAccVector));

  if (currAccVector - prevAccVector > 3) {
    stepCounts++;
    stepDetected = true;
  }
  prevAccVector = currAccVector;
  return stepDetected;
}

/********************/
/*** PROCESS OLED ***/
void initOLED() {
  // Serial.println("");
  while (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // Serial.println("Failed to allocate SSD1306! Try again...");
  }
  // Serial.println("SSD1306 allocated!");
}

void printWatchScreen() {
  oled.clearDisplay();
  oled.display();

  oled.setTextSize(TEXT_SIZE_1);
  oled.setTextColor(TEXT_COLOR_WHITE);

  printDate();
  printTemperature();
  printTime();
  printTotalSteps();

  // container of current walking round
  oled.drawRect(0, 40, 128, SCREEN_HEIGHT - 40, TEXT_COLOR_WHITE);

  oled.display();
}

void showLogo() {
  const uint8_t logo_map[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xef, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xcf, 0xe7, 0xfc, 0x7f, 0xff, 0xff, 
    0xff, 0xff, 0xe7, 0xe7, 0xf8, 0x7f, 0xff, 0xff, 
    0xff, 0xff, 0xf7, 0xef, 0xf8, 0x3f, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xef, 0xb8, 0x3f, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xb8, 0x7f, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xb8, 0x7f, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xfe, 0x00, 0x7f, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xe0, 0x81, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xc1, 0x00, 0x7f, 0xff, 0xff, 
    0xff, 0xff, 0xf9, 0x07, 0x00, 0x7f, 0xff, 0xff, 
    0xff, 0xff, 0xf0, 0x0f, 0x00, 0x78, 0xff, 0xff, 
    0xff, 0xfe, 0x00, 0x0f, 0x00, 0x78, 0x3f, 0xff, 
    0xff, 0xfc, 0x00, 0x4f, 0x00, 0x38, 0x07, 0xff, 
    0xff, 0xfc, 0x30, 0x0d, 0x00, 0x06, 0x83, 0xff, 
    0xff, 0xfc, 0x30, 0x0b, 0x00, 0x02, 0x83, 0xff, 
    0xff, 0xfc, 0x78, 0x1f, 0x01, 0x00, 0xc3, 0xff, 
    0xff, 0xfc, 0xfc, 0x1e, 0x03, 0x80, 0x83, 0xff, 
    0xff, 0xfd, 0xf8, 0x1e, 0x07, 0xc0, 0x03, 0xff, 
    0xff, 0xff, 0xfe, 0x1a, 0x07, 0x80, 0x07, 0xbf, 
    0xff, 0xfd, 0xfe, 0xb8, 0x0f, 0x00, 0x3f, 0x7f, 
    0xff, 0xfc, 0xff, 0xb8, 0x0c, 0x00, 0x3e, 0x7f, 
    0xff, 0xfc, 0xff, 0xb8, 0x00, 0x00, 0x3c, 0xff, 
    0xff, 0xfd, 0xff, 0x30, 0x00, 0x00, 0x91, 0xff, 
    0xff, 0xfd, 0xff, 0x40, 0x00, 0x1f, 0x87, 0xff, 
    0xff, 0xff, 0xfe, 0x40, 0x20, 0x3f, 0x8f, 0xff, 
    0xff, 0xfb, 0xfc, 0xc0, 0x40, 0x3f, 0xff, 0xff, 
    0xff, 0xc1, 0xf8, 0xc0, 0x40, 0x3f, 0xff, 0xff, 
    0xff, 0x88, 0x10, 0x80, 0x80, 0x1f, 0xff, 0xff, 
    0xff, 0x18, 0x00, 0x01, 0x80, 0x1f, 0xff, 0xff, 
    0xff, 0x39, 0xf0, 0x03, 0x00, 0x0f, 0xff, 0xff, 
    0xff, 0x39, 0xfc, 0x06, 0x06, 0x0f, 0xff, 0xff, 
    0xff, 0x7b, 0xfe, 0x1c, 0x0f, 0x07, 0xff, 0xff, 
    0xff, 0xfb, 0xff, 0x40, 0x1f, 0x87, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0x00, 0x3f, 0xc7, 0xff, 0xff, 
    0xff, 0xff, 0xf2, 0x00, 0xdf, 0xc7, 0xff, 0xff, 
    0xff, 0xff, 0xe0, 0x0f, 0xcf, 0x87, 0xff, 0xff, 
    0xff, 0xff, 0xe0, 0xff, 0xcf, 0x83, 0xff, 0xff, 
    0xff, 0xff, 0xe0, 0xff, 0xcf, 0x19, 0xdf, 0xff, 
    0xff, 0xff, 0xc4, 0x7f, 0xff, 0x18, 0x7f, 0xff, 
    0xff, 0xff, 0xdc, 0x7f, 0xfe, 0x3d, 0xff, 0xff, 
    0xff, 0xe7, 0xf8, 0xff, 0xfe, 0x3c, 0xff, 0xff, 
    0xff, 0xc3, 0xf9, 0xff, 0xfe, 0x7f, 0xff, 0xff, 
    0xff, 0xc7, 0xfb, 0xff, 0xfe, 0x7f, 0xff, 0xff, 
    0xff, 0xef, 0xff, 0xff, 0x7e, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0x7c, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0x78, 0x7f, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xf8, 0x3f, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  };
  oled.clearDisplay();
  oled.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TEXT_COLOR_WHITE);
  oled.drawBitmap(SCREEN_HEIGHT / 2, 0, logo_map, SCREEN_HEIGHT, SCREEN_HEIGHT, TEXT_COLOR_BLACK);
  oled.display();
}

void printDate() {
  currentDate = getDate();
  oled.fillRect(0, 0, currentDate.length() * CHAR_WIDTH * TEXT_SIZE_1, 8, TEXT_COLOR_BLACK);
  oled.setTextSize(TEXT_SIZE_1);
  oled.setTextColor(TEXT_COLOR_WHITE);
  oled.setCursor(0, 0);
  oled.print(currentDate);
}

void printTime() {
  currentTime = getTime();
  unsigned int timeStartX = alignCenterX(currentTime, TEXT_SIZE_2);
  oled.fillRoundRect(timeStartX - 4, 9, currentTime.length() * CHAR_WIDTH * TEXT_SIZE_2 + 8, 18, 8, TEXT_COLOR_WHITE);
  oled.setTextSize(TEXT_SIZE_2);
  oled.setTextColor(TEXT_COLOR_BLACK);
  oled.setCursor(timeStartX, 10);
  oled.print(currentTime);
}

void printTemperature() {
  unsigned int startTempX = 12 * CHAR_WIDTH * TEXT_SIZE_1;
  oled.fillRect(startTempX, 0, 128 - startTempX, 8, TEXT_COLOR_BLACK);
  char ch = 248; // degree symbol
  String temp = String(getTemperature()) + String(ch) + "C";
  oled.setTextSize(TEXT_SIZE_1);
  oled.setTextColor(TEXT_COLOR_WHITE);
  oled.setCursor(SCREEN_WIDTH - temp.length() * CHAR_WIDTH * TEXT_SIZE_1, 0);
  oled.print(temp);
}

void printTotalSteps() {
  oled.fillRect(0, 29, 128, 8, TEXT_COLOR_BLACK);
  String strTotalSteps = "Total: " + String(totalSteps) + " steps";
  oled.setTextSize(TEXT_SIZE_1);
  oled.setTextColor(TEXT_COLOR_WHITE);
  oled.setCursor(alignCenterX(strTotalSteps, TEXT_SIZE_1), 29);
  oled.print(strTotalSteps);
}

void printStartTime() {
  stepCounts = 0;
  startTime = getTime();
  String strStart = "Started at " + startTime;
  oled.setTextSize(TEXT_SIZE_1);
  oled.setTextColor(TEXT_COLOR_WHITE);
  oled.setCursor(alignCenterX(strStart, TEXT_SIZE_1), 43);
  oled.print(strStart);
}

void printEndTime() {
  endTime = getTime();
  String walkDuration = "From " + startTime + " to " + endTime;
  oled.setTextSize(TEXT_SIZE_1);
  oled.setTextColor(TEXT_COLOR_WHITE);
  oled.setCursor(alignCenterX(walkDuration, TEXT_SIZE_1), 43);
  oled.print(walkDuration);
}

void printCounting() {
  oled.fillRect(1, 54, 126, 8, TEXT_COLOR_BLACK);
  String counting = "Counting: " + String(stepCounts);
  oled.setTextSize(TEXT_SIZE_1);
  oled.setTextColor(TEXT_COLOR_WHITE);
  oled.setCursor(alignCenterX(counting, TEXT_SIZE_1), 54);
  oled.print(counting);
}

/******************************/
/*** INTERACT WITH DATABASE ***/

/*** findOne - Get utility parameters ***/
JsonDocument requestFindUtilityParams() {
  JsonDocument req;
  req["dataSource"] = DATA_SOURCE;
  req["database"] = DATABASE;
  req["collection"] = "utilityparams";
  req["filter"]["unique"] = true;

  JsonDocument res = updateDatabase(req, "findOne");
  monitorResponse(res);

  return res;
}

/*** updateOne - Update index counts ***/
bool requestUpdateUtilityParamas() {
  JsonDocument req;
  req["dataSource"] = DATA_SOURCE;
  req["database"] = DATABASE;
  req["collection"] = "utilityparams";
  req["filter"]["unique"] = true;
  req["update"]["$set"]["idxcount"] = idxCount + 1;

  JsonDocument res = updateDatabase(req, "updateOne");
  if (res.isNull() || res["matchedCount"] == 0 || res["modifiedCount"] == 0) {
    return false;
  }
  monitorResponse(res);

  return true;
}

/*** findOne - Find device using 'macaddress' ***/
bool requestFindDevice() {
  JsonDocument req;
  req["dataSource"] = DATA_SOURCE;
  req["database"] = DATABASE;
  req["collection"] = "devices";
  req["filter"]["macaddress"] = deviceUID;

  JsonDocument res = updateDatabase(req, "findOne");
  if (res.isNull() || res["document"].isNull()) {
    return false;
  }
  monitorResponse(res);

  return true;
}

/*** insertOne - Insert new device ***/
bool requestInsertDevice() {
  JsonDocument utilityParams = requestFindUtilityParams();
  if (utilityParams.isNull() || utilityParams["document"].isNull()) {
    // Serial.println("Failed to retrieve utility parameters to insert new device!");
    return false;
  }
  idxCount = utilityParams["document"]["idxcount"];
  String defaultPassword = utilityParams["document"]["defaultpassword"];

  JsonDocument req;
  req["dataSource"] = DATA_SOURCE;
  req["database"] = DATABASE;
  req["collection"] = "devices";
  req["document"]["macaddress"] = deviceUID;
  req["document"]["username"] = "pedometer" + String(idxCount);
  req["document"]["password"] = defaultPassword;
  
  JsonDocument res = updateDatabase(req, "insertOne");
  if (res.isNull()) {
    return false;
  }

  bool idxCountUpdated = requestUpdateUtilityParamas();
  if (!idxCountUpdated) {
    // Serial.println("Failed to update index counts!");
  }

  monitorResponse(res);

  return true;
}

/*** updateOne - Update last active date & time ***/
bool requestUpdateLastActive() {
  JsonDocument req;
  req["dataSource"] = DATA_SOURCE;
  req["database"] = DATABASE;
  req["collection"] = "devices";
  req["filter"]["macaddress"] = deviceUID;
  req["update"]["$set"]["lastactive"] = getISODate();

  JsonDocument res = updateDatabase(req, "updateOne");
  if (res.isNull() || res["matchedCount"] == 0 || res["modifiedCount"] == 0) {
    return false;
  }
  monitorResponse(res);

  return true;
}

/*** findOne - Find daily indicators using 'deviceuid' and 'date' ***/
bool requestFindDailyIndicators() {
  JsonDocument req;
  req["dataSource"] = DATA_SOURCE;
  req["database"] = DATABASE;
  req["collection"] = "dailyindicators";
  req["filter"]["deviceuid"] = deviceUID;
  req["filter"]["date"] = currentDate;
  
  JsonDocument res = updateDatabase(req, "findOne");
  if (res.isNull() || res["document"].isNull()) {
    return false;
  }
  monitorResponse(res);

  totalSteps = res["document"]["totalsteps"];
  totalTime = res["document"]["totaltime"];

  return true;
}

/*** insertOne - Insert new daily indicators ***/
bool requestInsertDailyIndicators() {
  JsonDocument req;
  req["dataSource"] = DATA_SOURCE;
  req["database"] = DATABASE;
  req["collection"] = "dailyindicators";
  req["document"]["deviceuid"] = deviceUID;
  req["document"]["date"] = currentDate;
  req["document"]["totalsteps"] = 0;
  req["document"]["totaltime"] = 0;
  JsonDocument emptyArr;
  req["document"]["rounds"] = emptyArr.to<JsonArray>();

  JsonDocument res = updateDatabase(req, "insertOne");
  if (res.isNull()) {
    return false;
  }
  monitorResponse(res);

  totalSteps = 0;
  totalTime = 0;

  return true;
}

/*** updateOne - Update totalsteps ***/
bool requestUpdateTotal() {
  JsonDocument req;
  req["dataSource"] = DATA_SOURCE;
  req["database"] = DATABASE;
  req["collection"] = "dailyindicators";
  req["filter"]["deviceuid"] = deviceUID;
  req["filter"]["date"] = currentDate;
  req["update"]["$set"]["totalsteps"] = totalSteps;
  req["update"]["$set"]["totaltime"] = totalTime;

  JsonDocument res = updateDatabase(req, "updateOne");
  if (res.isNull() || res["matchedCount"] == 0 || res["modifiedCount"] == 0) {
    return false;
  }
  monitorResponse(res);

  return true;
}

/*** updateOne - Update rounds ***/
bool requestUpdateRounds() {
  JsonDocument req;
  req["dataSource"] = DATA_SOURCE;
  req["database"] = DATABASE;
  req["collection"] = "dailyindicators";
  req["filter"]["deviceuid"] = deviceUID;
  req["filter"]["date"] = currentDate;
  req["update"]["$push"]["rounds"]["start"] = startTime;
  req["update"]["$push"]["rounds"]["end"] = endTime;
  req["update"]["$push"]["rounds"]["steps"] = stepCounts;
  
  JsonDocument res = updateDatabase(req, "updateOne");
  if (res.isNull() || res["matchedCount"] == 0 || res["modifiedCount"] == 0) {
    return false;
  }
  monitorResponse(res);

  return true;
}

JsonDocument updateDatabase(JsonDocument request, String action) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure();
    
    HTTPClient https;
    https.begin(*client, mongodbServer + action);
    https.addHeader("Content-Type", "application/json");
    https.addHeader("api-key", "yM9BTyna6pAQWNQ3F6CpMq9vtG84p5tcZq6Xyu5KjZLMRJicYsOnXFIKdvup4qhT");
    
    String payload;
    serializeJson(request, payload);
    // Serial.println("");
    // Serial.println("### Request ###");
    // Serial.println(payload);

    int httpCode = https.sendRequest("POST", payload);
    // Serial.print("~ HTTP response code: ");
    // Serial.println(httpCode);

    JsonDocument doc;
    if (httpCode == 200 || httpCode == 201 || httpCode == 202) {
      String res = https.getString();
      deserializeJson(doc, res);
    }  
    https.end();

    return doc;
  }
  else {
    // Serial.println("WiFi disconneted!");
  }
}

/*************************/
/*** UTILITY FUNCTIONS ***/
unsigned int alignCenterX(String str, unsigned int textSize) {
  return (SCREEN_WIDTH - str.length() * CHAR_WIDTH * textSize) / 2;
}

void monitorResponse(JsonDocument res) {
  String output;
  // Serial.println("=> Response:");
  serializeJsonPretty(res, output);
  // Serial.println(output);
}