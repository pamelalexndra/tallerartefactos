#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Configuracion WIFI
const char* WIFI_SSID = "pam";
const char* WIFI_PASSWORD = "internet1";

// Configuracino MQTT
const char* MQTT_SERVER = "172.21.214.48";
const uint16_t MQTT_PORT = 1883;

const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";

// pines
#define PIN_DHT 4
#define TIPO_DHT DHT11
#define PIN_LDR 32

DHT dht(PIN_DHT, TIPO_DHT);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const unsigned long INTERVALO_ENVIO = 2000;
unsigned long ultimoEnvio = 0;

void conectarWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("Conectando a WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {

  while (!mqttClient.connected()
         && WiFi.status() == WL_CONNECTED) {

    String clientId = "ESP32-";

    clientId += String(
      (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
      HEX
    );

    Serial.print("Conectando a Mosquitto...");

    if (mqttClient.connect(clientId.c_str())) {

      Serial.println(" conectado");

    } else {

      Serial.print(" error, rc=");
      Serial.println(mqttClient.state());

      delay(2000);
    }
  }
}

void publicarSensores() {

  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  if (isnan(temperatura) || isnan(humedad)) {

    Serial.println("Error leyendo DHT11");
    return;
  }

  int lecturaLDR = analogRead(PIN_LDR);

  int porcentajeLuz = map(
    lecturaLDR,
    0,
    4095,
    0,
    100
  );

  porcentajeLuz = constrain(
    porcentajeLuz,
    0,
    100
  );

  String tempTexto = String(temperatura, 1);
  String humTexto = String(humedad, 1);
  String luzTexto = String(porcentajeLuz);

  bool okTemp = mqttClient.publish(
    TOPIC_TEMPERATURA,
    tempTexto.c_str(),
    true
  );

  bool okHum = mqttClient.publish(
    TOPIC_HUMEDAD,
    humTexto.c_str(),
    true
  );

  bool okLuz = mqttClient.publish(
    TOPIC_LUZ,
    luzTexto.c_str(),
    true
  );

  Serial.print("Temperatura: ");
  Serial.print(temperatura, 1);
  Serial.println(" °C");

  Serial.print("Humedad: ");
  Serial.print(humedad, 1);
  Serial.println(" %");

  Serial.print("LDR ADC: ");
  Serial.print(lecturaLDR);

  Serial.print(" | Luz: ");
  Serial.print(porcentajeLuz);
  Serial.println(" %");

  if (okTemp && okHum && okLuz) {
    Serial.println("MQTT: publicado");
  } else {
    Serial.println("MQTT: fallo al publicar");
  }

  Serial.println("------------------------------");
}

void setup() {

  Serial.begin(115200);
  delay(300);

  dht.begin();

  pinMode(PIN_LDR, INPUT);

  analogReadResolution(12);

  conectarWiFi();

  mqttClient.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {

    ultimoEnvio = millis();

    publicarSensores();
  }
}