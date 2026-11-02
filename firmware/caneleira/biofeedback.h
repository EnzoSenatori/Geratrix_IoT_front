#pragma once
// Biofeedback (P9): padrões de vibração tocados sem bloquear a amostragem de 100 Hz.
// Lógica pura: a cada instante, diz se o motor deve estar ligado; o .ino aplica no pino.
// Padrões: repetição válida = 1 pulso curto; fora do padrão = 2 curtos;
// fim de série = 3 curtos; fim do descanso = 1 pulso longo.

#include <stdint.h>

// Tempos provisórios; ajustar com o motor real até os padrões serem distinguíveis.
constexpr uint16_t VIB_CURTO_MS = 150;
constexpr uint16_t VIB_LONGO_MS = 600;
constexpr uint16_t VIB_PAUSA_MS = 150;          // entre pulsos de um mesmo padrão
constexpr uint16_t VIB_ENTRE_PADROES_MS = 500;  // silêncio depois de cada padrão
constexpr uint8_t VIB_TAMANHO_FILA = 4;

enum class Padrao : uint8_t { REP_VALIDA, REP_FORA, FIM_SERIE, FIM_DESCANSO };

struct Biofeedback {
  Padrao fila[VIB_TAMANHO_FILA];
  uint8_t inicio, tamanho;  // fila circular de padrões a tocar
  bool tocando;
  uint8_t trecho;           // par = pulso (motor ligado); ímpar = pausa
  uint32_t fimTrechoMs;
};

// Número de pulsos e duração de cada pulso do padrão.
inline void descreverPadrao(Padrao p, uint8_t& pulsos, uint16_t& duracaoMs) {
  pulsos = 1;
  duracaoMs = VIB_CURTO_MS;
  if (p == Padrao::REP_FORA) pulsos = 2;
  if (p == Padrao::FIM_SERIE) pulsos = 3;
  if (p == Padrao::FIM_DESCANSO) duracaoMs = VIB_LONGO_MS;
}

// Põe um padrão no fim da fila; com a fila cheia, o pedido é descartado.
inline void pedirPadrao(Biofeedback& b, Padrao p) {
  if (b.tamanho >= VIB_TAMANHO_FILA) return;
  b.fila[(b.inicio + b.tamanho) % VIB_TAMANHO_FILA] = p;
  b.tamanho++;
}

// Avança a fila até o instante agoraMs e diz se o motor deve estar ligado.
inline bool atualizarBiofeedback(Biofeedback& b, uint32_t agoraMs) {
  while (b.tamanho > 0) {
    uint8_t pulsos;
    uint16_t duracaoMs;
    descreverPadrao(b.fila[b.inicio], pulsos, duracaoMs);

    if (!b.tocando) {  // começa o padrão da frente da fila
      b.tocando = true;
      b.trecho = 0;
      b.fimTrechoMs = agoraMs + duracaoMs;
    }

    // Trechos do padrão: pulso, pausa, pulso, ..., pulso, silêncio final (2 × pulsos).
    while ((int32_t)(agoraMs - b.fimTrechoMs) >= 0 && b.trecho < 2 * pulsos) {
      b.trecho++;
      const bool pulso = (b.trecho % 2 == 0);
      const bool silencioFinal = (b.trecho == 2 * pulsos - 1);
      b.fimTrechoMs += pulso ? duracaoMs : (silencioFinal ? VIB_ENTRE_PADROES_MS : VIB_PAUSA_MS);
    }

    if (b.trecho < 2 * pulsos) return b.trecho % 2 == 0;

    b.inicio = (b.inicio + 1) % VIB_TAMANHO_FILA;  // padrão terminado: passa ao próximo
    b.tamanho--;
    b.tocando = false;
  }
  return false;
}
