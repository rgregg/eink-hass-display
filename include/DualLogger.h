#ifndef DUAL_LOGGER_H
#define DUAL_LOGGER_H

#include <ArduinoLog.h>
#include <LittleFS.h>

#define MAX_LOG_FILES 4  // Keep 5 log files
#define LOG_FILENAME "/log.txt"
#define MAX_LOG_SIZE 50000  // 50 KB per file

// Custom log printer to handle Serial + File logging
class DualLogger : public Print {
public:
    size_t write(uint8_t c) {
        Serial.write(c);  // Print to Serial
        writeToFile(String((char)c));  // Print to File
        return 1;
    }

private:
    void writeToFile(const String &message) {
        File logFile = LittleFS.open(LOG_FILENAME, "a");  // Open in append mode
        if (!logFile) return;
        logFile.print(message);
        logFile.close();

        // Check file size and truncate if necessary
        if (LittleFS.open(LOG_FILENAME, "r").size() > MAX_LOG_SIZE) {
            Serial.println("Log file too large, rotating...");
            rotateLogs();
        }
    }

  void rotateLogs() {
      Serial.println("Rotating logs...");

      // Delete the oldest log
      String oldestFile = String(LOG_FILENAME) + String(".") + String(MAX_LOG_FILES);
      if (LittleFS.exists(oldestFile)) {
          LittleFS.remove(oldestFile);
      }

      // Rename logs: log4 -> log5, log3 -> log4, etc.
      for (int i = MAX_LOG_FILES - 1; i > 0; i--) {
          String oldName = String(LOG_FILENAME) + String(".") + String(i);
          String newName = String(LOG_FILENAME) + String(".") + String(i+1);
          if (LittleFS.exists(oldName)) {
              LittleFS.rename(oldName, newName);
          }
      }

      // Rename the current log file to log.txt.1
      if (LittleFS.exists(LOG_FILENAME)) {
          LittleFS.rename(LOG_FILENAME, LOG_FILENAME + String(".1"));
      }
  }

  void logToFile(Print* _logOutput, int level, const char* message) {
      File logFile = LittleFS.open(LOG_FILENAME, "a");
      if (!logFile) return;

      logFile.printf("[%lu] %s\n", millis(), message);
      logFile.close();

      // Check if the file size exceeds the limit
      if (LittleFS.open("/log.txt", "r").size() > MAX_LOG_SIZE) {
          rotateLogs();
      }
  }
};

#endif