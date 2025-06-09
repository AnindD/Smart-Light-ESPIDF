#ifndef PAGE_HANDLER_H
#define PAGE_HANDLER_H
#include "driver/ledc.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "hal/adc_types.h"
#include "nvs_flash.h"
#include "secret.h"
#define GPIO_MASTER_PIN 17
#define PWM_RESOLUTION ((1 << 13) - 1)  // 2^13 - 1
#define FREQUENCY 1000

esp_err_t front_url_handler();
esp_err_t pwm_handler();
esp_err_t css_handler();
esp_err_t flickering_handler();
esp_err_t deflickering_handler();

#endif