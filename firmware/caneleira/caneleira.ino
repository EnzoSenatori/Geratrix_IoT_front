#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

// P3: leitura da IMU a 100 Hz. Eixos ainda serão confirmados com a placa real.
constexpr uint8_t PIN_SDA = 21;
constexpr uint8_t PIN_SCL = 22;
constexpr uint32_t PERIODO_US = 10000;
constexpr float RAD_PARA_GRAUS = 180.0f / PI;

struct AmostraImu {
  float ax;
  float ay;
  float az;
  float gx;
  float gy;
  float gz;
};

Adafruit_MPU6050 imu;
uint32_t proximaLeituraUs = 0;
uint32_t leituraAnteriorUs = 0;
uint32_t leituras = 0;
uint32_t atrasos = 0;

// Lê aceleração e giro da MPU-6050; retorna falso se a leitura falhar.
bool lerImu(AmostraImu& amostra) {
  sensors_event_t aceleracao, giro, temperatura;
  if (!imu.getEvent(&aceleracao, &giro, &temperatura)) {
    return false;
  }
  amostra.ax = aceleracao.acceleration.x;
  amostra.ay = aceleracao.acceleration.y;
  amostra.az = aceleracao.acceleration.z;
  amostra.gx = giro.gyro.x;
  amostra.gy = giro.gyro.y;
  amostra.gz = giro.gyro.z;
  return true;
}

// Calcula a inclinação provisória e a velocidade no eixo Y; os eixos serão conferidos em bancada.
void estimarAnguloProvisorio(const AmostraImu& amostra,
                            float& anguloAccGraus,
                            float& velocidadeGrausPorSegundo) {
  // Rotacao suposta no plano X/Z, em torno de Y. Confirmar eixos no P4.
  anguloAccGraus = atan2f(amostra.ax, amostra.az) * RAD_PARA_GRAUS;
  velocidadeGrausPorSegundo = amostra.gy * RAD_PARA_GRAUS;
}

// Reserva a atualização da máquina de estados das repetições para P6.
void atualizarRepeticoes(float anguloGraus, float velocidadeGrausPorSegundo) {
  // P6: maquina de estados com histerese.
  (void)anguloGraus;
  (void)velocidadeGrausPorSegundo;
}

// Reserva a lógica do motor de vibração para P9.
void atualizarBiofeedback() {
  // P9: motor de vibracao.
}

// Reserva a comunicação MQTT do contrato g01 v1 para P10.
void atualizarComunicacao() {
  // P10: MQTT conforme o contrato g01 v1.
}

// Inicializa barramento I²C, sensor e monitor serial.
void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  if (!imu.begin(0x68, &Wire)) {
    Serial.println("ERRO: MPU-6050 nao encontrado em 0x68. Confira os quatro fios.");
    while (true) {
      delay(1000);
    }
  }

  imu.setAccelerometerRange(MPU6050_RANGE_2_G);
  imu.setGyroRange(MPU6050_RANGE_250_DEG);
  imu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  leituraAnteriorUs = micros();
  proximaLeituraUs = leituraAnteriorUs + PERIODO_US;
  Serial.println("P3 iniciado: IMU a 100 Hz; saida resumida a 10 Hz.");
  Serial.println("amostra,dt_ms,angulo_acc_graus,vel_y_graus_s,atrasos");
}

// Executa as rotinas sem bloquear a amostragem periódica de 100 Hz.
void loop() {
  atualizarComunicacao();
  atualizarBiofeedback();

  uint32_t agoraUs = micros();
  if ((int32_t)(agoraUs - proximaLeituraUs) < 0) {
    return;
  }

  const uint32_t inicioUs = agoraUs;
  const float dtMs = (inicioUs - leituraAnteriorUs) / 1000.0f;
  leituraAnteriorUs = inicioUs;

  proximaLeituraUs += PERIODO_US;
  if ((int32_t)(micros() - proximaLeituraUs) >= 0) {
    atrasos++;
    proximaLeituraUs = micros() + PERIODO_US;
  }

  AmostraImu amostra;
  if (!lerImu(amostra)) {
    Serial.println("ERRO: falha na leitura da MPU-6050.");
    return;
  }

  float anguloAccGraus = 0;
  float velocidadeGrausPorSegundo = 0;
  estimarAnguloProvisorio(amostra, anguloAccGraus,
                         velocidadeGrausPorSegundo);
  atualizarRepeticoes(anguloAccGraus, velocidadeGrausPorSegundo);

  leituras++;
  if (leituras % 10 == 0) {
    Serial.print(leituras);
    Serial.print(',');
    Serial.print(dtMs, 2);
    Serial.print(',');
    Serial.print(anguloAccGraus, 1);
    Serial.print(',');
    Serial.print(velocidadeGrausPorSegundo, 1);
    Serial.print(',');
    Serial.println(atrasos);
  }
}
