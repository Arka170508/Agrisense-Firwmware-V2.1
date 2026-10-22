#pragma once

#include <stdint.h>

namespace secrets {

constexpr char FW_NAME[] = "AgriSense V2.0 MQTT";
constexpr char FW_VERSION[] = "2.0.0";
constexpr char DEVICE_ID[] = "AGRISENSE-CC-004";

constexpr char WIFI_AP_SSID[] = "AgriSense-Setup";
constexpr char WIFI_AP_PASSWORD[] = "CHANGE_ME_TO_A_UNIQUE_PASSWORD";

constexpr char MQTT_BROKER[] = "broker.emqx.io";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_USERNAME[] = "";
constexpr char MQTT_PASSWORD[] = "";
constexpr char MQTT_TOPIC[] = "agrisense/iot/readings";
constexpr char MQTT_CONFIG_TOPIC[] = "agrisense/iot/config";

constexpr char GITHUB_REPOSITORY_OWNER[] = "ISI_OWNER_REPO";
constexpr char GITHUB_REPOSITORY_NAME[] = "ISI_NAMA_REPO";
constexpr char GITHUB_FIRMWARE_ASSET[] = "firmware.bin";

}  // namespace secrets
