#include <Arduino.h>

#include <Chirale_TensorFlowLite.h>

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "esp_heap_caps.h"
#include "esp_camera.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "model_data.h"


// ============================================================
// ESP-NOW SETTINGS
// ============================================================

#define WIFI_CHANNEL 1

// Main ESP32 MAC address
uint8_t mainESP32MAC[] =
{
  0x6C, 0xC8, 0x40, 0x4E, 0x4C, 0x10
};


// ============================================================
// SETTINGS
// ============================================================

constexpr size_t TENSOR_ARENA_SIZE = 6 * 1024 * 1024;

constexpr int IMAGE_SIZE = 128;
constexpr int CHANNELS = 3;
constexpr int NUM_CLASSES = 10;


// ============================================================
// CAMERA PINS
// OV3660 - ESP32-S3-CAM
// ============================================================

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1

#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM      4
#define SIOC_GPIO_NUM      5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM        8
#define Y3_GPIO_NUM        9
#define Y2_GPIO_NUM       11

#define VSYNC_GPIO_NUM     6
#define HREF_GPIO_NUM      7
#define PCLK_GPIO_NUM      13


// ============================================================
// MODEL CLASSES
// ============================================================

const char* classNames[NUM_CLASSES] =
{
  "Bacterial Spot",
  "Early Blight",
  "Healthy",
  "Late Blight",
  "Leaf Mold",
  "Mosaic Virus",
  "Septoria Leaf Spot",
  "Spider Mites",
  "Target Spot",
  "Yellow Leaf Curl Virus"
};


// ============================================================
// GLOBALS
// ============================================================

uint8_t* tensorArena = nullptr;

const tflite::Model* model = nullptr;

tflite::MicroInterpreter* interpreter = nullptr;

TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;


// ============================================================
// ESP-NOW VISION PACKET
// ============================================================

typedef struct
{
  char disease[32];
  float confidence;
  bool valid;
  bool lowConfidence;
} VisionData;

VisionData visionData;


// ============================================================
// ESP-NOW SEND CALLBACK
// Compatible with ESP32 Arduino core 2.0.17
// ============================================================

void OnDataSent(
  const uint8_t *mac_addr,
  esp_now_send_status_t status)
{
  Serial.print("ESP-NOW send status: ");

  if (status == ESP_NOW_SEND_SUCCESS)
  {
    Serial.println("SUCCESS");
  }
  else
  {
    Serial.println("FAILED");
  }
}


// ============================================================
// INITIALIZE ESP-NOW
// ============================================================

bool initESPNow()
{
  Serial.println();
  Serial.println("[ESP-NOW] Initializing...");

  WiFi.mode(WIFI_STA);

  // Keep the S3-CAM on the same channel as the Main ESP32.
  esp_err_t channelResult =
    esp_wifi_set_channel(
      WIFI_CHANNEL,
      WIFI_SECOND_CHAN_NONE
    );

  if (channelResult != ESP_OK)
  {
    Serial.println("[ESP-NOW] ERROR: Failed to set WiFi channel.");
    return false;
  }

  Serial.print("[ESP-NOW] S3-CAM MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("[ESP-NOW] ERROR: Initialization failed.");
    return false;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};

  memcpy(
    peerInfo.peer_addr,
    mainESP32MAC,
    6
  );

  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("[ESP-NOW] ERROR: Failed to add Main ESP32.");
    return false;
  }

  memset(
    &visionData,
    0,
    sizeof(visionData)
  );

  Serial.println("[ESP-NOW] Ready.");
  Serial.println("[ESP-NOW] Target: Main ESP32");
  Serial.println();

  return true;
}


// ============================================================
// SEND VISION RESULT TO MAIN ESP32
// ============================================================

void sendVisionResult()
{
  esp_err_t result =
    esp_now_send(
      mainESP32MAC,
      (uint8_t*)&visionData,
      sizeof(visionData)
    );

  if (result == ESP_OK)
  {
    Serial.println("[ESP-NOW] Vision result queued.");
  }
  else
  {
    Serial.print("[ESP-NOW] Send error: ");
    Serial.println(result);
  }
}


// ============================================================
// CAMERA INITIALIZATION
// ============================================================

bool initCamera()
{
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  // Data pins
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  // Control pins
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;

  // We need RGB pixels, not JPEG
  config.pixel_format = PIXFORMAT_RGB565;

  // Camera frame = 320 x 240
  config.frame_size = FRAMESIZE_QVGA;

  config.jpeg_quality = 12;

  config.fb_count = 1;

  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  // Use PSRAM for camera framebuffer
  if (psramFound())
  {
    config.fb_location = CAMERA_FB_IN_PSRAM;
  }
  else
  {
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK)
  {
    Serial.print("Camera init failed. Error: 0x");
    Serial.println(err, HEX);

    return false;
  }

  Serial.println("Camera initialized successfully.");

  sensor_t* sensor = esp_camera_sensor_get();

  if (sensor != nullptr)
  {
    Serial.print("Camera PID: 0x");
    Serial.println(sensor->id.PID, HEX);
  }

  return true;
}


// ============================================================
// RGB565 → RGB888
// ============================================================

void rgb565ToRgb888(
  uint16_t pixel,
  uint8_t& r,
  uint8_t& g,
  uint8_t& b
)
{
  // RGB565:
  // R = 5 bits
  // G = 6 bits
  // B = 5 bits

  r = ((pixel >> 11) & 0x1F) << 3;
  g = ((pixel >> 5)  & 0x3F) << 2;
  b = (pixel & 0x1F) << 3;

  // Improve 5/6-bit values to full 8-bit range
  r |= r >> 5;
  g |= g >> 6;
  b |= b >> 5;
}


// ============================================================
// CAPTURE + PREPROCESS CAMERA IMAGE
// ============================================================

bool captureAndPrepareImage()
{
  Serial.println();
  Serial.println("==============================================");
  Serial.println(" CAPTURING REAL CAMERA IMAGE");
  Serial.println("==============================================");

  Serial.println("Capturing OV3660 frame...");

  camera_fb_t* fb = esp_camera_fb_get();

  if (fb == nullptr)
  {
    Serial.println("ERROR: Camera capture failed!");
    return false;
  }

  Serial.println("Camera frame captured.");

  Serial.print("Frame width: ");
  Serial.println(fb->width);

  Serial.print("Frame height: ");
  Serial.println(fb->height);

  Serial.print("Frame length: ");
  Serial.println(fb->len);

  Serial.println("Pixel format: RGB565");


  // ----------------------------------------------------------
  // Expected camera size
  // ----------------------------------------------------------

  if (fb->width != 320 || fb->height != 240)
  {
    Serial.println("ERROR: Unexpected camera resolution.");

    esp_camera_fb_return(fb);

    return false;
  }


  // ----------------------------------------------------------
  // CENTER CROP
  //
  // 320 x 240
  //
  // We take the center 240 x 240 area:
  //
  // x = 40 ... 279
  // y = 0  ... 239
  // ----------------------------------------------------------

  const int cropSize = 240;

  const int cropX = (320 - cropSize) / 2;
  const int cropY = (240 - cropSize) / 2;


  Serial.println("Center cropping 240x240...");


  // ----------------------------------------------------------
  // RESIZE
  //
  // 240 x 240 → 128 x 128
  //
  // Nearest-neighbor resizing.
  // ----------------------------------------------------------

  Serial.println("Resizing to 128x128...");


  for (int y = 0; y < IMAGE_SIZE; y++)
  {
    int srcY =
      cropY +
      (y * cropSize) / IMAGE_SIZE;

    for (int x = 0; x < IMAGE_SIZE; x++)
    {
      int srcX =
        cropX +
        (x * cropSize) / IMAGE_SIZE;


      // RGB565 uses 2 bytes per pixel
      int pixelIndex =
        (srcY * fb->width + srcX) * 2;


      // RGB565 byte order
      uint16_t pixel =
        ((uint16_t)fb->buf[pixelIndex] << 8) |
        ((uint16_t)fb->buf[pixelIndex + 1]);


      // Convert RGB565 → RGB888
      uint8_t r;
      uint8_t g;
      uint8_t b;

      rgb565ToRgb888(pixel, r, g, b);


      // ------------------------------------------------------
      // MODEL QUANTIZATION
      //
      // Input:
      //   scale = 1.0
      //   zero point = -128
      //
      // Quantization:
      //
      // int8 = pixel / 1.0 - 128
      //
      // Therefore:
      //   0   → -128
      //   128 → 0
      //   255 → 127
      // ------------------------------------------------------

      int index =
        (y * IMAGE_SIZE + x) * CHANNELS;

      input->data.int8[index + 0] =
        (int8_t)((int)r - 128);

      input->data.int8[index + 1] =
        (int8_t)((int)g - 128);

      input->data.int8[index + 2] =
        (int8_t)((int)b - 128);
    }
  }


  // Return camera framebuffer
  esp_camera_fb_return(fb);

  Serial.println("Image preprocessing complete.");

  return true;
}


// ============================================================
// RUN AI INFERENCE
// ============================================================

bool runInference()
{
  Serial.println();
  Serial.println("==============================================");
  Serial.println(" RUNNING INT8 AI INFERENCE");
  Serial.println("==============================================");

  unsigned long startTime = micros();

  TfLiteStatus status = interpreter->Invoke();

  unsigned long elapsed = micros() - startTime;

  if (status != kTfLiteOk)
  {
    Serial.println("ERROR: AI inference failed!");

    memset(
      &visionData,
      0,
      sizeof(visionData)
    );

    visionData.valid = false;
    visionData.lowConfidence = true;

    sendVisionResult();

    return false;
  }

  Serial.println("Inference successful.");

  Serial.print("Inference time: ");
  Serial.print(elapsed / 1000.0, 2);
  Serial.println(" ms");


  // ==========================================================
  // FIND BEST PREDICTION
  // ==========================================================

  int bestClass = 0;

  float bestScore = -1000.0;

  Serial.println();
  Serial.println("PREDICTION SCORES");
  Serial.println("----------------------------------------------");


  for (int i = 0; i < NUM_CLASSES; i++)
  {
    int8_t q = output->data.int8[i];

    // Dequantize INT8 output
    float score =
      (static_cast<int>(q) - output->params.zero_point)
      * output->params.scale;


    // Safety clamp
    if (score < 0.0)
      score = 0.0;

    if (score > 1.0)
      score = 1.0;


    Serial.print(classNames[i]);
    Serial.print(" = ");
    Serial.print(score * 100.0, 2);
    Serial.println("%");


    if (score > bestScore)
    {
      bestScore = score;
      bestClass = i;
    }
  }


  // ==========================================================
  // CONFIDENCE
  // ==========================================================

  float confidence = bestScore * 100.0;


  // ==========================================================
  // PREPARE VISION RESULT FOR MAIN ESP32
  // ==========================================================

  memset(
    &visionData,
    0,
    sizeof(visionData)
  );

  strncpy(
    visionData.disease,
    classNames[bestClass],
    sizeof(visionData.disease) - 1
  );

  visionData.disease[
    sizeof(visionData.disease) - 1
  ] = '\0';

  visionData.confidence = confidence;
  visionData.valid = true;
  visionData.lowConfidence =
    (confidence < 70.0);


  Serial.println();
  Serial.println("----------------------------------------------");

  Serial.print("Prediction: ");
  Serial.println(classNames[bestClass]);

  Serial.print("Confidence: ");
  Serial.print(confidence, 2);
  Serial.println("%");


  // ==========================================================
  // CONFIDENCE THRESHOLD
  // ==========================================================

  const float CONFIDENCE_THRESHOLD = 70.0;


  Serial.println();
  Serial.println("==============================================");
  Serial.println(" AI DECISION");
  Serial.println("==============================================");


  // ==========================================================
  // LOW CONFIDENCE
  // ==========================================================

  if (confidence < CONFIDENCE_THRESHOLD)
  {
    Serial.println("STATUS: LOW CONFIDENCE");

    Serial.println();
    Serial.println("ADVISORY:");
    Serial.println("Image is unclear for reliable diagnosis.");
    Serial.println("Please capture another image.");
    Serial.println("Keep the tomato leaf centered.");
    Serial.println("Ensure good lighting.");
    Serial.println("Keep the leaf close to the camera.");

    // Send the low-confidence result to the Main ESP32.
    sendVisionResult();

    return true;
  }


  // ==========================================================
  // HEALTHY
  // ==========================================================

  if (bestClass == 2)
  {
    Serial.println("STATUS: HEALTHY");

    Serial.println();
    Serial.println("ADVISORY:");
    Serial.println("No major tomato disease detected.");
    Serial.println("Continue regular field monitoring.");
  }


  // ==========================================================
  // SPIDER MITES
  // ==========================================================

  else if (bestClass == 7)
  {
    Serial.println("STATUS: PREDICTION DETECTED");

    Serial.println();
    Serial.println("RESULT: Spider Mites");

    Serial.println();
    Serial.println("ADVISORY:");
    Serial.println("Inspect the underside of leaves.");
    Serial.println("Check nearby plants for spread.");
    Serial.println("Take appropriate control action.");
  }


  // ==========================================================
  // BACTERIAL SPOT
  // ==========================================================

  else if (bestClass == 0)
  {
    Serial.println("STATUS: DISEASE DETECTED");

    Serial.println();
    Serial.println("DISEASE: Bacterial Spot");

    Serial.println();
    Serial.println("ADVISORY:");
    Serial.println("Remove severely affected leaves.");
    Serial.println("Avoid unnecessary leaf wetness.");
    Serial.println("Monitor nearby plants for symptoms.");
  }


  // ==========================================================
  // EARLY BLIGHT
  // ==========================================================

  else if (bestClass == 1)
  {
    Serial.println("STATUS: DISEASE DETECTED");

    Serial.println();
    Serial.println("DISEASE: Early Blight");

    Serial.println();
    Serial.println("ADVISORY:");
    Serial.println("Inspect lower leaves for spreading lesions.");
    Serial.println("Remove severely affected foliage.");
    Serial.println("Improve field airflow.");
  }


  // ==========================================================
  // LATE BLIGHT
  // ==========================================================

  else if (bestClass == 3)
  {
    Serial.println("STATUS: DISEASE DETECTED");

    Serial.println();
    Serial.println("DISEASE: Late Blight");

    Serial.println();
    Serial.println("ADVISORY:");
    Serial.println("Inspect surrounding plants immediately.");
    Serial.println("Remove severely affected plant material.");
    Serial.println("Avoid prolonged leaf wetness.");
  }


  // ==========================================================
  // LEAF MOLD
  // ==========================================================

  else if (bestClass == 4)
  {
    Serial.println("STATUS: DISEASE DETECTED");

    Serial.println();
    Serial.println("DISEASE: Leaf Mold");

    Serial.println();
    Serial.println("ADVISORY:");
    Serial.println("Check leaves for further spread.");
    Serial.println("Improve ventilation around plants.");
    Serial.println("Avoid excessive humidity.");
  }


  // ==========================================================
  // MOSAIC VIRUS
  // ==========================================================

  else if (bestClass == 5)
  {
    Serial.println("STATUS: DISEASE DETECTED");

    Serial.println();
    Serial.println("DISEASE: Tomato Mosaic Virus");

    Serial.println();
    Serial.println("ADVISORY:");
    Serial.println("Isolate suspicious plants.");
    Serial.println("Remove severely affected plants.");
    Serial.println("Disinfect tools after handling plants.");
  }


  // ==========================================================
  // SEPTORIA LEAF SPOT
  // ==========================================================

  else if (bestClass == 6)
  {
    Serial.println("STATUS: DISEASE DETECTED");

    Serial.println();
    Serial.println("DISEASE: Septoria Leaf Spot");

    Serial.println();
    Serial.println("ADVISORY:");
    Serial.println("Remove severely affected leaves.");
    Serial.println("Avoid overhead watering.");
    Serial.println("Monitor nearby plants.");
  }


  // ==========================================================
  // TARGET SPOT
  // ==========================================================

  else if (bestClass == 8)
  {
    Serial.println("STATUS: DISEASE DETECTED");

    Serial.println();
    Serial.println("DISEASE: Target Spot");

    Serial.println();
    Serial.println("ADVISORY:");
    Serial.println("Inspect nearby leaves for similar lesions.");
    Serial.println("Remove severely affected foliage.");
    Serial.println("Improve airflow around plants.");
  }


  // ==========================================================
  // YELLOW LEAF CURL VIRUS
  // ==========================================================

  else if (bestClass == 9)
  {
    Serial.println("STATUS: DISEASE DETECTED");

    Serial.println();
    Serial.println("DISEASE: Tomato Yellow Leaf Curl Virus");

    Serial.println();
    Serial.println("ADVISORY:");
    Serial.println("Inspect nearby plants for similar symptoms.");
    Serial.println("Check plants for whitefly activity.");
    Serial.println("Isolate severely affected plants.");
  }


  // ==========================================================
  // COMPLETE
  // ==========================================================

  Serial.println();
  Serial.println("==============================================");
  Serial.println(" AI ANALYSIS COMPLETE");
  Serial.println("==============================================");

  // Send the final prediction to the Main ESP32.
  sendVisionResult();

  return true;
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(2000);


  Serial.println();

  Serial.println("==============================================");
  Serial.println(" KRISHI SAATHI");
  Serial.println(" ESP32-S3 CAMERA + INT8 AI");
  Serial.println("==============================================");


  // ==========================================================
  // ESP-NOW
  // ==========================================================

  if (!initESPNow())
  {
    Serial.println("ERROR: ESP-NOW initialization failed!");

    while (1)
    {
      delay(1000);
    }
  }


  // ==========================================================
  // PSRAM
  // ==========================================================

  Serial.println();

  Serial.println("[1] Checking PSRAM...");

  if (!psramFound())
  {
    Serial.println("ERROR: PSRAM NOT FOUND!");

    while (1)
    {
      delay(1000);
    }
  }

  Serial.println("PSRAM FOUND");

  Serial.print("Free PSRAM: ");

  Serial.print(
    ESP.getFreePsram() / 1024
  );

  Serial.println(" KB");


  // ==========================================================
  // CAMERA
  // ==========================================================

  Serial.println();

  Serial.println("[2] Initializing OV3660 camera...");

  if (!initCamera())
  {
    Serial.println("ERROR: Camera initialization failed!");

    while (1)
    {
      delay(1000);
    }
  }


  // ==========================================================
  // TENSOR ARENA
  // ==========================================================

  Serial.println();

  Serial.println("[3] Allocating tensor arena...");

  tensorArena =
    (uint8_t*)heap_caps_malloc(
      TENSOR_ARENA_SIZE,
      MALLOC_CAP_SPIRAM |
      MALLOC_CAP_8BIT
    );


  if (tensorArena == nullptr)
  {
    Serial.println(
      "ERROR: Tensor arena allocation failed!"
    );

    while (1)
    {
      delay(1000);
    }
  }


  Serial.println(
    "Tensor arena allocated."
  );


  Serial.print("Arena size: ");

  Serial.print(
    TENSOR_ARENA_SIZE / 1024
  );

  Serial.println(" KB");


  // ==========================================================
  // LOAD MODEL
  // ==========================================================

  Serial.println();

  Serial.println("[4] Loading model...");

  model =
    tflite::GetModel(
      krishi_saathi_tomato_model
    );


  if (model == nullptr)
  {
    Serial.println("ERROR: Model is NULL!");

    while (1)
    {
      delay(1000);
    }
  }


  Serial.println("Model loaded.");

  Serial.print(
    "Model schema version: "
  );

  Serial.println(
    model->version()
  );


  // ==========================================================
  // OPERATOR RESOLVER
  // ==========================================================

  Serial.println();

  Serial.println(
    "[5] Registering operators..."
  );


  static tflite::MicroMutableOpResolver<7>
    resolver;


  resolver.AddConv2D();

  resolver.AddDepthwiseConv2D();

  resolver.AddPad();

  resolver.AddAdd();

  resolver.AddMean();

  resolver.AddFullyConnected();

  resolver.AddSoftmax();


  Serial.println(
    "7 operators registered."
  );


  // ==========================================================
  // INTERPRETER
  // ==========================================================

  Serial.println();

  Serial.println(
    "[6] Creating interpreter..."
  );


  static tflite::MicroInterpreter
    staticInterpreter(
      model,
      resolver,
      tensorArena,
      TENSOR_ARENA_SIZE
    );


  interpreter =
    &staticInterpreter;


  Serial.println(
    "Interpreter created."
  );


  // ==========================================================
  // ALLOCATE TENSORS
  // ==========================================================

  Serial.println();

  Serial.println(
    "[7] Allocating tensors..."
  );


  if (
    interpreter->AllocateTensors()
    != kTfLiteOk
  )
  {
    Serial.println(
      "ERROR: AllocateTensors() failed!"
    );

    while (1)
    {
      delay(1000);
    }
  }


  Serial.println(
    "Tensors allocated successfully."
  );


  // ==========================================================
  // INPUT / OUTPUT
  // ==========================================================

  input =
    interpreter->input(0);

  output =
    interpreter->output(0);


  Serial.println();

  Serial.println(
    "=============================================="
  );

  Serial.println(
    " MODEL INFORMATION"
  );

  Serial.println(
    "=============================================="
  );


  // Input type

  Serial.print(
    "Input type: "
  );

  if (
    input->type == kTfLiteInt8
  )
  {
    Serial.println("INT8");
  }
  else
  {
    Serial.println("NOT INT8");
  }


  // Input shape

  Serial.print(
    "Input shape: "
  );


  for (
    int i = 0;
    i < input->dims->size;
    i++
  )
  {
    Serial.print(
      input->dims->data[i]
    );

    if (
      i <
      input->dims->size - 1
    )
    {
      Serial.print(" x ");
    }
  }

  Serial.println();


  // Input quantization

  Serial.print(
    "Input scale: "
  );

  Serial.println(
    input->params.scale,
    6
  );


  Serial.print(
    "Input zero point: "
  );

  Serial.println(
    input->params.zero_point
  );


  // Output type

  Serial.print(
    "Output type: "
  );


  if (
    output->type == kTfLiteInt8
  )
  {
    Serial.println("INT8");
  }
  else
  {
    Serial.println("NOT INT8");
  }


  // Output quantization

  Serial.print(
    "Output scale: "
  );

  Serial.println(
    output->params.scale,
    6
  );


  Serial.print(
    "Output zero point: "
  );

  Serial.println(
    output->params.zero_point
  );


  // ==========================================================
  // REAL CAMERA IMAGE
  // ==========================================================

  if (!captureAndPrepareImage())
  {
    Serial.println(
      "ERROR: Image preparation failed!"
    );

    while (1)
    {
      delay(1000);
    }
  }


  // ==========================================================
  // AI INFERENCE
  // ==========================================================

  if (!runInference())
  {
    Serial.println(
      "ERROR: AI inference failed!"
    );

    while (1)
    {
      delay(1000);
    }
  }


  // ==========================================================
  // MEMORY
  // ==========================================================

  Serial.println();

  Serial.println(
    "=============================================="
  );

  Serial.println(
    " MEMORY"
  );

  Serial.println(
    "=============================================="
  );


  Serial.print(
    "Free PSRAM: "
  );

  Serial.print(
    ESP.getFreePsram() / 1024
  );

  Serial.println(" KB");


  Serial.print(
    "Free Heap: "
  );

  Serial.print(
    ESP.getFreeHeap() / 1024
  );

  Serial.println(" KB");


  // ==========================================================
  // COMPLETE
  // ==========================================================

  Serial.println();

  Serial.println(
    "=============================================="
  );

  Serial.println(
    " REAL CAMERA AI TEST COMPLETE"
  );

  Serial.println(
    "=============================================="
  );
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  // One-shot camera + AI test for now.
  // Continuous capture/inference will be added during final integration.

  delay(1000);
}
