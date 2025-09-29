#include <SPI.h>
#include "Adafruit_MCP23X17.h"

#define NUM_CHIPS 5
Adafruit_MCP23X17 mcp[NUM_CHIPS];
int csPins[NUM_CHIPS] = {10, 9, 8, 7, 6};

// letzter Zustand der Eingänge (für Flankenerkennung)
uint16_t lastState[NUM_CHIPS];

// ----------------- Taster-Mapping -----------------
struct ButtonMap {
  const char* name;
  int chip;
  int pin;
};

ButtonMap buttonMap[] = {
  {"Batterie aus", 2, 14},
  {"Spiegelheizung", 4, 10},
  {"Heizung", 2, 12},
  {"Scheibenwischer dauer", 2, 15},
  {"600V", 2, 13},
  {"Störung im Zug", 2, 11},
  {"ZUB", 2, 9},
  {"Türfreigabe", 4, 11},
  {"Zentrales öffnen", 2, 7},
  {"Zentrales schließen", 4, 3},
  {"Zahltischbeleuchtung", 4, 12},
  {"Warnblinker", 4, 13},
  {"Fahrertür", 4, 4},
  {"Federspeicher", 4, 6},
  {"Kupplungsabriss", 4, 0},
  {"Funk senden", 4, 2},
  {"Ansage stumm", 4, 9},
  {"Hochbahnsteig", 4, 7},
  {"Niedrigbahnsteig", 2, 6},
  {"Weiche links", 4, 1},
  {"Weiche rechts", 4, 8},
  {"Blinker links", 4, 5},
  {"Blinker rechts", 4, 14}
};
const int buttonCount = sizeof(buttonMap) / sizeof(buttonMap[0]);

const char* getButtonName(int chip, int pin) {
  for (int i = 0; i < buttonCount; i++) {
    if (buttonMap[i].chip == chip && buttonMap[i].pin == pin) {
      return buttonMap[i].name;
    }
  }
  return NULL;
}

// ----------------- Setup -----------------
void setup() {
  Serial.begin(115200);
  SPI.begin();

  // alle MCPs initialisieren
  for (int i = 0; i < NUM_CHIPS; i++) {
    if (!mcp[i].begin_SPI(csPins[i])) {
      Serial.print("MCP "); Serial.print(i); Serial.println(" nicht gefunden!");
      while (1);
    }
  }

  // Chip 0 & 1 als Outputs
  for (int c = 0; c < 2; c++) {
    for (int p = 0; p < 16; p++) {
      mcp[c].pinMode(p, OUTPUT);
      mcp[c].digitalWrite(p, LOW);
    }
  }

  // Chip 2–4 als Inputs mit Pullups
  for (int c = 2; c < NUM_CHIPS; c++) {
    for (int p = 0; p < 16; p++) {
      mcp[c].pinMode(p, INPUT_PULLUP);
    }
    lastState[c] = mcp[c].readGPIOAB();
  }

  // Lampentest
  Serial.println("Starte Lampentest...");
  for (int chip = 0; chip < 2; chip++) {
    for (int pin = 0; pin < 16; pin++) {
      mcp[chip].digitalWrite(pin, HIGH);
      Serial.print("Lampe Chip "); Serial.print(chip);
      Serial.print(" Pin "); Serial.print(pin);
      Serial.println(" AN");
      delay(500);
      mcp[chip].digitalWrite(pin, LOW);
    }
  }
  Serial.println("Lampentest beendet.");
}

// ----------------- Loop -----------------
void loop() {
  // Eingänge Chip 2–4 überwachen
  for (int c = 2; c < NUM_CHIPS; c++) {
    uint16_t state = mcp[c].readGPIOAB();
    uint16_t changes = state ^ lastState[c];

    if (changes) {
      for (int p = 0; p < 16; p++) {
        if (changes & (1 << p)) {
          bool pressed = !(state & (1 << p));
          const char* name = getButtonName(c, p);

          if (name) {
            Serial.print(name);
            Serial.print(" -> ");
            Serial.println(pressed ? "gedrückt" : "losgelassen");
          } else {
            Serial.print("Chip "); Serial.print(c);
            Serial.print(", Pin "); Serial.print(p);
            Serial.print(" -> ");
            Serial.println(pressed ? "gedrückt" : "losgelassen");
          }
        }
      }
      lastState[c] = state;
    }
  }

  // Serielle Befehle für Outputs
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith("SET")) {
      int chip, pin, val;
      if (sscanf(cmd.c_str(), "SET %d %d %d", &chip, &pin, &val) == 3) {
        if (chip >= 0 && chip < 2 && pin >= 0 && pin < 16) {
          mcp[chip].digitalWrite(pin, val ? HIGH : LOW);
          Serial.print("Output gesetzt: Chip ");
          Serial.print(chip);
          Serial.print(", Pin ");
          Serial.print(pin);
          Serial.print(" = ");
          Serial.println(val ? "HIGH" : "LOW");
        } else {
          Serial.println("Ungültiger Chip oder Pin (nur Chip 0–1, Pins 0–15)");
        }
      } else {
        Serial.println("Syntax: SET <chip> <pin> <0|1>");
      }
    }
  }
}
