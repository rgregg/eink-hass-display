#include "HassWebsocketManager.h"
#include <ArduinoLog.h>
#include <WiFi.h>


using namespace websockets;

HassWebsocketManager::HassWebsocketManager()
{   
    ws_client.onMessage([&](WebsocketsMessage message) {
        Log.infoln("Received WS message: %s", message.data().c_str());
        process_websocket_message(message.data());
    });
    
    ws_client.onEvent([&](WebsocketsEvent event, String data) {
        if(event == WebsocketsEvent::ConnectionOpened) {
            Log.infoln("Connnection Opened");
        } else if(event == WebsocketsEvent::ConnectionClosed) {
            Log.warningln("WebSocket disconnected! Attempting to reconnect...");
            delay(5000);  // Wait before retrying
            connect(this->websocket_url, this->auth_token);  // Try reconnecting
        }
    });
}

void HassWebsocketManager::connect(String url, String auth_token) {
    if (ws_client.available()) {
        Log.infoln("WebSocket is already connected.");
        return;
    }

    this->websocket_url = url;
    this->auth_token = auth_token;

    bool connected = ws_client.connect(url);
    if (connected) {
        Log.infoln("Connected to HASS websocket: %s", url.c_str());
        ws_client.send("{\"type\": \"auth\", \"access_token\": \"" + auth_token + "\"}");
    } else {
        Serial.println("WebSocket connection failed!");
        error_callback(-1, "WS connection failed");
    }
}

void HassWebsocketManager::disconnect() {
    if (ws_client.available()) {
       ws_client.close();
    }
}

void HassWebsocketManager::loop() {
    ws_client.poll();
}


int HassWebsocketManager::send_message(String json_text) {
    if (json_text.length() == 0) {
        Log.warningln("Error: Cannot send an empty message.");
        return -1;  // Indicate error
    }
    
    int id = request_id++;

    // Build the request JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, json_text);
    if (error) {
        Log.errorln("Error parsing JSON: %s", error.c_str());
        return -1;
    }
    
    doc["id"] = id;  // Ensure 'id' is preserved

    String request_json;
    serializeJson(doc, request_json);
    Log.verboseln("Sending WS message: %s", request_json.c_str());

    if (ws_client.available()) {
        ws_client.send(request_json);
    } else {
        Log.errorln("Websocket not available, message not sent");
        return -1;
    }
    
    return id;
}

void HassWebsocketManager::process_websocket_message(String json_text) {
    // Process message (parse JSON, extract sensor values, etc.)
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, json_text);
    if (error) {
        Log.errorln("Error parsing JSON: %s", error.c_str());
        if (error_callback != nullptr) {
            error_callback(-1, error.c_str());
        }
        return;
    }

    int request_id = doc["id"].as<int>();
    String type = doc["type"];
    Log.verboseln("Received response to request_id %d with type %s", request_id, type);

    if (type == "auth_required" || type == "auth_ok" || type == "result") {
      return;
    }


    if (data_callback != nullptr) {
        data_callback(request_id, type, doc);
    }
}

bool HassWebsocketManager::available() {
  return ws_client.available();
}


int HassWebsocketManager::ping() {
    return send_message("{\"type\": \"ping\"}");
}

int HassWebsocketManager::subscribe_to_event(String event_type) {
    if (event_type.length() == 0) {
        Log.warningln("Error: Cannot subscribe to an empty event.");
        return -1;
    }
    return send_message("{\"type\": \"subscribe_events\", \"event_type\": \"" + event_type + "\"}");
}

int HassWebsocketManager::subscribe_to_trigger(String trigger) {
  if (trigger.length() == 0) {
    Log.warningln("Error: Cannot subscribe to an empty trigger");
    return -1;
  }
  return send_message("{\"type\": \"subscribe_trigger\", \"trigger\": " + trigger + " }");
}

int HassWebsocketManager::unsubscribe_from_event(int request_id) {
    if (request_id < 1) {
        Log.warningln("Error: Cannot unsubscribe from an invalid request ID.");
        return -1;
    }

    return send_message("{\"type\": \"unsubscribe_events\", \"subscription\": \"" + String(request_id) + "\"}");
}

int HassWebsocketManager::render_template(String templateStr) {
    if (templateStr.length() == 0) {
        Log.warningln("Error: Cannot render an empty template.");
        return -1;
    }

    return send_message("{\"type\": \"render_template\", \"template\": \"" + templateStr + "\"}");
}

void HassWebsocketManager::setMessageCallback(DataCallback callback) {
    this->data_callback = callback;
}

void HassWebsocketManager::setErrorCallback(ErrorCallback callback) {
    this->error_callback = callback;
}