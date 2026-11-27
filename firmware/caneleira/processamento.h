#pragma once
// Processamento local da caneleira (P5 a P7): calibração do giroscópio, ângulo da
// perna por filtro complementar, detecção e classificação de cada repetição,
// controle das séries e resumo no formato do contrato MQTT (v1.2: desvios.cad).
// Não depende do Arduino: o mesmo arquivo compila no computador (firmware/testes).

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ajustes.h"  // valores de calibração e de operação: só lá se ajusta

constexpr uint16_t AMOSTRAS_CALIBRACAO = 200;     // 2 s parado, a 100 Hz, para medir o offset

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

// ===== Eixos e ângulo da perna =====

struct Vetor3 {
  float x, y, z;
};

// Vetor unitário de um eixo da MPU-6050, com o sinal.
constexpr Vetor3 vetorEixo(Eixo e) {
  return e == Eixo::X_POS   ? Vetor3{1, 0, 0}
         : e == Eixo::X_NEG ? Vetor3{-1, 0, 0}
         : e == Eixo::Y_POS ? Vetor3{0, 1, 0}
         : e == Eixo::Y_NEG ? Vetor3{0, -1, 0}
         : e == Eixo::Z_POS ? Vetor3{0, 0, 1}
                            : Vetor3{0, 0, -1};
}

// Produto vetorial a × b.
constexpr Vetor3 vetorial(Vetor3 a, Vetor3 b) {
  return Vetor3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// Produto escalar a · b.
constexpr float escalar(Vetor3 a, Vetor3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// Eixo que aponta para cima com a perna pendente e com ela estendida (ajustes.h, seção 2).
constexpr Vetor3 V_PENDENTE = vetorEixo(EIXO_PENDENTE);
constexpr Vetor3 V_ESTENDIDO = vetorEixo(EIXO_ESTENDIDO);
// Eixo do giroscópio em que a extensão é positiva. A perna gira em torno do joelho e o
// vetor "para cima" medido passa do eixo pendente para o estendido; visto pelo sensor, a
// gravidade gira ao contrário do sensor, então o giro da extensão é estendido × pendente.
constexpr Vetor3 V_GIRO = vetorial(V_ESTENDIDO, V_PENDENTE);
static_assert(escalar(V_PENDENTE, V_ESTENDIDO) == 0.0f,
              "ajustes.h: EIXO_PENDENTE e EIXO_ESTENDIDO precisam ser eixos diferentes (X, Y ou Z)");

// Ângulo pela gravidade: 0° com a perna pendente, 90° com ela estendida na horizontal.
inline float anguloAcelerometro(const AmostraImu& a, Vetor3 pendente = V_PENDENTE,
                                Vetor3 estendido = V_ESTENDIDO) {
  const Vetor3 acc{a.ax, a.ay, a.az};
  return atan2f(escalar(acc, estendido), escalar(acc, pendente)) * RAD_PARA_GRAUS;
}

// Velocidade angular da perna (°/s, positiva na extensão), antes de tirar o offset.
inline float omegaExtensao(const AmostraImu& a, Vetor3 giro = V_GIRO) {
  return escalar(Vetor3{a.gx, a.gy, a.gz}, giro) * RAD_PARA_GRAUS * SINAL_GIRO;
}

// Eixo da MPU que aponta para cima agora (o de maior aceleração), para o MODO_EIXOS.
inline const char* eixoParaCima(const AmostraImu& a, float& valor) {
  const float v[3] = {a.ax, a.ay, a.az};
  static const char* const nomes[6] = {"X_POS", "X_NEG", "Y_POS", "Y_NEG", "Z_POS", "Z_NEG"};
  uint8_t maior = 0;
  for (uint8_t i = 1; i < 3; i++) {
    if (fabsf(v[i]) > fabsf(v[maior])) maior = i;
  }
  valor = v[maior];
  return nomes[2 * maior + (v[maior] < 0 ? 1 : 0)];
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

// ===== Uso efetivo (FSR na tira de fixação) =====

// Limiares USO_LIGA_MV, USO_DESLIGA_MV e USO_TEMPO_SOLTA_S em ajustes.h (seção 4).

struct DetectorUso {
  bool emUso;
  float tAbaixo;  // s seguidos abaixo de USO_DESLIGA_MV
};

// Atualiza o estado de uso com histerese de tensão e de tempo: prende na hora ao passar de
// USO_LIGA_MV; só solta depois de USO_TEMPO_SOLTA_S abaixo de USO_DESLIGA_MV, para que a
// folga da tira durante a extensão não descarte a repetição.
inline bool atualizarUso(DetectorUso& u, float milivolts, float dt) {
  if (!u.emUso) {
    if (milivolts >= USO_LIGA_MV) {
      u.emUso = true;
      u.tAbaixo = 0;
    }
    return u.emUso;
  }
  if (milivolts <= USO_DESLIGA_MV) {
    u.tAbaixo += dt;
    if (u.tAbaixo >= USO_TEMPO_SOLTA_S) u.emUso = false;
  } else {
    u.tAbaixo = 0;
  }
  return u.emUso;
}

// ===== Detecção da repetição (máquina de estados com histerese) =====

// Os limiares valem acima do repouso (o menor ângulo visto em REPOUSO antes da subida), o
// que funciona com a perna pendente em 0° ou com o pé apoiado no chão em 20°.
// REPOUSO -> SUBINDO ao passar de repouso + THETA_INICIO; SUBINDO -> DESCENDO ao cair
// DELTA_PICO abaixo do pico; DESCENDO -> FINALIZANDO ao ficar abaixo de repouso + THETA_FIM;
// a repetição é contada quando a descida termina. Se o ângulo voltar a passar do pico, retoma SUBINDO,
// de modo que tremor ou pausa no topo não geram duas contagens.
enum class Fase : uint8_t { REPOUSO, SUBINDO, DESCENDO, FINALIZANDO };

// Métricas de uma repetição concluída. Fases medidas pelo giroscópio: concêntrica é o
// tempo com a perna subindo (ω > limiar); excêntrica, o tempo descendo (ω < -limiar).
struct Repeticao {
  float adm;      // graus: pico menos o ângulo de repouso anterior
  float velPico;  // °/s: maior velocidade de subida
  float tConc;    // s
  float tExc;     // s
  float tTotal;   // s, do início da subida ao fim da descida (cadência)
  bool uso;       // caneleira presa à perna durante toda a repetição
  bool admOk;
  bool velOk;
  bool cadOk;     // descida controlada e repetição sem travar
  bool valida;
};

struct DetectorRepeticao {
  Fase fase;
  float thetaMin, thetaPico, velPico;
  float tConc, tExc;
  float tSubidaPre, velPre;  // subida iniciada ainda abaixo do limiar de início
  float tRep;                // s desde o início da subida (cadência)
  bool usoContinuo;
};

// Volta o detector ao repouso, usando o ângulo atual como repouso de partida; se a perna
// ainda estiver descendo, o repouso desce junto com ela.
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
      if (theta < d.thetaMin) d.thetaMin = theta;  // o repouso acompanha a perna que desce
      if (subindo) {
        d.tSubidaPre += dt;
        if (omega > d.velPre) d.velPre = omega;
      } else {
        d.tSubidaPre = 0;
        d.velPre = 0;
      }
      if (theta > d.thetaMin + THETA_INICIO_GRAUS) {
        d.fase = Fase::SUBINDO;
        d.thetaPico = theta;
        d.tConc = d.tSubidaPre;
        d.velPico = d.velPre;
        d.tExc = 0;
        d.tRep = d.tSubidaPre;
        d.usoContinuo = uso;
      }
      return false;

    case Fase::SUBINDO:
    case Fase::DESCENDO:
      d.tRep += dt;
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
      } else if (d.fase == Fase::DESCENDO && theta < d.thetaMin + THETA_FIM_GRAUS) {
        d.fase = Fase::FINALIZANDO;
      }
      return false;

    case Fase::FINALIZANDO:
      d.tRep += dt;
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
      rep.tTotal = d.tRep;
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
  uint8_t n, validas, total, desviosAdm, desviosVel, desviosCad;
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
  bool porBotao;      // P12: cada série só começa quando o botão é apertado
  bool esperaBotao;   // série atual aguardando o botão (status segue em_execucao)
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
  EV_SERIE_LIBERADA = 128,  // P12: o botão começou a série que aguardava
};

// Confere se a prescrição recebida pelo cmd é utilizável. São limites técnicos de
// coerência dos dados, não limites clínicos.
inline bool prescricaoValida(const Prescricao& p) {
  const size_t n = strnlen(p.sid, sizeof(p.sid));
  if (n == 0 || n >= sizeof(p.sid)) return false;
  if (strchr(p.sid, '/') || strchr(p.sid, '+') || strchr(p.sid, '#')) return false;
  return p.series >= 1 && p.series <= MAX_SERIES && p.reps >= 1 && p.reps <= 50 &&
         p.admMin > 0 && p.admMin <= 120 && p.velMax > 0 && p.velMax <= 500 &&
         p.descanso <= 600;
}

// Começa a sessão pela primeira série, com a prescrição recebida. Com porBotao, a série
// fica aguardando o botão antes de contar repetições.
inline void iniciarSessao(Sessao& s, const Prescricao& p, float thetaAtual, bool porBotao = false) {
  s = Sessao{};
  s.p = p;
  if (s.p.series < 1) s.p.series = 1;
  if (s.p.series > MAX_SERIES) s.p.series = MAX_SERIES;
  if (s.p.reps < 1) s.p.reps = 1;
  for (uint8_t i = 0; i < MAX_SERIES; i++) s.resumos[i].n = i + 1;
  s.status = StatusSessao::EM_EXECUCAO;
  s.porBotao = porBotao;
  s.esperaBotao = porBotao;
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
  rep.cadOk = rep.tExc >= T_DESCIDA_MIN_S && rep.tTotal <= T_REPETICAO_MAX_S;
  rep.valida = rep.admOk && rep.velOk && rep.cadOk;

  ResumoSerie& r = s.resumos[s.serie];
  r.total++;
  if (rep.valida) r.validas++;
  if (!rep.admOk) r.desviosAdm++;  // uma repetição pode ter mais de um desvio
  if (!rep.velOk) r.desviosVel++;
  if (!rep.cadOk) r.desviosCad++;
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
      s.esperaBotao = s.porBotao;
      reiniciarDetector(s.det, theta);
      return EV_FIM_DESCANSO;

    case StatusSessao::EM_EXECUCAO:
      if (s.esperaBotao) return 0;  // movimento antes do botão não conta
      if (!detectarRepeticao(s.det, theta, omega, dt, uso, rep)) return 0;
      if (!rep.uso) return EV_REP_SEM_USO;  // sem uso efetivo, o movimento não conta
      return registrarRepeticao(s, rep, t);
  }
  return 0;
}

// Trata um aperto do botão (P12). No descanso, encerra-o e começa a próxima série na hora;
// numa série que aguarda o botão, libera a contagem. Nos demais estados, não faz nada.
inline uint8_t apertarBotao(Sessao& s, float theta) {
  if (s.status == StatusSessao::DESCANSO) {
    s.serie++;
    s.status = StatusSessao::EM_EXECUCAO;
    s.esperaBotao = false;
    reiniciarDetector(s.det, theta);
    return EV_FIM_DESCANSO | EV_SERIE_LIBERADA;
  }
  if (s.status == StatusSessao::EM_EXECUCAO && s.esperaBotao) {
    s.esperaBotao = false;
    reiniciarDetector(s.det, theta);
    return EV_SERIE_LIBERADA;
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
                           "\"vel_pico\":%.1f,\"desvios\":{\"adm\":%u,\"vel\":%u,\"cad\":%u},"
                           "\"t_conc\":%.1f,\"t_exc\":%.1f}",
                           i > 0 ? "," : "", (unsigned)r.n, (unsigned)r.validas,
                           (unsigned)r.total, r.somaAdm / total, r.velPico,
                           (unsigned)r.desviosAdm, (unsigned)r.desviosVel, (unsigned)r.desviosCad,
                           r.somaConc / total,
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
  bool inicioPeloBotao;  // P12: cada série espera o botão
};

// Prepara o processamento. Com uma prescrição (séries > 0), a sessão começa assim que o
// offset do giroscópio for medido; sem ela, aguarda a prescrição pelo cmd.
inline void iniciarCaneleira(Caneleira& c, const Prescricao& p, bool inicioPeloBotao = false) {
  c = Caneleira{};
  c.inicioPeloBotao = inicioPeloBotao;
  c.prescricao = p;
  c.sessao.status = StatusSessao::AGUARDANDO;
}

// Recebe a prescrição do app: começa a sessão já, ou logo após a calibração.
inline void receberPrescricao(Caneleira& c, const Prescricao& p) {
  c.prescricao = p;
  if (c.calib.pronta) iniciarSessao(c.sessao, p, c.theta, c.inicioPeloBotao);
}

// Processa uma leitura da IMU (chamada a 100 Hz): calibração, ângulo, repetição e sessão.
inline uint8_t processarLeitura(Caneleira& c, const AmostraImu& a, float dt, bool uso,
                                Repeticao& rep) {
  const float omegaBruto = omegaExtensao(a);
  c.thetaAcc = anguloAcelerometro(a);

  if (!c.calib.pronta) {
    if (!acumularCalibracao(c.calib, omegaBruto)) return 0;
    c.theta = atualizarFiltro(c.filtro, c.thetaAcc, 0.0f, dt);
    if (c.prescricao.series > 0) {  // senão, aguarda o cmd
      iniciarSessao(c.sessao, c.prescricao, c.theta, c.inicioPeloBotao);
    }
    return EV_CALIBRADO;
  }

  c.tempo += dt;
  c.omega = omegaBruto - c.calib.offset;
  c.theta = atualizarFiltro(c.filtro, c.thetaAcc, c.omega, dt);
  return processarAmostra(c.sessao, c.theta, c.omega, c.tempo, dt, uso, rep);
}
