#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ArduinoLog.h>

// Default configuration file path
#define CONFIG_FILE_PATH "/config.json"

class ConfigManager {
public:
  // Constructor
  ConfigManager();
  
  // Initialize the configuration manager
  bool begin();
  
  // Save current configuration to file
  bool save_config();
  
  // Load configuration from file
  bool load_config();
  
  // Reset to default values
  void reset_to_defaults();

  // WiFi settings
  String wifi_ssid;
  String wifi_password;
  
  // Home Assistant settings
  String hass_url;
  String hass_token;
  
  // Data refresh settings
  int data_refresh_seconds;
  int data_wait_ms;
  
  // Entity IDs
  String weather_entity_id;
  String alarm_entity_id;
  String temperature_unit;
  
  // Feature flags
  bool listen_for_events;
  bool wait_for_serial;
  
  // Power management settings
  bool enable_deep_sleep;
  int sleep_duration_minutes;
  bool disable_wifi_between_updates;
  
private:
  // Configuration file path
  const char* _config_file_path;
  
  // JSON document size
  const size_t _json_capacity = 2048;
};

// Global configuration instance
extern ConfigManager config_manager;

#endif // CONFIG_MANAGER_H