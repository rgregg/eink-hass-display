/***************************************************
  Copyright 2025 - Ryan Gregg
 ****************************************************/

// #include <Adafruit_EPD.h>
#include <Arduino.h>
#include "Adafruit_ThinkInk.h"
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <TickTwo.h>
#include <array>
#include "time.h"
#include "ConfigManager.h"
#include "HassWebsocketManager.h"
#include "RequestData.h"
#include "FS.h"
#include <LittleFS.h>
#include "DisplayManager.h"
#include "EPaper213MonoDisplayManager.h"
#include "WebConfigServer.h"
#include "CaptivePortal.h"
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_MAX1704X.h"
#include "DualLogger.h"

// Forward declarations
void refresh_data_points();
void update_display();
void update_display(bool force_refresh);
void start_captive_portal(const String& reason);
void setLEDPower(bool power_on);
void setLEDColor(uint8_t r, uint8_t g, uint8_t b);
void setup_battery();
void setup_data_points();
void register_for_events();
void data_callback(int request_id, String type, JsonDocument& json_doc);

// Global timer pointers - will be initialized in setup() after loading config
TickTwo* timer_refresh_data_ptr = nullptr;
TickTwo* timer_update_display_ptr = nullptr;

// Power management tracking
unsigned long last_data_refresh_time = 0;
bool data_cycle_complete = false;
bool ready_for_sleep = false;

int alarm_trigger_id = -1;

// Define RTC memory data structure
RTC_DATA_ATTR int bootCount = 0;

// Display manager will be initialized in setup()
EPaper213MonoDisplayManager* display;

// Websocket Manager for HASS
HassWebsocketManager websocket;

// Web configuration server
WebConfigServer* web_config_server;

// Captive portal for WiFi configuration
CaptivePortal* captive_portal;

// Flag to indicate we're in captive portal mode
bool captive_portal_mode = false;

// Battery monitor
Adafruit_MAX17048 maxlipo;

DualLogger dualLog;

// Create the NeoPixel object
//#define PIN_NEOPIXEL 8
#define NUM_PIXELS 1
Adafruit_NeoPixel pixel(NUM_PIXELS, PIN_NEOPIXEL);

// Start the captive portal for WiFi configuration
void start_captive_portal(const String& reason) {
    Log.infoln("Starting captive portal: %s", reason.c_str());
    display->show_message(reason, "Starting Setup Portal");
    
    // Unique MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);

    char macStr[7]; // 6 digits + null terminator
    sprintf(macStr, "%02X%02X%02X", mac[3], mac[4], mac[5]);

    // Convert to String
    String macString = String(macStr);

    // Start captive portal for configuration
    captive_portal = new CaptivePortal(config_manager, display);
    String ap_name = "EInkMonitor-" + macString;  // Unique identifier to avoid conflict with other devices
    captive_portal->begin(ap_name);
    
    // Set captive portal mode flag
    captive_portal_mode = true;
}

void setup_wifi() {
    // Check if WiFi SSID is defined - go straight to captive portal if not
    setLEDColor(255, 0, 0); // Red - WiFi disconnected
    if (config_manager.wifi_ssid.length() == 0 || config_manager.wifi_ssid == "") {
        start_captive_portal("WiFi Not Configured");
        return;
    }
    
    // Show connecting message
    display->show_message("Connecting to WiFi", "" + config_manager.wifi_ssid);
    
    // Disconnect if connected
    WiFi.disconnect();
    
    // Set WiFi mode to station
    WiFi.mode(WIFI_STA);
    
    // Start WiFi connection
    WiFi.begin(config_manager.wifi_ssid.c_str(), config_manager.wifi_password.c_str());
    
    // Wait some time to connect to WiFi
    int connection_attempts = 0;
    const int max_attempts = 20; // 20 seconds timeout
    
    while (WiFi.status() != WL_CONNECTED && connection_attempts < max_attempts) {
        Serial.print(".");
        delay(1000);
        connection_attempts++;
    }

    // Check if connected to WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Log.errorln("No WiFi Found: %s", config_manager.wifi_ssid.c_str());
        start_captive_portal("WiFi Failed");
        return;
    }
    
    setLEDColor(255, 255, 0); // Yellow - WiFi connected, no WebSocket

    // Successfully connected
    Log.infoln("WiFi connected! IP: %s", WiFi.localIP().toString().c_str());
    display->show_message("WiFi Connected", WiFi.localIP().toString());
}

// Function declarations for power management
void enter_deep_sleep();
void begin_normal_operation();
void wifi_disconnect_if_needed();

// Primary initialization method when ESP32 boots
void setup() {
  Serial.begin(115200);

  // Increment boot count (for deep sleep wake tracking)
  bootCount++;

  // Mount filesystem first
  if(!LittleFS.begin(true)){
      Serial.println("LittleFS Mount Failed");
      return;
  }
  
  // Initialize logger with log level
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.infoln("Device booting (boot count: %d)", bootCount);
  Log.verboseln("LittleFS Mounted Successfully");
  
  // Initialize configuration manager
  if (config_manager.begin()) {
    Log.infoln("Configuration loaded from file");
  } else {
    Log.warningln("Using default configuration");
  }
  
  // Wait for serial if needed
  if (config_manager.wait_for_serial) {
    // Waits until the serial connection is established before continuing...
    while (!Serial) {
      delay(10);
    }
  }

  // Power up the NeoPixel LED
  setLEDPower(true);

#ifdef PIN_NEOPIXEL
  pixel.begin();
  pixel.setBrightness(50); // Set to 50% brightness
  setLEDColor(255, 0, 0); // Start with red (disconnected)
  Log.verboseln("Neopixel set to red");
#endif

  setup_battery();
  
  // Create display manager
  display = new EPaper213MonoDisplayManager();
  
  // Initialize display
  display->begin();
  
  // Initialize timers with config values
  timer_refresh_data_ptr = new TickTwo([]() { refresh_data_points(); }, 
                                    config_manager.data_refresh_seconds * 1000, 0, MILLIS);
  timer_update_display_ptr = new TickTwo([]() { update_display(); }, 
                                   config_manager.data_wait_ms, 1, MILLIS);
    
  display->show_message("Connecting...", "WiFi: " + config_manager.wifi_ssid);

  setup_wifi();

  Log.infoln("IP Address: %s", WiFi.localIP().toString().c_str());

  // Only continue with normal operation if we're not in captive portal mode
  if (captive_portal_mode) {
    return;
  }
  display->show_message("Waiting for data", "IP: " + WiFi.localIP().toString());

  // Setup data points
  setup_data_points();

  // Connect via WebSocket to HASS
  websocket.setMessageCallback(data_callback);
  websocket.connect(config_manager.hass_url.c_str(), config_manager.hass_token.c_str());

  // Set color to Green when connected.
  setLEDColor(0, 255, 0);
  
  // Run the refresh_data action every X seconds (from config)
  refresh_data_points();
  timer_refresh_data_ptr->start();

  // Check to see if we should register for events
  if (config_manager.listen_for_events) {
    register_for_events();
  }
  
  // Initialize web configuration server
  web_config_server = new WebConfigServer(config_manager);
  web_config_server->begin();

  setLEDPower(false);
}

void loop() {
  // Check if we're in captive portal mode
  if (captive_portal_mode) {
    // Process captive portal requests
    if (captive_portal) {
      captive_portal->handle_client();
      
      // Check if WiFi configuration completed and restart requested
      if (captive_portal->should_restart()) {
        Log.infoln("WiFi configuration completed, restarting device...");
        delay(1000); // Give time for the response to be sent
        ESP.restart();
      }
    }
    
    return; // Skip the rest of the loop when in captive portal mode
  }
  
  // Normal operation mode
  if (!websocket.available()) {
    Log.infoln("Websocket was not connected -- attempting to reconnect.");
    websocket.connect(config_manager.hass_url.c_str(), config_manager.hass_token.c_str());
  }

  websocket.loop();

  // Handle web configuration requests
  if (web_config_server) {
    web_config_server->handle_client();
    
    // Check if restart was requested
    if (web_config_server->should_restart()) {
      Log.infoln("Restart requested via web interface, restarting device...");
      display->show_message("Restarting...", "Config updated");
      delay(2000); // Give time for the response to be sent
      ESP.restart();
    }
  }

  // Update timers if they exist
  if (timer_refresh_data_ptr) timer_refresh_data_ptr->update();
  if (timer_update_display_ptr) timer_update_display_ptr->update();
  
  // Power management - check if we should go to sleep
  if (config_manager.enable_deep_sleep && data_cycle_complete && ready_for_sleep) {
    // Clean disconnect from WiFi/websockets
    wifi_disconnect_if_needed();
    
    // Only go to sleep if we're not actively handling web config or captive portal
    if (!web_config_server->is_client_connected()) {
      Log.infoln("Data cycle complete, entering deep sleep mode");
      delay(100); // Brief delay to let logs finish
      enter_deep_sleep();
    }
  }
  
  // Check if we should disconnect WiFi to save power (if deep sleep is disabled)
  if (config_manager.disable_wifi_between_updates && !config_manager.enable_deep_sleep && 
      data_cycle_complete && millis() - last_data_refresh_time > 5000) {
    wifi_disconnect_if_needed();
  }
}

bool found_battery = false;
// Battery Monitor
void setup_battery() {
  for(int i=0; i<10; i++) {
    if (maxlipo.begin())
    {
      found_battery = true;
      Log.verboseln("Found MAX17048 with Chip ID: 0x%x", maxlipo.getChipID());
      return;
    }
    delay(1000);
  }

  found_battery = false;
  Log.warningln("Unable to find battery chip.");
}

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
    pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();
}

void setLEDPower(bool power_on) {
#ifdef NEOPIXEL_POWER
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, power_on ? HIGH : LOW);
  Log.verboseln("NeoPixel power set to %d", power_on);
#else
  Log.warningln("NEOPIXEL_POWER is undefined.");
#endif
}

// Data point constants
const char* DATA_TEMPERATURE = "temperature";
const char* DATA_CONDITIONS = "conditions";
const char* DATA_ALARM = "alarm";

std::array<RequestData, 3> data_points;

RequestData data_temperature, data_conditions, data_alarm_state;

void setup_data_points() {
  data_temperature.name = DATA_TEMPERATURE;
  data_temperature.templateStr = String("{{ state_attr('") + config_manager.weather_entity_id + String("', 'temperature') }}");

  data_conditions.name = DATA_CONDITIONS;
  data_conditions.templateStr = String("{{ states('") + config_manager.weather_entity_id + String("') }}");

  // Only set up alarm data point if entity is configured
  data_alarm_state.name = DATA_ALARM;
  if (config_manager.alarm_entity_id.length() > 0) {
    data_alarm_state.templateStr = String("{{ states('") + config_manager.alarm_entity_id + String("') }}");
  } else {
    // Empty template will result in no API request being made
    data_alarm_state.templateStr = "";
    data_alarm_state.latest_value = "";
    Log.infoln("No alarm entity configured, skipping alarm data");
  }

  data_points = { data_temperature, data_conditions, data_alarm_state };
}

void refresh_data_points() {
  // Loop through the array and request data for each element
  Log.verboseln("Refreshing data from HASS...");
  
  // If WiFi was disconnected for power saving, reconnect now
  if (config_manager.disable_wifi_between_updates && WiFi.status() != WL_CONNECTED) {
    Log.infoln("Reconnecting WiFi for data refresh");
    setup_wifi();
    
    // Reconnect to Home Assistant WebSocket if needed
    if (!websocket.available()) {
      Log.infoln("Reconnecting WebSocket");
      websocket.connect(config_manager.hass_url.c_str(), config_manager.hass_token.c_str());
      delay(1000); // Give some time to establish the connection
    }
  }
  
  // Request data from Home Assistant
  for (RequestData& data : data_points) {
    // Only request data if template string is not empty
    if (data.templateStr.length() > 0) {
      data.pending_request_id = websocket.render_template(data.templateStr);
    }
  }
  
  // Record the time of this data refresh
  last_data_refresh_time = millis();
  data_cycle_complete = false;
}

String get_data_value(String name) {
    for (RequestData& data : data_points) {
        if (data.name == name) {
          return data.latest_value;
        }
    }

    return "---";
}

void data_callback(int request_id, String type, JsonDocument& json_doc) {
  if (type == "event") {
    // Check if it matches a subscription
    if (request_id == alarm_trigger_id) {
      // Alarm state was changed.
      String new_state = json_doc["event"]["variables"]["trigger"]["to_state"]["state"];
      if (new_state.length() > 0) {
        Log.verboseln("Alarm state changed to %s", new_state);
        data_alarm_state.latest_value = new_state;

        // Update screen after a short delay
        if (timer_update_display_ptr) timer_update_display_ptr->start();
      }
      return;
    }

    // Otherwise see if it matches a request we made
    for (RequestData& data : data_points) {
      if (request_id == data.pending_request_id) {
        String new_value = json_doc["event"]["result"];
        Log.infoln("Updating data %s with new value %s", data.name, new_value);
        data.update_value(new_value);

        // Triger an update within a delay
        if (timer_update_display_ptr) timer_update_display_ptr->start();
        return;
      }
    }
  }

  Log.warningln("Unprocessed data response: %d", request_id);
}

void update_display() {
  update_display(false);
}

void update_display(bool force_refresh) {
  // Delegate to our display manager
  if (display) {
    // Get battery level
    String battery_level = "";
    if (found_battery) {
      float battery_percent = maxlipo.cellPercent();
      if (battery_percent > 100.0) battery_percent = 100.0;
      battery_level = String(battery_percent, 1) + "%";
      Log.verboseln("Battery percentage: %s", battery_level);
    } else {
      Log.verboseln("No battery detected");
    }
    display->update_display(data_points, force_refresh, battery_level);
    
    // Mark data cycle as complete after we've updated the display
    data_cycle_complete = true;
    
    // Set ready_for_sleep flag to true after a short delay
    // This gives time for any pending processes to complete
    TickTwo* sleep_timer = new TickTwo([]() { 
      ready_for_sleep = true; 
      Log.verboseln("Device ready for sleep");
    }, 5000, 1, MILLIS);
    
    sleep_timer->start();
  }
}

// Register to receive events from Home Assistant
void register_for_events() {
  // Only register for alarm state changes if an entity is configured
  if (config_manager.alarm_entity_id.length() > 0) {
    alarm_trigger_id = websocket.subscribe_to_trigger(String("{ \"platform\": \"state\", \"to\": null, \"entity_id\": \"") + config_manager.alarm_entity_id + String("\"}"));
    Log.infoln("Registered for alarm state changes: %s", config_manager.alarm_entity_id.c_str());
  } else {
    Log.infoln("No alarm entity configured, skipping event registration");
    alarm_trigger_id = -1;
  }
}



// Power management functions

// Enter deep sleep mode
void enter_deep_sleep() {
  // Calculate sleep time in microseconds
  uint64_t sleep_time_us = config_manager.sleep_duration_minutes * 60 * 1000000ULL;
  
  // Prepare display for sleep
  if (display) {
    display->sleep();
  }
  
  // Turn off LED
  setLEDPower(false);
  
  // Disconnect from WiFi to save power
  wifi_disconnect_if_needed();
  
  // Enable wake up timer
  Log.infoln("Going to sleep for %d minutes...", config_manager.sleep_duration_minutes);
  esp_sleep_enable_timer_wakeup(sleep_time_us);
  
  // Enter deep sleep
  esp_deep_sleep_start();
}

// Disconnect from WiFi if connected
void wifi_disconnect_if_needed() {
  if (WiFi.status() == WL_CONNECTED) {
    Log.infoln("Disconnecting WiFi to save power");
    
    // Clean up WebSocket connection
    if (websocket.available()) {
      websocket.disconnect();
    }
    
    // Disconnect from WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Log.verboseln("WiFi disconnected");
  }
}

/*
TODO: 
  * Enable push updates for all data properties - with throttling to protect the screen
  * Add more validation to web configuration form inputs
  * Add UI for power management settings
*/

