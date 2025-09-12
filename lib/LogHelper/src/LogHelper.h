#pragma once

#include <Arduino.h>
#include <esp_log.h>

#define DTU_LOGE(fmt, ...) ESP_LOGE(TAG, "[%s] " fmt, SUBTAG, ##__VA_ARGS__)
#define DTU_LOGW(fmt, ...) ESP_LOGW(TAG, "[%s] " fmt, SUBTAG, ##__VA_ARGS__)
#define DTU_LOGI(fmt, ...) ESP_LOGI(TAG, "[%s] " fmt, SUBTAG, ##__VA_ARGS__)
#define DTU_LOGD(fmt, ...) ESP_LOGD(TAG, "[%s] " fmt, SUBTAG, ##__VA_ARGS__)
#define DTU_LOGV(fmt, ...) ESP_LOGV(TAG, "[%s] " fmt, SUBTAG, ##__VA_ARGS__)

#define DTU_LOG_IS_ERROR    (esp_log_level_get(TAG) >= ESP_LOG_ERROR)
#define DTU_LOG_IS_WARNING  (esp_log_level_get(TAG) >= ESP_LOG_WARNING)
#define DTU_LOG_IS_INFO     (esp_log_level_get(TAG) >= ESP_LOG_INFO)
#define DTU_LOG_IS_DEBUG    (esp_log_level_get(TAG) >= ESP_LOG_DEBUG)
#define DTU_LOG_IS_VERBOSE  (esp_log_level_get(TAG) >= ESP_LOG_VERBOSE)

class LogHelper {
public:
    static void dumpBytes(const char* tag, const char* subtag, const uint8_t* data, size_t len);
};
