#ifndef EPAPER_213_MONO_DISPLAY_MANAGER_H
#define EPAPER_213_MONO_DISPLAY_MANAGER_H

#include "DisplayManager.h"
#include "Adafruit_ThinkInk.h"
#include <array>
#include "RequestData.h"

// ePaper Display IO details - hardcoded for 2.13" mono display
#define EPD_DC 10
#define EPD_CS 9
#define EPD_BUSY -1 // can set to -1 to not use a pin (will wait a fixed delay)
#define EPD_SRCS 6  // SRAM select pin
#define EPD_RST -1  // can set to -1 and share with microcontroller Reset!
#define EPD_SPI &SPI // primary SPI

// Using data point names from main sketch
extern const char* DATA_TEMPERATURE;
extern const char* DATA_CONDITIONS;
extern const char* DATA_ALARM;

// Forward declaration of ConfigManager
class ConfigManager;
extern ConfigManager config_manager;

class EPaper213MonoDisplayManager : public DisplayManager {
public:
  EPaper213MonoDisplayManager();
  
  // Initialize the display
  void begin() override;
  
  // Show a message with optional second line
  void show_message(const String& message, const String& second_line = "") override;
  
  // Update display with all data points
  void update_display(const std::array<RequestData, 3>& data_points, bool force_refresh = false, String battery_level = "") override;
  
  // Draw a bitmap from the filesystem
  void draw_bitmap_from_path(const char *path, int x, int y) override;
  
  // Put display into sleep mode to save power
  void sleep() override;
  
private:
  ThinkInk_213_Mono_GDEY0213B74 _display;
  
  // Helper method to determine weather icon path based on condition
  String determine_weather_icon_path(const String& weather_condition);
};

#endif // EPAPER_213_MONO_DISPLAY_MANAGER_H