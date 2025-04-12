#include "IconManager.h"
#include <ArduinoLog.h>

String IconManager::determine_weather_icon_path(const String& weather_condition) {
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