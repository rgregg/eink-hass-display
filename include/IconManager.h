#ifndef ICON_MANAGER_H
#define ICON_MANAGER_H

#include <Arduino.h>

class IconManager {

public:

  // Helper method to determine weather icon path based on condition
  static String determine_weather_icon_path(const String& weather_condition);

};

#endif