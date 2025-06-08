#ifndef PAGE_HANDLER_H
#define PAGE_HANDLER_H
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

esp_err_t front_url_handler();
esp_err_t pwm_handler();
esp_err_t css_handler();

#endif