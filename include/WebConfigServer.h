#ifndef WEB_CONFIG_SERVER_H
#define WEB_CONFIG_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <WebServer.h>  // ESP32 WebServer (ESP32 core 2.0.0+)
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <LittleFS.h>
#include "ConfigManager.h"

class WebConfigServer {
public:
  // Constructor
  WebConfigServer(ConfigManager& config_manager, int port = 80);
  
  // Initialize the web server
  void begin();
  
  // Handle client requests (call this in the main loop)
  void handle_client();
  
  // Check if we need to restart the device
  bool should_restart() const;
  
  // Check if a client is currently connected
  bool is_client_connected() const;
  
private:
  // Reference to the config manager
  ConfigManager& _config_manager;
  
  // Web server instance
  WebServer _server;
  
  // Flag to indicate if device should restart
  bool _restart_requested;
  
  // Setup server routes
  void setup_routes();
  
  // Handler methods
  void handle_root();
  void handle_static_files();
  void handle_get_config();
  void handle_update_config();
  void handle_not_found();
  void handle_restart();
  
  // Helper to serve static files from LittleFS
  bool serve_file_from_fs(const String& path, const String& content_type);
};

#endif // WEB_CONFIG_SERVER_H