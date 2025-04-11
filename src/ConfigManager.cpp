#include "ConfigManager.h"

// Initialize the global configuration instance
ConfigManager config_manager;

ConfigManager::ConfigManager() : _config_file_path(CONFIG_FILE_PATH) {
  // Set defaults in constructor
  reset_to_defaults();
}

bool ConfigManager::begin() {
  // Try to load configuration from file
  if (!load_config()) {
    // If loading fails, use defaults and save them
    Log.infoln("Using default configuration and saving to file");
    save_config();
    return false;
  }
  return true;
}

void ConfigManager::reset_to_defaults() {
  // WiFi settings
  wifi_ssid = "";
  wifi_password = "";
  
  // Home Assistant settings
  hass_url = "ws://homeassistant.local:8123/api/websocket";
  hass_token = "";
  
  // Data refresh settings
  data_refresh_seconds = 300;
  data_wait_ms = 250;
  
  // Entity IDs
  weather_entity_id = "weather.accuweather";
  alarm_entity_id = "";
  temperature_unit = "F";
  
  // Feature flags
  listen_for_events = true;
  wait_for_serial = false;
  
  // Power management settings (enabled by default)
  enable_deep_sleep = true;
  sleep_duration_minutes = 5; // Default matches data_refresh_seconds
  disable_wifi_between_updates = true;
}

bool ConfigManager::save_config() {
  // Create a JSON document
  JsonDocument doc;
  
  // WiFi settings
  doc["wifi_ssid"] = wifi_ssid;
  doc["wifi_password"] = wifi_password;
  
  // Home Assistant settings
  doc["hass_url"] = hass_url;
  doc["hass_token"] = hass_token;
  
  // Data refresh settings
  doc["data_refresh_seconds"] = data_refresh_seconds;
  doc["data_wait_ms"] = data_wait_ms;
  
  // Entity IDs
  doc["weather_entity_id"] = weather_entity_id;
  doc["alarm_entity_id"] = alarm_entity_id;
  doc["temperature_unit"] = temperature_unit;
  
  // Feature flags
  doc["listen_for_events"] = listen_for_events;
  doc["wait_for_serial"] = wait_for_serial;
  
  // Power management settings
  doc["enable_deep_sleep"] = enable_deep_sleep;
  doc["sleep_duration_minutes"] = sleep_duration_minutes;
  doc["disable_wifi_between_updates"] = disable_wifi_between_updates;
  
  // Open file for writing
  File configFile = LittleFS.open(_config_file_path, "w");
  if (!configFile) {
    Log.errorln("Failed to open config file for writing");
    return false;
  }
  
  // Write to file
  if (serializeJson(doc, configFile) == 0) {
    Log.errorln("Failed to write config to file");
    configFile.close();
    return false;
  }
  
  // Close file
  configFile.close();
  Log.infoln("Configuration saved successfully");
  return true;
}

bool ConfigManager::load_config() {
  // Check if config file exists
  if (!LittleFS.exists(_config_file_path)) {
    Log.warningln("Config file %s does not exist", _config_file_path);
    return false;
  }
  
  // Open file for reading
  File configFile = LittleFS.open(_config_file_path, "r");
  if (!configFile) {
    Log.errorln("Failed to open config file for reading");
    return false;
  }
  
  // Deserialize JSON from the file
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Log.errorln("Failed to parse config file: %s", error.c_str());
    return false;
  }
  
  // WiFi settings
  if (doc.containsKey("wifi_ssid")) wifi_ssid = doc["wifi_ssid"].as<String>();
  if (doc.containsKey("wifi_password")) wifi_password = doc["wifi_password"].as<String>();
  
  // Home Assistant settings
  if (doc.containsKey("hass_url")) hass_url = doc["hass_url"].as<String>();
  if (doc.containsKey("hass_token")) hass_token = doc["hass_token"].as<String>();
  
  // Data refresh settings
  if (doc.containsKey("data_refresh_seconds")) data_refresh_seconds = doc["data_refresh_seconds"].as<int>();
  if (doc.containsKey("data_wait_ms")) data_wait_ms = doc["data_wait_ms"].as<int>();
  
  // Entity IDs
  if (doc.containsKey("weather_entity_id")) weather_entity_id = doc["weather_entity_id"].as<String>();
  if (doc.containsKey("alarm_entity_id")) alarm_entity_id = doc["alarm_entity_id"].as<String>();
  if (doc.containsKey("temperature_unit")) temperature_unit = doc["temperature_unit"].as<String>();
  
  // Feature flags
  if (doc.containsKey("listen_for_events")) listen_for_events = doc["listen_for_events"].as<bool>();
  if (doc.containsKey("wait_for_serial")) wait_for_serial = doc["wait_for_serial"].as<bool>();
  
  // Power management settings
  if (doc.containsKey("enable_deep_sleep")) enable_deep_sleep = doc["enable_deep_sleep"].as<bool>();
  if (doc.containsKey("sleep_duration_minutes")) sleep_duration_minutes = doc["sleep_duration_minutes"].as<int>();
  if (doc.containsKey("disable_wifi_between_updates")) disable_wifi_between_updates = doc["disable_wifi_between_updates"].as<bool>();
  
  Log.infoln("Configuration loaded successfully");
  return true;
}