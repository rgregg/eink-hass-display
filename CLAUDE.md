# EPaperSystemMonitor Development Guide

## Build & Flash Commands
- Compile: Arduino IDE → Verify button or Ctrl+R
- Upload: Arduino IDE → Upload button or Ctrl+U
- Monitor serial output: Arduino IDE → Serial Monitor or Ctrl+Shift+M
- Flash filesystem: Use ESP32 Sketch Data Upload tool (Tools → ESP32 Sketch Data Upload)

## Code Style Guidelines
- Indentation: 2 spaces
- Naming: snake_case for variables/methods, PascalCase for classes
- Header files: Use proper include guards (#ifndef/#define/#endif)
- Functions: Concise, single-purpose with descriptive names
- Error handling: Log errors to ArduinoLog and implement appropriate recovery
- Comments: Document class purposes and non-obvious implementation details
- Libraries: ArduinoJson for parsing, ArduinoWebsockets for communication
- Memory: Prefer stack allocation, minimize dynamic allocation
- Files: Keep related functionality in dedicated classes

## Project Structure
- Main sketch (.ino): Contains setup() and loop()
- ConfigManager.h/.cpp: Manages configuration settings (Wi-Fi, Home Assistant)
- WebConfigServer.h/.cpp: Implements web-based configuration interface
- EPaper213MonoDisplayManager.h/.cpp: Display handling with hardcoded pin configuration
- Class implementations: Each in separate .h/.cpp pairs
- Hardware-specific settings: Kept in their respective manager classes

## Web Configuration
- After connecting to WiFi, access configuration at: http://[device-ip]/
- Settings can be modified and saved without restarting
- Use "Save and Restart" to apply changes requiring a restart (WiFi, HASS connection settings)
- All settings are stored in LittleFS and persist across reboots
- Web interface files are stored in LittleFS under /www directory:
  - /www/index.html - Main HTML interface
  - /www/styles.css - CSS styles
  - /www/script.js - JavaScript for dynamic functionality

## Captive Portal for WiFi Setup
- If WiFi connection fails, the device automatically starts in captive portal mode
- Creates an access point with name "EPaperMonitor-XXXX" (where XXXX is a random number)
- Connect to this network with any device (phone, laptop, etc.)
- A configuration page will automatically open (or navigate to 192.168.4.1)
- The page shows available WiFi networks and allows configuration of:
  - WiFi credentials
  - Home Assistant connection parameters
- After saving, the device will restart and connect to the configured WiFi network
- The captive portal has a 3-minute timeout, after which the device will restart