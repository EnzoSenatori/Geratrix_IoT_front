#pragma once
// Botão da caneleira (P12): filtro de trepidação (debounce) sem bloquear a amostragem.
// Lógica pura: recebe a leitura do pino e o instante; o .ino lê o GPIO 33 (ativo em LOW).

#include <stdint.h>

constexpr uint16_t BOTAO_ESTAVEL_MS = 30;  // tempo que a leitura precisa ficar igual

struct Botao {
  bool estavel;      // estado aceito: true = apertado
  bool bruto;        // última leitura do pino
  uint32_t desdeMs;  // instante da última mudança da leitura
};

// Começa no estado atual do pino, para que um botão já apertado na partida (por exemplo,
// o mesmo toque que acordou a ESP32) não gere um segundo aperto.
inline void iniciarBotao(Botao& b, bool apertado, uint32_t agoraMs) {
  b.estavel = apertado;
  b.bruto = apertado;
  b.desdeMs = agoraMs;
}

// Atualiza com a leitura atual; retorna true uma única vez por aperto, quando o botão
// fica apertado por BOTAO_ESTAVEL_MS seguidos.
inline bool atualizarBotao(Botao& b, bool apertado, uint32_t agoraMs) {
  if (apertado != b.bruto) {
    b.bruto = apertado;
    b.desdeMs = agoraMs;
    return false;
  }
  if (apertado == b.estavel || agoraMs - b.desdeMs < BOTAO_ESTAVEL_MS) return false;
  b.estavel = apertado;
  return apertado;
}
