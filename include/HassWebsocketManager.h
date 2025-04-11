#ifndef HASS_WEBSOCKET_MANAGER_H
#define HASS_WEBSOCKET_MANAGER_H

#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <WiFi.h>

using namespace websockets;

class HassWebsocketManager {
    public:
    // Function pointer for receiving data back from the websocket. Includes the request ID, response type, and data.
    typedef void (*DataCallback)(int, String, JsonDocument&);
    typedef void (*ErrorCallback)(int, String);

    HassWebsocketManager(WiFiClient &wifi_client);
    HassWebsocketManager();

    void connect(String url, String auth_token);
    void disconnect();
    void loop();

    // Send a request to the websocket, returns the request ID
    int send_message(String message);

    // Helpers for managing subscriptions
    int subscribe_to_event(String event_type);
    int unsubscribe_from_event(int request_id);

    int subscribe_to_trigger(String trigger);

    int render_template(String templateStr);
    int ping();

    // Message callbacks
    void setMessageCallback(DataCallback callback);
    void setErrorCallback(ErrorCallback callback);

    bool available();

    private:
    // internal variables
    WebsocketsClient ws_client;
    int request_id = 1;                         // Next request ID, initialized as 1
    DataCallback data_callback = nullptr;        // Function pointer, initialized as null
    ErrorCallback error_callback = nullptr;      // Function pointer, initialized as null
    void process_websocket_message(String message);
    String websocket_url;
    String auth_token;
};    

#endif