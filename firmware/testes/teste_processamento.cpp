// Teste, no computador, da lógica da caneleira sobre o roteiro simulado (P5 a P7).
// Confere o erro do ângulo filtrado, a contagem e a classificação de cada série e,
// desde o P12, o botão (debounce e início da série) e a decisão de deep sleep; desde a
// v1.2, os eixos configuráveis, os limiares relativos ao repouso, o uso efetivo com
// tempo e a cadência.
// A partir desta pasta:
//   g++ -std=c++17 -Wall -Wextra -I../caneleira teste_processamento.cpp -o teste && ./teste

#include <cstdio>
#include <cstring>

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

// Gera uma repetição em perfil cossenoidal a partir do ângulo de repouso base e a entrega ao
// detector, amostra por amostra; termina com a perna parada por tPausa. Devolve quantas
// repetições o detector contou e guarda a última.
int alimentarRepeticao(DetectorRepeticao& d, float base, float amplitude, float tSubida,
                       float tTopo, float tDescida, float tPausa, Repeticao& ultima) {
  int contadas = 0;
  const float total = tSubida + tTopo + tDescida + tPausa;
  for (float t = 0; t < total; t += DT) {
    float theta = base, omega = 0;
    if (t < tSubida) {
      theta = base + amplitude / 2 * (1 - cosf(PI_F * t / tSubida));
      omega = amplitude / 2 * (PI_F / tSubida) * sinf(PI_F * t / tSubida);
    } else if (t < tSubida + tTopo) {
      theta = base + amplitude;
    } else if (t < tSubida + tTopo + tDescida) {
      const float tau = t - tSubida - tTopo;
      theta = base + amplitude / 2 * (1 + cosf(PI_F * tau / tDescida));
      omega = -amplitude / 2 * (PI_F / tDescida) * sinf(PI_F * tau / tDescida);
    }
    Repeticao rep;
    if (detectarRepeticao(d, theta, omega, DT, true, rep)) {
      contadas++;
      ultima = rep;
    }
  }
  return contadas;
}

// Classifica uma repetição isolada contra a prescrição de teste e devolve o resumo da série.
ResumoSerie classificarUma(Repeticao rep) {
  Sessao s{};
  const Prescricao p = {"t01", 1, 10, 60.0f, 150.0f, 60};
  iniciarSessao(s, p, 0.0f);
  registrarRepeticao(s, rep, 0.0f);
  return s.resumos[0];
}

// Confere, por cinemática, o eixo do giro deduzido de um par de eixos: para um vetor fixo
// no mundo (a gravidade), visto do sensor, da/dt = -ω × a. Com a perna em θ, o sensor
// lê a = g(cos θ·p + sin θ·e); a extensão (θ crescente) tem de dar ω positivo em V_GIRO
// e o ângulo do acelerômetro tem de devolver θ.
bool eixosCoerentes(Eixo pendente, Eixo estendido) {
  const Vetor3 p = vetorEixo(pendente), e = vetorEixo(estendido);
  const Vetor3 giro = vetorial(e, p);
  bool ok = true;
  for (float graus = 0; graus <= 90.0f; graus += 15.0f) {
    const float th = graus / RAD_PARA_GRAUS, dth = 1e-3f;
    const Vetor3 a{cosf(th) * p.x + sinf(th) * e.x, cosf(th) * p.y + sinf(th) * e.y,
                   cosf(th) * p.z + sinf(th) * e.z};
    const Vetor3 a2{cosf(th + dth) * p.x + sinf(th + dth) * e.x, cosf(th + dth) * p.y + sinf(th + dth) * e.y,
                    cosf(th + dth) * p.z + sinf(th + dth) * e.z};
    const Vetor3 derivada{(a2.x - a.x) / dth, (a2.y - a.y) / dth, (a2.z - a.z) / dth};
    const Vetor3 w = vetorial(giro, a);  // ω × a, com ω = +1 rad/s em V_GIRO
    ok = ok && fabsf(derivada.x + w.x) < 1e-2f && fabsf(derivada.y + w.y) < 1e-2f &&
         fabsf(derivada.z + w.z) < 1e-2f;
    const AmostraImu leitura{9.8f * a.x, 9.8f * a.y, 9.8f * a.z, 0, 0, 0};
    ok = ok && fabsf(anguloAcelerometro(leitura, p, e) - graus) < 0.1f;
  }
  return ok;
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
  // O offset simulado entra nos três eixos; projetado no eixo do giro, vale ±1,7 °/s.
  verificar(fabsf(c.calib.offset) > 1.5f && fabsf(c.calib.offset) < 1.9f, "offset do giroscopio ~1,7 graus/s removido");
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
  for (uint8_t i = 0; i < SIM_SERIES; i++) {
    verificar(c.sessao.resumos[i].desviosCad == 0, "roteiro normal: nenhum desvio de cadencia");
  }
  verificar(strstr(json, "\"cad\":0") != nullptr, "resumo publica desvios.cad (contrato v1.2)");

  // Uso efetivo: prende acima de USO_LIGA_MV; só solta depois de USO_TEMPO_SOLTA_S abaixo de
  // USO_DESLIGA_MV; entre os dois limiares, mantém o estado.
  const float entre = (USO_LIGA_MV + USO_DESLIGA_MV) / 2, abaixo = USO_DESLIGA_MV - 50, acima = USO_LIGA_MV + 50;
  DetectorUso u{};
  bool histereseOk = !atualizarUso(u, entre, DT) && atualizarUso(u, acima, DT) && atualizarUso(u, entre, DT);
  for (int k = 0; k < 20; k++) histereseOk = atualizarUso(u, abaixo, DT) && histereseOk;  // 0,2 s de folga
  histereseOk = atualizarUso(u, entre, DT) && histereseOk;
  verificar(histereseOk, "uso: folga de 0,2 s na tira nao solta a caneleira");
  bool soltou = false;
  for (int k = 0; k < 60; k++) soltou = !atualizarUso(u, abaixo, DT) || soltou;  // 0,6 s abaixo
  verificar(soltou && !atualizarUso(u, entre, DT) && atualizarUso(u, acima, DT),
            "uso: 0,6 s abaixo do limiar solta; entre os limiares mantem; acima prende");

  // Eixos: qualquer par de eixos perpendiculares dá ângulo e giro coerentes.
  bool todosEixos = true;
  const Eixo eixos[6] = {Eixo::X_POS, Eixo::X_NEG, Eixo::Y_POS, Eixo::Y_NEG, Eixo::Z_POS, Eixo::Z_NEG};
  for (Eixo pe : eixos) {
    for (Eixo es : eixos) {
      if (escalar(vetorEixo(pe), vetorEixo(es)) == 0.0f) todosEixos = eixosCoerentes(pe, es) && todosEixos;
    }
  }
  verificar(todosEixos, "eixos: os 24 pares possiveis dao angulo de 0 a 90 graus e giro positivo na extensao");
  verificar((V_GIRO.x == 0 && V_GIRO.y == -1 && V_GIRO.z == 0) || EIXO_PENDENTE != Eixo::Z_POS ||
                EIXO_ESTENDIDO != Eixo::X_POS,
            "eixos do Wokwi (Z pendente, X estendido): a extensao e -gy");
  float valorCima;
  const AmostraImu deitada{0.1f, -9.7f, 0.3f, 0, 0, 0};
  verificar(strcmp(eixoParaCima(deitada, valorCima), "Y_NEG") == 0, "MODO_EIXOS identifica o eixo para cima e o sinal");

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

  // ===== Limiares relativos ao repouso, cadência e reinício do detector =====
  Repeticao ultima{};
  DetectorRepeticao dp;
  reiniciarDetector(dp, 18.0f);
  int noChao = alimentarRepeticao(dp, 18.0f, 0, 0, 0.5f, 0, 0, ultima);  // parada a 18°
  for (int i = 0; i < 3; i++) noChao += alimentarRepeticao(dp, 18.0f, 60, 1.4f, 0.3f, 1.7f, 1.0f, ultima);
  verificar(noChao == 3 && fabsf(ultima.adm - 60.0f) < 2.0f,
            "repouso com o pe no chao (18 graus): 3 de 3 repeticoes, ADM de 60 graus");

  DetectorRepeticao dm;
  reiniciarDetector(dm, 0.0f);
  int pequenas = alimentarRepeticao(dm, 0.0f, 0, 0, 0.5f, 0, 0, ultima);
  for (int i = 0; i < 3; i++) pequenas += alimentarRepeticao(dm, 0.0f, 12, 0.6f, 0.2f, 0.6f, 0.5f, ultima);
  verificar(pequenas == 0, "balanco de 12 graus nao e repeticao");

  DetectorRepeticao ds;
  reiniciarDetector(ds, 0.0f);
  alimentarRepeticao(ds, 0.0f, 0, 0, 0.5f, 0, 0, ultima);
  const int solta = alimentarRepeticao(ds, 0.0f, 65, 1.4f, 0.3f, 0.3f, 1.0f, ultima);
  const ResumoSerie rs = classificarUma(ultima);
  verificar(solta == 1 && rs.desviosCad == 1 && rs.desviosAdm == 0 && rs.desviosVel == 0 && rs.validas == 0,
            "perna que cai (descida em 0,3 s): contada, fora do padrao so por cadencia");
  printf("  (descida solta: t_exc medido %.2f s)\n", ultima.tExc);

  DetectorRepeticao dk;
  reiniciarDetector(dk, 0.0f);
  alimentarRepeticao(dk, 0.0f, 0, 0, 0.5f, 0, 0, ultima);
  const int rapidaOk = alimentarRepeticao(dk, 0.0f, 65, 1.4f, 0.3f, 0.8f, 1.0f, ultima);
  verificar(rapidaOk == 1 && classificarUma(ultima).desviosCad == 0,
            "descida normal mais rapida (0,8 s): nao e desvio de cadencia");

  DetectorRepeticao dt;
  reiniciarDetector(dt, 0.0f);
  alimentarRepeticao(dt, 0.0f, 0, 0, 0.5f, 0, 0, ultima);
  const int travada = alimentarRepeticao(dt, 0.0f, 65, 1.4f, 9.0f, 1.7f, 1.0f, ultima);
  const ResumoSerie rt = classificarUma(ultima);
  verificar(travada == 1 && rt.desviosCad == 1 && rt.desviosAdm == 0 && rt.validas == 0,
            "parada de 9 s no topo: contada uma vez, fora por cadencia");
  printf("  (parada longa: duracao medida %.1f s)\n", ultima.tTotal);

  DetectorRepeticao dn;
  reiniciarDetector(dn, 0.0f);
  alimentarRepeticao(dn, 0.0f, 0, 0, 0.5f, 0, 0, ultima);
  const int normal = alimentarRepeticao(dn, 0.0f, 65, 1.4f, 0.3f, 1.7f, 1.0f, ultima);
  const ResumoSerie rn = classificarUma(ultima);
  verificar(normal == 1 && rn.validas == 1 && rn.desviosCad == 0,
            "repeticao controlada (1,4 s / 1,7 s): valida");

  DetectorRepeticao dc;
  reiniciarDetector(dc, 0.0f);
  int seguidas = alimentarRepeticao(dc, 0.0f, 0, 0, 0.5f, 0, 0, ultima);
  for (int i = 0; i < 5; i++) seguidas += alimentarRepeticao(dc, 0.0f, 65, 1.3f, 0.2f, 1.5f, 0.1f, ultima);
  verificar(seguidas == 5, "5 repeticoes seguidas, sem pausa embaixo: 5 contadas");

  DetectorRepeticao dr;
  reiniciarDetector(dr, 60.0f);  // botão apertado com a perna estendida e parada
  int alto = alimentarRepeticao(dr, 60.0f, 0, 0, 0.5f, 0, 0, ultima);
  for (int k = 0; k <= 150; k++) {  // desce de 60° a 0° em 1,5 s, a -40 °/s
    Repeticao rep;
    alto += detectarRepeticao(dr, 60.0f * (1 - k / 150.0f), -40.0f, DT, true, rep);
  }
  alto += alimentarRepeticao(dr, 0.0f, 0, 0, 0.5f, 0, 0, ultima);  // parada embaixo
  alto += alimentarRepeticao(dr, 0.0f, 65, 1.4f, 0.3f, 1.7f, 1.0f, ultima);
  verificar(alto == 1 && fabsf(ultima.adm - 65.0f) < 2.0f,
            "reiniciado com a perna no alto: a descida nao conta; a repeticao seguinte conta");

  DetectorRepeticao dj;
  reiniciarDetector(dj, 12.0f);  // botão apertado com a perna já subindo, a 12°
  int jaSubindo = 0;
  for (int k = 0; k < 150; k++) {  // sobe de 12° a 65° em 1,5 s
    jaSubindo += detectarRepeticao(dj, 12.0f + 53.0f * k / 150.0f, 40.0f, DT, true, ultima);
  }
  for (int k = 0; k <= 170; k++) {  // desce de 65° a 0° em 1,7 s
    jaSubindo += detectarRepeticao(dj, 65.0f * (1 - k / 170.0f), -38.0f, DT, true, ultima);
  }
  jaSubindo += alimentarRepeticao(dj, 0.0f, 0, 0, 0.5f, 0, 0, ultima);  // parada embaixo
  verificar(jaSubindo == 1, "botao apertado com a perna ja subindo (12 graus): a repeticao conta");

  // Pior caso do resumo: 6 séries com os maiores valores aceitos cabem no buffer de 1024 bytes.
  Sessao grande{};
  const Prescricao pg = {"abcdefghijklmno", MAX_SERIES, 50, 120.0f, 500.0f, 600};
  iniciarSessao(grande, pg, 0.0f);
  for (uint8_t i = 0; i < MAX_SERIES; i++) {
    ResumoSerie& r = grande.resumos[i];
    r.validas = 50; r.total = 50; r.desviosAdm = 50; r.desviosVel = 50; r.desviosCad = 50;
    r.somaAdm = 50 * 120.0f; r.velPico = 500.0f; r.somaConc = 50 * 99.9f; r.somaExc = 50 * 99.9f;
  }
  grande.serie = MAX_SERIES - 1;
  grande.status = StatusSessao::CONCLUIDA;
  const int tamanho = montarJsonSessao(grande, json, sizeof(json));
  verificar(tamanho > 0 && tamanho < 1024, "pior caso do resumo cabe no buffer de 1024 bytes");
  printf("  (pior caso: %d bytes)\n", tamanho);

  printf("\n%s\n", falhas == 0 ? "RESULTADO: todas as verificacoes passaram." : "RESULTADO: ha falhas.");
  return falhas == 0 ? 0 : 1;
}
