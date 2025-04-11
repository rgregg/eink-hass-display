#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ArduinoLog.h>
#include "ConfigManager.h"
#include "DisplayManager.h"

class CaptivePortal {
public:
  // Constructor
  CaptivePortal(ConfigManager& config_manager, DisplayManager* display);
  
  // Start the captive portal with a custom AP name
  void begin(const String& ap_name = "EInkMonitor");
  
  // Process DNS and web server requests (call in loop)
  void handle_client();
  
  // Check if a wifi network was configured and we should restart
  bool should_restart() const;
  
private:
  // Constants
  static const int DNS_PORT = 53;
  static const int WEB_PORT = 80;
  static const int TIMEOUT_MS = 10 * 60 * 1000; // 10 minutes timeout
  
  // Reference to configuration manager
  ConfigManager& _config_manager;
  
  // Reference to display manager
  DisplayManager* _display;
  
  // Servers
  DNSServer _dns_server;
  WebServer _web_server;
  
  // State
  bool _restart_requested;
  unsigned long _portal_start_time;
  
  // Setup routes
  void setup_routes();
  
  // Handler methods
  void handle_root();
  void handle_static_files();
  void handle_save_config();
  void handle_not_found();
  
  // Helper to serve files from LittleFS
  bool serve_file_from_fs(const String& path, const String& content_type);
  
  // Helper to generate networks list HTML
  String generate_networks_html();
  
  // Helper to scan for available WiFi networks
  void scan_networks();
  
  // Store for WiFi scan results
  String _networks_list;
};

#endif // CAPTIVE_PORTAL_H