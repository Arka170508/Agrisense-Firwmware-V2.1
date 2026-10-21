#include <Adafruit_ADS1X15.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <HardwareSerial.h>
#include <MakerindoWiFiManager.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <esp_idf_version.h>
#include <esp_err.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <time.h>

#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

constexpr char FW_NAME[] = "AgriSense V2.0 MQTT";
constexpr char FW_VERSION[] = "2.0.0";
constexpr char DEVICE_ID[] = "AGRISENSE-CC-004";
constexpr char MQTT_BROKER[] = "broker.emqx.io";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_TOPIC[] = "agrisense/iot/readings";
constexpr char MQTT_CONFIG_TOPIC[] = "agrisense/iot/config";
constexpr char PREFERENCES_NAMESPACE[] = "agrisense";
constexpr char WIFI_PREFERENCES_NAMESPACE[] = "wifi-config";
constexpr char WIFI_AP_SSID[] = "AgriSense-Setup";
constexpr char WIFI_AP_PASSWORD[] = "Agrisense";

// Isi tiga nilai ini setelah URL repo dan nama asset release GitHub tersedia.
constexpr char GITHUB_REPOSITORY_OWNER[] = "Arka170508";
constexpr char GITHUB_REPOSITORY_NAME[] = "agrisense-firmware";
constexpr char GITHUB_FIRMWARE_ASSET[] = "firmware.bin";
// CA certificates used by GitHub's API and redirected release asset hosts.
constexpr char GITHUB_ROOT_CA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIIGTDCCBDSgAwIBAgIQOXpmzCdWNi4NqofKbqvjsTANBgkqhkiG9w0BAQwFADBf
MQswCQYDVQQGEwJHQjEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQD
Ey1TZWN0aWdvIFB1YmxpYyBTZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBSNDYw
HhcNMjEwMzIyMDAwMDAwWhcNMzYwMzIxMjM1OTU5WjBgMQswCQYDVQQGEwJHQjEY
MBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTcwNQYDVQQDEy5TZWN0aWdvIFB1Ymxp
YyBTZXJ2ZXIgQXV0aGVudGljYXRpb24gQ0EgRFYgUjM2MIIBojANBgkqhkiG9w0B
AQEFAAOCAY8AMIIBigKCAYEAljZf2HIz7+SPUPQCQObZYcrxLTHYdf1ZtMRe7Yeq
RPSwygz16qJ9cAWtWNTcuICc++p8Dct7zNGxCpqmEtqifO7NvuB5dEVexXn9RFFH
12Hm+NtPRQgXIFjx6MSJcNWuVO3XGE57L1mHlcQYj+g4hny90aFh2SCZCDEVkAja
EMMfYPKuCjHuuF+bzHFb/9gV8P9+ekcHENF2nR1efGWSKwnfG5RawlkaQDpRtZTm
M64TIsv/r7cyFO4nSjs1jLdXYdz5q3a4L0NoabZfbdxVb+CUEHfB0bpulZQtH1Rv
38e/lIdP7OTTIlZh6OYL6NhxP8So0/sht/4J9mqIGxRFc0/pC8suja+wcIUna0HB
pXKfXTKpzgis+zmXDL06ASJf5E4A2/m+Hp6b84sfPAwQ766rI65mh50S0Di9E3Pn
2WcaJc+PILsBmYpgtmgWTR9eV9otfKRUBfzHUHcVgarub/XluEpRlTtZudU5xbFN
xx/DgMrXLUAPaI60fZ6wA+PTAgMBAAGjggGBMIIBfTAfBgNVHSMEGDAWgBRWc1hk
lfmSGrASKgRieaFAFYghSTAdBgNVHQ4EFgQUaMASFhgOr872h6YyV6NGUV3LBycw
DgYDVR0PAQH/BAQDAgGGMBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYDVR0lBBYwFAYI
KwYBBQUHAwEGCCsGAQUFBwMCMBsGA1UdIAQUMBIwBgYEVR0gADAIBgZngQwBAgEw
VAYDVR0fBE0wSzBJoEegRYZDaHR0cDovL2NybC5zZWN0aWdvLmNvbS9TZWN0aWdv
UHVibGljU2VydmVyQXV0aGVudGljYXRpb25Sb290UjQ2LmNybDCBhAYIKwYBBQUH
AQEEeDB2ME8GCCsGAQUFBzAChkNodHRwOi8vY3J0LnNlY3RpZ28uY29tL1NlY3Rp
Z29QdWJsaWNTZXJ2ZXJBdXRoZW50aWNhdGlvblJvb3RSNDYucDdjMCMGCCsGAQUF
BzABhhdodHRwOi8vb2NzcC5zZWN0aWdvLmNvbTANBgkqhkiG9w0BAQwFAAOCAgEA
YtOC9Fy+TqECFw40IospI92kLGgoSZGPOSQXMBqmsGWZUQ7rux7cj1du6d9rD6C8
ze1B2eQjkrGkIL/OF1s7vSmgYVafsRoZd/IHUrkoQvX8FZwUsmPu7amgBfaY3g+d
q1x0jNGKb6I6Bzdl6LgMD9qxp+3i7GQOnd9J8LFSietY6Z4jUBzVoOoz8iAU84OF
h2HhAuiPw1ai0VnY38RTI+8kepGWVfGxfBWzwH9uIjeooIeaosVFvE8cmYUB4TSH
5dUyD0jHct2+8ceKEtIoFU/FfHq/mDaVnvcDCZXtIgitdMFQdMZaVehmObyhRdDD
4NQCs0gaI9AAgFj4L9QtkARzhQLNyRf87Kln+YU0lgCGr9HLg3rGO8q+Y4ppLsOd
unQZ6ZxPNGIfOApbPVf5hCe58EZwiWdHIMn9lPP6+F404y8NNugbQixBber+x536
WrZhFZLjEkhp7fFXf9r32rNPfb74X/U90Bdy4lzp3+X1ukh1BuMxA/EEhDoTOS3l
7ABvc7BYSQubQ2490OcdkIzUh3ZwDrakMVrbaTxUM2p24N6dB+ns2zptWCva6jzW
r8IWKIMxzxLPv5Kt3ePKcUdvkBU/smqujSczTzzSjIoR5QqQA6lN1ZRSnuHIWCvh
JEltkYnTAH41QJ6SAWO66GrrUESwN/cgZzL4JLEqz1Y=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICOjCCAcGgAwIBAgIQQvLM2htpN0RfFf51KBC49DAKBggqhkjOPQQDAzBfMQsw
CQYDVQQGEwJHQjEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQDEy1T
ZWN0aWdvIFB1YmxpYyBTZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBFNDYwHhcN
MjEwMzIyMDAwMDAwWhcNNDYwMzIxMjM1OTU5WjBfMQswCQYDVQQGEwJHQjEYMBYG
A1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQDEy1TZWN0aWdvIFB1YmxpYyBT
ZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBFNDYwdjAQBgcqhkjOPQIBBgUrgQQA
IgNiAAR2+pmpbiDt+dd34wc7qNs9Xzjoq1WmVk/WSOrsfy2qw7LFeeyZYX8QeccC
WvkEN/U0NSt3zn8gj1KjAIns1aeibVvjS5KToID1AZTc8GgHHs3u/iVStSBDHBv+
6xnOQ6OjQjBAMB0GA1UdDgQWBBTRItpMWfFLXyY4qp3W7usNw/upYTAOBgNVHQ8B
Af8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAwNnADBkAjAn7qRa
qCG76UeXlImldCBteU/IvZNeWBj7LRoAasm4PdCkT0RHlAFWovgzJQxC36oCMB3q
4S6ILuH5px0CMk7yn2xVdOOurvulGu7t0vzCAxHrRVxgED1cf5kDW21USAGKcw==
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)PEM";

constexpr uint32_t SERIAL_BAUDRATE = 115200;
constexpr uint32_t SEND_INTERVAL_MS = 900000;
constexpr uint32_t MIN_SEND_INTERVAL_MS = 1000;
constexpr uint32_t MAX_SEND_INTERVAL_MS = 3600000;
constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 5000;
constexpr uint32_t OTA_CHECK_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;
constexpr uint32_t WDT_TIMEOUT_MS = 60000;
constexpr uint32_t GMT_OFFSET_SEC = 7 * 3600;
constexpr uint32_t DAYLIGHT_OFFSET_SEC = 0;
constexpr char NTP_SERVER_1[] = "pool.ntp.org";
constexpr char NTP_SERVER_2[] = "time.nist.gov";

constexpr uint8_t I2C_SDA_PIN = 8;
constexpr uint8_t I2C_SCL_PIN = 9;
constexpr uint8_t BATTERY_ADC_PIN = 5;
constexpr uint8_t WIFI_LED_PIN = 4;
constexpr uint8_t DHT_PIN = 7;
constexpr uint8_t DHT_TYPE = DHT22;
constexpr uint8_t ADS_ADDRESS = 0x48;
constexpr uint8_t MQ4_ADC_CHANNEL = 1;
constexpr uint8_t MQ135_ADC_CHANNEL = 2;
constexpr uint8_t NO2_ADC_CHANNEL = 3;
constexpr uint8_t GPS_RX_PIN = 1;
constexpr uint8_t GPS_TX_PIN = 15;
constexpr uint8_t ANEMOMETER_RX_PIN = 18;
constexpr uint8_t ANEMOMETER_TX_PIN = 17;
constexpr uint32_t GPS_BAUD = 9600;
constexpr uint32_t ANEMOMETER_BAUD = 4800;
constexpr uint32_t SENSOR_SAMPLE_INTERVAL_MS = 2000;
constexpr uint32_t MIN_VALID_ADC = 260;
constexpr uint32_t MAX_VALID_ADC = 26600;
constexpr float BATTERY_DIVIDER_RATIO = 4.066f;
constexpr float BATTERY_CALIBRATION_FACTOR = 0.9899f;
constexpr float BATTERY_LOW_VOLTAGE = 11.0f;
constexpr float BATTERY_HIGH_VOLTAGE = 12.8f;
constexpr uint8_t BATTERY_OVERSAMPLING_COUNT = 16;
constexpr float SENSOR_VCC = 5.0f;
constexpr float RL_CO2 = 10.0f;
constexpr float R0_CO2 = 4.909f;
constexpr float CURVE_A_CO2 = 110.7432567f;
constexpr float CURVE_B_CO2 = -2.856935538f;
constexpr float RL_CH4 = 10.0f;
constexpr float R0_CH4 = 11.393f;
constexpr float CURVE_A_CH4 = 1012.2f;
constexpr float CURVE_B_CH4 = -2.786f;
constexpr float RL_NO2 = 4.7f;
constexpr float R0_NO2 = 67.581f;
constexpr float CURVE_A_NO2 = 0.0907f;
constexpr float CURVE_B_NO2 = -0.82f;

const uint8_t ANEMOMETER_REQUEST[] = {
    0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};

struct GlobalConfiguration {
  uint32_t publishIntervalMs = SEND_INTERVAL_MS;
};

struct SensorData {
  String messageId;
  String timestamp;
  float latitude = 0.0f;
  float longitude = 0.0f;
  float altitudeM = 0.0f;
  float co2Ppm = NAN;
  float ch4Ppm = NAN;
  float no2Ppb = NAN;
  float airTemperatureC = NAN;
  float airHumidityPercent = NAN;
  float windSpeedKmh = NAN;
  float batteryVoltage = NAN;
  int batteryPercent = 0;
  int rssiDbm = -999;
  String nodeStatus;
  String sensorStatus;
};

struct FirmwareVersion {
  uint32_t major;
  uint32_t minor;
  uint32_t patch;
};

Preferences preferences;
DHT dht(DHT_PIN, DHT_TYPE);
HardwareSerial gpsSerial(2);
HardwareSerial anemometerSerial(1);
TinyGPSPlus gps;
Adafruit_ADS1115 ads;
WiFiClient mqttNetworkClient;
PubSubClient mqttClient(mqttNetworkClient);
MakerindoWiFiManager wifiManager;
WiFiClientSecure otaClient;
HTTPUpdate httpUpdater;

SemaphoreHandle_t gpsMutex = nullptr;
SemaphoreHandle_t anemometerMutex = nullptr;
portMUX_TYPE dhtReadingMux = portMUX_INITIALIZER_UNLOCKED;
GlobalConfiguration globalConfig;
float currentLatitude = 0.0f;
float currentLongitude = 0.0f;
float currentAltitudeM = 0.0f;
float currentWindSpeedKmh = NAN;
float currentAirTemperatureC = NAN;
float currentAirHumidityPercent = NAN;
bool adsReady = false;
bool rtcSynced = false;
bool ntpConfigured = false;
bool preferencesReady = false;
uint32_t bootCount = 0;
uint32_t lastPublishAt = 0;
uint32_t lastMqttAttemptAt = 0;
uint32_t lastOtaCheckAt = 0;
bool lastWifiLedState = false;

void logError(const char* message) {
  Serial.printf("[ERROR] %s\n", message);
}

void sensorTask(void* parameter) {
  (void)parameter;
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t interval = pdMS_TO_TICKS(SENSOR_SAMPLE_INTERVAL_MS);

  for (;;) {
    const float humidity = dht.readHumidity();
    const float temperature = dht.readTemperature();

    portENTER_CRITICAL(&dhtReadingMux);
    currentAirHumidityPercent = humidity;
    currentAirTemperatureC = temperature;
    portEXIT_CRITICAL(&dhtReadingMux);

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("[DHT22] Pembacaan gagal; periksa sensor dan koneksi GPIO 7.");
    } else {
      Serial.printf("[DHT22] Suhu: %.1f C, kelembapan: %.1f %%\n",
                    temperature, humidity);
    }
    vTaskDelayUntil(&lastWakeTime, interval);
  }
}

float readAnemometerRaw() {
  while (anemometerSerial.available() > 0) {
    anemometerSerial.read();
  }
  anemometerSerial.write(ANEMOMETER_REQUEST, sizeof(ANEMOMETER_REQUEST));
  anemometerSerial.flush();

  const uint32_t startedAt = millis();
  while (anemometerSerial.available() < 7 &&
         millis() - startedAt < 300) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  if (anemometerSerial.available() < 7) {
    return NAN;
  }

  uint8_t response[7];
  if (anemometerSerial.readBytes(response, sizeof(response)) !=
      sizeof(response)) {
    return NAN;
  }
  const uint16_t rawSpeed =
      (static_cast<uint16_t>(response[3]) << 8) | response[4];
  return static_cast<float>(rawSpeed) / 10.0f;
}

void gpsTask(void* parameter) {
  (void)parameter;
  for (;;) {
    while (gpsSerial.available() > 0) {
      if (gps.encode(static_cast<char>(gpsSerial.read()))) {
        if (xSemaphoreTake(gpsMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
          if (gps.location.isValid()) {
            currentLatitude = gps.location.lat();
            currentLongitude = gps.location.lng();
          }
          if (gps.altitude.isValid()) {
            currentAltitudeM = gps.altitude.meters();
          }
          xSemaphoreGive(gpsMutex);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void anemometerTask(void* parameter) {
  (void)parameter;
  for (;;) {
    const float speedMs = readAnemometerRaw();
    if (xSemaphoreTake(anemometerMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      currentWindSpeedKmh = isnan(speedMs) ? NAN : speedMs * 3.6f;
      xSemaphoreGive(anemometerMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

float readGasPpm(uint8_t channel, float loadResistance, float baselineResistance,
                 float curveA, float curveB) {
  if (!adsReady) {
    return NAN;
  }
  float totalVolts = 0.0f;
  constexpr uint8_t SAMPLE_COUNT = 20;
  for (uint8_t sample = 0; sample < SAMPLE_COUNT; ++sample) {
    const int16_t raw = ads.readADC_SingleEnded(channel);
    if (raw < MIN_VALID_ADC || raw > MAX_VALID_ADC) {
      return NAN;
    }
    totalVolts += ads.computeVolts(raw);
    vTaskDelay(pdMS_TO_TICKS(5));
  }

  float averageVolts = totalVolts / SAMPLE_COUNT;
  averageVolts = constrain(averageVolts, 0.05f, SENSOR_VCC - 0.001f);
  const float sensorResistance =
      (SENSOR_VCC * loadResistance / averageVolts) - loadResistance;
  const float ratio = sensorResistance / baselineResistance;
  if (ratio <= 0.0f || !isfinite(ratio)) {
    return NAN;
  }
  const float value = curveA * powf(ratio, curveB);
  return isfinite(value) ? value : NAN;
}

float readCO2Ppm() {
  const float value = readGasPpm(MQ135_ADC_CHANNEL, RL_CO2, R0_CO2,
                                 CURVE_A_CO2, CURVE_B_CO2);
  return isnan(value) ? NAN : fmaxf(value, 400.0f);
}

float readCH4Ppm() {
  const float value = readGasPpm(MQ4_ADC_CHANNEL, RL_CH4, R0_CH4,
                                 CURVE_A_CH4, CURVE_B_CH4);
  return isnan(value) ? NAN : fmaxf(value, 1.81f);
}

float readNO2Ppb() {
  const float value = readGasPpm(NO2_ADC_CHANNEL, RL_NO2, R0_NO2,
                                 CURVE_A_NO2, CURVE_B_NO2);
  return isnan(value) ? NAN : value * 1000.0f;
}

float readBatteryVoltage() {
  uint32_t sumMilliVolts = 0;
  for (uint8_t sample = 0; sample < BATTERY_OVERSAMPLING_COUNT; ++sample) {
    sumMilliVolts += analogReadMilliVolts(BATTERY_ADC_PIN);
  }
  const float averageMilliVolts =
      static_cast<float>(sumMilliVolts) / BATTERY_OVERSAMPLING_COUNT;
  return (averageMilliVolts / 1000.0f) * BATTERY_DIVIDER_RATIO *
         BATTERY_CALIBRATION_FACTOR;
}

int batteryPercentFromVoltage(float voltage) {
  if (!isfinite(voltage) || voltage <= BATTERY_LOW_VOLTAGE) {
    return 0;
  }
  if (voltage >= BATTERY_HIGH_VOLTAGE) {
    return 100;
  }
  return static_cast<int>(
      ((voltage - BATTERY_LOW_VOLTAGE) /
       (BATTERY_HIGH_VOLTAGE - BATTERY_LOW_VOLTAGE)) *
      100.0f);
}

String formattedTimestamp() {
  struct tm timeInfo;
  if (getLocalTime(&timeInfo, 10)) {
    rtcSynced = true;
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S+07:00", &timeInfo);
    return String(buffer);
  }
  const uint32_t seconds = millis() / 1000U;
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "1970-01-01T00:%02lu:%02luZ",
           static_cast<unsigned long>((seconds / 60U) % 60U),
           static_cast<unsigned long>(seconds % 60U));
  return String(buffer);
}

String makeMessageId() {
  char buffer[56];
  snprintf(buffer, sizeof(buffer), "MSG-%s-%010lu", DEVICE_ID,
           static_cast<unsigned long>(millis()));
  return String(buffer);
}

SensorData readAllSensors() {
  SensorData data;
  data.messageId = makeMessageId();
  data.timestamp = formattedTimestamp();

  if (gpsMutex != nullptr &&
      xSemaphoreTake(gpsMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    data.latitude = currentLatitude;
    data.longitude = currentLongitude;
    data.altitudeM = currentAltitudeM;
    xSemaphoreGive(gpsMutex);
  }

  data.co2Ppm = readCO2Ppm();
  data.ch4Ppm = readCH4Ppm();
  data.no2Ppb = readNO2Ppb();
  if (anemometerMutex != nullptr &&
      xSemaphoreTake(anemometerMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    data.windSpeedKmh = currentWindSpeedKmh;
    xSemaphoreGive(anemometerMutex);
  }

  portENTER_CRITICAL(&dhtReadingMux);
  data.airTemperatureC = currentAirTemperatureC;
  data.airHumidityPercent = currentAirHumidityPercent;
  portEXIT_CRITICAL(&dhtReadingMux);
  data.batteryVoltage = readBatteryVoltage();
  data.batteryPercent = batteryPercentFromVoltage(data.batteryVoltage);
  data.rssiDbm = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -999;

  const bool sensorsValid = isfinite(data.co2Ppm) && isfinite(data.ch4Ppm) &&
                            isfinite(data.no2Ppb) &&
                            isfinite(data.windSpeedKmh) &&
                            isfinite(data.airTemperatureC) &&
                            isfinite(data.airHumidityPercent);
  data.sensorStatus = sensorsValid ? "normal" : "degraded";
  data.nodeStatus =
      WiFi.status() == WL_CONNECTED && mqttClient.connected() ? "online"
                                                               : "offline";
  return data;
}

String buildJsonPayload(const SensorData& data) {
  JsonDocument document;
  document["message_id"] = data.messageId;
  document["device_id"] = DEVICE_ID;
  document["timestamp"] = data.timestamp;

  JsonObject location = document["location"].to<JsonObject>();
  location["latitude"] = data.latitude;
  location["longitude"] = data.longitude;
  location["altitude_m"] = data.altitudeM;

  JsonObject carbon = document["carbon_data"].to<JsonObject>();
  carbon["co2_ppm"] = isfinite(data.co2Ppm) ? data.co2Ppm : 0.0f;
  carbon["ch4_ppm"] = isfinite(data.ch4Ppm) ? data.ch4Ppm : 0.0f;
  carbon["no2_ppb"] = isfinite(data.no2Ppb) ? data.no2Ppb : 0.0f;

  JsonObject environment = document["environment"].to<JsonObject>();
  environment["wind_speed_kmh"] =
      isfinite(data.windSpeedKmh) ? data.windSpeedKmh : 0.0f;
  environment["air_temperature_c"] =
      isfinite(data.airTemperatureC) ? data.airTemperatureC : 0.0f;
  environment["air_humidity_percent"] =
      isfinite(data.airHumidityPercent) ? data.airHumidityPercent : 0.0f;

  JsonObject power = document["power"].to<JsonObject>();
  power["battery_voltage"] = data.batteryVoltage;
  power["battery_percent"] = data.batteryPercent;

  JsonObject communication = document["communication"].to<JsonObject>();
  communication["network_type"] = "WiFi";
  communication["rssi_dbm"] = data.rssiDbm;

  JsonObject status = document["status"].to<JsonObject>();
  status["node_status"] = data.nodeStatus;
  status["sensor_status"] = data.sensorStatus;
  status["firmware_name"] = FW_NAME;
  status["firmware_version"] = FW_VERSION;
  status["boot_count"] = bootCount;
  status["publish_interval_ms"] = globalConfig.publishIntervalMs;

  String payload;
  serializeJson(document, payload);
  return payload;
}

void handleMqttConfiguration(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, MQTT_CONFIG_TOPIC) != 0) {
    return;
  }

  JsonDocument document;
  const DeserializationError error =
      deserializeJson(document, payload, length);
  if (error) {
    Serial.printf("[CONFIG] JSON tidak valid: %s\n", error.c_str());
    return;
  }

  const JsonVariantConst intervalField =
      document["publish_interval_ms"];
  if (intervalField.isNull() || !intervalField.is<uint32_t>()) {
    Serial.println("[CONFIG] publish_interval_ms wajib berupa integer.");
    return;
  }
  const uint32_t requestedInterval = intervalField.as<uint32_t>();
  if (requestedInterval < MIN_SEND_INTERVAL_MS ||
      requestedInterval > MAX_SEND_INTERVAL_MS) {
    Serial.printf("[CONFIG] Interval harus %lu-%lu ms.\n",
                  static_cast<unsigned long>(MIN_SEND_INTERVAL_MS),
                  static_cast<unsigned long>(MAX_SEND_INTERVAL_MS));
    return;
  }

  if (!preferencesReady ||
      preferences.putUInt("send_interval", requestedInterval) !=
          sizeof(requestedInterval)) {
    logError("Gagal menyimpan publish_interval_ms ke NVS.");
    return;
  }
  globalConfig.publishIntervalMs = requestedInterval;
  Serial.printf("[CONFIG] Interval telemetry diubah menjadi %lu ms.\n",
                static_cast<unsigned long>(requestedInterval));
}

void connectMqttIfNeeded() {
  const uint32_t now = millis();
  if (mqttClient.connected() || WiFi.status() != WL_CONNECTED ||
      now - lastMqttAttemptAt < MQTT_RECONNECT_INTERVAL_MS) {
    return;
  }
  lastMqttAttemptAt = now;

  char clientId[40];
  snprintf(clientId, sizeof(clientId), "AGRISENSE-%06llX",
           static_cast<unsigned long long>(ESP.getEfuseMac() & 0xFFFFFF));
  Serial.printf("[MQTT] Menghubungkan ke %s:%u...\n", MQTT_BROKER, MQTT_PORT);
  if (!mqttClient.connect(clientId)) {
    Serial.printf("[MQTT] Gagal terhubung, state=%d\n", mqttClient.state());
    return;
  }
  if (!mqttClient.subscribe(MQTT_CONFIG_TOPIC)) {
    logError("Subscribe topik konfigurasi MQTT gagal.");
  } else {
    Serial.printf("[MQTT] Terhubung; subscribe %s\n", MQTT_CONFIG_TOPIC);
    lastPublishAt = millis() - globalConfig.publishIntervalMs;
  }
}

void publishTelemetryIfDue() {
  if (!mqttClient.connected()) {
    return;
  }
  const uint32_t now = millis();
  if (now - lastPublishAt < globalConfig.publishIntervalMs) {
    return;
  }
  lastPublishAt = now;

  const SensorData data = readAllSensors();
  const String payload = buildJsonPayload(data);
  if (!preferencesReady || preferences.putString("last_payload", payload) == 0) {
    logError("Gagal menyimpan payload telemetry terakhir.");
  }
  if (preferencesReady &&
      preferences.putUInt("last_sent_ms", now) != sizeof(now)) {
    logError("Gagal menyimpan waktu publish terakhir.");
  }

  if (mqttClient.publish(MQTT_TOPIC, payload.c_str())) {
    Serial.printf("[MQTT] Payload terkirim: %s\n", payload.c_str());
  } else {
    logError("Publish telemetry MQTT gagal.");
  }
}

bool parseFirmwareVersion(const String& text, FirmwareVersion& version) {
  String normalized = text;
  if (normalized.startsWith("v") || normalized.startsWith("V")) {
    normalized.remove(0, 1);
  }

  unsigned int major = 0;
  unsigned int minor = 0;
  unsigned int patch = 0;
  char trailing = '\0';
  const int parsed =
      sscanf(normalized.c_str(), "%u.%u.%u%c", &major, &minor, &patch,
             &trailing);
  if (parsed != 3) {
    return false;
  }
  version.major = major;
  version.minor = minor;
  version.patch = patch;
  return true;
}

bool isNewerFirmware(const String& candidate) {
  FirmwareVersion latest{};
  FirmwareVersion current{};
  if (!parseFirmwareVersion(candidate, latest) ||
      !parseFirmwareVersion(FW_VERSION, current)) {
    Serial.printf("[OTA] Format versi tidak didukung: saat ini=%s, rilis=%s\n",
                  FW_VERSION, candidate.c_str());
    return false;
  }
  if (latest.major != current.major) {
    return latest.major > current.major;
  }
  if (latest.minor != current.minor) {
    return latest.minor > current.minor;
  }
  return latest.patch > current.patch;
}

void checkForFirmwareUpdate() {
  if (WiFi.status() != WL_CONNECTED || !rtcSynced ||
      strcmp(GITHUB_REPOSITORY_OWNER, "ISI_OWNER_REPO") == 0 ||
      strcmp(GITHUB_REPOSITORY_NAME, "ISI_NAMA_REPO") == 0) {
    return;
  }

  const uint32_t now = millis();
  if (lastOtaCheckAt != 0 &&
      now - lastOtaCheckAt < OTA_CHECK_INTERVAL_MS) {
    return;
  }
  lastOtaCheckAt = now;

  char apiUrl[192];
  snprintf(apiUrl, sizeof(apiUrl),
           "https://api.github.com/repos/%s/%s/releases/latest",
           GITHUB_REPOSITORY_OWNER, GITHUB_REPOSITORY_NAME);
  otaClient.setCACert(GITHUB_ROOT_CA);

  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(otaClient, apiUrl)) {
    logError("Gagal memulai request GitHub Releases.");
    return;
  }
  http.addHeader("User-Agent", DEVICE_ID);
  const int statusCode = http.GET();
  if (statusCode != HTTP_CODE_OK) {
    Serial.printf("[OTA] GitHub API mengembalikan HTTP %d\n", statusCode);
    http.end();
    return;
  }

  JsonDocument release;
  const DeserializationError parseError =
      deserializeJson(release, http.getStream());
  http.end();
  if (parseError) {
    Serial.printf("[OTA] Gagal membaca metadata release: %s\n",
                  parseError.c_str());
    return;
  }

  const String latestVersion = release["tag_name"] | "";
  if (latestVersion.isEmpty()) {
    logError("GitHub release tidak memiliki tag_name.");
    return;
  }
  if (!isNewerFirmware(latestVersion)) {
    Serial.printf("[OTA] Firmware %s sudah terbaru (rilis %s).\n",
                  FW_VERSION, latestVersion.c_str());
    return;
  }

  const JsonArray assets = release["assets"].as<JsonArray>();
  String firmwareUrl;
  for (JsonObject asset : assets) {
    const char* assetName = asset["name"] | "";
    if (strcmp(assetName, GITHUB_FIRMWARE_ASSET) == 0) {
      firmwareUrl = asset["browser_download_url"] | "";
      break;
    }
  }
  if (firmwareUrl.isEmpty()) {
    Serial.printf("[OTA] Asset '%s' tidak ditemukan pada release %s.\n",
                  GITHUB_FIRMWARE_ASSET, latestVersion.c_str());
    return;
  }

  Serial.printf("[OTA] Memasang firmware %s dari GitHub Releases...\n",
                latestVersion.c_str());
  httpUpdater.rebootOnUpdate(true);
  httpUpdater.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdater.onProgress([](int progress, int total) {
    esp_task_wdt_reset();
    if (total > 0) {
      Serial.printf("[OTA] Progress: %d%%\r", (progress * 100) / total);
    }
  });
  const t_httpUpdate_return result =
      httpUpdater.update(otaClient, firmwareUrl, FW_VERSION);
  if (result == HTTP_UPDATE_FAILED) {
    Serial.printf("[OTA] Update gagal: %s\n",
                  httpUpdater.getLastErrorString().c_str());
  } else if (result == HTTP_UPDATE_NO_UPDATES) {
    Serial.println("[OTA] Tidak ada update firmware.");
  }
}

void setWifiStatusLed() {
  const bool connected = wifiManager.isConnected();
  if (connected != lastWifiLedState) {
    digitalWrite(WIFI_LED_PIN, connected ? HIGH : LOW);
    lastWifiLedState = connected;
  }
}

void initializeWatchdog() {
#if ESP_IDF_VERSION_MAJOR >= 5
  esp_task_wdt_config_t config{};
  config.timeout_ms = WDT_TIMEOUT_MS;
  config.idle_core_mask = 0;
  config.trigger_panic = true;
  esp_err_t result = esp_task_wdt_init(&config);
  if (result == ESP_ERR_INVALID_STATE) {
    result = esp_task_wdt_reconfigure(&config);
  }
#else
  const esp_err_t result =
      esp_task_wdt_init(WDT_TIMEOUT_MS / 1000U, true);
#endif
  if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
    Serial.printf("[WDT] Inisialisasi gagal: %s\n", esp_err_to_name(result));
    return;
  }

  const esp_err_t addResult = esp_task_wdt_add(nullptr);
  if (addResult != ESP_OK && addResult != ESP_ERR_INVALID_STATE) {
    Serial.printf("[WDT] Registrasi task loop gagal: %s\n",
                  esp_err_to_name(addResult));
  } else {
    Serial.printf("[WDT] Aktif dengan timeout %lu ms.\n",
                  static_cast<unsigned long>(WDT_TIMEOUT_MS));
  }
}

void initializeSensors() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  ads.setGain(GAIN_TWOTHIRDS);
  adsReady = ads.begin(ADS_ADDRESS, &Wire);
  if (!adsReady) {
    logError("ADS1115 tidak terdeteksi; pembacaan CO2/CH4/NO2 dinonaktifkan.");
  }

  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
  dht.begin();
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  anemometerSerial.begin(ANEMOMETER_BAUD, SERIAL_8N1,
                         ANEMOMETER_RX_PIN, ANEMOMETER_TX_PIN);
}

void initializeTasks() {
  gpsMutex = xSemaphoreCreateMutex();
  anemometerMutex = xSemaphoreCreateMutex();
  if (gpsMutex == nullptr || anemometerMutex == nullptr) {
    logError("Gagal membuat mutex FreeRTOS.");
    return;
  }

  if (xTaskCreatePinnedToCore(sensorTask, "Task_DHT22", 4096, nullptr, 1,
                              nullptr, 0) != pdPASS) {
    logError("Gagal membuat task sensor DHT22.");
  }
  if (xTaskCreatePinnedToCore(gpsTask, "Task_GPS", 4096, nullptr, 1,
                              nullptr, 0) != pdPASS) {
    logError("Gagal membuat task GPS.");
  }
  if (xTaskCreatePinnedToCore(anemometerTask, "Task_Anemometer", 4096,
                              nullptr, 2, nullptr, 0) != pdPASS) {
    logError("Gagal membuat task anemometer.");
  }
}

void loadConfiguration() {
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
    logError("Gagal membuka namespace konfigurasi NVS.");
    return;
  }
  preferencesReady = true;
  bootCount = preferences.getUInt("boot_count", 0) + 1;
  if (preferences.putUInt("boot_count", bootCount) != sizeof(bootCount)) {
    logError("Gagal menyimpan penghitung boot.");
  }

  const uint32_t savedInterval =
      preferences.getUInt("send_interval", SEND_INTERVAL_MS);
  if (savedInterval >= MIN_SEND_INTERVAL_MS &&
      savedInterval <= MAX_SEND_INTERVAL_MS) {
    globalConfig.publishIntervalMs = savedInterval;
  } else {
    Serial.println("[CONFIG] Nilai interval tersimpan tidak valid; memakai default.");
  }
  Serial.printf("[CONFIG] publish_interval_ms=%lu\n",
                static_cast<unsigned long>(globalConfig.publishIntervalMs));
}

}  // namespace

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  delay(300);
  Serial.printf("\n[%s] Firmware %s mulai.\n", FW_NAME, FW_VERSION);

  pinMode(WIFI_LED_PIN, OUTPUT);
  digitalWrite(WIFI_LED_PIN, LOW);
  initializeWatchdog();
  loadConfiguration();
  initializeSensors();
  initializeTasks();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(handleMqttConfiguration);
  mqttClient.setBufferSize(2048);

  if (!wifiManager.begin(WIFI_AP_SSID, WIFI_AP_PASSWORD,
                         WIFI_PREFERENCES_NAMESPACE)) {
    Serial.println("[WiFiManager] Portal tidak berhasil dimulai sepenuhnya.");
  }
  setWifiStatusLed();

  Serial.printf("[WiFiManager] SSID portal: %s; LED koneksi: GPIO %u; "
                "DHT22: GPIO %u\n",
                WIFI_AP_SSID, WIFI_LED_PIN, DHT_PIN);
}

void loop() {
  wifiManager.loop();
  setWifiStatusLed();

  if (WiFi.status() == WL_CONNECTED) {
    if (!ntpConfigured) {
      configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1,
                 NTP_SERVER_2);
      ntpConfigured = true;
    }
    if (!rtcSynced) {
      struct tm timeInfo;
      if (getLocalTime(&timeInfo, 10)) {
        rtcSynced = true;
        Serial.println("[NTP] Waktu berhasil disinkronkan.");
      }
    }
    connectMqttIfNeeded();
    mqttClient.loop();
    publishTelemetryIfDue();
    checkForFirmwareUpdate();
  } else if (mqttClient.connected()) {
    mqttClient.disconnect();
  }

  esp_task_wdt_reset();
  vTaskDelay(pdMS_TO_TICKS(10));
}
