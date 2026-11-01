#pragma once
// Processamento local da caneleira (P5 a P7): calibração do giroscópio, ângulo da
// perna por filtro complementar, detecção e classificação de cada repetição,
// controle das séries e resumo no formato do contrato MQTT v1.
// Não depende do Arduino: o mesmo arquivo compila no computador (firmware/testes).

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// ===== Constantes de calibração (valores propostos; ajustar em bancada no P4 a P6) =====
constexpr float ALFA_FILTRO = 0.98f;              // peso do giroscópio no filtro complementar
constexpr float THETA_INICIO_GRAUS = 15.0f;       // acima deste ângulo, a subida começou
constexpr float DELTA_PICO_GRAUS = 5.0f;          // queda abaixo do pico que confirma a descida
constexpr float THETA_FIM_GRAUS = 10.0f;          // abaixo deste ângulo, a perna voltou ao repouso
constexpr float OMEGA_MOVIMENTO_GRAUS_S = 10.0f;  // |ω| acima disto conta como perna em movimento
constexpr uint16_t AMOSTRAS_CALIBRACAO = 200;     // 2 s parado, a 100 Hz, para medir o offset
constexpr float SINAL_GIRO = 1.0f;                // trocar para -1 se a extensão sair negativa (P4)

constexpr uint8_t MAX_SERIES = 6;
constexpr float PI_F = 3.14159265f;
constexpr float RAD_PARA_GRAUS = 180.0f / PI_F;

// Leitura bruta da IMU, nas unidades da biblioteca Adafruit: m/s² e rad/s.
struct AmostraImu {
  float ax, ay, az;
  float gx, gy, gz;
};

// ===== Calibração do giroscópio =====

struct CalibracaoGiro {
  float soma;
  uint16_t n;
  float offset;  // °/s, subtraído de toda leitura posterior
  bool pronta;
};

// Acumula ω com a perna parada; ao completar as amostras, fixa o offset e retorna true.
inline bool acumularCalibracao(CalibracaoGiro& c, float omegaBrutoGrausS) {
  c.soma += omegaBrutoGrausS;
  c.n++;
  if (c.n < AMOSTRAS_CALIBRACAO) return false;
  c.offset = c.soma / c.n;
  c.pronta = true;
  return true;
}

// ===== Ângulo da perna =====

// Ângulo pela gravidade: 0° com a perna pendente (gravidade em Z), 90° estendida (em X).
// Eixos provisórios, iguais aos do P3; confirmar com a placa presa à caneleira (P4).
inline float anguloAcelerometro(const AmostraImu& a) {
  return atan2f(a.ax, a.az) * RAD_PARA_GRAUS;
}

struct FiltroComplementar {
  float theta;
  bool iniciado;
};

// Combina o giroscópio (suave durante o movimento) e o acelerômetro (sem deriva).
inline float atualizarFiltro(FiltroComplementar& f, float thetaAcc, float omega, float dt) {
  if (!f.iniciado) {
    f.theta = thetaAcc;
    f.iniciado = true;
    return f.theta;
  }
  f.theta = ALFA_FILTRO * (f.theta + omega * dt) + (1.0f - ALFA_FILTRO) * thetaAcc;
  return f.theta;
}

// ===== Detecção da repetição (máquina de estados com histerese) =====

// REPOUSO -> SUBINDO ao passar de THETA_INICIO; SUBINDO -> DESCENDO ao cair DELTA_PICO
// abaixo do pico; DESCENDO -> FINALIZANDO ao ficar abaixo de THETA_FIM; a repetição é
// contada quando a descida termina. Se o ângulo voltar a passar do pico, retoma SUBINDO,
// de modo que tremor ou pausa no topo não geram duas contagens.
enum class Fase : uint8_t { REPOUSO, SUBINDO, DESCENDO, FINALIZANDO };

// Métricas de uma repetição concluída. Fases medidas pelo giroscópio: concêntrica é o
// tempo com a perna subindo (ω > limiar); excêntrica, o tempo descendo (ω < -limiar).
struct Repeticao {
  float adm;      // graus: pico menos o ângulo de repouso anterior
  float velPico;  // °/s: maior velocidade de subida
  float tConc;    // s
  float tExc;     // s
  bool uso;       // caneleira presa à perna durante toda a repetição
  bool admOk;
  bool velOk;
  bool valida;
};

struct DetectorRepeticao {
  Fase fase;
  float thetaMin, thetaPico, velPico;
  float tConc, tExc;
  float tSubidaPre, velPre;  // subida iniciada ainda abaixo de THETA_INICIO
  bool usoContinuo;
};

// Volta o detector ao repouso, usando o ângulo atual como referência de partida.
inline void reiniciarDetector(DetectorRepeticao& d, float theta) {
  d = DetectorRepeticao{};
  d.fase = Fase::REPOUSO;
  d.thetaMin = theta;
  d.usoContinuo = true;
}

// Avança a máquina de estados com uma amostra; retorna true quando uma repetição termina.
inline bool detectarRepeticao(DetectorRepeticao& d, float theta, float omega, float dt,
                              bool uso, Repeticao& rep) {
  const bool subindo = omega > OMEGA_MOVIMENTO_GRAUS_S;
  const bool descendo = omega < -OMEGA_MOVIMENTO_GRAUS_S;

  switch (d.fase) {
    case Fase::REPOUSO:
      if (theta < d.thetaMin) d.thetaMin = theta;
      if (subindo) {
        d.tSubidaPre += dt;
        if (omega > d.velPre) d.velPre = omega;
      } else {
        d.tSubidaPre = 0;
        d.velPre = 0;
      }
      if (theta > THETA_INICIO_GRAUS) {
        d.fase = Fase::SUBINDO;
        d.thetaPico = theta;
        d.tConc = d.tSubidaPre;
        d.velPico = d.velPre;
        d.tExc = 0;
        d.usoContinuo = uso;
      }
      return false;

    case Fase::SUBINDO:
    case Fase::DESCENDO:
      d.usoContinuo = d.usoContinuo && uso;
      if (subindo) {
        d.tConc += dt;
        if (omega > d.velPico) d.velPico = omega;
      }
      if (descendo) d.tExc += dt;
      if (theta > d.thetaPico) {
        d.thetaPico = theta;
        d.fase = Fase::SUBINDO;
      } else if (d.fase == Fase::SUBINDO && theta < d.thetaPico - DELTA_PICO_GRAUS) {
        d.fase = Fase::DESCENDO;
      } else if (d.fase == Fase::DESCENDO && theta < THETA_FIM_GRAUS) {
        d.fase = Fase::FINALIZANDO;
      }
      return false;

    case Fase::FINALIZANDO:
      d.usoContinuo = d.usoContinuo && uso;
      if (descendo) {  // ainda terminando a descida abaixo de THETA_FIM
        d.tExc += dt;
        return false;
      }
      rep = Repeticao{};
      rep.adm = d.thetaPico - d.thetaMin;
      rep.velPico = d.velPico;
      rep.tConc = d.tConc;
      rep.tExc = d.tExc;
      rep.uso = d.usoContinuo;
      reiniciarDetector(d, theta);
      return true;
  }
  return false;
}

// ===== Prescrição, séries e sessão =====

// Parâmetros recebidos do app pelo tópico cmd (P10); valores de teste não são clínicos.
struct Prescricao {
  char sid[16];       // identificador anônimo da sessão
  uint8_t series;
  uint8_t reps;
  float admMin;       // graus
  float velMax;       // °/s
  uint16_t descanso;  // s
};

// Acumulados de uma série; as médias são calculadas na hora de montar o JSON.
struct ResumoSerie {
  uint8_t n, validas, total, desviosAdm, desviosVel;
  float somaAdm, velPico, somaConc, somaExc;
};

enum class StatusSessao : uint8_t { AGUARDANDO, EM_EXECUCAO, DESCANSO, CONCLUIDA };

struct Sessao {
  Prescricao p;
  StatusSessao status;
  uint8_t serie;  // índice da série atual (0 = primeira)
  ResumoSerie resumos[MAX_SERIES];
  float fimDescanso;  // instante (s) em que o descanso termina
  DetectorRepeticao det;
};

// Eventos devolvidos a cada amostra; podem vir combinados (ex.: repetição + fim de série).
enum : uint8_t {
  EV_REP_VALIDA = 1,
  EV_REP_FORA = 2,
  EV_REP_SEM_USO = 4,
  EV_FIM_SERIE = 8,
  EV_FIM_DESCANSO = 16,
  EV_SESSAO_CONCLUIDA = 32,
  EV_CALIBRADO = 64,
};

// Começa a sessão pela primeira série, com a prescrição recebida.
inline void iniciarSessao(Sessao& s, const Prescricao& p, float thetaAtual) {
  s = Sessao{};
  s.p = p;
  if (s.p.series < 1) s.p.series = 1;
  if (s.p.series > MAX_SERIES) s.p.series = MAX_SERIES;
  if (s.p.reps < 1) s.p.reps = 1;
  for (uint8_t i = 0; i < MAX_SERIES; i++) s.resumos[i].n = i + 1;
  s.status = StatusSessao::EM_EXECUCAO;
  reiniciarDetector(s.det, thetaAtual);
}

// Quantas séries já terminaram (as que entram no resumo publicado).
inline uint8_t seriesConcluidas(const Sessao& s) {
  if (s.status == StatusSessao::DESCANSO || s.status == StatusSessao::CONCLUIDA) return s.serie + 1;
  return s.serie;
}

// Classifica a repetição contra a prescrição e soma-a ao resumo da série atual.
inline uint8_t registrarRepeticao(Sessao& s, Repeticao& rep, float t) {
  rep.admOk = rep.adm >= s.p.admMin;
  rep.velOk = rep.velPico <= s.p.velMax;
  rep.valida = rep.admOk && rep.velOk;

  ResumoSerie& r = s.resumos[s.serie];
  r.total++;
  if (rep.valida) r.validas++;
  if (!rep.admOk) r.desviosAdm++;  // uma repetição pode ter os dois desvios
  if (!rep.velOk) r.desviosVel++;
  r.somaAdm += rep.adm;
  r.somaConc += rep.tConc;
  r.somaExc += rep.tExc;
  if (rep.velPico > r.velPico) r.velPico = rep.velPico;

  uint8_t ev = rep.valida ? EV_REP_VALIDA : EV_REP_FORA;
  if (r.total >= s.p.reps) {
    ev |= EV_FIM_SERIE;
    if (s.serie + 1 >= s.p.series) {
      s.status = StatusSessao::CONCLUIDA;
      ev |= EV_SESSAO_CONCLUIDA;
    } else {
      s.status = StatusSessao::DESCANSO;
      s.fimDescanso = t + s.p.descanso;
    }
  }
  return ev;
}

// Processa uma amostra de ângulo e velocidade; movimento no descanso é ignorado.
inline uint8_t processarAmostra(Sessao& s, float theta, float omega, float t, float dt,
                                bool uso, Repeticao& rep) {
  switch (s.status) {
    case StatusSessao::AGUARDANDO:
    case StatusSessao::CONCLUIDA:
      return 0;

    case StatusSessao::DESCANSO:
      if (t < s.fimDescanso) return 0;
      s.serie++;
      s.status = StatusSessao::EM_EXECUCAO;
      reiniciarDetector(s.det, theta);
      return EV_FIM_DESCANSO;

    case StatusSessao::EM_EXECUCAO:
      if (!detectarRepeticao(s.det, theta, omega, dt, uso, rep)) return 0;
      if (!rep.uso) return EV_REP_SEM_USO;  // sem uso efetivo, o movimento não conta
      return registrarRepeticao(s, rep, t);
  }
  return 0;
}

// Texto do status, como definido no contrato.
inline const char* textoStatus(StatusSessao st) {
  switch (st) {
    case StatusSessao::AGUARDANDO: return "aguardando";
    case StatusSessao::EM_EXECUCAO: return "em_execucao";
    case StatusSessao::DESCANSO: return "descanso";
    case StatusSessao::CONCLUIDA: return "concluida";
  }
  return "aguardando";
}

// Monta o resumo acumulado das séries concluídas (tópico sessao/<sid>); retorna o
// tamanho do texto ou -1 se não couber no buffer. Sem hora confiável, ts = null.
inline int montarJsonSessao(const Sessao& s, char* buf, size_t tam) {
  int pos = snprintf(buf, tam, "{\"sid\":\"%s\",\"status\":\"%s\",\"series\":[", s.p.sid,
                     textoStatus(s.status));
  if (pos < 0 || (size_t)pos >= tam) return -1;

  const uint8_t concluidas = seriesConcluidas(s);
  for (uint8_t i = 0; i < concluidas; i++) {
    const ResumoSerie& r = s.resumos[i];
    const float total = r.total > 0 ? (float)r.total : 1.0f;
    const int n = snprintf(buf + pos, tam - pos,
                           "%s{\"n\":%u,\"validas\":%u,\"total\":%u,\"adm_media\":%.1f,"
                           "\"vel_pico\":%.1f,\"desvios\":{\"adm\":%u,\"vel\":%u},"
                           "\"t_conc\":%.1f,\"t_exc\":%.1f}",
                           i > 0 ? "," : "", (unsigned)r.n, (unsigned)r.validas,
                           (unsigned)r.total, r.somaAdm / total, r.velPico,
                           (unsigned)r.desviosAdm, (unsigned)r.desviosVel, r.somaConc / total,
                           r.somaExc / total);
    if (n < 0 || (size_t)(pos + n) >= tam) return -1;
    pos += n;
  }

  const int n = snprintf(buf + pos, tam - pos, "],\"ts\":null}");
  if (n < 0 || (size_t)(pos + n) >= tam) return -1;
  return pos + n;
}

// ===== Processamento completo de uma leitura =====

struct Caneleira {
  Prescricao prescricao;
  CalibracaoGiro calib;
  FiltroComplementar filtro;
  Sessao sessao;
  float tempo;     // s desde o fim da calibração (relógio da amostragem)
  float thetaAcc;  // graus, só acelerômetro
  float theta;     // graus, filtrado
  float omega;     // °/s, já sem offset
};

// Prepara o processamento; a sessão começa assim que o offset do giroscópio for medido.
inline void iniciarCaneleira(Caneleira& c, const Prescricao& p) {
  c = Caneleira{};
  c.prescricao = p;
  c.sessao.status = StatusSessao::AGUARDANDO;
}

// Processa uma leitura da IMU (chamada a 100 Hz): calibração, ângulo, repetição e sessão.
inline uint8_t processarLeitura(Caneleira& c, const AmostraImu& a, float dt, bool uso,
                                Repeticao& rep) {
  const float omegaBruto = a.gy * RAD_PARA_GRAUS * SINAL_GIRO;
  c.thetaAcc = anguloAcelerometro(a);

  if (!c.calib.pronta) {
    if (!acumularCalibracao(c.calib, omegaBruto)) return 0;
    c.theta = atualizarFiltro(c.filtro, c.thetaAcc, 0.0f, dt);
    iniciarSessao(c.sessao, c.prescricao, c.theta);
    return EV_CALIBRADO;
  }

  c.tempo += dt;
  c.omega = omegaBruto - c.calib.offset;
  c.theta = atualizarFiltro(c.filtro, c.thetaAcc, c.omega, dt);
  return processarAmostra(c.sessao, c.theta, c.omega, c.tempo, dt, uso, rep);
}
