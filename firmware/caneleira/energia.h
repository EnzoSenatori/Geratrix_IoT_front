#pragma once
// Baixo consumo (P12): decide quando a ESP32 pode entrar em deep sleep e por quanto tempo.
// Lógica pura, testada no computador; o .ino prepara os periféricos e chama o deep sleep.
//
// Dois casos:
//  - DESCANSO: descanso longo entre séries. Dorme até pouco antes do fim e acorda pelo
//    timer (ou antes, pelo botão). Ao acordar, recalibra e a próxima série começa.
//  - OCIOSO: sem sessão (aguardando o cmd) ou com a sessão concluída, parada há muito
//    tempo. Só o botão acorda.
// Movimento parcial nunca se perde: só se dorme fora de uma série, e as séries concluídas
// já estão gravadas na memória não volátil (P11).

#include <stdint.h>

#include "processamento.h"

// Valores provisórios; revisar com o consumo medido e com o comportamento do power bank.
constexpr float DESCANSO_MIN_PARA_DORMIR_S = 20.0f;  // descanso mais curto não compensa religar o Wi-Fi
constexpr float ANTECEDENCIA_DESPERTAR_S = 4.0f;     // partida + 2 s de calibração antes do fim
constexpr uint32_t OCIOSO_PARA_DORMIR_MS = 5UL * 60UL * 1000UL;  // 5 min sem sessão

enum class MotivoSono : uint8_t { NENHUM, DESCANSO, OCIOSO };

struct DecisaoSono {
  MotivoSono motivo;
  uint32_t dormirMs;  // duração do timer; 0 = só o botão acorda
};

// Decide se a ESP32 pode dormir agora. ocupada cobre o que o processamento não enxerga:
// calibração em curso, vibração tocando ou espera curta pelo ack. ociosoMs é o tempo desde
// a última atividade (botão, cmd, mudança de estado da sessão ou do uso).
inline DecisaoSono decidirSono(const Sessao& s, float tempoS, bool ocupada, uint32_t ociosoMs,
                               uint32_t ociosoLimiteMs = OCIOSO_PARA_DORMIR_MS) {
  DecisaoSono d{MotivoSono::NENHUM, 0};
  if (ocupada) return d;

  if (s.status == StatusSessao::DESCANSO) {
    const float restante = s.fimDescanso - tempoS;
    if (restante >= DESCANSO_MIN_PARA_DORMIR_S) {
      d.motivo = MotivoSono::DESCANSO;
      d.dormirMs = (uint32_t)((restante - ANTECEDENCIA_DESPERTAR_S) * 1000.0f);
    }
    return d;
  }

  // Série em andamento ou aguardando o botão: a pessoa está no exercício, não dorme.
  if (s.status == StatusSessao::EM_EXECUCAO) return d;

  if (ociosoMs >= ociosoLimiteMs) d.motivo = MotivoSono::OCIOSO;
  return d;
}
