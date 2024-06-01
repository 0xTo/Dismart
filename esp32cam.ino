#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "quirc.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "ssid";
const char* password = "password";
WiFiClient client;

#define MOTOR_QUANTITY 4
int motor_pins[MOTOR_QUANTITY] = {14, 15, 16, 17};

#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#endif

camera_fb_t * fb = NULL;
struct quirc *q = NULL;
uint8_t *image = NULL;
struct quirc_code code;
struct quirc_data data;
quirc_decode_error_t err;
String QRCodeResult = "";

void setup () {
  
  for (int i = 0; i < MOTOR_QUANTITY; i++) {
    pinMode(motor_pins[i], OUTPUT);
    digitalWrite(motor_pins[i], LOW);
  }

  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("[*] Connecting..\n");
  }
  Serial.print("\n\n[+] Connected\n");

  // Disable brownout detector.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Camera configuration.
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);
}

void loop() {
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  q = quirc_new();
  if (q == NULL){
    Serial.print("can't create quirc object\r\n");
    return;
  }

  quirc_resize(q, fb->width, fb->height);
  image = quirc_begin(q, NULL, NULL);
  memcpy(image, fb->buf, fb->len);
  quirc_end(q);

  int count = quirc_count(q);
  if (count > 0) {
    quirc_extract(q, 0, &code);
    err = quirc_decode(&code, &data);

    if (err){
      Serial.println("Decoding FAILED");
    } else {
      Serial.printf("Decoding successful:\n");
      dumpData(&data);
      String product_id = String((const char *)data.payload);
      HTTPClient http;

      String url = "http://167.86.100.109:5000/distribution/" + product_id;
      http.begin(client, url);

      int httpCode = http.GET();

      if (httpCode == 200) {
        Serial.println("Payload:\n");
        String payload = http.getString();
        Serial.println(payload);

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }

        // Check reservation existante
        bool exists = doc["exists"];
        if (exists) {
          int product_id = doc["product_id"];
          int quantity = doc["quantity"];

          if (product_id >= 1 && product_id <= MOTOR_QUANTITY) {
            for (int j = 0; j < quantity; j++)
              digitalWrite(motor_pins[product_id - 1], HIGH);
              delay(3000);
              for (int i = 0; i < MOTOR_QUANTITY; i++) {
                digitalWrite(motor_pins[i], LOW);
              }
              delay(1000);
          } else {
            Serial.println("Product ID not recognized");
          }
        } else {
          Serial.println("Reservation does not exist");
        }

      } else {
        Serial.println("Reservation inexistante...\n");
      }

      http.end();
    }
    Serial.println();
  }

  esp_camera_fb_return(fb);
  fb = NULL;
  image = NULL;
  quirc_destroy(q);
}

void dumpData(const struct quirc_data *data)
{
  Serial.printf("Version: %d\n", data->version);
  Serial.printf("ECC level: %c\n", "MLHQ"[data->ecc_level]);
  Serial.printf("Mask: %d\n", data->mask);
  Serial.printf("Length: %d\n", data->payload_len);
  Serial.printf("Payload: %s\n", data->payload);

  QRCodeResult = (const char *)data->payload;
}
