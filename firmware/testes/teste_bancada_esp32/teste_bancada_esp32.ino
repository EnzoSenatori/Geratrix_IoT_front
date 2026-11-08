// Teste de bancada da caneleira Geratrix (ESP32 DevKit V1)
// Confere, um por um, os componentes da protoboard com os mesmos pinos do caneleira.ino.
// Nao precisa de biblioteca: a MPU-6050 e lida direto pelo I2C.
//
//  MPU-6050  SDA -> GPIO 21, SCL -> GPIO 22, VCC -> 3V3, GND -> GND
//  FSR 402   divisor com 10 k para GND, sinal no GPIO 34
//  Motor     BC337 com base por 1 k no GPIO 26, 1N4148 em paralelo com o motor
//  Botao     GPIO 33 para GND (pull-up interno)
//
// O que esperar no Monitor Serial (115200):
//  - "MPU-6050 ok (WHO_AM_I = 0x68)" na partida e o angulo mudando ao inclinar a placa
//  - FSR perto de 0 mV solto e subindo ao apertar ("PRESA" acima de 1200 mV)
//  - ao apertar o botao: "botao" e um pulso de 300 ms no motor
#include <Wire.h>

constexpr uint8_t PIN_SDA = 21, PIN_SCL = 22, PIN_FSR = 34, PIN_MOTOR = 26, PIN_BOTAO = 33;
constexpr uint8_t MPU = 0x68;
constexpr uint8_t INTENSIDADE_MOTOR = 200;  // o mesmo duty do caneleira.ino

bool mpuOk = false;

void escreverMpu(uint8_t reg, uint8_t valor) {
  Wire.beginTransmission(MPU); Wire.write(reg); Wire.write(valor); Wire.endTransmission();
}

int16_t ler16(uint8_t reg) {
  Wire.beginTransmission(MPU); Wire.write(reg); Wire.endTransmission(false);
  Wire.requestFrom(MPU, (uint8_t)2);
  return (int16_t)((Wire.read() << 8) | Wire.read());
}

void motor(bool ligado) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_MOTOR, ligado ? INTENSIDADE_MOTOR : 0);
#else
  ledcWrite(0, ligado ? INTENSIDADE_MOTOR : 0);
#endif
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(PIN_BOTAO, INPUT_PULLUP);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_MOTOR, 5000, 8);
#else
  ledcSetup(0, 5000, 8); ledcAttachPin(PIN_MOTOR, 0);
#endif
  motor(false);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.beginTransmission(MPU); Wire.write(0x75); Wire.endTransmission(false);
  Wire.requestFrom(MPU, (uint8_t)1);
  const uint8_t quem = Wire.available() ? Wire.read() : 0;
  mpuOk = (quem == 0x68 || quem == 0x70 || quem == 0x72);
  if (mpuOk) {
    escreverMpu(0x6B, 0);  // tira do modo sleep
    Serial.printf("MPU-6050 ok (WHO_AM_I = 0x%02X)\n", quem);
  } else {
    Serial.println("MPU-6050 NAO respondeu: confira VCC no 3V3, GND, SDA no 21 e SCL no 22.");
  }

  Serial.println("Teste do motor: 3 pulsos.");
  for (int i = 0; i < 3; i++) { motor(true); delay(150); motor(false); delay(150); }
}

void loop() {
  static uint32_t ultimo = 0;
  static bool botaoAntes = true;

  const bool botao = digitalRead(PIN_BOTAO);
  if (botaoAntes && !botao) {  // apertou
    Serial.println("botao");
    motor(true); delay(300); motor(false);
  }
  botaoAntes = botao;

  if (millis() - ultimo >= 250) {
    ultimo = millis();
    const uint32_t fsrMv = analogReadMilliVolts(PIN_FSR);
    float angulo = NAN;
    if (mpuOk) {
      const float ax = ler16(0x3B), az = ler16(0x3F);
      angulo = atan2f(ax, az) * 57.2958f;
    }
    Serial.printf("FSR %4u mV %-6s | angulo %6.1f graus | botao %s\n",
                  fsrMv, fsrMv >= 1200 ? "PRESA" : (fsrMv < 800 ? "solta" : "-"),
                  angulo, botao ? "solto" : "APERTADO");
  }
}
