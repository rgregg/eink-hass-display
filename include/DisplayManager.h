#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "RequestData.h"
#include <array>

class DisplayManager {
public:
  // Constructor/Destructor
  DisplayManager() {}
  virtual ~DisplayManager() {}
  
  // Initialization
  virtual void begin() = 0;
  
  // Display common methods
  virtual void show_message(const String& message, const String& second_line = "") = 0;
  virtual void update_display(const std::array<RequestData, 3>& data_points, bool force_refresh = false, String battery_level = "") = 0;
  
  // Image handling
  virtual void draw_bitmap_from_path(const char* path, int x, int y) = 0;
  
  // Power management
  virtual void sleep() = 0;
  
protected:
  // Common display properties
  int _width;
  int _height;
  
  // Helper method to determine if refresh is needed based on data changes
  bool needs_refresh(const std::array<RequestData, 3>& data_points, bool force_refresh) {
    if (force_refresh) return true;
    
    for (const RequestData& data : data_points) {
      if (data.has_value_changed) {
        return true;
      }
    }
    return false;
  }
  
  // Get value from data points by name
  String get_data_value(const std::array<RequestData, 3>& data_points, const String& name) {
    for (const RequestData& data : data_points) {
      if (data.name == name && data.latest_value.length() > 0) {
        return data.latest_value;
      }
    }
    return "---";
  }
};

#endif // DISPLAY_MANAGER_H