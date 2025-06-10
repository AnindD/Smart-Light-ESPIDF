#include <stdio.h>

#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "hal/adc_types.h"
#include "nvs_flash.h"
#include "page_handler.h"
#include "secret.h"

static void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id,
                               void* event_data) {
  // Base event is WIFI-type and it checks whether it is starting or
  // disconnected and connects
  if ((base == WIFI_EVENT) && (id == WIFI_EVENT_STA_START)) {
    ESP_LOGW("INFO: ", "Wi-Fi interface has started!");
    ESP_ERROR_CHECK(esp_wifi_connect());
  } else if ((base == WIFI_EVENT) && (id == WIFI_EVENT_STA_DISCONNECTED)) {
    ESP_LOGE("ERROR: ", "Wi-Fi interface has not started.");
    ESP_ERROR_CHECK(esp_wifi_connect());
  }
}

static httpd_handle_t http_server_start() {
  httpd_handle_t http_server = NULL;
  httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();

  if (httpd_start(&http_server, &http_config) == ESP_OK) {
    // HTML PAGE HANDLERS
    httpd_uri_t front_page = {.uri = "/",
                              .method = HTTP_GET,
                              .handler = front_url_handler,
                              .user_ctx = NULL};
    httpd_register_uri_handler(http_server, &front_page);

    httpd_uri_t pwm = {.uri = "/PWM",
                       .method = HTTP_GET,
                       .handler = pwm_handler,
                       .user_ctx = NULL};
    httpd_register_uri_handler(http_server, &pwm);

    httpd_uri_t timer_page = {.uri = "/Timer",
                              .method = HTTP_GET,
                              .handler = timer_handler,
                              .user_ctx = NULL};
    httpd_register_uri_handler(http_server, &timer_page);

    // FEATURE HANDLER
    httpd_uri_t flickering = {.uri = "/flickeringActivate",
                              .method = HTTP_GET,
                              .handler = flickering_handler,
                              .user_ctx = NULL};
    httpd_register_uri_handler(http_server, &flickering);

    httpd_uri_t deflickering = {.uri = "/flickeringDeactivate",
                                .method = HTTP_GET,
                                .handler = deflickering_handler,
                                .user_ctx = NULL};
    httpd_register_uri_handler(http_server, &deflickering);

    httpd_uri_t timer_activate = {.uri = "/timerActivate",
                                  .method = HTTP_GET,
                                  .handler = timer_activate_handler,
                                  .user_ctx = NULL};
    httpd_register_uri_handler(http_server, &timer_activate);

    httpd_uri_t custom_timer_activate = {.uri = "/customTimerActivate",
                                         .method = HTTP_POST,
                                         .handler = submit_time_handler,
                                         .user_ctx = NULL};
    httpd_register_uri_handler(http_server, &custom_timer_activate);

    // CSS HANDLER
    httpd_uri_t css = {.uri = "/style.css",
                       .method = HTTP_GET,
                       .handler = css_handler,
                       .user_ctx = NULL};
    httpd_register_uri_handler(http_server, &css);
    ESP_LOGW("INFO: ", "HTTP Server set up");
    return http_server;
  } else {
    ESP_LOGE("ERROR: ", "Failed to start HTTP server");
    return NULL;
  }
}

static void ip_event_handler(void* arg, esp_event_base_t base, int32_t id,
                             void* event_data) {
  // Base event is IP-type and prints out IP
  if ((base == IP_EVENT) && (id == IP_EVENT_STA_GOT_IP)) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    ESP_LOGW("INFO: ", "IP: " IPSTR, IP2STR(&event->ip_info.ip));
    http_server_start();
  }
}

static void initialize_wifi() {
  // Initialize non-volatile storage library to store key-value pairs in flash
  // memory.
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated or is from a different IDF version
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Create TCP/IP (Internet Suite) protocol which facilitates communication
  // protocols for internet
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Create default network interface and driver
  assert(esp_netif_create_default_wifi_sta());
  wifi_init_config_t cfg_wifi = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg_wifi));

  // Register event handler
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, WIFI_EVENT_STA_START, &wifi_event_handler, NULL, NULL));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL, NULL));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_event_handler, NULL,
      NULL));

  // Set to station mode (where it searches for existing networks to connect
  // too)
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  wifi_config_t wifi_config = {
      .sta = {.ssid = SSID, .password = PASSWORD},
  };

  // If station connected, then configure wifi-network with given SSID and
  // Password (finds SSID and inputs password)
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGW("INFO: ", "Wifi station has been initialized.");
}

static void initialize_spiffs() {
  ESP_LOGW("INFO: ", "Initializing SPIFFs");

  esp_vfs_spiffs_conf_t spiffs_config = {.base_path = "/spiffs",
                                         .partition_label = "storage",
                                         .max_files = 10,
                                         .format_if_mount_failed = true};

  ESP_ERROR_CHECK(esp_vfs_spiffs_register(&spiffs_config));
}

void app_main(void) {
  initialize_spiffs();
  initialize_wifi();
}