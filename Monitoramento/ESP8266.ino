#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <BH1750.h>

// ==================== DEFINIÇÕES ====================
#define NUMERO_AMOSTRAS 10 // Número de amostras para média
#define DEBUG true // Ativa/desativa mensagens de debug
#define MAX_SENSOR 5 // Número máximo de sensores DS18B20 suportados
#define DHTTYPE DHT22 // Tipo de sensor DHT usado
#define DHTPIN1 7 // Pino do primeiro sensor DHT22
#define DHTPIN2 5 // Pino do segundo sensor DHT22
#define R1 10000 // Resistor de 10kΩ usado no divisor de tensão
// Pinos analógicos para os sensores de umidade do solo capacitivo
#define CAPACIPIN1 A0
#define CAPACIPIN2 A1
#define CAPACIPIN3 A5
#define CAPACIPIN4 A6
#define CAPACIPIN5 A7
#define CAPACIPIN6 A8
#define CAPACIPIN7 A9
#define CAPACIPIN8 A10
#define CAPACIPIN9 A11
#define CAPACIPIN10 A12
// Pinos analógicos para os sensores de umidade do solo resistivo
#define RESISTPIN1 A2
#define RESISTPIN2 A3
#define RESISTPIN3 A4

// ==================== OBJETOS DOS SENSORES ====================
OneWire oneWire(12); // Comunicação OneWire no pino 12
DallasTemperature sensor(&oneWire); // Objeto de leitura DS18B20
DeviceAddress enderecos[MAX_SENSOR]; // Vetor para armazenar endereços dos sensores DS18B20
BH1750 sensorLux; // Objeto para sensor de luminosidade
DHT dht1(DHTPIN1, DHTTYPE); // Objeto do primeiro sensor DHT22
DHT dht2(DHTPIN2, DHTTYPE); // Objeto do segundo sensor DHT22

// ==================== VARIÁVEIS GLOBAIS ====================
int nsensor, somatorio;
float media, leitura, resis_solo, tensao;
int umidadeCapaci;
int umidadeResist;
uint16_t luminosidade;


// ==================== SETUP ====================
void setup(){
  Serial.begin(9600); // Comunicação serial a 9600 bps
  Serial1.begin(115200); // Comunicação com ESP8266 (baud rate padrão AT)
  
  // Reinicia o módulo e espera resposta completa
  sendData("AT+RST\r\n", 3000, DEBUG);
  delay(5000);

  // Verifica firmware
  sendData("AT+GMR\r\n", 1000, DEBUG);

  // Modo estação
  sendData("AT+CWMODE=1\r\n", 2000, DEBUG);
  delay(2000);

  // Conecta Wi-Fi
  sendData("AT+CWJAP=\"WIFI_UFSJ\",\"\"\r\n", 5000, DEBUG);
  delay(8000);

  // Mostra IP obtido
  sendData("AT+CIFSR\r\n", 2000, DEBUG);
  delay(1000);

  // Habilita múltiplas conexões
  sendData("AT+CIPMUX=1\r\n", 1000, DEBUG);
  delay(1000);

  // Inicia servidor na porta 80 (HTTP)
  sendData("AT+CIPSERVER=1,80\r\n", 1000, DEBUG);
  delay(1000);

  // Inicialização dos sensores
  sensor.begin();
  sensorLux.begin();
  dht1.begin();
  dht2.begin();

  // Configuração dos pinos como entrada
  pinMode(CAPACIPIN1, INPUT);
  pinMode(CAPACIPIN2, INPUT);
  pinMode(CAPACIPIN3, INPUT);
  pinMode(CAPACIPIN4, INPUT);
  pinMode(CAPACIPIN5, INPUT);
  pinMode(CAPACIPIN6, INPUT);
  pinMode(CAPACIPIN7, INPUT);
  pinMode(CAPACIPIN8, INPUT);
  pinMode(CAPACIPIN9, INPUT);
  pinMode(CAPACIPIN10, INPUT);
  pinMode(RESISTPIN1, INPUT);
  pinMode(RESISTPIN2, INPUT);
  pinMode(RESISTPIN3, INPUT);

  // Descobre quantos sensores DS18B20 estão conectados
  nsensor = sensor.getDeviceCount();
  for(int i = 0; i < nsensor && i < MAX_SENSOR; i++){
    sensor.getAddress(enderecos[i], i); // Salva endereço de cada sensor
  }
}

// ==================== LOOP PRINCIPAL ====================
void loop(){
  // Verifica se o ESP8266 recebeu uma requisição
  if(Serial1.available()){
    if(Serial1.find("+IPD,")){  // Identifica início da requisição
      delay(300);
      int connectionId = Serial1.read() - 48; // ID da conexão
      
      // Limpa o buffer até encontrar os dados da requisição
      while(Serial1.available() && Serial1.read() != ':') {}
      
      // Lê a linha da requisição HTTP
      String request = Serial1.readStringUntil('\r');
      
      // Solicita atualização das temperaturas DS18B20
      sensor.requestTemperatures();
      
      String response;
      if(request.indexOf("GET /html") != -1){
        // ------------------ RESPOSTA HTML ------------------
        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Connection: close\r\n\r\n";
        response += "<!DOCTYPE html><html>";
        response += "<head><meta charset=\"UTF-8\"></head>";
        response += "<body><h1><u>ESP8266 - Sensores da estufa</u></h1>";

        // Mostra hora da leitura
        response += "<h3 id='hora'></h3>";
        response += "<script>";
        response += "const agora = new Date();";
        response += "document.getElementById('hora').innerText = ";
        response += "\"Data/Hora da leitura: \" + agora.toLocaleString();";
        response += "</script>";
        response += "<hr>";

        // --- Temperaturas DS18B20 ---
        response += "<h3><u>Sensores de Temperatura</u></h3>";
        for(int i = 0; i < nsensor && i < MAX_SENSOR; i++) {
          float tempC = sensor.getTempC(enderecos[i]);
          if(tempC == DEVICE_DISCONNECTED_C) {
            response += "<h2>Sensor " + String(i+1) + " não encontrado!</h2>";
          } else {
            response += "<h2>Sensor " + String(i+1) + " (" + imprimeEndereco(enderecos[i]) + "): ";
            response += String(tempC, 1) + " &deg;C</h2>";
          }
        }
        response += "<hr>";

        // --- DHT22 ---
        response += "<h3><u>Sensores de umidade do ar e temperatura</u></h3>";
        float tempDHT1 = dht1.readTemperature();
        float humDHT1 = dht1.readHumidity();
        float tempDHT2 = dht2.readTemperature();
        float humDHT2 = dht2.readHumidity();

        if(isnan(humDHT1)){
          response += "<h2>Sensor 1 desconectado!</h2>";
        } else {
          response += "<h2>Umidade: " + String(humDHT1) + "% | ";
          response += "Temperatura: " + String(tempDHT1) + " &deg;C</h2>";
        }

        if(isnan(humDHT2)){
          response += "<h2>Sensor 2 desconectado!</h2>";
        } else {
          response += "<h2>Umidade: " + String(humDHT2) + "% | ";
          response += "Temperatura: " + String(tempDHT2) + " &deg;C</h2>";
        }
        response += "<hr>";

        // --- Umidade do solo capacitiva ---
        response += "<h3><u>Sensores de umidade do solo capacitivo</u></h3>";
        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeCapaci = analogRead(CAPACIPIN1);
          somatorio += umidadeCapaci;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        response += "<h2>Valor: " + String(media) + "g/m³";
        umidadeCapaci = constrain(map(media, 546, 242, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeCapaci) + "%</h2>";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeCapaci = analogRead(CAPACIPIN2);
          somatorio += umidadeCapaci;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        response += "<h2>Valor: " + String(media) + "g/m³";
        umidadeCapaci = constrain(map(media, 546, 242, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeCapaci) + "%</h2>";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeCapaci = analogRead(CAPACIPIN3);
          somatorio += umidadeCapaci;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        response += "<h2>Valor: " + String(media) + "g/m³";
        umidadeCapaci = constrain(map(media, 546, 242, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeCapaci) + "%</h2>";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeCapaci = analogRead(CAPACIPIN4);
          somatorio += umidadeCapaci;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        response += "<h2>Valor: " + String(media) + "g/m³";
        umidadeCapaci = constrain(map(media, 546, 242, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeCapaci) + "%</h2>";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeCapaci = analogRead(CAPACIPIN5);
          somatorio += umidadeCapaci;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        response += "<h2>Valor: " + String(media) + "g/m³";
        umidadeCapaci = constrain(map(media, 546, 242, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeCapaci) + "%</h2>";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeCapaci = analogRead(CAPACIPIN6);
          somatorio += umidadeCapaci;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        response += "<h2>Valor: " + String(media) + "g/m³";
        umidadeCapaci = constrain(map(media, 546, 242, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeCapaci) + "%</h2>";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeCapaci = analogRead(CAPACIPIN7);
          somatorio += umidadeCapaci;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        response += "<h2>Valor: " + String(media) + "g/m³";
        umidadeCapaci = constrain(map(media, 546, 242, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeCapaci) + "%</h2>";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeCapaci = analogRead(CAPACIPIN8);
          somatorio += umidadeCapaci;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        response += "<h2>Valor: " + String(media) + "g/m³";
        umidadeCapaci = constrain(map(media, 546, 242, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeCapaci) + "%</h2>";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeCapaci = analogRead(CAPACIPIN9);
          somatorio += umidadeCapaci;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        response += "<h2>Valor: " + String(media) + "g/m³";
        umidadeCapaci = constrain(map(media, 546, 242, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeCapaci) + "%</h2>";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeCapaci = analogRead(CAPACIPIN10);
          somatorio += umidadeCapaci;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        response += "<h2>Valor: " + String(media) + "g/m³";
        umidadeCapaci = constrain(map(media, 546, 242, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeCapaci) + "%</h2>";

        // --- Umidade do solo resistiva ---
        response += "<h3><u>Sensor de umidade do solo resistivo</u></h3>";
        tensao = analogRead(RESISTPIN1) * 5.0 / 1023.0;
        resis_solo = (tensao * R1) / (5.0 - tensao);
        leitura = (5.0 * resis_solo) / (R1 + resis_solo);
        leitura = leitura * 1023.0 / 5.0;
        response += "<h2>Valor: " + String(leitura) + "g/m³";
        umidadeResist = constrain(map(leitura, 1023, 535, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeResist) + "%</h2>";

        tensao = analogRead(RESISTPIN2) * 5.0 / 1023.0;
        resis_solo = (tensao * R1) / (5.0 - tensao);
        leitura = (5.0 * resis_solo) / (R1 + resis_solo);
        leitura = leitura * 1023.0 / 5.0;
        response += "<h2>Valor: " + String(leitura) + "g/m³";
        umidadeResist = constrain(map(leitura, 1023, 535, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeResist) + "%</h2>";

        tensao = analogRead(RESISTPIN3) * 5.0 / 1023.0;
        resis_solo = (tensao * R1) / (5.0 - tensao);
        leitura = (5.0 * resis_solo) / (R1 + resis_solo);
        leitura = leitura * 1023.0 / 5.0;
        response += "<h2>Valor: " + String(leitura) + "g/m³";
        umidadeResist = constrain(map(leitura, 1023, 535, 0, 100), 0, 100);
        response += " | Umidade: " + String(umidadeResist) + "%</h2>";
        response += "<hr>";

        // --- Sensor de luminosidade BH1750 ---
        response += "<h3><u>Sensor de luminosidade</u></h3>";
        luminosidade = sensorLux.readLightLevel();
        if(isnan(luminosidade) || luminosidade < 0 || luminosidade > 10000){
            response += "<h2>Sensor de luminosidade desconectado!</h2>";
        } else {
            response += "<h2>Luminosidade: " + String(luminosidade) + " lux</h2>";
        }

        response += "</body></html>";
      } else {
        // ------------------ RESPOSTA JSON ------------------
        StaticJsonDocument<512> jsonDoc;
        JsonArray sensors = jsonDoc.createNestedArray("sensors");

        // Preenche JSON com os dados dos sensores
        // Cada leitura é organizada em objetos JSON com id, tipo, valor e unidade
        int sensorID = 1;
        
        for(int i = 0; i < nsensor && i < MAX_SENSOR; i++) {
          float tempC = sensor.getTempC(enderecos[i]);
          if(tempC != DEVICE_DISCONNECTED_C) {
            JsonObject tempData = sensors.createNestedObject();
            tempData["id"] = sensorID++;
            tempData["type"] = "DS18B20";
            tempData["value"] = tempC;
            tempData["unit"] = "C";
          }
        }
        
        float tempDHT1 = dht1.readTemperature();
        float humDHT1 = dht1.readHumidity();
        float tempDHT2 = dht2.readTemperature();
        float humDHT2 = dht2.readHumidity();

        JsonObject dht1Data = sensors.createNestedObject();
        dht1Data["id"] = sensorID++;
        dht1Data["type"] = "DHT22";
        dht1Data["value"] = humDHT1;
        dht1Data["unit"] = "%";
        
        JsonObject dht2Data = sensors.createNestedObject();
        dht2Data["id"] = sensorID++;
        dht2Data["type"] = "DHT22";
        dht2Data["value"] = humDHT2;
        dht2Data["unit"] = "%";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeResist = analogRead(CAPACIPIN1);
          somatorio += umidadeResist;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        JsonObject capacitivo1 = sensors.createNestedObject();
        capacitivo1["id"] = sensorID++;
        capacitivo1["type"] = "capacitivo";
        capacitivo1["value"] = constrain(map(media, 546, 242, 0, 100), 0, 100);
        capacitivo1["unit"] = "%";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeResist = analogRead(CAPACIPIN2);
          somatorio += umidadeResist;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        JsonObject capacitivo2 = sensors.createNestedObject();
        capacitivo2["id"] = sensorID++;
        capacitivo2["type"] = "capacitivo";
        capacitivo2["value"] = constrain(map(media, 546, 242, 0, 100), 0, 100);
        capacitivo2["unit"] = "%";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeResist = analogRead(CAPACIPIN3);
          somatorio += umidadeResist;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        JsonObject capacitivo3 = sensors.createNestedObject();
        capacitivo3["id"] = sensorID++;
        capacitivo3["type"] = "capacitivo";
        capacitivo3["value"] = constrain(map(media, 546, 242, 0, 100), 0, 100);
        capacitivo3["unit"] = "%";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeResist = analogRead(CAPACIPIN4);
          somatorio += umidadeResist;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        JsonObject capacitivo4 = sensors.createNestedObject();
        capacitivo4["id"] = sensorID++;
        capacitivo4["type"] = "capacitivo";
        capacitivo4["value"] = constrain(map(media, 546, 242, 0, 100), 0, 100);
        capacitivo4["unit"] = "%";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeResist = analogRead(CAPACIPIN5);
          somatorio += umidadeResist;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        JsonObject capacitivo5 = sensors.createNestedObject();
        capacitivo5["id"] = sensorID++;
        capacitivo5["type"] = "capacitivo";
        capacitivo5["value"] = constrain(map(media, 546, 242, 0, 100), 0, 100);
        capacitivo5["unit"] = "%";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeResist = analogRead(CAPACIPIN6);
          somatorio += umidadeResist;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        JsonObject capacitivo6 = sensors.createNestedObject();
        capacitivo6["id"] = sensorID++;
        capacitivo6["type"] = "capacitivo";
        capacitivo6["value"] = constrain(map(media, 546, 242, 0, 100), 0, 100);
        capacitivo6["unit"] = "%";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeResist = analogRead(CAPACIPIN7);
          somatorio += umidadeResist;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        JsonObject capacitivo7 = sensors.createNestedObject();
        capacitivo7["id"] = sensorID++;
        capacitivo7["type"] = "capacitivo";
        capacitivo7["value"] = constrain(map(media, 546, 242, 0, 100), 0, 100);
        capacitivo7["unit"] = "%";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeResist = analogRead(CAPACIPIN8);
          somatorio += umidadeResist;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        JsonObject capacitivo8 = sensors.createNestedObject();
        capacitivo8["id"] = sensorID++;
        capacitivo8["type"] = "capacitivo";
        capacitivo8["value"] = constrain(map(media, 546, 242, 0, 100), 0, 100);
        capacitivo8["unit"] = "%";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeResist = analogRead(CAPACIPIN9);
          somatorio += umidadeResist;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        JsonObject capacitivo9 = sensors.createNestedObject();
        capacitivo9["id"] = sensorID++;
        capacitivo9["type"] = "capacitivo";
        capacitivo9["value"] = constrain(map(media, 546, 242, 0, 100), 0, 100);
        capacitivo9["unit"] = "%";

        somatorio = 0;
        for(int i = 0; i < NUMERO_AMOSTRAS; i++){
          umidadeResist = analogRead(CAPACIPIN10);
          somatorio += umidadeResist;
        }
        media = 1.0 * somatorio / NUMERO_AMOSTRAS;
        JsonObject capacitivo10 = sensors.createNestedObject();
        capacitivo10["id"] = sensorID++;
        capacitivo10["type"] = "capacitivo";
        capacitivo10["value"] = constrain(map(media, 546, 242, 0, 100), 0, 100);
        capacitivo10["unit"] = "%";
        
        tensao = analogRead(RESISTPIN1) * 5.0 / 1023.0;
        resis_solo = (tensao * R1) / (5.0 - tensao);
        leitura = (5.0 * resis_solo) / (R1 + resis_solo);
        leitura = leitura * 1023.0 / 5.0;
        JsonObject resistivo1 = sensors.createNestedObject();
        resistivo1["id"] = sensorID++;
        resistivo1["type"] = "resistivo";
        resistivo1["value"] = constrain(map(leitura, 1023, 515, 0, 100), 0, 100);
        resistivo1["unit"] = "%";

        tensao = analogRead(RESISTPIN2) * 5.0 / 1023.0;
        resis_solo = (tensao * R1) / (5.0 - tensao);
        leitura = (5.0 * resis_solo) / (R1 + resis_solo);
        leitura = leitura * 1023.0 / 5.0;
        JsonObject resistivo2 = sensors.createNestedObject();
        resistivo2["id"] = sensorID++;
        resistivo2["type"] = "resistivo";
        resistivo2["value"] = constrain(map(leitura, 1023, 515, 0, 100), 0, 100);
        resistivo2["unit"] = "%";

        tensao = analogRead(RESISTPIN3) * 5.0 / 1023.0;
        resis_solo = (tensao * R1) / (5.0 - tensao);
        leitura = (5.0 * resis_solo) / (R1 + resis_solo);
        leitura = leitura * 1023.0 / 5.0;
        JsonObject resistivo3 = sensors.createNestedObject();
        resistivo3["id"] = sensorID++;
        resistivo3["type"] = "resistivo";
        resistivo3["value"] = constrain(map(leitura, 1023, 515, 0, 100), 0, 100);
        resistivo3["unit"] = "%";

        luminosidade = sensorLux.readLightLevel();
        JsonObject luxData = sensors.createNestedObject();
        luxData["id"] = sensorID++;
        luxData["type"] = "BH1750";
        luxData["value"] = luminosidade;
        luxData["unit"] = "lux";

        // Converte JSON em string e prepara resposta HTTP
        String jsonResponse;
        serializeJson(jsonDoc, jsonResponse);
        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Connection: close\r\n\r\n";
        response += jsonResponse;
      }
      
      // Envia a resposta
      String cipSend = "AT+CIPSEND=" + String(connectionId) + "," + String(response.length()) + "\r\n";
      sendData(cipSend, 1000, DEBUG);
      sendData(response, 1000, DEBUG);
      
      // Fecha a conexão
      sendData("AT+CIPCLOSE=" + String(connectionId) + "\r\n", 1000, DEBUG);
    }
  }
}

// ==================== FUNÇÕES AUXILIARES ====================
// Envia comando AT ao ESP8266 e retorna resposta
String sendData(String command, const int timeout, boolean debug) {
  String response = "";
  Serial1.print(command);
  long int time = millis();
  while((time + timeout) > millis()) {
    while(Serial1.available()) {
      char c = Serial1.read();
      response += c;
    }
  }
  if(debug) Serial.println(response);
  return response;
}

// Converte endereço do DS18B20 em string hex
String imprimeEndereco(DeviceAddress endereco) {
  String sEndereco = "";
  for(uint8_t i = 0; i < 8; i++) {
    if(endereco[i] < 16) sEndereco += "0";
    sEndereco += String(endereco[i], HEX);
  }
  sEndereco.toUpperCase();
  return sEndereco;
}