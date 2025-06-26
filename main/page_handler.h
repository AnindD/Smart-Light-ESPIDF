#ifndef PAGE_HANDLER_H
#define PAGE_HANDLER_H

#include "driver/gptimer.h"
#include "driver/i2c_master.h"
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

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_MASTER_TIMEOUT_MS 1000

#define CALIBRATION_REGISTER_ADDRESS 0x88
#define CTRL_HUMIDITY_ADDRESS 0xF2
#define MEASUREMENT_CONTROL_ADDRESS 0xF4
#define REG_TEMP_MSB 0xFA
#define BME280_DEVICE_ADDRESS 0x76

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
esp_err_t sensor_handler(httpd_req_t* req);
esp_err_t read_byte_i2c(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr,
                        uint8_t* data, size_t len);
esp_err_t start_sensor();
esp_err_t stop_sensor();

void redirect(httpd_req_t* req, char redirect_location[]);
void set_brightness(int percentage);
void flicker_pwm();
void start_timer();
void i2c_master_init_bus(i2c_master_bus_handle_t* bus_handle);
void i2c_master_init_handle(i2c_master_bus_handle_t* bus_handle,
                            i2c_master_dev_handle_t* dev_handle,
                            uint8_t address);
void read_bme_280_task();

bool IRAM_ATTR timer_ISR(gptimer_handle_t timer,
                         const gptimer_alarm_event_data_t* event_data,
                         void* user_data);

#endif