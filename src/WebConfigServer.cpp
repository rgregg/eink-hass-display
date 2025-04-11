#include "WebConfigServer.h"
#include <functional>  // For std::bind

WebConfigServer::WebConfigServer(ConfigManager& config_manager, int port)
  : _config_manager(config_manager), _server(port), _restart_requested(false) {
}

void WebConfigServer::begin() {
  setup_routes();
  _server.begin();
  Log.infoln("Web configuration server started.");
}

void WebConfigServer::handle_client() {
  _server.handleClient();
}

bool WebConfigServer::should_restart() const {
  return _restart_requested;
}

bool WebConfigServer::is_client_connected() {
  // Check if there are any connected clients
  return _server.client();
}

void WebConfigServer::setup_routes() {
  // Create static function callbacks that call the appropriate instance methods
  // This is a common pattern for ESP32 WebServer
  
  // Root handler - serve index.html
  _server.on("/", [this]() {
    handle_root();
  });
  
  // Static files handler - CSS and JavaScript
  _server.on("/styles.css", [this]() {
    serve_file_from_fs("/www/styles.css", "text/css");
  });
  
  _server.on("/script.js", [this]() {
    serve_file_from_fs("/www/script.js", "application/javascript");
  });
  
  // Handle preflight OPTIONS requests
  _server.on("/api/config", HTTP_OPTIONS, [this]() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    _server.send(200);
  });
  
  // API endpoints
  _server.on("/api/config", [this]() {
    if (_server.method() == HTTP_GET) {
      handle_get_config();
    } else if (_server.method() == HTTP_POST) {
      handle_update_config();
    }
  });
  
  // Handle preflight OPTIONS requests
  _server.on("/api/restart", HTTP_OPTIONS, [this]() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    _server.send(200);
  });
  
  _server.on("/api/restart", [this]() {
    if (_server.method() == HTTP_POST) {
      handle_restart();
    }
  });
  
  // 404 Not Found handler
  _server.onNotFound([this]() {
    handle_not_found();
  });
}

void WebConfigServer::handle_root() {
  serve_file_from_fs("/www/index.html", "text/html");
}

bool WebConfigServer::serve_file_from_fs(const String& path, const String& content_type) {
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    if (file) {
      _server.streamFile(file, content_type);
      file.close();
      return true;
    }
  }
  
  Log.warningln("File not found: %s", path.c_str());
  _server.send(404, "text/plain", "File Not Found");
  return false;
}

void WebConfigServer::handle_get_config() {
  JsonDocument doc;
  
  // WiFi settings
  doc["wifi_ssid"] = _config_manager.wifi_ssid;
  doc["wifi_password"] = _config_manager.wifi_password;
  
  // Home Assistant settings
  doc["hass_url"] = _config_manager.hass_url;
  doc["hass_token"] = _config_manager.hass_token;
  
  // Data refresh settings
  doc["data_refresh_seconds"] = _config_manager.data_refresh_seconds;
  doc["data_wait_ms"] = _config_manager.data_wait_ms;
  
  // Entity IDs
  doc["weather_entity_id"] = _config_manager.weather_entity_id;
  doc["alarm_entity_id"] = _config_manager.alarm_entity_id;
  doc["temperature_unit"] = _config_manager.temperature_unit;
  
  // Feature flags
  doc["listen_for_events"] = _config_manager.listen_for_events;
  doc["wait_for_serial"] = _config_manager.wait_for_serial;
  
  // Power management settings
  doc["enable_deep_sleep"] = _config_manager.enable_deep_sleep;
  doc["sleep_duration_minutes"] = _config_manager.sleep_duration_minutes;
  doc["disable_wifi_between_updates"] = _config_manager.disable_wifi_between_updates;
  
  String response;
  serializeJson(doc, response);
  
  // Set CORS headers for browser compatibility
  _server.sendHeader("Access-Control-Allow-Origin", "*");
  _server.sendHeader("Access-Control-Allow-Methods", "GET, POST");
  _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  _server.send(200, "application/json", response);
}

void WebConfigServer::handle_update_config() {
  // Check if we have a valid JSON body
  if (!_server.hasArg("plain")) {
    _server.send(400, "text/plain", "Missing request body");
    return;
  }
  
  String body = _server.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    _server.send(400, "text/plain", "Invalid JSON: " + String(error.c_str()));
    return;
  }
  
  // Update config values if they exist in the request
  if (doc["wifi_ssd"].is<String>()) _config_manager.wifi_ssid = doc["wifi_ssid"].as<String>();
  if (doc["wifi_password"].is<String>()) _config_manager.wifi_password = doc["wifi_password"].as<String>();
  
  if (doc.containsKey("hass_url")) _config_manager.hass_url = doc["hass_url"].as<String>();
  if (doc.containsKey("hass_token")) _config_manager.hass_token = doc["hass_token"].as<String>();
  
  if (doc.containsKey("data_refresh_seconds")) _config_manager.data_refresh_seconds = doc["data_refresh_seconds"].as<int>();
  if (doc.containsKey("data_wait_ms")) _config_manager.data_wait_ms = doc["data_wait_ms"].as<int>();
  
  if (doc.containsKey("weather_entity_id")) _config_manager.weather_entity_id = doc["weather_entity_id"].as<String>();
  if (doc.containsKey("alarm_entity_id")) _config_manager.alarm_entity_id = doc["alarm_entity_id"].as<String>();
  if (doc.containsKey("temperature_unit")) _config_manager.temperature_unit = doc["temperature_unit"].as<String>();
  
  if (doc.containsKey("listen_for_events")) _config_manager.listen_for_events = doc["listen_for_events"].as<bool>();
  if (doc.containsKey("wait_for_serial")) _config_manager.wait_for_serial = doc["wait_for_serial"].as<bool>();
  
  // Power management settings
  if (doc.containsKey("enable_deep_sleep")) _config_manager.enable_deep_sleep = doc["enable_deep_sleep"].as<bool>();
  if (doc.containsKey("sleep_duration_minutes")) _config_manager.sleep_duration_minutes = doc["sleep_duration_minutes"].as<int>();
  if (doc.containsKey("disable_wifi_between_updates")) _config_manager.disable_wifi_between_updates = doc["disable_wifi_between_updates"].as<bool>();
  
  // Save the updated configuration
  bool success = _config_manager.save_config();
  
  // Set CORS headers for browser compatibility
  _server.sendHeader("Access-Control-Allow-Origin", "*");
  _server.sendHeader("Access-Control-Allow-Methods", "GET, POST");
  _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  if (success) {
    _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Configuration updated successfully\"}");
  } else {
    _server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save configuration\"}");
  }
}

void WebConfigServer::handle_restart() {
  // Set CORS headers for browser compatibility
  _server.sendHeader("Access-Control-Allow-Origin", "*");
  _server.sendHeader("Access-Control-Allow-Methods", "GET, POST");
  _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Device will restart shortly\"}");
  _restart_requested = true;
}

void WebConfigServer::handle_not_found() {
  // Set CORS headers for browser compatibility
  _server.sendHeader("Access-Control-Allow-Origin", "*");
  _server.sendHeader("Access-Control-Allow-Methods", "GET, POST");
  _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  _server.send(404, "text/plain", "404: Not found");
}
