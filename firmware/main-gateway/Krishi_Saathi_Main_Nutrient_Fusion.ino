#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define WIFI_CHANNEL 1

// ============================================================
// IRRIGATION LED
// ============================================================

#define IRRIGATION_LED_PIN 25

// ============================================================
// TFT DISPLAY
// ============================================================

#define TFT_CS     5
#define TFT_DC     27
#define TFT_RST    33
#define TFT_SCLK   18
#define TFT_MOSI   23

Adafruit_ST7735 tft = Adafruit_ST7735(
  TFT_CS,
  TFT_DC,
  TFT_MOSI,
  TFT_SCLK,
  TFT_RST
);

unsigned long lastDisplayChange = 0;
int displayPage = 0;

#define DISPLAY_PAGE_TIME 4000UL


// ============================================================
// SENSOR DATA RECEIVED FROM SENSOR NODE
// ============================================================

typedef struct
{
  float soilMoisture;
  float soilTemperature;
  float soilPH;
  uint16_t soilEC;

  uint16_t nitrogen;
  uint16_t phosphorus;
  uint16_t potassium;

  float airTemperature;
  float airHumidity;

  int waterRaw;
  int rainRaw;

  char waterStatus[16];
  char rainStatus[16];

  bool soilValid;
  bool dhtValid;

} SensorData;

SensorData receivedData;


// ============================================================
// VISION DATA RECEIVED FROM S3-CAM
// ============================================================

typedef struct
{
  char disease[32];
  float confidence;
  bool valid;
  bool lowConfidence;

} VisionData;

VisionData receivedVision;

bool visionDataAvailable = false;
bool visionDisplayPending = false;


// ============================================================
// FIELD STATE
// ============================================================

typedef struct
{
  String soilCondition;
  String weatherCondition;
  String rainCondition;
  String waterCondition;

} FieldState;

FieldState field;


// ============================================================
// ENVIRONMENTAL RISK STATE
// ============================================================

String droughtRisk = "NORMAL";
String excessiveRainRisk = "NORMAL";
String floodRisk = "NORMAL";
String heatRisk = "NORMAL";


// ============================================================
// IRRIGATION STATE
// ============================================================

String irrigationDecision = "WAITING";
bool irrigationON = false;


// ============================================================
// TIME TRACKING
// ============================================================

unsigned long lastSensorTime = 0;

unsigned long drySoilDuration = 0;
unsigned long rainDuration = 0;


// ============================================================
// DEMO THRESHOLDS
// ============================================================

// These are intentionally short for testing.
// We can increase them for the final demonstration.

#define DROUGHT_TIME_MS 30000UL
#define EXCESSIVE_RAIN_TIME_MS 30000UL


// ============================================================
// SENSOR FUSION
// ============================================================

void updateFieldState()
{
  // ----------------------------------------------------------
  // SOIL CONDITION
  // ----------------------------------------------------------

  if (!receivedData.soilValid)
  {
    field.soilCondition = "SENSOR ERROR";
  }
  else if (receivedData.soilMoisture < 20)
  {
    field.soilCondition = "DRY";
  }
  else if (receivedData.soilMoisture < 40)
  {
    field.soilCondition = "MODERATE";
  }
  else
  {
    field.soilCondition = "WET";
  }


  // ----------------------------------------------------------
  // WEATHER CONDITION
  // ----------------------------------------------------------

  if (!receivedData.dhtValid)
  {
    field.weatherCondition = "SENSOR ERROR";
  }
  else if (receivedData.airTemperature >= 35)
  {
    field.weatherCondition = "HOT";
  }
  else if (receivedData.airTemperature >= 30)
  {
    field.weatherCondition = "WARM";
  }
  else
  {
    field.weatherCondition = "NORMAL";
  }


  // ----------------------------------------------------------
  // RAIN CONDITION
  // ----------------------------------------------------------

  field.rainCondition = receivedData.rainStatus;


  // ----------------------------------------------------------
  // FIELD WATER LEVEL
  // ----------------------------------------------------------

  field.waterCondition = receivedData.waterStatus;
}


// ============================================================
// NUTRIENT ANALYSIS
// ============================================================

String getNutrientStatus(uint16_t value)
{
  if (value < 20)
  {
    return "LOW";
  }
  else if (value <= 80)
  {
    return "NORMAL";
  }
  else
  {
    return "HIGH";
  }
}


// ============================================================
// pH ANALYSIS
// ============================================================

String getPHStatus(float ph)
{
  if (ph < 5.5)
  {
    return "ACIDIC";
  }
  else if (ph <= 7.5)
  {
    return "NORMAL";
  }
  else
  {
    return "ALKALINE";
  }
}


// ============================================================
// EC ANALYSIS
// ============================================================

String getECStatus(uint16_t ec)
{
  if (ec < 100)
  {
    return "LOW";
  }
  else if (ec <= 1000)
  {
    return "NORMAL";
  }
  else
  {
    return "HIGH";
  }
}


// ============================================================
// UPDATE TIME-BASED CONDITIONS
// ============================================================

void updateDurations()
{
  unsigned long currentTime = millis();

  if (lastSensorTime == 0)
  {
    lastSensorTime = currentTime;
    return;
  }

  unsigned long elapsed = currentTime - lastSensorTime;

  lastSensorTime = currentTime;


  // ----------------------------------------------------------
  // DRY SOIL DURATION
  // ----------------------------------------------------------

  if (receivedData.soilValid &&
      receivedData.soilMoisture < 20)
  {
    drySoilDuration += elapsed;
  }
  else
  {
    drySoilDuration = 0;
  }


  // ----------------------------------------------------------
  // RAIN DURATION
  // ----------------------------------------------------------

  if (strcmp(receivedData.rainStatus, "DRY") != 0)
  {
    rainDuration += elapsed;
  }
  else
  {
    rainDuration = 0;
  }
}


// ============================================================
// ENVIRONMENTAL RISK ENGINE
// ============================================================

void updateEnvironmentalRisks()
{
  // ----------------------------------------------------------
  // RESET
  // ----------------------------------------------------------

  droughtRisk = "NORMAL";
  excessiveRainRisk = "NORMAL";
  floodRisk = "NORMAL";
  heatRisk = "NORMAL";


  // ----------------------------------------------------------
  // DROUGHT RISK
  // ----------------------------------------------------------

  if (receivedData.soilValid &&
      drySoilDuration >= DROUGHT_TIME_MS)
  {
    droughtRisk = "HIGH";
  }


  // ----------------------------------------------------------
  // EXCESSIVE RAIN RISK
  // ----------------------------------------------------------

  if (rainDuration >= EXCESSIVE_RAIN_TIME_MS)
  {
    excessiveRainRisk = "HIGH";
  }


  // ----------------------------------------------------------
  // FLOOD / WATERLOGGING RISK
  // ----------------------------------------------------------

  if (rainDuration >= EXCESSIVE_RAIN_TIME_MS &&
      strcmp(receivedData.waterStatus, "HIGH") == 0)
  {
    floodRisk = "HIGH";
  }
  else if (strcmp(receivedData.waterStatus, "HIGH") == 0)
  {
    floodRisk = "WARNING";
  }


  // ----------------------------------------------------------
  // HEAT STRESS RISK
  // ----------------------------------------------------------

  if (receivedData.dhtValid)
  {
    if (receivedData.airTemperature >= 35 &&
        receivedData.soilValid &&
        receivedData.soilMoisture < 20)
    {
      heatRisk = "HIGH";
    }
    else if (receivedData.airTemperature >= 35)
    {
      heatRisk = "WARNING";
    }
  }
}


// ============================================================
// IRRIGATION DECISION
// ============================================================

void makeIrrigationDecision()
{
  irrigationON = false;

  irrigationDecision = "NO IRRIGATION";


  // ----------------------------------------------------------
  // SOIL SENSOR ERROR
  // ----------------------------------------------------------

  if (!receivedData.soilValid)
  {
    irrigationDecision = "BLOCKED - SOIL SENSOR ERROR";

    digitalWrite(IRRIGATION_LED_PIN, LOW);

    return;
  }


  // ----------------------------------------------------------
  // FLOOD / WATERLOGGING
  // ----------------------------------------------------------

  if (floodRisk == "HIGH" ||
      floodRisk == "WARNING")
  {
    irrigationDecision = "BLOCKED - WATERLOGGING";

    digitalWrite(IRRIGATION_LED_PIN, LOW);

    return;
  }


  // ----------------------------------------------------------
  // RAIN DETECTED
  // ----------------------------------------------------------

  if (strcmp(receivedData.rainStatus, "DRY") != 0)
  {
    irrigationDecision = "WAIT - RAIN DETECTED";

    digitalWrite(IRRIGATION_LED_PIN, LOW);

    return;
  }


  // ----------------------------------------------------------
  // DRY SOIL
  // ----------------------------------------------------------

  if (receivedData.soilMoisture < 20)
  {
    irrigationDecision = "IRRIGATION RECOMMENDED";

    irrigationON = true;

    digitalWrite(IRRIGATION_LED_PIN, HIGH);

    return;
  }


  // ----------------------------------------------------------
  // OTHERWISE
  // ----------------------------------------------------------

  irrigationDecision = "NO IRRIGATION";

  irrigationON = false;

  digitalWrite(IRRIGATION_LED_PIN, LOW);
}



// ============================================================
// TFT DISPLAY FUNCTIONS
// ============================================================

void clearDisplay()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
}

void displayHeader(const char *title)
{
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.setCursor(4, 3);
  tft.println(title);
  tft.drawFastHLine(4, 14, 152, ST77XX_WHITE);
}

void displayPage1()
{
  clearDisplay();
  displayHeader("KRISHI BONDHU");

  tft.setCursor(4, 22);
  tft.print("Soil: ");
  tft.println(field.soilCondition);

  tft.setCursor(4, 34);
  tft.print("Moisture: ");
  if (receivedData.soilValid) {
    tft.print(receivedData.soilMoisture, 1);
    tft.println("%");
  } else {
    tft.println("ERROR");
  }

  tft.setCursor(4, 46);
  tft.print("Air Temp: ");
  if (receivedData.dhtValid) {
    tft.print(receivedData.airTemperature, 1);
    tft.println(" C");
  } else {
    tft.println("ERROR");
  }

  tft.setCursor(4, 58);
  tft.print("Humidity: ");
  if (receivedData.dhtValid) {
    tft.print(receivedData.airHumidity, 1);
    tft.println("%");
  } else {
    tft.println("ERROR");
  }

  tft.setCursor(4, 70);
  tft.print("Rain: ");
  tft.println(field.rainCondition);

  tft.setCursor(4, 82);
  tft.print("Water: ");
  tft.println(field.waterCondition);

  tft.setCursor(4, 98);
  tft.setTextColor(irrigationON ? ST77XX_GREEN : ST77XX_WHITE);
  tft.print("Irrigation: ");
  tft.println(irrigationON ? "ON" : "OFF");

  tft.setCursor(4, 112);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Heat Risk: ");
  tft.println(heatRisk);
}

void displayPage2()
{
  clearDisplay();
  displayHeader("FIELD HEALTH");

  tft.setCursor(4, 22);
  tft.print("N: ");
  tft.print(receivedData.nitrogen);
  tft.print(" ");
  tft.println(getNutrientStatus(receivedData.nitrogen));

  tft.setCursor(4, 34);
  tft.print("P: ");
  tft.print(receivedData.phosphorus);
  tft.print(" ");
  tft.println(getNutrientStatus(receivedData.phosphorus));

  tft.setCursor(4, 46);
  tft.print("K: ");
  tft.print(receivedData.potassium);
  tft.print(" ");
  tft.println(getNutrientStatus(receivedData.potassium));

  tft.setCursor(4, 62);
  tft.print("pH: ");
  tft.print(receivedData.soilPH, 1);
  tft.print(" ");
  tft.println(getPHStatus(receivedData.soilPH));

  tft.setCursor(4, 74);
  tft.print("EC: ");
  tft.print(receivedData.soilEC);
  tft.print(" ");
  tft.println(getECStatus(receivedData.soilEC));

  tft.setCursor(4, 92);
  tft.print("Drought: ");
  tft.println(droughtRisk);

  tft.setCursor(4, 104);
  tft.print("Rain Risk: ");
  tft.println(excessiveRainRisk);

  tft.setCursor(4, 116);
  tft.print("Flood: ");
  tft.println(floodRisk);
}

void displayPage3()
{
  clearDisplay();
  displayHeader("CROP AI");

  if (!visionDataAvailable)
  {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(4, 28);
    tft.println("WAITING FOR AI");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(4, 44);
    tft.println("No camera result");
    tft.setCursor(4, 56);
    tft.println("received yet.");

    tft.setCursor(4, 82);
    tft.println("S3-CAM: WAITING");
    return;
  }

  if (!receivedVision.valid)
  {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(4, 28);
    tft.println("AI INFERENCE ERROR");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(4, 44);
    tft.println("Camera result");
    tft.setCursor(4, 56);
    tft.println("unavailable.");

    tft.setCursor(4, 82);
    tft.println("Check S3-CAM.");
    return;
  }

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(4, 22);
  tft.println("PREDICTION");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(4, 36);
  tft.println(receivedVision.disease);

  tft.setCursor(4, 54);
  tft.print("Confidence: ");
  tft.print(receivedVision.confidence, 1);
  tft.println("%");

  tft.setCursor(4, 70);
  if (receivedVision.lowConfidence)
  {
    tft.setTextColor(ST77XX_YELLOW);
    tft.println("LOW CONFIDENCE");
  }
  else
  {
    tft.setTextColor(ST77XX_GREEN);
    tft.println("AI RESULT RELIABLE");
  }

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(4, 92);
  tft.print("Soil: ");
  tft.println(field.soilCondition);

  tft.setCursor(4, 104);
  tft.print("Temp: ");
  if (receivedData.dhtValid)
  {
    tft.print(receivedData.airTemperature, 1);
    tft.println(" C");
  }
  else
  {
    tft.println("ERROR");
  }

  tft.setCursor(4, 116);
  tft.print("Risk: ");
  if (floodRisk == "HIGH" || floodRisk == "WARNING")
    tft.println("WATERLOG");
  else if (heatRisk == "HIGH")
    tft.println("HEAT");
  else if (droughtRisk == "HIGH")
    tft.println("DROUGHT");
  else
    tft.println("NORMAL");
}


void displayPage4()
{
  clearDisplay();
  displayHeader("FARMER ADVISORY");

  // ----------------------------------------------------------
  // HIGHEST-PRIORITY ENVIRONMENTAL ALERT
  // ----------------------------------------------------------

  if (floodRisk == "HIGH" || floodRisk == "WARNING")
  {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(4, 24);
    tft.println("WATERLOGGING");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(4, 38);
    tft.println("Stop irrigation");
    tft.setCursor(4, 50);
    tft.println("and check drainage.");
    tft.setCursor(4, 62);
    tft.println("Field water level high.");

    if (visionDataAvailable && receivedVision.valid)
    {
      tft.setCursor(4, 80);
      tft.print("AI: ");
      tft.println(receivedVision.disease);
    }
  }

  // ----------------------------------------------------------
  // AI RESULT + ENVIRONMENTAL CONDITION
  // ----------------------------------------------------------

  else if (visionDataAvailable &&
           receivedVision.valid &&
           receivedVision.lowConfidence)
  {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(4, 24);
    tft.println("AI CHECK NEEDED");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(4, 38);
    tft.print("Result: ");
    tft.println(receivedVision.disease);

    tft.setCursor(4, 50);
    tft.print("Confidence: ");
    tft.print(receivedVision.confidence, 0);
    tft.println("%");

    tft.setCursor(4, 64);
    tft.println("Retake leaf image");

    tft.setCursor(4, 76);
    tft.println("in good lighting.");
  }

  else if (visionDataAvailable &&
           receivedVision.valid &&
           strcmp(receivedVision.disease, "Healthy") != 0)
  {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(4, 24);
    tft.println("CROP HEALTH ALERT");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(4, 38);
    tft.print("AI: ");
    tft.println(receivedVision.disease);

    tft.setCursor(4, 50);
    tft.print("Conf: ");
    tft.print(receivedVision.confidence, 0);
    tft.println("%");

    tft.setCursor(4, 66);
    tft.println("Inspect affected");
    tft.setCursor(4, 78);
    tft.println("plants and nearby");
    tft.setCursor(4, 90);
    tft.println("leaves.");

    if (heatRisk == "HIGH")
    {
      tft.setCursor(4, 106);
      tft.println("Heat stress also HIGH");
    }
    else if (droughtRisk == "HIGH")
    {
      tft.setCursor(4, 106);
      tft.println("Drought risk HIGH");
    }
  }

  // ----------------------------------------------------------
  // HEAT + DRY SOIL
  // ----------------------------------------------------------

  else if (irrigationON && heatRisk == "HIGH")
  {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(4, 24);
    tft.println("HEAT + DRY SOIL");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(4, 38);
    tft.println("Irrigation");
    tft.setCursor(4, 50);
    tft.println("recommended.");
    tft.setCursor(4, 64);
    tft.println("Protect crop from");
    tft.setCursor(4, 76);
    tft.println("heat stress.");
  }

  // ----------------------------------------------------------
  // DRY SOIL
  // ----------------------------------------------------------

  else if (irrigationON)
  {
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(4, 24);
    tft.println("DRY SOIL");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(4, 38);
    tft.println("Irrigation");
    tft.setCursor(4, 50);
    tft.println("recommended.");
  }

  // ----------------------------------------------------------
  // RAIN
  // ----------------------------------------------------------

  else if (strcmp(receivedData.rainStatus, "DRY") != 0)
  {
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(4, 24);
    tft.println("RAIN DETECTED");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(4, 38);
    tft.println("Wait before");
    tft.setCursor(4, 50);
    tft.println("irrigation.");
  }

  // ----------------------------------------------------------
  // HEALTHY / NORMAL
  // ----------------------------------------------------------

  else
  {
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(4, 24);

    if (visionDataAvailable &&
        receivedVision.valid &&
        strcmp(receivedVision.disease, "Healthy") == 0)
    {
      tft.println("CROP HEALTHY");
    }
    else
    {
      tft.println("FIELD NORMAL");
    }

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(4, 38);

    if (visionDataAvailable &&
        receivedVision.valid &&
        strcmp(receivedVision.disease, "Healthy") == 0)
    {
      tft.println("No disease detected.");
    }
    else
    {
      tft.println("No major alert now.");
    }

    tft.setCursor(4, 54);
    tft.print("Soil: ");
    tft.println(field.soilCondition);

    tft.setCursor(4, 66);
    tft.print("Rain: ");
    tft.println(field.rainCondition);

    tft.setCursor(4, 78);
    tft.print("Heat: ");
    tft.println(heatRisk);
  }
}


void updateTFT()
{
  unsigned long now = millis();

  // A new AI result should be shown immediately on the AI page.
  if (visionDataAvailable && visionDisplayPending)
  {
    displayPage = 2;
    lastDisplayChange = now;
    visionDisplayPending = false;

    displayPage3();
    return;
  }

  if (now - lastDisplayChange >= DISPLAY_PAGE_TIME)
  {
    lastDisplayChange = now;

    displayPage++;

    if (displayPage > 3)
    {
      displayPage = 0;
    }
  }

  if (displayPage == 0)
  {
    displayPage1();
  }
  else if (displayPage == 1)
  {
    displayPage2();
  }
  else if (displayPage == 2)
  {
    displayPage3();
  }
  else
  {
    displayPage4();
  }
}


// ============================================================
// RECEIVE ESP-NOW DATA
// ============================================================

void OnDataRecv(const uint8_t *mac,
                const uint8_t *incomingData,
                int len)
{
  // ==========================================================
  // VISION PACKET FROM S3-CAM
  // ==========================================================

  if (len == sizeof(VisionData))
  {
    memcpy(
      &receivedVision,
      incomingData,
      sizeof(receivedVision)
    );

    visionDataAvailable = true;
    visionDisplayPending = true;

    Serial.println();
    Serial.println("================================================");
    Serial.println("       VISION RESULT RECEIVED");
    Serial.println("================================================");

    if (receivedVision.valid)
    {
      Serial.print("Disease              : ");
      Serial.println(receivedVision.disease);

      Serial.print("Confidence           : ");
      Serial.print(receivedVision.confidence, 2);
      Serial.println("%");

      Serial.print("Low Confidence       : ");
      Serial.println(
        receivedVision.lowConfidence ? "YES" : "NO"
      );
    }
    else
    {
      Serial.println("AI inference result  : INVALID");
      Serial.println("S3-CAM reported an inference failure.");
    }

    Serial.println("================================================");

    return;
  }


  // ==========================================================
  // SENSOR PACKET FROM SENSOR NODE
  // ==========================================================

  if (len != sizeof(SensorData))
  {
    Serial.println();
    Serial.print("ERROR: Unexpected packet size: ");
    Serial.println(len);

    Serial.print("Expected SensorData: ");
    Serial.println(sizeof(SensorData));

    Serial.print("Expected VisionData: ");
    Serial.println(sizeof(VisionData));

    return;
  }


  // ----------------------------------------------------------
  // COPY SENSOR DATA
  // ----------------------------------------------------------

  memcpy(
    &receivedData,
    incomingData,
    sizeof(receivedData)
  );


  // ----------------------------------------------------------
  // SENSOR FUSION
  // ----------------------------------------------------------

  updateFieldState();


  // ----------------------------------------------------------
  // UPDATE TIME CONDITIONS
  // ----------------------------------------------------------

  updateDurations();


  // ----------------------------------------------------------
  // ENVIRONMENTAL RISKS
  // ----------------------------------------------------------

  updateEnvironmentalRisks();


  // ----------------------------------------------------------
  // IRRIGATION
  // ----------------------------------------------------------

  makeIrrigationDecision();


  // ==========================================================
  // SENSOR DATA
  // ==========================================================

  Serial.println();
  Serial.println("================================================");
  Serial.println("          SENSOR DATA RECEIVED");
  Serial.println("================================================");


  // ----------------------------------------------------------
  // SOIL
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("[ SOIL PARAMETERS ]");

  if (receivedData.soilValid)
  {
    Serial.print("Soil Moisture     : ");
    Serial.print(receivedData.soilMoisture, 2);
    Serial.println(" %");

    Serial.print("Soil Temperature  : ");
    Serial.print(receivedData.soilTemperature, 2);
    Serial.println(" C");

    Serial.print("Soil pH           : ");
    Serial.print(receivedData.soilPH, 2);
    Serial.println();

    Serial.print("Soil EC           : ");
    Serial.println(receivedData.soilEC);
  }
  else
  {
    Serial.println("Soil sensor data unavailable.");
  }


  // ----------------------------------------------------------
  // NPK
  // ----------------------------------------------------------

  if (receivedData.soilValid)
  {
    Serial.println();
    Serial.println("[ NUTRIENT STATUS ]");

    Serial.print("Nitrogen          : ");
    Serial.print(receivedData.nitrogen);
    Serial.print(" (");
    Serial.print(getNutrientStatus(receivedData.nitrogen));
    Serial.println(")");

    Serial.print("Phosphorus        : ");
    Serial.print(receivedData.phosphorus);
    Serial.print(" (");
    Serial.print(getNutrientStatus(receivedData.phosphorus));
    Serial.println(")");

    Serial.print("Potassium         : ");
    Serial.print(receivedData.potassium);
    Serial.print(" (");
    Serial.print(getNutrientStatus(receivedData.potassium));
    Serial.println(")");

    Serial.print("pH Status         : ");
    Serial.println(getPHStatus(receivedData.soilPH));

    Serial.print("EC Status         : ");
    Serial.println(getECStatus(receivedData.soilEC));
  }
  else
  {
    Serial.println("Nutrient analysis unavailable.");
  }


  // ==========================================================
  // AIR / WEATHER
  // ==========================================================

  Serial.println();
  Serial.println("[ WEATHER ]");

  if (receivedData.dhtValid)
  {
    Serial.print("Air Temperature   : ");
    Serial.print(receivedData.airTemperature, 2);
    Serial.println(" C");

    Serial.print("Air Humidity      : ");
    Serial.print(receivedData.airHumidity, 2);
    Serial.println(" %");

    Serial.print("Weather Condition : ");
    Serial.println(field.weatherCondition);
  }
  else
  {
    Serial.println("DHT22 data unavailable.");
  }


  // ==========================================================
  // RAIN / WATER
  // ==========================================================

  Serial.println();
  Serial.println("[ FIELD CONDITIONS ]");

  Serial.print("Rain              : ");
  Serial.println(receivedData.rainStatus);

  Serial.print("Water Level       : ");
  Serial.println(receivedData.waterStatus);


  // ==========================================================
  // ENVIRONMENTAL RISK
  // ==========================================================

  Serial.println();
  Serial.println("[ ENVIRONMENTAL RISK ]");

  Serial.print("Drought Risk          : ");
  Serial.println(droughtRisk);

  Serial.print("Excessive Rain Risk   : ");
  Serial.println(excessiveRainRisk);

  Serial.print("Flood/Waterlogging    : ");
  Serial.println(floodRisk);

  Serial.print("Heat Stress Risk      : ");
  Serial.println(heatRisk);


  // ==========================================================
  // DURATION INFORMATION
  // ==========================================================

  Serial.println();
  Serial.println("[ PATTERN MONITORING ]");

  Serial.print("Dry Soil Duration     : ");
  Serial.print(drySoilDuration / 1000);
  Serial.println(" seconds");

  Serial.print("Rain Duration         : ");
  Serial.print(rainDuration / 1000);
  Serial.println(" seconds");


  // ==========================================================
  // IRRIGATION
  // ==========================================================

  Serial.println();
  Serial.println("[ IRRIGATION DECISION ]");

  Serial.print("Decision              : ");
  Serial.println(irrigationDecision);

  Serial.print("Pump Simulation       : ");
  Serial.println(irrigationON ? "ON" : "OFF");


  // ==========================================================
  // SENSOR STATUS
  // ==========================================================

  Serial.println();
  Serial.println("[ SENSOR STATUS ]");

  Serial.print("7-in-1 Soil       : ");
  Serial.println(receivedData.soilValid ? "OK" : "ERROR");

  Serial.print("DHT22             : ");
  Serial.println(receivedData.dhtValid ? "OK" : "ERROR");

  Serial.println("Water Level       : OK");
  Serial.println("Raindrop Sensor   : OK");

  Serial.println();
  Serial.println("================================================");
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(2000);


  // ----------------------------------------------------------
  // IRRIGATION LED
  // ----------------------------------------------------------

  pinMode(IRRIGATION_LED_PIN, OUTPUT);

  digitalWrite(IRRIGATION_LED_PIN, LOW);

  // ----------------------------------------------------------
  // TFT DISPLAY
  // ----------------------------------------------------------

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 25);
  tft.println("KRISHI");
  tft.setCursor(10, 50);
  tft.println("BONDHU");

  tft.setTextSize(1);
  tft.setCursor(10, 80);
  tft.println("EDGE GATEWAY");
  tft.setCursor(10, 100);
  tft.println("Starting...");

  delay(1500);


  // ----------------------------------------------------------
  // WIFI
  // ----------------------------------------------------------

  WiFi.mode(WIFI_STA);

  esp_wifi_set_channel(
    WIFI_CHANNEL,
    WIFI_SECOND_CHAN_NONE
  );


  // ==========================================================
  // HEADER
  // ==========================================================

  Serial.println();
  Serial.println("================================================");
  Serial.println("              MAIN ESP32");
  Serial.println("             EDGE GATEWAY");
  Serial.println("================================================");

  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());


  // ==========================================================
  // ESP-NOW
  // ==========================================================

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW initialization FAILED!");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP-NOW initialized successfully.");
  Serial.println("Environmental Risk Engine ready.");
  Serial.println("Irrigation LED ready.");
  Serial.println("TFT display ready.");
  Serial.println("Waiting for Sensor Node / S3-CAM data...");
  Serial.println("Sensor packet size : " + String(sizeof(SensorData)));
  Serial.println("Vision packet size : " + String(sizeof(VisionData)));

  displayPage = 0;
  lastDisplayChange = millis();

  displayPage1();
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  updateTFT();
  delay(5000);
}
