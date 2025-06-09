// front page url_handler
#include "page_handler.h"

bool PWMOn = false;
TaskHandle_t pwm_task = NULL;

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

// css handler
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
      vTaskDelay(100);
      ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0));
      ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
      vTaskDelay(100);
    }
  }
}

// Flicker LED using PWM
// Since we use a while loop to continiously turn off and on we need to use RTOS
// tasks to start and stop an independent task
esp_err_t flickering_handler(httpd_req_t* req) {
  ESP_LOGW("INFO: ", "Flickering Activated!");
  if (pwm_task == NULL) {
    xTaskCreate(flicker_pwm, "Flicker Task", 4096, NULL, 2, &pwm_task);
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
