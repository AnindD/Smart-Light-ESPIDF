// front page url_handler
#include "page_handler.h"

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