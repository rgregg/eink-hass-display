#include "CaptivePortal.h"
#include <LittleFS.h>

CaptivePortal::CaptivePortal(ConfigManager& config_manager, DisplayManager* display)
  : _config_manager(config_manager), 
    _display(display), 
    _web_server(WEB_PORT),
    _restart_requested(false),
    _portal_start_time(0) {
}

void CaptivePortal::begin(const String& ap_name) {
  // Start the access point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_name.c_str());
  
  // Configure DNS server to redirect all domains to the ESP's IP
  IPAddress ap_ip = WiFi.softAPIP();
  _dns_server.start(DNS_PORT, "*", ap_ip);
  
  // Setup web server routes
  setup_routes();
  _web_server.begin();
  
  // Scan for networks
  scan_networks();
  
  // Display message on ePaper
  if (_display) {
    _display->show_message(ap_name, "http://" + ap_ip.toString());
  } else {
    Log.warningln("Display not initialized, unable to show captive portal message.");
  }
  
  // Record start time
  _portal_start_time = millis();
  
  Log.infoln("Captive portal started. SSID: %s, IP: %s", ap_name.c_str(), ap_ip.toString().c_str());
}

void CaptivePortal::handle_client() {
  // Process DNS requests
  _dns_server.processNextRequest();
  
  // Process web server requests
  _web_server.handleClient();
  
  // Check if portal timeout has been reached
  if (_portal_start_time > 0 && millis() - _portal_start_time > TIMEOUT_MS) {
    Log.infoln("Captive portal timeout reached. Restarting device...");
    if (_display) {
      _display->show_message("Portal Timeout", "Restarting...");
    }
    delay(2000);
    ESP.restart();
  }
}

bool CaptivePortal::should_restart() const {
  return _restart_requested;
}

void CaptivePortal::setup_routes() {
  // Root route (captive portal landing page)
  _web_server.on("/", [this]() {
    handle_root();
  });
  
  // Route to handle saving configuration
  _web_server.on("/save", HTTP_POST, [this]() {
    handle_save_config();
  });
  
  // CSS and JS static files
  _web_server.on("/styles.css", [this]() {
    serve_file_from_fs("/www/portal-styles.css", "text/css");
  });
  
  _web_server.on("/script.js", [this]() {
    serve_file_from_fs("/www/portal-script.js", "application/javascript");
  });
  
  // Default handler for all other requests
  _web_server.onNotFound([this]() {
    handle_not_found();
  });
}

void CaptivePortal::handle_root() {
  // Prepare current values to pre-populate the form
  String current_ssid = _config_manager.wifi_ssid;
  String current_hass_url = _config_manager.hass_url;
  String current_hass_token = _config_manager.hass_token;

  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>EPaper System Monitor Setup</title>
  <link rel="stylesheet" href="/styles.css">
</head>
<body>
  <div class="container">
    <h1>WiFi Setup</h1>
    
    <div class="status" id="status"></div>
    
    <form id="setupForm" action="/save" method="post">
      <div class="section">
        <h2>Available Networks</h2>
        <div class="networks-list">)";
  
  html += _networks_list;
  
  html += R"(
        </div>
        <div class="form-group">
          <label for="ssid">WiFi Network:</label>
          <input type="text" id="ssid" name="ssid" placeholder="Enter network name" value=")" + current_ssid + R"(">
        </div>
        <div class="form-group">
          <label for="password">WiFi Password:</label>
          <input type="password" id="password" name="password" placeholder="Enter password">
          <small>Leave blank to keep current password</small>
        </div>
      </div>
      
      <div class="section">
        <h2>Home Assistant Settings</h2>
        <div class="form-group">
          <label for="hass_url">Home Assistant WebSocket URL:</label>
          <input type="text" id="hass_url" name="hass_url" placeholder="ws://homeassistant.local:8123/api/websocket" value=")" + current_hass_url + R"(">
        </div>
        <div class="form-group">
          <label for="hass_token">Long-Lived Access Token:</label>
          <input type="password" id="hass_token" name="hass_token" placeholder="Enter token" value=")" + current_hass_token + R"(">
          <small>Leave blank to keep current token</small>
        </div>
      </div>
      
      <div class="form-group">
        <button type="submit">Save and Connect</button>
      </div>
    </form>
  </div>

  <script>
    document.addEventListener('DOMContentLoaded', function() {
      // Handle network selection
      const networkLinks = document.querySelectorAll('.network-item');
      networkLinks.forEach(link => {
        link.addEventListener('click', function(e) {
          e.preventDefault();
          document.getElementById('ssid').value = this.getAttribute('ssid').trim();
        });
      });
    });
  </script>
</body>
</html>
  )";
  
  _web_server.send(200, "text/html", html);
}

void CaptivePortal::handle_save_config() {
  // Make sure we have the latest configuration before making changes
  // This ensures we don't overwrite any settings that aren't part of the form
  _config_manager.load_config();
  
  // Get form parameters
  String ssid = _web_server.arg("ssid");
  String password = _web_server.arg("password");
  String hass_url = _web_server.arg("hass_url");
  String hass_token = _web_server.arg("hass_token");
  
  // Update configuration - only change what was provided
  if (ssid.length() > 0) {
    _config_manager.wifi_ssid = ssid;
  }
  
  if (password.length() > 0) {
    _config_manager.wifi_password = password;
  }
  
  if (hass_url.length() > 0) {
    _config_manager.hass_url = hass_url;
  }
  
  if (hass_token.length() > 0) {
    _config_manager.hass_token = hass_token;
  }
  
  // Save configuration
  bool success = _config_manager.save_config();
  
  if (success) {
    // Send success response
    String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>WiFi Setup Complete</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background-color: #f5f5f5;
      text-align: center;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      background-color: white;
      padding: 30px;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
    }
    h1 {
      color: #4CAF50;
    }
    p {
      font-size: 16px;
      line-height: 1.5;
    }
    .spinner {
      margin: 30px auto;
      border: 6px solid #f3f3f3;
      border-top: 6px solid #4CAF50;
      border-radius: 50%;
      width: 50px;
      height: 50px;
      animation: spin 2s linear infinite;
    }
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Setup Complete!</h1>
    <p>Your WiFi settings have been saved.</p>
    <p>The device will now restart and connect to your network.</p>
    <div class="spinner"></div>
  </div>
  <script>
    // Restart automatically
    setTimeout(function() {
      window.close();
    }, 5000);
  </script>
</body>
</html>
    )";
    
    _web_server.send(200, "text/html", html);
    
    // Show success on display
    if (_display) {
      _display->show_message("Setup Complete", "Connecting to: " + ssid);
    }
    
    // Set restart flag
    _restart_requested = true;
  } else {
    // Failed to save config
    _web_server.send(500, "text/plain", "Failed to save configuration");
  }
}

void CaptivePortal::handle_not_found() {
  // For the captive portal to work, we redirect all requests to the root
  _web_server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  _web_server.send(302, "text/plain", "");
}

bool CaptivePortal::serve_file_from_fs(const String& path, const String& content_type) {
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    if (file) {
      _web_server.streamFile(file, content_type);
      file.close();
      return true;
    }
  }
  
  // If we get here, serve a default response
  if (content_type == "text/css") {
    // Default CSS if file not found
    _web_server.send(200, "text/css", R"(
      body {
        font-family: Arial, sans-serif;
        margin: 0;
        padding: 20px;
        background-color: #f5f5f5;
      }
      .container {
        max-width: 600px;
        margin: 0 auto;
        background-color: white;
        padding: 20px;
        border-radius: 8px;
        box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
      }
      h1 {
        color: #333;
        margin-top: 0;
        text-align: center;
      }
      h2 {
        font-size: 18px;
        margin-bottom: 10px;
      }
      .form-group {
        margin-bottom: 15px;
      }
      label {
        display: block;
        margin-bottom: 5px;
        font-weight: bold;
      }
      input[type="text"],
      input[type="password"] {
        width: 100%;
        padding: 8px;
        border: 1px solid #ddd;
        border-radius: 4px;
        box-sizing: border-box;
      }
      button {
        background-color: #4CAF50;
        color: white;
        border: none;
        padding: 10px 15px;
        border-radius: 4px;
        cursor: pointer;
        font-size: 16px;
      }
      button:hover {
        background-color: #45a049;
      }
      .section {
        border-top: 1px solid #eee;
        padding-top: 20px;
        margin-top: 20px;
      }
      .networks-list {
        margin-bottom: 15px;
        max-height: 150px;
        overflow-y: auto;
        border: 1px solid #ddd;
        border-radius: 4px;
        padding: 8px;
      }
      .network-item {
        padding: 8px;
        cursor: pointer;
        border-bottom: 1px solid #eee;
      }
      .network-item:hover {
        background-color: #f9f9f9;
      }
      .network-item .signal {
        float: right;
        color: #888;
      }
    )");
    return true;
  }
  
  // We didn't serve anything
  return false;
}

void CaptivePortal::scan_networks() {
  // Clear old list
  _networks_list = "";
  
  // Scan for networks
  int networks = WiFi.scanNetworks();
  Log.infoln("Found %d networks", networks);
  
  if (networks == 0) {
    _networks_list = "<div class='network-item'>No networks found</div>";
  } else {
    // Sort networks by RSSI (signal strength)
    struct NetworkInfo {
      String ssid;
      int rssi;
      bool isEncrypted;
    };
    
    std::vector<NetworkInfo> networkList;
    
    for (int i = 0; i < networks; i++) {
      if (WiFi.SSID(i).length() > 0) {  // Skip hidden networks
        NetworkInfo net;
        net.ssid = WiFi.SSID(i);
        net.rssi = WiFi.RSSI(i);
        net.isEncrypted = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        networkList.push_back(net);
      }
    }
    
    // Sort by signal strength
    std::sort(networkList.begin(), networkList.end(), 
      [](const NetworkInfo& a, const NetworkInfo& b) {
        return a.rssi > b.rssi;
      }
    );
    
    // Generate HTML
    for (const auto& net : networkList) {
      String signalStrength;
      if (net.rssi > -50) {
        signalStrength = "Strong";
      } else if (net.rssi > -70) {
        signalStrength = "Good";
      } else if (net.rssi > -80) {
        signalStrength = "Fair";
      } else {
        signalStrength = "Weak";
      }
      
      String secureIcon = net.isEncrypted ? "🔒 " : "";
      
      _networks_list += "<div class='network-item' ssid='" + net.ssid + "'>" + secureIcon + net.ssid;
      _networks_list += "<span class='signal'>" + signalStrength + "</span></div>";
    }
  }
  
  // Free memory used by scan
  WiFi.scanDelete();
}