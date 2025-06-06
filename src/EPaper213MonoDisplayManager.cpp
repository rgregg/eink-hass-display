#include "EPaper213MonoDisplayManager.h"
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include "FS.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <ArduinoLog.h>
#include "ConfigManager.h"

EPaper213MonoDisplayManager::EPaper213MonoDisplayManager() 
  : _display(EPD_DC, EPD_RST, EPD_CS, EPD_SRCS, EPD_BUSY, EPD_SPI) {
}

void EPaper213MonoDisplayManager::begin() {
  _display.begin();
  _width = _display.width();
  _height = _display.height();
}

void EPaper213MonoDisplayManager::show_message(const String& message, const String& second_line) {
  _display.clearBuffer();
  
  // Display first line (larger font)
  _display.setFont(&FreeSansBold12pt7b);
  _display.setTextSize(1);
  _display.setTextColor(EPD_BLACK);
  _display.setCursor(10, 40);
  _display.print(message);
  
  // Display second line if provided (smaller font)
  if (second_line.length() > 0) {
    _display.setFont(&FreeSans9pt7b);
    _display.setTextSize(1);
    _display.setCursor(10, 70);
    _display.print(second_line);
  }
  
  _display.display();  // Full refresh
}

void EPaper213MonoDisplayManager::update_display(const std::array<RequestData, 3>& data_points, bool force_refresh, String battery_level) {
  Log.verboseln("Updating display with latest data");
  
  // Check if refresh is needed
  bool should_refresh = needs_refresh(data_points, force_refresh);
  if (!should_refresh) {
    Log.infoln("No values have changed - skipping screen refresh.");
    return;
  }
  
  // Clear any previous content
  _display.clearBuffer();
  
  // Layout constants
  int section_width = _width / 2;
  const int temp_x = 16, temp_y = 40;
  const int BITMAP_X = 176, BITMAP_Y = 12;
  
  // Draw temperature
  _display.setCursor(16, _height / 2);
  _display.setFont(&FreeSansBold12pt7b);
  _display.setTextColor(EPD_BLACK, EPD_WHITE);
  _display.setTextSize(3);
  String temperature = get_data_value(data_points, DATA_TEMPERATURE);
  _display.print(temperature);
  _display.setTextSize(1);
  _display.print(config_manager.temperature_unit);
  
  // Draw weather conditions icon
  String weather_condition = get_data_value(data_points, DATA_CONDITIONS);
  String path = determine_weather_icon_path(weather_condition);
  draw_bitmap_from_path(path.c_str(), BITMAP_X, BITMAP_Y);
  
  // Draw alarm state (if configured)
  const int alarm_y = 108;
  int alarm_x = temp_x;
  const int box_height = 122-88, box_y = 88;
  
  // Check if alarm entity is configured
  if (config_manager.alarm_entity_id.length() > 0) {
    _display.setFont(&FreeSansBold12pt7b);
    _display.setTextSize(1);
    
    String alarm_state = get_data_value(data_points, DATA_ALARM);
    if (alarm_state == "armed_home") { 
      _display.fillRect(0, 88, _width, box_height, EPD_BLACK);
      _display.setTextColor(EPD_WHITE, EPD_BLACK);
      alarm_state = "ARMED - HOME";
    } else if (alarm_state == "armed_away") {
      _display.fillRect(0, 88, _width, box_height, EPD_BLACK);
      _display.setTextColor(EPD_WHITE, EPD_BLACK);
      alarm_state = "ARMED - AWAY";    
    } else if (alarm_state == "disarmed") {
      _display.setTextColor(EPD_BLACK, EPD_WHITE);
      alarm_state = "DISARMED";
    } else {
      _display.setTextColor(EPD_BLACK, EPD_WHITE);
      alarm_state = "UNKNOWN";
    }
    _display.setCursor(alarm_x, alarm_y);
    _display.print(alarm_state);
  } else {
    // No alarm entity configured - leave this section blank
    _display.setTextColor(EPD_BLACK, EPD_WHITE);
  }
  
  // Draw IP address
  _display.setFont();
  _display.setTextSize(1);
  _display.setCursor(alarm_x, _height - 9);
  String ipString = WiFi.localIP().toString();
  _display.print(ipString);

  if (battery_level.length() > 0) {
    int16_t x1, y1;
    uint16_t w, h;
    _display.getTextBounds(battery_level, 0, 0, &x1, &y1, &w, &h);
    _display.setCursor(_width - (w + 10), _height - 9);
    _display.print(battery_level);
  }
  
  // Update the physical display
  _display.display();
  
  // Reset the changed flags after display update
  for (const RequestData& data : data_points) {
    const_cast<RequestData&>(data).has_value_changed = false;
  }
}

void EPaper213MonoDisplayManager::draw_bitmap_from_path(const char *path, int x, int y) {
  File bmpFile = LittleFS.open(path);
  if (!bmpFile) {
    Log.warningln("Failed to open BMP file: %s", path);
    return;
  }
  
  // Skip BMP Header (54 bytes)
  bmpFile.seek(54);
  
  int bmpWidth = 64;  // Change to match your BMP dimensions
  int bmpHeight = 64;
  int rowPadding = (4 - (bmpWidth % 4)) % 4;  // BMP rows are padded to 4-byte alignment
  GFXcanvas1 canvas(bmpWidth, bmpHeight);  // 1-bit buffer
  
  for (int row = bmpHeight - 1; row >= 0; row--) {  // BMP starts from the bottom row
    for (int col = 0; col < bmpWidth; col++) {
      uint8_t b = bmpFile.read();
      uint8_t g = bmpFile.read();
      uint8_t r = bmpFile.read();
      
      // Convert to grayscale: (0.3 * R + 0.59 * G + 0.11 * B)
      uint8_t gray = (r * 30 + g * 59 + b * 11) / 100;
      
      // Convert to 1-bit (black/white) thresholding
      if (gray < 128) {
        canvas.drawPixel(col, row, 1);  // Black
      } else {
        canvas.drawPixel(col, row, 0);  // White
      }
    }
    bmpFile.seek(bmpFile.position() + rowPadding);  // Skip row padding
  }
  bmpFile.close();
  
  _display.drawBitmap(x, y, canvas.getBuffer(), bmpWidth, bmpHeight, EPD_BLACK);
  Log.verboseln("BMP drawn: %s", path);
}

void EPaper213MonoDisplayManager::sleep() {
  // Put the display in sleep mode to save power
  _display.powerDown();
  Log.verboseln("E-paper display powered down");
}

String EPaper213MonoDisplayManager::determine_weather_icon_path(const String& weather_condition) {
  String path = "/day/no_image.bmp";
  
  // Based on values here: https://developers.home-assistant.io/docs/core/entity/weather/
  if (weather_condition == "clear-night") {
    path = "/night/clear.bmp";
  } else if (weather_condition == "cloudy") {
    path = "/day/cloudy.bmp";
  } else if (weather_condition == "fog") {
    path = "/day/foggy.bmp";
  } else if (weather_condition == "lightning") {
    path = "/day/storm.bmp";
  } else if (weather_condition == "lightning-rainy") {
    path = "/day/storm.bmp";
  } else if (weather_condition == "partlycloudy") {
    path = "/day/partly-cloudy.bmp";
  } else if (weather_condition == "pouring") {
    path = "/day/rain.bmp";
  } else if (weather_condition == "rainy") {
    path = "/day/rain.bmp";
  } else if (weather_condition == "snowy") {
    path = "/day/snow.bmp";
  } else if (weather_condition == "snowy-rainy") {
    path = "/day/sleet.bmp";
  } else if (weather_condition == "sunny") {
    path = "/day/sunny.bmp";
  } else if (weather_condition == "windy") {
    path = "/day/wind.bmp";
  } else if (weather_condition == "windy-variant") {
    path = "/day/wind.bmp";
  } else {
    Log.infoln("Unexpected weather condition: %s", weather_condition);
  }
  
  return path;
}