#ifndef REQUEST_DATA_H  // Include guard
#define REQUEST_DATA_H

#include <Arduino.h>
#include <ArduinoLog.h>

struct RequestData {
    int pending_request_id;
    String name;
    String latest_value;
    String previous_value;
    String templateStr;
    bool has_value_changed = false;

    // Constructor (optional)
    RequestData() : pending_request_id(0), name(""), latest_value(""), templateStr("") {}

    // Helper function to display data
    void print() {
        Serial.println("Request ID: " + String(pending_request_id));
        Serial.println("Name: " + name);
        Serial.println("Latest Value: " + latest_value);
        Serial.println("Template: " + templateStr);
    }

    void update_value(String newValue) {
      if (previous_value != newValue)
      {
        previous_value = latest_value;
        latest_value = newValue;
        has_value_changed = true;
        Log.verboseln("Value changed for %s from %s to %s", name, previous_value, latest_value);
      }
    }
};

#endif  // REQUEST_DATA_H