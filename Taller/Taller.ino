#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#define DHTPIN 4
#define DHTTYPE DHT11
const int LDR_PIN = 32;
DHT dht(DHTPIN, DHTTYPE);

// ======================================================
// CONFIGURACIÓN WIFI
// ======================================================

const char* WIFI_SSID = "Flipi";
const char* WIFI_PASSWORD = "FAS12345";


// ======================================================
// CONFIGURACIÓN MQTT
// ======================================================


const char* MQTT_SERVER = "172.25.239.67";
const int MQTT_PORT = 1883;

// Topics utilizados por el dashboard React.
const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";


// ======================================================
// CONFIGURACIÓN DE SENSORES
// ======================================================

// (Estudiante)
// Definir los pines y objetos necesarios para los sensores.

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;

unsigned long ultimoEnvio = 0;

void conectarWiFi() {

  Serial.print("Conectando a WiFi");

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

  while (!mqttClient.connected()) {

    Serial.print("Conectando a Mosquitto...");

    // Crear un Client ID diferente para este ESP32.
    String clientId = "ESP32-";
    clientId += String(
      (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
      HEX
    );

    if (mqttClient.connect(clientId.c_str())) {

      Serial.println(" conectado");

    } else {

      Serial.print(" error, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 segundos...");

      delay(2000);
    }
  }
}


// ======================================================
// LEER Y PUBLICAR SENSORES
// ======================================================

void publicarSensores() {

  /*
    (Estudiante)
      1. Obtener la temperatura.

      2. Obtener la humedad.

      3. Obtener la lectura del sensor de iluminación.

      4. Preparar los valores para poder publicarlos mediante MQTT.

      5. Publicar cada dato en su topic correspondiente:

         TOPIC_TEMPERATURA
         TOPIC_HUMEDAD
         TOPIC_LUZ
         
      6. Mostrar las mediciones en el Monitor Serial.
  */

  float tempActual = dht.readTemperature();
  float humActual = dht.readHumidity();
  int valorCrudoLDR = analogRead(LDR_PIN);
  int nivelIluminacion = (valorCrudoLDR * 100) / 4095;

  if (isnan(tempActual) || isnan(humActual)) {
    Serial.println("Error al leer el DHT11");
    return;
  }

  String textoTemp = String(tempActual, 1);
  String textoHum = String(humActual, 0);
  String textoLuz = String(nivelIluminacion);

  mqttClient.publish(TOPIC_TEMPERATURA, textoTemp.c_str(), true);
  mqttClient.publish(TOPIC_HUMEDAD, textoHum.c_str(), true);
  mqttClient.publish(TOPIC_LUZ, textoLuz.c_str(), true);

  Serial.printf("Temperatura: %.2f °C\n", tempActual);
  Serial.printf("Humedad: %.2f %%\n", humActual);
  Serial.printf("LDR ADC: %d | Luz: %d %%\n", valorCrudoLDR, nivelIluminacion);
  Serial.println("------------------------------");


  
}

void setup() {

  Serial.begin(115200);

  // (Estudiante)
  // Inicializar los sensores y configurar los pines necesarios.

  dht.begin();
  pinMode(LDR_PIN, INPUT);
  analogReadResolution(12);

  conectarWiFi();

  mqttClient.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );
}

void loop() {

  // Si se pierde WiFi, volver a conectarse.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Si se pierde MQTT, volver a conectarse.
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();


  // Publicar sensores cada 2 segundos.
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {

    ultimoEnvio = millis();

    publicarSensores();
  }
}