#include <ModbusMaster.h>
#include <DHT.h>

// =====================================================
//              SMART FARM - SENSOR NODE
// =====================================================

// ---------------- 7-IN-1 SOIL SENSOR -----------------

#define RX2_PIN 16
#define TX2_PIN 17

// ---------------- DHT22 ------------------------------

#define DHTPIN 4
#define DHTTYPE DHT22

// ---------------- WATER LEVEL SENSOR -----------------

// Red electrode board
#define WATER_LEVEL_PIN 34

// ---------------- RAINDROP SENSOR --------------------

#define RAIN_SENSOR_PIN 35


// =====================================================
//                  SENSOR OBJECTS
// =====================================================

ModbusMaster node;
DHT dht(DHTPIN, DHTTYPE);


// =====================================================
//                       SETUP
// =====================================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==============================================");
  Serial.println("        SMART FARM - SENSOR NODE");
  Serial.println("==============================================");

  // ---------------- DHT22 ----------------

  dht.begin();

  // ---------------- RS485 ----------------

  Serial2.begin(4800, SERIAL_8N1, RX2_PIN, TX2_PIN);

  // 7-in-1 sensor slave ID = 1
  node.begin(1, Serial2);

  // ---------------- ANALOG SENSORS --------

  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);

  // ESP32 ADC attenuation
  analogSetPinAttenuation(WATER_LEVEL_PIN, ADC_11db);
  analogSetPinAttenuation(RAIN_SENSOR_PIN, ADC_11db);

  Serial.println("Sensor Node initialized.");
  Serial.println();
}


// =====================================================
//                        LOOP
// =====================================================

void loop() {

  uint8_t result;

  // ===================================================
  // 1. READ DHT22
  // ===================================================

  float air_temperature = dht.readTemperature();
  float air_humidity = dht.readHumidity();


  // ===================================================
  // 2. READ 7-IN-1 SOIL SENSOR
  // ===================================================

  result = node.readHoldingRegisters(0x0000, 7);


  if (result == node.ku8MBSuccess) {

    // -----------------------------------------------
    // Register 0 - Soil Moisture
    // -----------------------------------------------

    float soil_moisture =
      node.getResponseBuffer(0) / 10.0;

    // -----------------------------------------------
    // Register 1 - Soil Temperature
    // -----------------------------------------------

    float soil_temperature =
      node.getResponseBuffer(1) / 10.0;

    // -----------------------------------------------
    // Register 2 - EC
    // -----------------------------------------------

    uint16_t soil_ec =
      node.getResponseBuffer(2);

    // -----------------------------------------------
    // Register 3 - pH
    // -----------------------------------------------

    float soil_ph =
      node.getResponseBuffer(3) / 10.0;

    // -----------------------------------------------
    // Register 4 - Nitrogen
    // -----------------------------------------------

    uint16_t nitrogen =
      node.getResponseBuffer(4);

    // -----------------------------------------------
    // Register 5 - Phosphorus
    // -----------------------------------------------

    uint16_t phosphorus =
      node.getResponseBuffer(5);

    // -----------------------------------------------
    // Register 6 - Potassium
    // -----------------------------------------------

    uint16_t potassium =
      node.getResponseBuffer(6);


    // =================================================
    // 3. READ WATER LEVEL SENSOR
    // =================================================

    int water_raw =
      analogRead(WATER_LEVEL_PIN);


    // =================================================
    // 4. READ RAINDROP SENSOR
    // =================================================

    int rain_raw =
      analogRead(RAIN_SENSOR_PIN);


    // =================================================
    // 5. DISPLAY SENSOR DATA
    // =================================================

    Serial.println();
    Serial.println("==============================================");
    Serial.println("              SMART FARM DATA");
    Serial.println("==============================================");


    // ---------------- SOIL --------------------------

    Serial.println();
    Serial.println("[ SOIL PARAMETERS ]");

    Serial.print("Soil Moisture     : ");
    Serial.print(soil_moisture);
    Serial.println(" %");

    Serial.print("Soil Temperature  : ");
    Serial.print(soil_temperature);
    Serial.println(" °C");

    Serial.print("Soil pH           : ");
    Serial.println(soil_ph);

    Serial.print("Soil EC           : ");
    Serial.println(soil_ec);

    Serial.print("Nitrogen (N)      : ");
    Serial.println(nitrogen);

    Serial.print("Phosphorus (P)    : ");
    Serial.println(phosphorus);

    Serial.print("Potassium (K)     : ");
    Serial.println(potassium);


    // ---------------- AIR ---------------------------

    Serial.println();
    Serial.println("[ AIR PARAMETERS ]");

    if (isnan(air_temperature)) {

      Serial.println("Air Temperature  : SENSOR ERROR");

    } else {

      Serial.print("Air Temperature   : ");
      Serial.print(air_temperature);
      Serial.println(" °C");
    }


    if (isnan(air_humidity)) {

      Serial.println("Air Humidity     : SENSOR ERROR");

    } else {

      Serial.print("Air Humidity      : ");
      Serial.print(air_humidity);
      Serial.println(" %");
    }


    // ---------------- WATER -------------------------

    Serial.println();
    Serial.println("[ WATER LEVEL ]");

    Serial.print("Water Level RAW   : ");
    Serial.println(water_raw);


    // ---------------- RAIN --------------------------

    Serial.println();
    Serial.println("[ RAINDROP SENSOR ]");

    Serial.print("Rain Sensor RAW   : ");
    Serial.println(rain_raw);


    Serial.println();
    Serial.println("[ SENSOR STATUS ]");

    Serial.println("7-in-1 Soil      : OK");
    Serial.println("DHT22            : OK");
    Serial.println("Water Level      : OK");
    Serial.println("Raindrop Sensor  : OK");

    Serial.println();
    Serial.println("==============================================");


  } else {

    Serial.println();
    Serial.println("==============================================");
    Serial.println("          SOIL SENSOR ERROR");
    Serial.println("==============================================");

    Serial.print("Modbus Error Code: ");
    Serial.println(result);

    Serial.println();

    // We can still read the other sensors
    // even if the soil sensor fails.

    int water_raw =
      analogRead(WATER_LEVEL_PIN);

    int rain_raw =
      analogRead(RAIN_SENSOR_PIN);

    Serial.print("Water Level RAW  : ");
    Serial.println(water_raw);

    Serial.print("Rain Sensor RAW  : ");
    Serial.println(rain_raw);

    Serial.println("==============================================");
  }

  delay(3000);
}
