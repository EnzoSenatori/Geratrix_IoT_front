#pragma once
// Gerador de repetições fictícias para testar a lógica sem a placa (P5 a P7).
// Produz leituras de IMU coerentes com um roteiro conhecido, com ruído e offset de
// giroscópio, para que o resultado esperado de cada série seja sabido de antemão.
// Limite: não simula aceleração linear do movimento, que existirá na placa real.

#include "processamento.h"

// Uma repetição do roteiro: amplitude (graus), tempos (s) e tremor no topo (graus).
struct RepSimulada {
  float amplitude;
  float tSubida;
  float tTopo;
  float tDescida;
  float tremor;
  const char* caso;
};

constexpr float GRAVIDADE_M_S2 = 9.80665f;
constexpr float SIM_REPOUSO_INICIAL_S = 3.0f;    // cobre os 2 s de calibração
constexpr float SIM_PAUSA_ENTRE_REPS_S = 1.0f;
constexpr float SIM_PAUSA_ENTRE_SERIES_S = 12.0f;
constexpr uint8_t SIM_SERIES = 3;                // séries da prescrição de teste
constexpr float SIM_FREQ_TREMOR_HZ = 5.0f;       // topo de 1 s = 5 ciclos completos
constexpr float SIM_OFFSET_GIRO_RAD_S = 0.03f;   // ≈1,7 °/s; a calibração deve removê-lo
constexpr float SIM_RUIDO_ACC_M_S2 = 0.3f;
constexpr float SIM_RUIDO_GIRO_RAD_S = 0.005f;

// Roteiro de uma série, repetido sem fim. Com a prescrição de teste
// (ADM mínima 60°, velocidade máxima 150 °/s), o esperado por série é:
// 10 repetições, 7 válidas, 2 desvios de ADM (4 e 8) e 1 de velocidade (9, ≈220 °/s).
const RepSimulada ROTEIRO_SERIE[] = {
    {70, 1.4f, 0.3f, 1.7f, 0.0f, "normal"},
    {68, 1.5f, 0.3f, 1.8f, 0.0f, "normal"},
    {72, 1.4f, 1.0f, 1.7f, 1.5f, "tremor no topo"},
    {45, 1.2f, 0.3f, 1.4f, 0.0f, "incompleta"},
    {70, 1.4f, 2.0f, 1.7f, 0.0f, "pausa no topo"},
    {66, 1.4f, 0.3f, 1.6f, 0.0f, "normal"},
    {71, 1.5f, 0.3f, 1.7f, 0.0f, "normal"},
    {50, 1.3f, 0.3f, 1.5f, 0.0f, "incompleta"},
    {70, 0.5f, 0.2f, 1.0f, 0.0f, "rapida"},
    {69, 1.4f, 0.3f, 1.7f, 0.0f, "normal"},
};
constexpr uint8_t SIM_REPS_POR_SERIE = sizeof(ROTEIRO_SERIE) / sizeof(ROTEIRO_SERIE[0]);

// Duração de uma série do roteiro, incluindo a pausa até a série seguinte (s).
inline float duracaoSerieSimulada() {
  float d = 0;
  for (uint8_t i = 0; i < SIM_REPS_POR_SERIE; i++) {
    const RepSimulada& r = ROTEIRO_SERIE[i];
    const bool ultima = (i == SIM_REPS_POR_SERIE - 1);
    d += r.tSubida + r.tTopo + r.tDescida + (ultima ? SIM_PAUSA_ENTRE_SERIES_S : SIM_PAUSA_ENTRE_REPS_S);
  }
  return d;
}

// Ângulo verdadeiro do roteiro (graus) no instante t; devolve também ω (°/s).
// O roteiro se repete sem fim: repetições descartadas num teste não esgotam o movimento.
inline float anguloRoteiro(float t, float& omega) {
  omega = 0;
  if (t < SIM_REPOUSO_INICIAL_S) return 0;
  float tau = fmodf(t - SIM_REPOUSO_INICIAL_S, duracaoSerieSimulada());

  for (uint8_t i = 0; i < SIM_REPS_POR_SERIE; i++) {
    const RepSimulada& r = ROTEIRO_SERIE[i];
    if (tau < r.tSubida) {  // subida em perfil cossenoidal
      omega = r.amplitude / 2 * (PI_F / r.tSubida) * sinf(PI_F * tau / r.tSubida);
      return r.amplitude / 2 * (1 - cosf(PI_F * tau / r.tSubida));
    }
    tau -= r.tSubida;

    if (tau < r.tTopo) {  // topo, com tremor opcional
      const float w = 2 * PI_F * SIM_FREQ_TREMOR_HZ;
      omega = r.tremor * w * cosf(w * tau);
      return r.amplitude + r.tremor * sinf(w * tau);
    }
    tau -= r.tTopo;

    if (tau < r.tDescida) {  // descida em perfil cossenoidal
      omega = -r.amplitude / 2 * (PI_F / r.tDescida) * sinf(PI_F * tau / r.tDescida);
      return r.amplitude / 2 * (1 + cosf(PI_F * tau / r.tDescida));
    }
    tau -= r.tDescida;

    const bool ultima = (i == SIM_REPS_POR_SERIE - 1);
    const float pausa = ultima ? SIM_PAUSA_ENTRE_SERIES_S : SIM_PAUSA_ENTRE_REPS_S;
    if (tau < pausa) return 0;  // perna em repouso entre repetições
    tau -= pausa;
  }
  return 0;
}

struct GeradorSimulado {
  uint32_t semente;
  float t;  // s
};

// Número pseudoaleatório em [-1, 1); a sequência é a mesma no ESP32 e no computador.
inline float ruido(GeradorSimulado& g) {
  g.semente = g.semente * 1664525u + 1013904223u;
  return (g.semente >> 8) * (2.0f / 16777216.0f) - 1.0f;
}

// Gera a próxima leitura simulada, nas unidades da Adafruit, e avança dt.
inline void lerImuSimulada(GeradorSimulado& g, float dt, AmostraImu& a) {
  float omegaGraus;
  const float theta = anguloRoteiro(g.t, omegaGraus) / RAD_PARA_GRAUS;
  a.ax = GRAVIDADE_M_S2 * sinf(theta) + SIM_RUIDO_ACC_M_S2 * ruido(g);
  a.ay = SIM_RUIDO_ACC_M_S2 * ruido(g);
  a.az = GRAVIDADE_M_S2 * cosf(theta) + SIM_RUIDO_ACC_M_S2 * ruido(g);
  a.gx = SIM_RUIDO_GIRO_RAD_S * ruido(g);
  a.gy = omegaGraus / RAD_PARA_GRAUS * SINAL_GIRO + SIM_OFFSET_GIRO_RAD_S +
         SIM_RUIDO_GIRO_RAD_S * ruido(g);
  a.gz = SIM_RUIDO_GIRO_RAD_S * ruido(g);
  g.t += dt;
}
