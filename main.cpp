#include <Arduino.h>
#include <IO7F32.h>

String user_html = "";

char* ssid_pfix = (char*)"fan1";  // SoftAP 이름 접두어
const int FAN_PIN = 21;             // 팬 제어 핀
const float TEMP_THRESHOLD = 28.0;  // 임계 온도
unsigned long lastPublishMillis = 0;

void publishData() {
    StaticJsonDocument<512> root;
    JsonObject data = root.createNestedObject("d");

    data["fan"] = digitalRead(FAN_PIN) == HIGH ? "on" : "off";

    serializeJson(root, msgBuffer);
    client.publish(evtTopic, msgBuffer);
}

void handleUserCommand(char* topic, JsonDocument* root) {
    JsonObject d = (*root)["d"];
    Serial.println(topic);

    if (d.containsKey("temperature")) {
        float temp = atof(d["temperature"] | "0.0");
        Serial.printf("Received temperature: %.2f°C\n", temp);

        if (temp >= TEMP_THRESHOLD) {
            digitalWrite(FAN_PIN, HIGH);  // 팬 ON
        } else {
            digitalWrite(FAN_PIN, LOW);   // 팬 OFF
        }

        lastPublishMillis = -pubInterval;  // 상태 재전송
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, LOW);

    initDevice();

    JsonObject meta = cfg["meta"];
    pubInterval = meta.containsKey("pubInterval") ? meta["pubInterval"] : 0;
    lastPublishMillis = -pubInterval;

    WiFi.mode(WIFI_STA);
    WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nIP address: ");
    Serial.println(WiFi.localIP());

    userCommand = handleUserCommand;
    set_iot_server();
    iot_connect();
}

void loop() {
    if (!client.connected()) {
        iot_connect();
    }
    client.loop();

    if ((pubInterval != 0) && (millis() - lastPublishMillis > pubInterval)) {
        publishData();
        lastPublishMillis = millis();
    }
}