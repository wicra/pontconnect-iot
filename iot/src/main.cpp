#include <Arduino.h>

// LMIC
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>

// DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>

// OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DISABLE_PING 0
#define DISABLE_BEACONS 0

// CONNEXION LORA 
static const u1_t PROGMEM APPEUI[8]  = { 0x31, 0xdf, 0x29, 0xff, 0xff, 0x40, 0xee, 0xc0  }; //  c0ee40ffff29df31 (MSB) ID Getway
static const u1_t PROGMEM DEVEUI[8]  = { 0x03, 0xde, 0xc6, 0xe5, 0xd7, 0x7d, 0x2e, 0xcd }; // cd2e7dd7e5c6de03  DEVEUI APP
static const u1_t PROGMEM APPKEY[16] = { 0x8d, 0xbc, 0x09, 0xac, 0x4c, 0x96, 0x69, 0xb1, 0xfc, 0x06, 0xc3, 0x3a, 0x98, 0x53, 0xd3, 0xb2 }; //  8dbc09ac4c9669b1fc06c33a985d3b2 (MSB) APPKEY


static uint8_t mydata[10]; // TAILLE DE DONNEES
static osjob_t sendjob;

#define ONE_WIRE_BUS 4  // GPIO4 DS18B20
#define TDS_PIN 36      // DFRobot SEN0244 GPIO36 (ADC1_CH0)
#define PRESSURE_PIN 39 // DFRobot SEN0257 GPIO39 (ADC1_CH3)

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// INTERVAL DE TRANSMISSION
const unsigned TX_INTERVAL = 20;

// PIN LORA POUR TTGO LORA32 V2.1
const lmic_pinmap lmic_pins = {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 14,
  .dio = {
    26,
    33,
    32
  }
};

// FORMAT DES CLÉS DE CONNEXION
void os_getArtEui(u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

void os_getDevEui(u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}

void os_getDevKey(u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}

void printHex2(unsigned v) {
  v &= 0xff;
  if (v < 16)
    Serial.print("0");
  Serial.print(v, HEX);
}

String loraStatus = "INIT";
String loraTxStatus = "";

// FONCTION DE MISE À JOUR DE L'AFFICHAGE OLED
void updateDisplay(float tempC, float qppm, float depth) {
  display.clearDisplay();
  
  // NOM DU PROJET
  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setTextSize(1);
  int16_t x, y;
  uint16_t w, h;
  display.getTextBounds("PONTCONNECT", 0, 0, &x, &y, &w, &h);
  display.setCursor((128-w)/2, 2);
  display.print("PONTCONNECT");
  
  // AFFICHAGE DU MONITEUR SÉRIE 
  Serial.println("\n------ PONTCONNECT STATUS ------");
  
  display.setTextColor(SSD1306_WHITE);
  
  // TEMPÉRATURE
  display.setCursor(8, 18);  display.print("T:");
  display.setCursor(22, 18); display.print(String(tempC, 1));
  display.setCursor(50, 18); display.print("C");
  
  Serial.print("Température: ");
  Serial.print(tempC, 1);
  Serial.println(" °C");
  
  // TDS
  display.setCursor(65, 18); display.print("Q:");
  display.setCursor(80, 18); display.print(String((int)qppm));
  display.setCursor(105, 18); display.print("ppm");
  
  Serial.print("Qualité eau: ");
  Serial.print((int)qppm);
  Serial.println(" ppm");
  
  // PROFONDEUR ( CAPTEUR PRESSION)
  display.setCursor(8, 30);  display.print("Prfd:");
  display.setCursor(45, 30); display.print(String(depth, 1));
  display.print("cm");
  
  Serial.print("Profondeur: ");
  Serial.print(depth, 1);
  Serial.println(" cm");
  
  // SÉPARATEUR
  display.drawLine(0, 40, 128, 40, SSD1306_WHITE);
  Serial.println("------------------------------");
  
  // STATUS LoRa
  display.setCursor(4, 44);  display.print("LoRa: ");
  display.print(loraStatus);
  display.setCursor(4, 52);  display.print(loraTxStatus);
  
  Serial.print("LoRa: ");
  Serial.println(loraStatus);
  Serial.println(loraTxStatus);
  Serial.println("------------------------------");
  
  display.display();
}

// GESTION DES ERREURS
void showMessage(const char* msg, int line = 0) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, line * 10);
  display.print(msg);
  display.display();
  
  // MEME CHOSE SUR LE MONITEUR SÉRIE
  Serial.print("[Message] ");
  Serial.println(msg);
}

// VARIABLES DE STOCKAGE DES DONNÉES
float lastTempC = 0.0;
float lastQppm = 0.0;
float lastDepth = 0.0;

// FONCTION D'ENVOI DES DONNÉES
void do_send(osjob_t* j) {
  loraStatus = "LECTURE";
  loraTxStatus = "capteurs";

  // UTILISER LES DERNIÈRES VALEURS SI LA LECTURE ÉCHOUE
  updateDisplay(lastTempC, lastQppm, lastDepth); 
  
  Serial.println("\n==== DÉBUT CYCLE DE MESURE ====");
  
  // LECTURE DE LA TEMPÉRATURE
  sensors.requestTemperatures();
  
  float tempC = sensors.getTempCByIndex(0); // C°
  
  // VERIF VALIDITÉ DE LA TEMPÉRATURE
  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Erreur de lecture du capteur");
    loraStatus = "ERREUR";
    loraTxStatus = "capteur temp";
    updateDisplay(lastTempC, lastQppm, lastDepth);
    return;
  }
  
  // LECTURE TDS (TURBIDITÉ)
  int rawTDS = analogRead(TDS_PIN);
  float vTDS = rawTDS * (3.3f / 4095.0f);
  // CONVERSION EN PPM
  float qppm = (133.42f*vTDS*vTDS*vTDS - 255.86f*vTDS*vTDS + 857.39f*vTDS)*0.5f;
  qppm = constrain(qppm, 0, 5000);
  
  // LECTURE DE LA PROFONDEUR (CAPTEUR DE PRESSION)
  int rawP = analogRead(PRESSURE_PIN);
  float vP = rawP * (3.3f / 4095.0f) * 2.0f;
  float depth = max(0.0f, (vP - 0.5f) / 0.05f);
  
  // SAUVEGARDER LES VALEURS POUR L'AFFICHAGE FUTUR
  lastTempC = tempC;
  lastQppm = qppm;
  lastDepth = depth;
  
  // CONVERSION DES VALEURS EN FORMAT ENTIER
  int16_t tempFixed = (int16_t)(tempC * 100);    // 2 décimales pour la température
  uint16_t tdsFixed = (uint16_t)(qppm);          // Valeur entière pour TDS en ppm
  uint16_t depthFixed = (uint16_t)(depth * 10);   // 1 décimale pour la profondeur
  
  // VERIFICATION DES VALEURS BRUTES DANS LE MONITEUR SÉRIE
  Serial.println("==== VALEURS MESURÉES ====");
  Serial.print("Température: "); Serial.print(tempC); Serial.println(" °C (brut: " + String(tempFixed) + ")");
  Serial.print("Turbidité: "); Serial.print(qppm); Serial.println(" ppm (brut: " + String(tdsFixed) + ")");
  Serial.print("Profondeur: "); Serial.print(depth); Serial.println(" cm (brut: " + String(depthFixed) + ")");
  
  // PREPARATION DU PAYLOAD (6 octets au total: 2 pour temp + 2 pour TDS + 2 pour profondeur)
  mydata[0] = tempFixed & 0xFF;        // Octet de poids faible temp
  mydata[1] = (tempFixed >> 8) & 0xFF; // Octet de poids fort temp
  mydata[2] = tdsFixed & 0xFF;         // Octet de poids faible TDS
  mydata[3] = (tdsFixed >> 8) & 0xFF;  // Octet de poids fort TDS
  mydata[4] = depthFixed & 0xFF;       // Octet de poids faible profondeur
  mydata[5] = (depthFixed >> 8) & 0xFF;// Octet de poids fort profondeur
  
  // MISE À JOUR OLED
  updateDisplay(lastTempC, lastQppm, lastDepth);
  
  loraStatus = "ENVOI";
  loraTxStatus = "preparation";
  updateDisplay(lastTempC, lastQppm, lastDepth);
  
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
    loraStatus = "ERROR";
    loraTxStatus = "TX en cours";
    updateDisplay(lastTempC, lastQppm, lastDepth);
  } else {
    // ENVOIYER DES DONNÉES 6 OCTETS
    LMIC_setTxData2(1, mydata, 6, 0);
    Serial.println("==== ENVOI LORAWAN ====");
    Serial.println("Payload hexadécimal: ");
    for (int i = 0; i < 6; i++) {
      if (mydata[i] < 16) Serial.print("0");
      Serial.print(mydata[i], HEX);
    }
    Serial.println();
    Serial.println(F("Packet queued"));
    loraStatus = "ENVOI";
    loraTxStatus = "en cours...";
    updateDisplay(lastTempC, lastQppm, lastDepth);
  }
}

void onEvent(ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");

  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      loraStatus = "CONNEXION";
      loraTxStatus = "...";
      updateDisplay(lastTempC, lastQppm, lastDepth);
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      {
        u4_t netid = 0;
        devaddr_t devaddr = 0;
        u1_t nwkKey[16];
        u1_t artKey[16];
        LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
        Serial.print("netid: ");
        Serial.println(netid, DEC);
        Serial.print("devaddr: ");
        Serial.println(devaddr, HEX);
        Serial.print("AppSKey: ");
        for (size_t i = 0; i < sizeof(artKey); ++i) {
          if (i != 0)
            Serial.print("-");
          printHex2(artKey[i]);
        }
        Serial.println("");
        Serial.print("NwkSKey: ");
        for (size_t i = 0; i < sizeof(nwkKey); ++i) {
          if (i != 0)
            Serial.print(" ");
          printHex2(nwkKey[i]);
        }
        Serial.println();
        loraStatus = "CONNECTÉ";
        loraTxStatus = "LoRaWAN";
        updateDisplay(lastTempC, lastQppm, lastDepth);
      }
      LMIC_setLinkCheckMode(0);
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      loraStatus = "ERREUR";
      loraTxStatus = "Join failed";
      updateDisplay(lastTempC, lastQppm, lastDepth);
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE"));
      loraStatus = "ENVOYÉ";
      if (LMIC.txrxFlags & TXRX_ACK) {
        Serial.println(F("Received ack"));
        loraTxStatus = "ACK reçu";
      } else if (LMIC.dataLen) {
        Serial.print(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
        loraTxStatus = "Data reçue";
      } else {
        loraTxStatus = "OK";
      }
      updateDisplay(lastTempC, lastQppm, lastDepth);
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    case EV_TXSTART:
      Serial.println(F("EV_TXSTART"));
      break;
    case EV_TXCANCELED:
      Serial.println(F("EV_TXCANCELED"));
      break;
    case EV_RXSTART:
      break;
    case EV_JOIN_TXCOMPLETE:
      Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
      break;
    default:
      Serial.print(F("Unknown event: "));
      Serial.println((unsigned) ev);
      break;
  }
}

void setup() {
  delay(5000);

  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("Starting"));

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // ECRAN DE DÉMARRAGE
  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  int16_t x, y;
  uint16_t w, h;
  display.getTextBounds("PONTCONNECT", 0, 0, &x, &y, &w, &h);
  display.setCursor((128-w)/2, 2);
  display.print("PONTCONNECT");
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 35);
  display.println("Demarrage...");
  display.display();
  delay(2000);
  
  // INITIALISATION DU CAPTEUR DS18B20
  sensors.begin();
  Serial.println("DS18B20 initialized");

  os_init();
  LMIC_reset();

  LMIC_setLinkCheckMode(0);
  LMIC_setDrTxpow(DR_SF7, 14);

  // INITIALISATION DES VARIABLES
  lastTempC = 0.0;
  lastQppm = 0.0;
  lastDepth = 0.0;

  // INITIALISATION DE L'AFFICHAGE
  loraStatus = "INIT";
  loraTxStatus = "LoRaWAN";
  updateDisplay(lastTempC, lastQppm, lastDepth);

  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
