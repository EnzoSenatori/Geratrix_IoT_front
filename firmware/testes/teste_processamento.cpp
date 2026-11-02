// Teste, no computador, da lógica da caneleira sobre o roteiro simulado (P5 a P7).
// Confere o erro do ângulo filtrado, a contagem e a classificação de cada série.
// A partir desta pasta:
//   g++ -std=c++17 -Wall -Wextra -I../caneleira teste_processamento.cpp -o teste && ./teste

#include <cstdio>

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

  printf("\n%s\n", falhas == 0 ? "RESULTADO: todas as verificacoes passaram." : "RESULTADO: ha falhas.");
  return falhas == 0 ? 0 : 1;
}
