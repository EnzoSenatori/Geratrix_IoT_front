// Caneleira instrumentada — firmware da ESP32 (P3 a P8).
// Lê a IMU a 100 Hz, estima o ângulo da perna, conta e classifica cada repetição e,
// ao fim de cada série, imprime o resumo no formato do tópico sessao/<sid> do contrato.
// Com USAR_SINAL_SIMULADO = true, roda sem sensor, sobre um roteiro de repetições
// fictícias cujo resultado esperado é conhecido (ver sinal_simulado.h).

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "processamento.h"
#include "sinal_simulado.h"

// ===== Configuração =====
constexpr bool USAR_SINAL_SIMULADO = true;  // false para usar a MPU-6050 real
constexpr bool IMPRIMIR_ANGULO = false;     // true: ângulos a 10 Hz, útil no P4 e no P5
constexpr uint8_t PIN_SDA = 21;
constexpr uint8_t PIN_SCL = 22;
constexpr uint8_t PIN_FSR = 34;  // FSR na tira (no Wokwi, o potenciômetro "fsr")
constexpr bool USAR_FSR = true;  // false: considera a caneleira sempre presa
constexpr uint32_t PERIODO_US = 10000;  // 100 Hz
constexpr float DT_S = PERIODO_US / 1000000.0f;

// Prescrição provisória, até o P10 recebê-la pelo tópico cmd. Valores fictícios, não
// clínicos. No modo simulado o descanso cai para 10 s, para o teste não demorar.
const Prescricao PRESCRICAO_TESTE = {"sim001", 3, 10, 60.0f, 150.0f,
                                     (uint16_t)(USAR_SINAL_SIMULADO ? 10 : 60)};

Adafruit_MPU6050 imu;
Caneleira caneleira;
DetectorUso detectorUso;
GeradorSimulado gerador = {12345u, 0.0f};
uint32_t proximaLeituraUs = 0;
uint32_t amostras = 0;
uint32_t atrasos = 0;

// Lê a próxima amostra: do roteiro simulado ou da MPU-6050 real.
bool obterLeitura(AmostraImu& a) {
  if (USAR_SINAL_SIMULADO) {
    lerImuSimulada(gerador, DT_S, a);
    return true;
  }
  sensors_event_t aceleracao, giro, temperatura;
  if (!imu.getEvent(&aceleracao, &giro, &temperatura)) return false;
  a.ax = aceleracao.acceleration.x;
  a.ay = aceleracao.acceleration.y;
  a.az = aceleracao.acceleration.z;
  a.gx = giro.gyro.x;
  a.gy = giro.gyro.y;
  a.gz = giro.gyro.z;
  return true;
}

// Lê o FSR e informa se a caneleira está presa à perna, avisando quando isso muda.
bool usoEfetivo() {
  if (!USAR_FSR) return true;
  const bool antes = detectorUso.emUso;
  const bool agora = atualizarUso(detectorUso, analogReadMilliVolts(PIN_FSR));
  if (agora != antes) {
    Serial.println(agora ? "Caneleira presa a perna." : "Caneleira solta: movimentos nao serao contados.");
  }
  return agora;
}

// Registra o padrão de vibração do evento; o motor real entra no P9.
void sinalizar(const char* padrao, const char* motivo) {
  Serial.printf("  [vibracao] %s - %s\n", padrao, motivo);
}

// Imprime o resumo acumulado da sessão, como será publicado em sessao/<sid> no P10.
void publicarResumo() {
  static char json[1024];
  if (montarJsonSessao(caneleira.sessao, json, sizeof(json)) < 0) {
    Serial.println("ERRO: resumo maior que o buffer.");
    return;
  }
  Serial.printf("sessao/%s -> %s\n", caneleira.sessao.p.sid, json);
}

// Texto curto com o resultado da repetição e o motivo, se estiver fora do padrão.
const char* textoResultado(const Repeticao& rep) {
  if (rep.valida) return "VALIDA";
  if (!rep.admOk && !rep.velOk) return "FORA (adm e vel)";
  return rep.admOk ? "FORA (vel)" : "FORA (adm)";
}

// Imprime cada evento da sessão e aciona o biofeedback correspondente.
void tratarEventos(uint8_t ev, const Repeticao& rep) {
  const Sessao& s = caneleira.sessao;

  if (ev & EV_CALIBRADO) {
    Serial.printf("Offset do giroscopio: %.2f graus/s.\n", caneleira.calib.offset);
    Serial.printf("Sessao %s iniciada: %u series x %u reps, ADM min %.0f graus, "
                  "vel max %.0f graus/s, descanso %u s.\n",
                  s.p.sid, (unsigned)s.p.series, (unsigned)s.p.reps, s.p.admMin, s.p.velMax,
                  (unsigned)s.p.descanso);
    Serial.println("Serie 1 iniciada.");
  }
  if (ev & EV_REP_SEM_USO) Serial.println("Movimento ignorado: caneleira fora da perna.");

  if (ev & (EV_REP_VALIDA | EV_REP_FORA)) {
    const ResumoSerie& r = s.resumos[s.serie];
    Serial.printf("Serie %u, rep %2u/%u | ADM %5.1f | vel %5.1f graus/s | sobe %.1f s | "
                  "desce %.1f s | %s\n",
                  (unsigned)r.n, (unsigned)r.total, (unsigned)s.p.reps, rep.adm, rep.velPico,
                  rep.tConc, rep.tExc, textoResultado(rep));
    if (rep.valida) sinalizar("1 pulso curto", "repeticao valida");
    else sinalizar("2 pulsos curtos", "repeticao fora do padrao");
  }
  if (ev & EV_FIM_SERIE) {
    sinalizar("3 pulsos curtos", "fim de serie");
    publicarResumo();
    if (!(ev & EV_SESSAO_CONCLUIDA)) Serial.printf("Descanso de %u s.\n", (unsigned)s.p.descanso);
  }
  if (ev & EV_FIM_DESCANSO) {
    sinalizar("1 pulso longo", "fim do descanso");
    Serial.printf("Serie %u iniciada.\n", (unsigned)(s.serie + 1));
  }
  if (ev & EV_SESSAO_CONCLUIDA) Serial.println("Sessao concluida.");
}

// Inicializa o monitor serial, o sensor (no modo real) e o processamento.
void setup() {
  Serial.begin(115200);

  if (!USAR_SINAL_SIMULADO) {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);
    if (!imu.begin(0x68, &Wire)) {
      Serial.println("ERRO: MPU-6050 nao encontrado em 0x68. Confira os quatro fios.");
      while (true) delay(1000);
    }
    imu.setAccelerometerRange(MPU6050_RANGE_2_G);
    imu.setGyroRange(MPU6050_RANGE_250_DEG);  // satura acima de 250 graus/s
    imu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }

  iniciarCaneleira(caneleira, PRESCRICAO_TESTE);
  Serial.println(USAR_SINAL_SIMULADO ? "Caneleira P7 - modo SIMULADO (repeticoes ficticias)."
                                     : "Caneleira P7 - MPU-6050 real.");
  Serial.println("Calibrando o giroscopio: mantenha a perna parada por 2 s...");
  proximaLeituraUs = micros() + PERIODO_US;
}

// Amostra a 100 Hz sem bloquear e entrega cada leitura ao processamento.
void loop() {
  const uint32_t agoraUs = micros();
  if ((int32_t)(agoraUs - proximaLeituraUs) < 0) return;

  proximaLeituraUs += PERIODO_US;
  if ((int32_t)(micros() - proximaLeituraUs) >= 0) {  // perdeu o próximo instante
    atrasos++;
    proximaLeituraUs = micros() + PERIODO_US;
  }

  AmostraImu leitura;
  if (!obterLeitura(leitura)) {
    Serial.println("ERRO: falha na leitura da MPU-6050.");
    return;
  }

  Repeticao rep;
  const uint8_t eventos = processarLeitura(caneleira, leitura, DT_S, usoEfetivo(), rep);
  if (eventos) tratarEventos(eventos, rep);

  amostras++;
  if (IMPRIMIR_ANGULO && amostras % 10 == 0) {
    Serial.printf("theta_acc %.1f | theta %.1f | omega %.1f | atrasos %lu\n", caneleira.thetaAcc,
                  caneleira.theta, caneleira.omega, (unsigned long)atrasos);
  }
}
