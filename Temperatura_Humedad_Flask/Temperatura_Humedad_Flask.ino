#include <Wire.h>
#include "SparkFun_SHTC3.h"
#include <WiFi.h>
#include <HTTPClient.h> // Nueva librería para hacer peticiones HTTP

const char* ssid = "WIFI-DCI";
const char* password = "DComInf_2K24";
const char* serverUrl = "http://52.200.139.211:5000/endpoint"; // Reemplaza <TU_IP_ELASTICA> y <PUERTO> con la IP elástica y puerto de tu servidor Flask

SHTC3 g_shtc3;
WiFiClient wifiClient;
HTTPClient http; // Nueva instancia de HTTPClient

void errorDecoder(SHTC3_Status_TypeDef message) {
  switch (message) {
    case SHTC3_Status_Nominal:
      Serial.print("Nominal");
      break;
    case SHTC3_Status_Error:
      Serial.print("Error");
      break;
    case SHTC3_Status_CRC_Fail:
      Serial.print("CRC Fail");
      break;
    default:
      Serial.print("Unknown return code");
      break;
  }
}

void shtc3_read_data() {
  float Temperature = 0;
  float Humidity = 0;

  g_shtc3.update();
  if (g_shtc3.lastStatus == SHTC3_Status_Nominal) {
    Temperature = g_shtc3.toDegC();
    Humidity = g_shtc3.toPercent();

    Serial.print("RH = ");
    Serial.print(g_shtc3.toPercent());
    Serial.print("% (checksum: ");

    if (g_shtc3.passRHcrc) {
      Serial.print("pass");
    } else {
      Serial.print("fail");
    }

    Serial.print("), T = ");
    Serial.print(g_shtc3.toDegC());
    Serial.print(" deg C (checksum: ");

    if (g_shtc3.passTcrc) {
      Serial.print("pass");
    } else {
      Serial.print("fail");
    }
    Serial.println(")");
  } else {
    Serial.print("Update failed, error: ");
    errorDecoder(g_shtc3.lastStatus);
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Conexión a la red WiFi
  WiFi.begin(ssid, password);
  Serial.println("Conectando al WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Dirección IP obtenida: ");
  Serial.println(WiFi.localIP());

  if (g_shtc3.begin() != SHTC3_Status_Nominal) {
    Serial.println("Error al inicializar el sensor SHTC3");
    while (1);
  }
}

void loop() {
  shtc3_read_data();

  float temperature = g_shtc3.toDegC();
  float humidity = g_shtc3.toPercent();

  // Construir el mensaje JSON
  String payload = "{\"temperature\":" + String(temperature) + ",\"humidity\":" + String(humidity) + "}";

  // Enviar los datos al servidor Flask
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(wifiClient, serverUrl);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Datos enviados correctamente: " + payload);
      Serial.println("Respuesta del servidor: " + response);
    } else {
      Serial.println("Error al enviar datos: " + String(httpResponseCode));
    }

    http.end(); // Terminar la conexión
  } else {
    Serial.println("Error al conectar al WiFi");
  }

  delay(5000); // Modifica si quieres cambiar la frecuencia de envío
}
