// Teste, no computador, da lógica da caneleira sobre o roteiro simulado (P5 a P7).
// Confere o erro do ângulo filtrado, a contagem e a classificação de cada série e,
// desde o P12, o botão (debounce e início da série) e a decisão de deep sleep.
// A partir desta pasta:
//   g++ -std=c++17 -Wall -Wextra -I../caneleira teste_processamento.cpp -o teste && ./teste

#include <cstdio>

#include "botao.h"
#include "energia.h"
#include "processamento.h"
#include "sinal_simulado.h"

constexpr float DT = 0.01f;
constexpr float ERRO_MAX_ANGULO_GRAUS = 3.0f;
constexpr unsigned ESPERADO_VALIDAS = 7, ESPERADO_ADM = 2, ESPERADO_VEL = 1;

int falhas = 0;

// Registra uma verificação e conta a falha, se houver.
void verificar(bool condicao, const char* descricao) {
  printf("  [%s] %s\n", condicao ? "ok" : "FALHA", descricao);
  if (!condicao) falhas++;
}

int main() {
  const Prescricao p = {"sim001", SIM_SERIES, SIM_REPS_POR_SERIE, 60.0f, 150.0f, 10};
  Caneleira c;
  iniciarCaneleira(c, p);
  GeradorSimulado g = {12345u, 0.0f};
  char json[1024];
  float erroMax = 0;

  for (int k = 0; k < 100 * 600 && c.sessao.status != StatusSessao::CONCLUIDA; k++) {
    float omegaReal;
    const float thetaReal = anguloRoteiro(g.t, omegaReal);
    AmostraImu a;
    lerImuSimulada(g, DT, a);

    Repeticao rep;
    const uint8_t ev = processarLeitura(c, a, DT, true, rep);
    if (c.calib.pronta) {
      const float erro = fabsf(c.theta - thetaReal);
      if (erro > erroMax) erroMax = erro;
    }
    if (ev & EV_CALIBRADO) printf("offset medido: %.2f graus/s\n", c.calib.offset);
    if (ev & (EV_REP_VALIDA | EV_REP_FORA)) {
      printf("serie %u rep %2u | ADM %5.1f | vel %5.1f | sobe %.2f | desce %.2f | %s\n",
             (unsigned)(c.sessao.serie + 1), (unsigned)c.sessao.resumos[c.sessao.serie].total,
             rep.adm, rep.velPico, rep.tConc, rep.tExc, rep.valida ? "valida" : "fora");
    }
    if (ev & EV_FIM_SERIE) {
      const int n = montarJsonSessao(c.sessao, json, sizeof(json));
      printf("sessao -> %s (%d bytes)\n", n > 0 ? json : "ERRO", n);
    }
  }

  printf("\nVerificacoes:\n");
  verificar(c.calib.offset > 1.5f && c.calib.offset < 1.9f, "offset do giroscopio ~1,7 graus/s removido");
  verificar(erroMax < ERRO_MAX_ANGULO_GRAUS, "erro do angulo filtrado < 3 graus em todo o roteiro");
  printf("  (erro maximo observado: %.2f graus)\n", erroMax);
  verificar(c.sessao.status == StatusSessao::CONCLUIDA, "sessao concluida apos 3 series");
  for (uint8_t i = 0; i < SIM_SERIES; i++) {
    const ResumoSerie& r = c.sessao.resumos[i];
    char d[96];
    snprintf(d, sizeof(d), "serie %u: %u/%u validas, desvios adm %u, vel %u", (unsigned)r.n,
             (unsigned)r.validas, (unsigned)r.total, (unsigned)r.desviosAdm, (unsigned)r.desviosVel);
    verificar(r.total == SIM_REPS_POR_SERIE && r.validas == ESPERADO_VALIDAS &&
                  r.desviosAdm == ESPERADO_ADM && r.desviosVel == ESPERADO_VEL,
              d);
  }
  // Uso efetivo: liga acima de 1,2 V, desliga abaixo de 0,8 V, mantém o estado entre eles.
  DetectorUso u{};
  const bool histereseOk = atualizarUso(u, 1300) && atualizarUso(u, 1000) && !atualizarUso(u, 700) &&
                           !atualizarUso(u, 1100) && atualizarUso(u, 1250);
  verificar(histereseOk, "histerese do uso efetivo (liga 1,2 V, desliga 0,8 V)");

  // Detector reiniciado com a perna no alto, como no fim de um descanso no meio de um
  // movimento: a descida em curso não pode virar repetição.
  DetectorRepeticao d;
  reiniciarDetector(d, 60.0f);
  Repeticao repReinicio;
  bool contou = false;
  for (int k = 0; k <= 100; k++) {  // desce de 60° a 0° em 1 s
    contou = detectarRepeticao(d, 60.0f * (1 - k / 100.0f), -60.0f, DT, true, repReinicio) || contou;
  }
  for (int k = 0; k < 50; k++) contou = detectarRepeticao(d, 0.0f, 0.0f, DT, true, repReinicio) || contou;
  verificar(!contou, "descida em curso ao reiniciar o detector nao vira repeticao");

  // Prescrição do cmd: aceita a do contrato, recusa sid com caractere de tópico e séries 0.
  const Prescricao doContrato = {"s7f3a", 3, 10, 60.0f, 150.0f, 60};
  Prescricao sidRuim = doContrato;
  snprintf(sidRuim.sid, sizeof(sidRuim.sid), "a/b");
  Prescricao semSeries = doContrato;
  semSeries.series = 0;
  verificar(prescricaoValida(doContrato) && !prescricaoValida(sidRuim) && !prescricaoValida(semSeries),
            "validacao da prescricao recebida pelo cmd");

  // Sem prescrição, a sessão espera o cmd depois da calibração; com ela, começa na hora.
  Caneleira espera;
  iniciarCaneleira(espera, Prescricao{});
  GeradorSimulado g2 = {12345u, 0.0f};
  Repeticao r2;
  for (int k = 0; k < AMOSTRAS_CALIBRACAO + 100; k++) {
    AmostraImu a;
    lerImuSimulada(g2, DT, a);
    processarLeitura(espera, a, DT, true, r2);
  }
  const bool aguardou = espera.calib.pronta && espera.sessao.status == StatusSessao::AGUARDANDO;
  receberPrescricao(espera, doContrato);
  verificar(aguardou && espera.sessao.status == StatusSessao::EM_EXECUCAO,
            "sessao aguarda o cmd e comeca ao receber a prescricao");

  // ===== P12: botão =====
  // Trepidação de 10 ms não conta; 30 ms estáveis contam uma vez; segurar não repete.
  Botao b;
  iniciarBotao(b, false, 0);
  int apertos = 0;
  const bool leituras[] = {true, false, true, false};  // trepida a cada 2 ms
  uint32_t ms = 100;
  for (bool l : leituras) apertos += atualizarBotao(b, l, ms += 2);
  for (int k = 0; k < 100; k++) apertos += atualizarBotao(b, true, ms += 1);  // segura 100 ms
  for (int k = 0; k < 50; k++) apertos += atualizarBotao(b, false, ms += 1);  // solta
  for (int k = 0; k < 50; k++) apertos += atualizarBotao(b, true, ms += 1);   // aperta de novo
  verificar(apertos == 2, "botao: trepidacao ignorada e um evento por aperto");
  Botao jaApertado;
  iniciarBotao(jaApertado, true, 0);  // o toque que acordou a ESP32 ainda está apertado
  int extra = 0;
  for (uint32_t t = 1; t < 200; t++) extra += atualizarBotao(jaApertado, true, t);
  verificar(extra == 0, "botao ja apertado na partida nao gera aperto");

  // ===== P12: série iniciada pelo botão, no roteiro simulado completo =====
  Caneleira cb;
  iniciarCaneleira(cb, p, true);
  GeradorSimulado gb = {12345u, 0.0f};
  bool contouAntesDoBotao = false, esperouNoDescanso = false;
  unsigned liberacoes = 0;
  for (int k = 0; k < 100 * 900 && cb.sessao.status != StatusSessao::CONCLUIDA; k++) {
    AmostraImu a;
    lerImuSimulada(gb, DT, a);
    Repeticao rep;
    uint8_t ev = processarLeitura(cb, a, DT, true, rep);
    if (cb.sessao.esperaBotao && (ev & (EV_REP_VALIDA | EV_REP_FORA))) contouAntesDoBotao = true;
    // Aperta 3 s depois de a série ficar à espera (no roteiro, a perna está parada no início).
    static int esperando = 0;
    esperando = cb.sessao.esperaBotao ? esperando + 1 : 0;
    if (esperando == 300) {
      if (cb.sessao.serie > 0) esperouNoDescanso = true;
      ev = apertarBotao(cb.sessao, cb.theta);
      if (ev & EV_SERIE_LIBERADA) liberacoes++;
    }
  }
  bool seriesOk = cb.sessao.status == StatusSessao::CONCLUIDA;
  for (uint8_t i = 0; i < SIM_SERIES; i++) seriesOk = seriesOk && cb.sessao.resumos[i].total == SIM_REPS_POR_SERIE;
  verificar(seriesOk && liberacoes == SIM_SERIES && esperouNoDescanso && !contouAntesDoBotao,
            "com INICIO_PELO_BOTAO, cada serie espera o botao e a sessao termina completa");

  // Botão no descanso: encerra o descanso e começa a próxima série sem nova espera.
  Sessao sd{};
  iniciarSessao(sd, doContrato, 0.0f, true);
  sd.esperaBotao = false;
  sd.status = StatusSessao::DESCANSO;
  sd.fimDescanso = 100.0f;
  const uint8_t evDescanso = apertarBotao(sd, 0.0f);
  verificar(evDescanso == (EV_FIM_DESCANSO | EV_SERIE_LIBERADA) && sd.serie == 1 &&
                sd.status == StatusSessao::EM_EXECUCAO && !sd.esperaBotao,
            "botao no descanso comeca a proxima serie na hora");
  Sessao semEfeito{};
  semEfeito.status = StatusSessao::AGUARDANDO;
  verificar(apertarBotao(semEfeito, 0.0f) == 0, "botao sem sessao nao muda nada no processamento");

  // ===== P12: decisão de deep sleep =====
  Sessao sono{};
  iniciarSessao(sono, doContrato, 0.0f);
  sono.status = StatusSessao::DESCANSO;
  sono.fimDescanso = 60.0f;  // descanso de 60 s, começando em t = 0
  const DecisaoSono dLongo = decidirSono(sono, 0.0f, false, 0);
  verificar(dLongo.motivo == MotivoSono::DESCANSO && dLongo.dormirMs == 56000,
            "descanso de 60 s: dorme 56 s pelo timer");
  verificar(decidirSono(sono, 45.0f, false, 0).motivo == MotivoSono::NENHUM,
            "faltando 15 s de descanso: nao dorme");
  verificar(decidirSono(sono, 0.0f, true, 0).motivo == MotivoSono::NENHUM,
            "ocupada (vibrando, calibrando ou esperando ack): nao dorme");
  sono.status = StatusSessao::EM_EXECUCAO;
  verificar(decidirSono(sono, 0.0f, false, 60UL * 60UL * 1000UL).motivo == MotivoSono::NENHUM,
            "serie em andamento ou esperando o botao: nunca dorme");
  sono.status = StatusSessao::CONCLUIDA;
  const DecisaoSono dOcioso = decidirSono(sono, 0.0f, false, OCIOSO_PARA_DORMIR_MS);
  verificar(decidirSono(sono, 0.0f, false, OCIOSO_PARA_DORMIR_MS - 1).motivo == MotivoSono::NENHUM &&
                dOcioso.motivo == MotivoSono::OCIOSO && dOcioso.dormirMs == 0,
            "sessao concluida e ociosa por 5 min: dorme e so o botao acorda");

  printf("\n%s\n", falhas == 0 ? "RESULTADO: todas as verificacoes passaram." : "RESULTADO: ha falhas.");
  return falhas == 0 ? 0 : 1;
}
