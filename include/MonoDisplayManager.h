#ifndef MONO_DISPLAY_MANAGER_H
#define MONO_DISPLAY_MANAGER_H

#include "DisplayManager.h"
#include "Adafruit_ThinkInk.h"
#include <array>
#include "RequestData.h"
#include "GPIO.h"

// ePaper Display IO details - hardcoded for 2.13" mono display
#define FEATHERWING_EPD_DC GPIO_D10 // Display data/command pin
#define FEATHERWING_EPD_CS GPIO_D9  // Display chip select pin
#define FEATHERWING_EPD_RESET -1 // Display reset pin (can set to -1 to share with microcontroller)
#define FEATHERWING_EPD_BUSY -1 // can set to -1 to not use a pin (will wait a fixed delay)
#define FEATHERWING_EPD_SRCS GPIO_D6  // SRAM select pin
#define FEATHERWING_ERD_SDCS GPIO_D5  // SD card chip select pin

// Using data point names from main
extern const char* DATA_TEMPERATURE;
extern const char* DATA_CONDITIONS;
extern const char* DATA_ALARM;

// Forward declaration of ConfigManager
class ConfigManager;
extern ConfigManager config_manager;

class MonoDisplayManager : public DisplayManager {
public:
  MonoDisplayManager(bool triColorScreen = false);
  
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
  void full_refresh();

  Adafruit_EPD *_display;
  int _extra_wait_time = 0;
};

#endif // MONO_DISPLAY_MANAGER_H