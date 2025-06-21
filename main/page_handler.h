#ifndef PAGE_HANDLER_H
#define PAGE_HANDLER_H

#include "driver/gptimer.h"
#include "driver/ledc.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "nvs_flash.h"
#include "secret.h"

#define GPIO_MASTER_PIN 17
#define PWM_RESOLUTION ((1 << 13) - 1)  // 2^13 - 1
#define FREQUENCY 1000
#define STACK_DEPTH 4096
#define TIMER_RESOLUTION 1000000
#define ALARM_COUNT 1000000
#define MAX_INPUT_LEN 200

esp_err_t front_url_handler(httpd_req_t* req);
esp_err_t pwm_handler(httpd_req_t* req);
esp_err_t css_handler(httpd_req_t* req);
esp_err_t flickering_handler(httpd_req_t* req);
esp_err_t deflickering_handler(httpd_req_t* req);
esp_err_t timer_handler(httpd_req_t* req);
esp_err_t timer_activate_handler(httpd_req_t* req);
esp_err_t submit_time_handler(httpd_req_t* req);
esp_err_t brightness_handler(httpd_req_t* req);
esp_err_t submit_brightness_handler(httpd_req_t* req);

void redirect(httpd_req_t* req, char redirect_location[]);
void set_brightness(int percentage);
void flicker_pwm();
void start_timer();

bool IRAM_ATTR timer_ISR(gptimer_handle_t timer,
                         const gptimer_alarm_event_data_t* event_data,
                         void* user_data);

#endif