// front page url_handler
#include "page_handler.h"

// GLOBAL VARIABLES
volatile bool PWMOn = false;
volatile bool timer_done = false;
uint16_t max_counter;
TaskHandle_t pwm_task = NULL;
TaskHandle_t timer_task = NULL;
TaskHandle_t sensor_task = NULL;
gptimer_handle_t gpTimer = NULL;

esp_err_t front_url_handler(httpd_req_t* req) {
  FILE* front_page_file = fopen("/spiffs/frontPage.html", "r");
  if (!front_page_file) {
    return ESP_FAIL;
  }
  char info[256];  // 256 for adequate size
  httpd_resp_set_type(req, "text/html");
  while (fgets(info, sizeof(info), front_page_file)) {
    httpd_resp_sendstr_chunk(req, info);
  }

  fclose(front_page_file);
  httpd_resp_sendstr_chunk(req, NULL);
  return ESP_OK;
}

esp_err_t pwm_handler(httpd_req_t* req) {
  FILE* pwm_file = fopen("/spiffs/PWM.html", "r");
  if (!pwm_file) {
    return ESP_FAIL;
  }
  char info[256];
  httpd_resp_set_type(req, "text/html");
  while (fgets(info, sizeof(info), pwm_file)) {
    httpd_resp_sendstr_chunk(req, info);
  }
  fclose(pwm_file);
  httpd_resp_sendstr_chunk(req, NULL);
  return ESP_OK;
}

// CSS handler
esp_err_t css_handler(httpd_req_t* req) {
  FILE* css_file = fopen("/spiffs/style.css", "r");
  if (!css_file) {
    return ESP_FAIL;
  }
  char info[256];
  httpd_resp_set_type(req, "text/css");
  while (fgets(info, sizeof(info), css_file)) {
    httpd_resp_sendstr_chunk(req, info);
  }

  fclose(css_file);
  httpd_resp_sendstr_chunk(req, NULL);
  return ESP_OK;
}

void flicker_pwm() {
  // Configure the LED timer & Channel
  // Frequency (Hz) can be adjusted for quicker or slower flicker
  if (PWMOn == false) {
    ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_HIGH_SPEED_MODE,
                                      .duty_resolution = LEDC_TIMER_13_BIT,
                                      .timer_num = LEDC_TIMER_0,
                                      .freq_hz = FREQUENCY,
                                      .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_config = {.gpio_num = GPIO_MASTER_PIN,
                                         .speed_mode = LEDC_HIGH_SPEED_MODE,
                                         .channel = LEDC_CHANNEL_0,
                                         .duty = 0,
                                         .intr_type = LEDC_INTR_DISABLE,
                                         .timer_sel = LEDC_TIMER_0,
                                         .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_config));

    PWMOn = true;
    // Alternate between high and low duty cycle -> high and low analog signal
    while (PWMOn) {
      ESP_ERROR_CHECK(
          ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, PWM_RESOLUTION));
      ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
      vTaskDelay(pdMS_TO_TICKS(100));
      ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0));
      ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

// Flicker LED using PWM
// Since we use a while loop to continiously turn off and on we need to use RTOS
// tasks to start and stop an independent task
esp_err_t flickering_handler(httpd_req_t* req) {
  ESP_LOGW("INFO: ", "Flickering Activated!");
  if (pwm_task == NULL) {
    xTaskCreate(flicker_pwm, "Flicker Task", STACK_DEPTH, NULL, 2, &pwm_task);
  } else {
    ESP_LOGE("ERROR: ", "PWM already running!");
  }
  return ESP_OK;
}

esp_err_t deflickering_handler(httpd_req_t* req) {
  ESP_LOGW("INFO: ", "Deflickering Activated!");
  PWMOn = false;
  pwm_task = NULL;
  esp_err_t ledc_error = ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
  if (ledc_error == ESP_ERR_INVALID_STATE) {
    ESP_LOGE("ERROR: ", "LEDC was not initialized to begin with!");
  }
  return ESP_OK;
}

/* ==[TIMER]== */

esp_err_t timer_handler(httpd_req_t* req) {
  FILE* timer_page_file = fopen("/spiffs/timer.html", "r");
  if (!timer_page_file) {
    return ESP_FAIL;
  }
  char info[256];
  httpd_resp_set_type(req, "text/html");
  while (fgets(info, sizeof(info), timer_page_file)) {
    httpd_resp_sendstr_chunk(req, info);
  }
  fclose(timer_page_file);
  httpd_resp_sendstr_chunk(req, NULL);
  return ESP_OK;
}

// Interrupt Service Routine for Timer

bool IRAM_ATTR timer_ISR(gptimer_handle_t timer,
                         const gptimer_alarm_event_data_t* event_data,
                         void* user_data) {
  gpio_set_level(GPIO_MASTER_PIN, 0);
  timer_done = true;
  return true;
}

void start_timer() {
  timer_done = false;
  // Configure the timer in RTOS task
  ESP_ERROR_CHECK(gpio_set_direction(GPIO_MASTER_PIN, GPIO_MODE_OUTPUT));
  gptimer_config_t gp_timer_config = {.clk_src = GPTIMER_CLK_SRC_DEFAULT,
                                      .direction = GPTIMER_COUNT_UP,
                                      .resolution_hz = TIMER_RESOLUTION};

  ESP_ERROR_CHECK(gptimer_new_timer(&gp_timer_config, &gpTimer));

  gptimer_alarm_config_t gp_alarm_config = {
      .alarm_count = ALARM_COUNT * max_counter,
      .reload_count = 0,
      .flags.auto_reload_on_alarm = false};
  gptimer_event_callbacks_t callbacks = {
      .on_alarm = timer_ISR,
  };

  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gpTimer, &callbacks, NULL));
  ESP_ERROR_CHECK(gptimer_set_alarm_action(gpTimer, &gp_alarm_config));
  ESP_ERROR_CHECK(gptimer_enable(gpTimer));
  ESP_ERROR_CHECK(gptimer_start(gpTimer));
  gpio_set_level(GPIO_MASTER_PIN, true);

  while (!timer_done) {
    // Convert milliseconds to ticks as GPtimer uses ticks
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  esp_err_t gp_timer_stop_error = gptimer_stop(gpTimer);
  if (gp_timer_stop_error == ESP_ERR_INVALID_STATE) {
    ESP_LOGE("ERROR: ", "Timer Stopped Suddenly!");
  }
  ESP_ERROR_CHECK(gptimer_disable(gpTimer));
  ESP_ERROR_CHECK(gptimer_del_timer(gpTimer));
  gpTimer = NULL;
  timer_task = NULL;

  // Delete the task when done
  vTaskDelete(NULL);
}

esp_err_t timer_activate_handler(httpd_req_t* req) {
  ESP_LOGW("INFO: ", "Timer Activated");

  if (timer_task != NULL) {
    timer_done = true;
    vTaskDelete(timer_task);
    timer_task = NULL;
    esp_err_t gp_timer_stop_error = gptimer_stop(gpTimer);
    if (gp_timer_stop_error == ESP_ERR_INVALID_STATE) {
      ESP_LOGE("ERROR: ", "Timer Stopped Suddenly!");
    }
    ESP_ERROR_CHECK(gptimer_disable(gpTimer));
    ESP_ERROR_CHECK(gptimer_del_timer(gpTimer));
    gpTimer = NULL;
  }
  max_counter = 60;
  if (timer_task == NULL) {
    xTaskCreate(start_timer, "Timer Task", STACK_DEPTH, NULL, 2, &timer_task);
  }
  redirect(req, "/Timer");
  return ESP_OK;
}

/* ==[TIMER FORM]== */
esp_err_t submit_time_handler(httpd_req_t* req) {
  ESP_LOGW("INFO: ", "Form has been submitted");
  if (req->content_len <= 0 || req->content_len >= MAX_INPUT_LEN) {
    ESP_LOGE("ERROR: ", "Not long enough");
    redirect(req, "/Timer");
    return ESP_OK;
  }

  char user_data[MAX_INPUT_LEN];
  // If request received is <= 0 then request has not been received
  // Check httpd_req_recv() to confirm
  int user_len = httpd_req_recv(req, user_data, req->content_len);
  if (user_len <= 0) {
    ESP_LOGE("ERROR: ", "Request not received");
    redirect(req, "/Timer");
    return ESP_OK;
  }
  user_data[user_len] = '\0';
  char time_extract[MAX_INPUT_LEN];

  // Extract into time_extract
  if (httpd_query_key_value(user_data, "timerset", time_extract,
                            sizeof(time_extract)) == ESP_OK) {
    // atoi() converts time_extract str -> int
    max_counter = atoi(time_extract);
    ESP_LOGW("INFO: ", "Timer Activated");

    if (timer_task != NULL) {
      timer_done = true;
      vTaskDelete(timer_task);
      timer_task = NULL;
      esp_err_t gp_timer_stop_error = gptimer_stop(gpTimer);
      if (gp_timer_stop_error == ESP_ERR_INVALID_STATE) {
        ESP_LOGE("ERROR: ", "Timer Stopped Suddenly!");
      }
      ESP_ERROR_CHECK(gptimer_disable(gpTimer));
      ESP_ERROR_CHECK(gptimer_del_timer(gpTimer));
      gpTimer = NULL;
    }
    if (timer_task == NULL) {
      xTaskCreate(start_timer, "Timer Task", STACK_DEPTH, NULL, 2, &timer_task);
    }
  }
  redirect(req, "/Timer");
  return ESP_OK;
}

/* ==[BRIGHTNESS]== */
esp_err_t brightness_handler(httpd_req_t* req) {
  FILE* brightness_page_file = fopen("/spiffs/brightness.html", "r");
  if (!brightness_page_file) {
    return ESP_FAIL;
  }
  char info[256];
  httpd_resp_set_type(req, "text/html");
  while (fgets(info, sizeof(info), brightness_page_file)) {
    httpd_resp_sendstr_chunk(req, info);
  }
  fclose(brightness_page_file);
  httpd_resp_sendstr_chunk(req, NULL);
  return ESP_OK;
}

void set_brightness(int percentage) {
  ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_HIGH_SPEED_MODE,
                                    .duty_resolution = LEDC_TIMER_13_BIT,
                                    .timer_num = LEDC_TIMER_0,
                                    .freq_hz = FREQUENCY,
                                    .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_config = {.gpio_num = GPIO_MASTER_PIN,
                                       .speed_mode = LEDC_HIGH_SPEED_MODE,
                                       .channel = LEDC_CHANNEL_0,
                                       .duty = 0,
                                       .intr_type = LEDC_INTR_DISABLE,
                                       .timer_sel = LEDC_TIMER_0,
                                       .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_config));
  uint32_t duty = (PWM_RESOLUTION * percentage) / 100;
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
}

esp_err_t submit_brightness_handler(httpd_req_t* req) {
  ESP_LOGW("INFO: ", "Form has been submitted");

  if (req->content_len <= 0 || req->content_len >= MAX_INPUT_LEN) {
    ESP_LOGE("ERROR: ", "Not long enough");
    redirect(req, "/Brightness");
    return ESP_OK;
  }
  char user_data[MAX_INPUT_LEN];

  // If request received is <= 0 then request has not been received
  // Check httpd_req_recv() to confirm
  int user_len = httpd_req_recv(req, user_data, req->content_len);
  if (user_len <= 0) {
    ESP_LOGE("ERROR: ", "Request not received");
    redirect(req, "/Brightness");
    return ESP_OK;
  }
  user_data[user_len] = '\0';
  char percentage_extract[MAX_INPUT_LEN];
  if (httpd_query_key_value(user_data, "brightnessSet", percentage_extract,
                            sizeof(percentage_extract)) == ESP_OK) {
    int percentage = atoi(percentage_extract);
    if (percentage < 0 || percentage > 100) {
      ESP_LOGE("ERROR: ",
               "Percentage cannot be less than 0 or higher than 100");
    } else {
      set_brightness(percentage);
    }
  }
  redirect(req, "/Brightness");
  return ESP_OK;
}

esp_err_t detach_brightness(httpd_req_t* req) {
  esp_err_t ledc_error = ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
  if (ledc_error == ESP_ERR_INVALID_STATE) {
    ESP_LOGE("ERROR: ", "LEDC was not initialized to begin with!");
  }
  return ESP_OK;
}

/* ==[SENSOR]== */

esp_err_t sensor_handler(httpd_req_t* req) {
  FILE* i2c_file = fopen("/spiffs/i2c.html", "r");
  if (!i2c_file) {
    return ESP_FAIL;
  }
  char info[256];  // 256 for adequate size
  httpd_resp_set_type(req, "text/html");
  while (fgets(info, sizeof(info), i2c_file)) {
    httpd_resp_sendstr_chunk(req, info);
  }

  fclose(i2c_file);
  httpd_resp_sendstr_chunk(req, NULL);
  return ESP_OK;
}

void i2c_master_init_bus(i2c_master_bus_handle_t* bus_handle) {
  i2c_master_bus_config_t bus_config = {.clk_source = I2C_CLK_SRC_DEFAULT,
                                        .i2c_port = I2C_NUM_0,
                                        .sda_io_num = I2C_SDA_PIN,
                                        .scl_io_num = I2C_SCL_PIN,
                                        .glitch_ignore_cnt = 7,
                                        .flags.enable_internal_pullup = true};
  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));
  ESP_LOGW("INFO: ", "I2C Bus Has Been Initialized");
}

void i2c_master_init_handle(i2c_master_bus_handle_t* bus_handle,
                            i2c_master_dev_handle_t* dev_handle,
                            uint8_t address) {
  i2c_device_config_t dev_cfg = {.dev_addr_length = I2C_ADDR_BIT_LEN_7,
                                 .device_address = address,
                                 .scl_speed_hz = 100000};
  ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_cfg, dev_handle));
  ESP_LOGW("INFO: ", "Device Configured");
}

esp_err_t read_byte_i2c(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr,
                        uint8_t* data, size_t len) {
  return i2c_master_transmit_receive(
      dev_handle, &reg_addr, 1, data, len,
      I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

void read_bme_280_task(void* arg) {
  ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_HIGH_SPEED_MODE,
                                    .duty_resolution = LEDC_TIMER_13_BIT,
                                    .timer_num = LEDC_TIMER_0,
                                    .freq_hz = FREQUENCY,
                                    .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_config = {.gpio_num = GPIO_MASTER_PIN,
                                       .speed_mode = LEDC_HIGH_SPEED_MODE,
                                       .channel = LEDC_CHANNEL_0,
                                       .duty = 0,
                                       .intr_type = LEDC_INTR_DISABLE,
                                       .timer_sel = LEDC_TIMER_0,
                                       .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_config));
  i2c_master_bus_handle_t bus_handle;
  i2c_master_dev_handle_t dev_handle;
  i2c_master_init_bus(&bus_handle);
  i2c_master_init_handle(&bus_handle, &dev_handle, BME280_DEVICE_ADDRESS);

  // Used for compensation when calculating temperature
  uint8_t data_cali[22] = {};
  read_byte_i2c(dev_handle, CALIBRATION_REGISTER_ADDRESS, data_cali, 22);
  uint16_t dig_T1 = (uint16_t)data_cali[0] | ((uint16_t)data_cali[1] << 8);
  int16_t dig_T2 =
      (int16_t)((uint16_t)data_cali[2] | ((uint16_t)data_cali[3] << 8));
  int16_t dig_T3 =
      (int16_t)((uint16_t)data_cali[4] | ((uint16_t)data_cali[5] << 8));

  // Turn off humidity measurement
  uint8_t cfg[2];
  cfg[0] = CTRL_HUMIDITY_ADDRESS;
  cfg[1] = 0x00;
  ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, cfg, 2, pdMS_TO_TICKS(1000)));

  // Measure only temperature
  cfg[0] = MEASUREMENT_CONTROL_ADDRESS;
  cfg[1] = 0b00100001;

  while (1) {
    ESP_ERROR_CHECK(
        i2c_master_transmit(dev_handle, cfg, 2, pdMS_TO_TICKS(1000)));
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t raw[3];
    ESP_ERROR_CHECK(i2c_master_transmit_receive(
        dev_handle, (uint8_t[]){REG_TEMP_MSB}, 1, raw, 3, pdMS_TO_TICKS(1000)));
    int32_t raw_temperature = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) |
                              ((int32_t)raw[2] >> 4);

    // Compensation formula check datasheet 4.2.3
    int32_t var1 =
        ((((raw_temperature >> 3) - ((int32_t)dig_T1 << 1))) * dig_T2) >> 11;
    int32_t var2 = (((((raw_temperature >> 4) - (int32_t)dig_T1) *
                      ((raw_temperature >> 4) - (int32_t)dig_T1)) >>
                     12) *
                    dig_T3) >>
                   14;
    int32_t t_fine = var1 + var2;
    float temperature = ((t_fine * 5 + 128) >> 8) / 100.0f;
    float duty_percent;
    printf("Temperature: %.2f °C\n", temperature);
    duty_percent = (temperature - 29.0f) * (50.0f / 3.0f);
    uint32_t duty = (uint32_t)((duty_percent / 100.0f) * PWM_RESOLUTION);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

esp_err_t start_sensor(httpd_req_t* req) {
  if (sensor_task == NULL) {
    xTaskCreatePinnedToCore(read_bme_280_task, "Read BME280", 4098, NULL, 10,
                            &sensor_task, 1);
    ESP_LOGW("INFO: ", "Sensor has started!");
  } else {
    ESP_LOGE("Error: ", "Sensor is already running!");
  }
  redirect(req, "/I2C");
  return ESP_OK;
}

esp_err_t stop_sensor(httpd_req_t* req) {
  vTaskDelete(sensor_task);
  sensor_task = NULL;
  esp_err_t ledc_error = ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
  if (ledc_error == ESP_ERR_INVALID_STATE) {
    ESP_LOGE("ERROR: ", "LEDC was not initialized to begin with!");
  }
  ESP_LOGW("INFO: ", "Sensor has stopped!");
  redirect(req, "/I2C");
  return ESP_OK;
}

// Redirect a standard URL
void redirect(httpd_req_t* req, char redirect_location[]) {
  httpd_resp_set_status(req, "302");
  httpd_resp_set_hdr(req, "Location", redirect_location);
  httpd_resp_send(req, NULL, 0);
}