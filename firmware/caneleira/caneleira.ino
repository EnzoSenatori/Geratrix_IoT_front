// Caneleira instrumentada — firmware da ESP32 (P3 a P12).
// Lê a IMU a 100 Hz, estima o ângulo da perna, conta e classifica cada repetição e,
// ao fim de cada série, publica o resumo no tópico sessao/<sid> do contrato MQTT.
// Com USAR_SINAL_SIMULADO = true, roda sem sensor, sobre um roteiro de repetições
// fictícias cujo resultado esperado é conhecido (ver sinal_simulado.h).
// P12: cada série começa pelo botão (GPIO 33), e a ESP32 dorme (deep sleep) nos descansos
// longos e quando fica ociosa, acordando pelo timer ou pelo botão.

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_random.h>
#include <esp_sleep.h>

#include "biofeedback.h"
#include "botao.h"
#include "certificado_ca.h"
#include "energia.h"
#include "processamento.h"
#include "segredos.h"
#include "sinal_simulado.h"

// ===== Configuração =====
constexpr bool USAR_SINAL_SIMULADO = true;  // false para usar a MPU-6050 real
constexpr bool IMPRIMIR_ANGULO = false;     // true: ângulos a 10 Hz, útil no P4 e no P5
constexpr bool USAR_REDE = true;            // false: funciona sem Wi-Fi nem broker
constexpr bool ESPERAR_CMD = USAR_REDE;     // com rede, a sessão só começa com a prescrição do app
constexpr bool NO_WOKWI = true;             // o simulador roda abaixo do tempo real (ver ESTADO_PERIODO_MS)
constexpr bool TESTAR_REINICIO_P11 = false;  // no Wokwi, reinicia antes de publicar a primeira série
constexpr bool TESTAR_OFFLINE_P11 = false;  // simula broker indisponível até o fim da sessão
constexpr uint8_t PIN_SDA = 21;
constexpr uint8_t PIN_SCL = 22;
constexpr uint8_t PIN_FSR = 34;  // FSR na tira (no Wokwi, o potenciômetro "fsr")
constexpr bool USAR_FSR = true;  // false: considera a caneleira sempre presa
constexpr uint8_t PIN_MOTOR = 26;            // motor de vibração (no Wokwi, o LED "motor")
constexpr uint8_t INTENSIDADE_MOTOR = 200;   // 0 a 255: duty do PWM com o motor ligado
constexpr uint32_t FREQ_PWM_MOTOR_HZ = 5000;
constexpr uint8_t CANAL_PWM_MOTOR = 0;       // usado só pelo core 2.x da ESP32
constexpr uint8_t PIN_BATERIA = 35;          // divisor 2 × 100 kΩ (no Wokwi, o potenciômetro "bateria")
constexpr float BATERIA_VAZIA_MV = 3300.0f;  // LiPo considerada vazia (aproximação linear)
constexpr float BATERIA_CHEIA_MV = 4200.0f;  // LiPo cheia
constexpr uint8_t PIN_BOTAO = 33;            // botão para o GND, pull-up interno (no Wokwi, "botao")
constexpr bool INICIO_PELO_BOTAO = true;     // P12: cada série só começa quando o botão é apertado
constexpr bool USAR_DEEP_SLEEP = USAR_REDE;  // P12: depende da gravação das séries na NVS (P11)
constexpr uint32_t ESPERA_ACK_ANTES_DE_DORMIR_MS = 5000;  // dá tempo ao app de confirmar a série
constexpr uint32_t PERIODO_US = 10000;  // 100 Hz
constexpr float DT_S = PERIODO_US / 1000000.0f;

// Prescrição usada só sem rede (USAR_REDE = false); com rede, ela chega pelo tópico cmd.
// Valores fictícios, não clínicos. No modo simulado o descanso cai para 10 s.
const Prescricao PRESCRICAO_TESTE = {"sim001", 3, 10, 60.0f, 150.0f,
                                     (uint16_t)(USAR_SINAL_SIMULADO ? 10 : 60)};

// ===== Comunicação (contrato MQTT v1.1) =====
const char MQTT_HOST[] = "z2f0d126.ala.eu-central-1.emqxsl.com";
constexpr uint16_t MQTT_PORTA = 8883;                 // MQTT sobre TLS
const char MQTT_CLIENT_ID[] = "caneleira-01";         // único por conexão
const char TOPICO_ESTADO[] = "imrea/g01/caneleira/estado";
const char TOPICO_CMD[] = "imrea/g01/caneleira/cmd";
const char TOPICO_ACK[] = "imrea/g01/caneleira/ack";
const char TOPICO_SESSAO_PREFIXO[] = "imrea/g01/caneleira/sessao/";
const char ESTADO_OFFLINE[] = "{\"online\":false}";   // last will, retido
// O broker conta o tempo real, e o Wokwi roda a cerca de 50% dele: no simulador, o estado
// sai mais vezes e o keepalive é maior, para o broker não dar a caneleira por desconectada.
constexpr uint32_t ESTADO_PERIODO_MS = NO_WOKWI ? 20000 : 60000;  // contrato: a cada 60 s
constexpr uint16_t MQTT_KEEPALIVE_S = NO_WOKWI ? 60 : 15;
constexpr uint32_t RECONEXAO_MIN_MS = 2000;           // espera entre tentativas: 2 s...
constexpr uint32_t RECONEXAO_MAX_MS = 30000;          // ...dobrando até 30 s
constexpr uint32_t ESPERA_BROKER_NA_PARTIDA_MS = 15000;

constexpr char CHAVE_PRESCRICAO[] = "prescricao-v1";
constexpr char CHAVE_SESSAO[] = "sessao-v1";
constexpr char SID_SEMENTE_P11[] = "p11-offline-01";

struct SessaoSalva {
  Prescricao prescricao;
  uint8_t concluidas;
  uint8_t confirmadas;
  ResumoSerie resumos[MAX_SERIES];
};

Adafruit_MPU6050 imu;
Caneleira caneleira;
DetectorUso detectorUso;
Biofeedback vibracao;
WiFiClientSecure clienteTls;
PubSubClient mqtt(clienteTls);
Preferences memoria;
bool memoriaDisponivel = false;
bool limpezaPendente = false;
Prescricao ultimaPrescricao{};
bool inicioOfflinePendente = false;
bool simularBrokerIndisponivel = false;
uint32_t inicioEsperaBrokerMs = 0;
bool estadoPendente = false;  // algo mudou: publicar estado na próxima volta
uint32_t ultimoEstadoMs = 0;
uint8_t seriesConfirmadas = 0;  // séries que o app confirmou ter salvo (ack)
GeradorSimulado gerador = {12345u, 0.0f};
uint32_t proximaLeituraUs = 0;
uint32_t amostras = 0;
uint32_t atrasos = 0;
Botao botao;
uint32_t ultimaAtividadeMs = 0;       // P12: base do tempo ocioso
bool inicioOfflineAgora = false;      // o botão pediu a partida offline sem esperar os 15 s
bool liberarSerieNaPartida = false;   // acordou pelo botão no descanso: a série já começa

// Sobrevive ao deep sleep (memória RTC): por que a ESP32 foi dormir.
RTC_DATA_ATTR uint8_t motivoUltimoSono = (uint8_t)MotivoSono::NENHUM;

// Registra atividade: adia o deep sleep por ociosidade e a espera pelo ack.
void marcarAtividade() { ultimaAtividadeMs = millis(); }

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
    estadoPendente = true;
    marcarAtividade();
  }
  return agora;
}

// Configura o PWM do motor (LEDC); a forma de configurar mudou na versão 3 do core.
void iniciarMotor() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_MOTOR, FREQ_PWM_MOTOR_HZ, 8);
#else
  ledcSetup(CANAL_PWM_MOTOR, FREQ_PWM_MOTOR_HZ, 8);
  ledcAttachPin(PIN_MOTOR, CANAL_PWM_MOTOR);
#endif
}

// Liga o motor na intensidade configurada, ou desliga.
void acionarMotor(bool ligado) {
  const uint32_t duty = ligado ? INTENSIDADE_MOTOR : 0;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_MOTOR, duty);
#else
  ledcWrite(CANAL_PWM_MOTOR, duty);
#endif
}

// Aplica no pino o que a fila de vibração pede neste instante, só quando isso muda.
void atualizarMotor() {
  static bool ligadoAntes = false;
  const bool ligado = atualizarBiofeedback(vibracao, millis());
  if (ligado != ligadoAntes) {
    acionarMotor(ligado);
    ligadoAntes = ligado;
  }
}

// Põe o padrão na fila do motor e registra no monitor; o motor toca sem bloquear.
void sinalizar(Padrao padrao, const char* descricao) {
  pedirPadrao(vibracao, padrao);
  Serial.printf("  [vibracao] %s\n", descricao);
}

// Inicia a conexão Wi-Fi sem esperar por ela; o loop() acompanha o resultado.
void iniciarWifi() {
  if (!USAR_REDE) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_SENHA);
  Serial.printf("Wi-Fi: conectando a %s...\n", WIFI_SSID);
}

// Avisa no monitor quando o Wi-Fi conecta ou cai; a própria ESP32 tenta reconectar.
void acompanharWifi() {
  if (!USAR_REDE) return;
  static bool conectadoAntes = false;
  const bool conectado = WiFi.status() == WL_CONNECTED;
  if (conectado == conectadoAntes) return;
  conectadoAntes = conectado;
  if (conectado) {
    Serial.printf("Wi-Fi conectado: IP %s, sinal %d dBm.\n", WiFi.localIP().toString().c_str(),
                  (int)WiFi.RSSI());
  } else {
    Serial.println("Wi-Fi desconectado; a ESP32 tenta reconectar sozinha.");
  }
}

// Nível da bateria em %, pela tensão do divisor (metade da tensão da LiPo).
uint8_t nivelBateria() {
  const float bateriaMv = 2.0f * analogReadMilliVolts(PIN_BATERIA);
  const float fracao = (bateriaMv - BATERIA_VAZIA_MV) / (BATERIA_CHEIA_MV - BATERIA_VAZIA_MV);
  if (fracao <= 0) return 0;
  if (fracao >= 1) return 100;
  return (uint8_t)(fracao * 100 + 0.5f);
}

// Publica o estado (retido): conexão, bateria, sinal, uso e sessão em andamento.
void publicarEstado() {
  const bool temSessao = caneleira.sessao.status != StatusSessao::AGUARDANDO;
  const bool emUso = !USAR_FSR || detectorUso.emUso;
  char json[128];
  snprintf(json, sizeof(json), "{\"online\":true,\"bat\":%u,\"rssi\":%d,\"uso\":%s,\"sid\":%s%s%s}",
           (unsigned)nivelBateria(), (int)WiFi.RSSI(), emUso ? "true" : "false",
           temSessao ? "\"" : "", temSessao ? caneleira.sessao.p.sid : "null", temSessao ? "\"" : "");
  mqtt.publish(TOPICO_ESTADO, json, true);
  ultimoEstadoMs = millis();
  estadoPendente = false;
  Serial.printf("MQTT: estado -> %s\n", json);
}

// Escreve em destino o tópico sessao/<sid> da sessão atual.
void topicoSessao(char* destino, size_t tamanho) {
  snprintf(destino, tamanho, "%s%s", TOPICO_SESSAO_PREFIXO, caneleira.sessao.p.sid);
}

// Grava apenas séries concluídas e o último ACK, em um único registro.
bool salvarResumoPendente() {
  if (!USAR_REDE) return false;
  const Sessao& s = caneleira.sessao;
  SessaoSalva salva{};
  salva.prescricao = s.p;
  salva.concluidas = seriesConcluidas(s);
  salva.confirmadas = seriesConfirmadas;
  for (uint8_t i = 0; i < salva.concluidas; i++) salva.resumos[i] = s.resumos[i];
  if (!memoriaDisponivel || memoria.putBytes(CHAVE_SESSAO, &salva, sizeof(salva)) != sizeof(salva)) {
    Serial.println("NVS: falha ao salvar o resumo.");
    return false;
  }
  return true;
}

// Confere os limites do registro antes de restaurar os índices da sessão.
bool sessaoSalvaValida(const SessaoSalva& salva) {
  if (!prescricaoValida(salva.prescricao) || salva.concluidas == 0 ||
      salva.concluidas > salva.prescricao.series || salva.confirmadas > salva.concluidas) return false;
  for (uint8_t i = 0; i < salva.concluidas; i++) {
    const ResumoSerie& r = salva.resumos[i];
    if (r.n != i + 1 || r.total != salva.prescricao.reps || r.validas > r.total ||
        r.desviosAdm > r.total || r.desviosVel > r.total) return false;
  }
  return true;
}

// Retoma do fim da última série salva. Se houver outra, recomeça o descanso.
void recuperarSessao() {
  if (!USAR_REDE || !memoriaDisponivel || !memoria.isKey(CHAVE_SESSAO)) return;
  SessaoSalva salva{};
  if (memoria.getBytes(CHAVE_SESSAO, &salva, sizeof(salva)) != sizeof(salva) ||
      !sessaoSalvaValida(salva)) {
    Serial.println("NVS: resumo salvo invalido.");
    return;
  }

  Sessao& s = caneleira.sessao;
  iniciarSessao(s, salva.prescricao, caneleira.theta, INICIO_PELO_BOTAO);
  for (uint8_t i = 0; i < salva.concluidas; i++) s.resumos[i] = salva.resumos[i];
  s.serie = salva.concluidas - 1;
  const bool concluida = salva.concluidas == salva.prescricao.series;
  s.status = concluida ? StatusSessao::CONCLUIDA : StatusSessao::DESCANSO;
  s.fimDescanso = salva.prescricao.descanso;
  seriesConfirmadas = salva.confirmadas;
  limpezaPendente = concluida && salva.confirmadas == salva.concluidas;
  Serial.printf("NVS: resumo recuperado: %s, %u serie(s), %u com ACK.\n", s.p.sid,
                (unsigned)salva.concluidas, (unsigned)salva.confirmadas);
}

// Limpa a publicação retida antes de apagar o registro local da sessão confirmada.
void limparResumoConfirmado() {
  char topico[64];
  topicoSessao(topico, sizeof(topico));
  if (!mqtt.publish(topico, (const uint8_t*)"", 0, true)) return;
  if (memoriaDisponivel && memoria.isKey(CHAVE_SESSAO) && !memoria.remove(CHAVE_SESSAO)) {
    Serial.println("NVS: falha ao remover o resumo confirmado.");
    return;
  }
  limpezaPendente = false;
  Serial.println("MQTT: sessao toda confirmada; mensagem retida apagada.");
}

// Publica, retido, o resumo acumulado das séries concluídas em sessao/<sid>. Sem conexão,
// ele sai na próxima reconexão, enquanto o app não confirmar todas as séries.
void publicarResumo() {
  static char json[1024];
  if (montarJsonSessao(caneleira.sessao, json, sizeof(json)) < 0) {
    Serial.println("ERRO: resumo maior que o buffer.");
    return;
  }
  Serial.printf("sessao/%s -> %s\n", caneleira.sessao.p.sid, json);
  if (!mqtt.connected()) {
    Serial.println("MQTT: sem conexao; o resumo sera enviado ao reconectar.");
    return;
  }
  char topico[64];
  topicoSessao(topico, sizeof(topico));
  mqtt.publish(topico, json, true);
}

// Anuncia no monitor a sessão que acabou de começar e pede a publicação do estado.
void anunciarSessao() {
  const Sessao& s = caneleira.sessao;
  Serial.printf("Sessao %s iniciada: %u series x %u reps, ADM min %.0f graus, "
                "vel max %.0f graus/s, descanso %u s.\n",
                s.p.sid, (unsigned)s.p.series, (unsigned)s.p.reps, s.p.admMin, s.p.velMax,
                (unsigned)s.p.descanso);
  Serial.println(s.esperaBotao ? "Serie 1: aperte o botao para comecar." : "Serie 1 iniciada.");
  estadoPendente = true;
  marcarAtividade();
}

// Usa os parâmetros salvos uma vez por partida sem broker, com um SID novo.
void iniciarOfflineSeNecessario(bool emUso) {
  if (!inicioOfflinePendente || !caneleira.calib.pronta || !emUso ||
      (!inicioOfflineAgora && millis() - inicioEsperaBrokerMs < ESPERA_BROKER_NA_PARTIDA_MS)) return;

  Prescricao offline = ultimaPrescricao;
  const unsigned long parteAlta = esp_random();
  const unsigned long parteBaixa = esp_random() & 0xFFFFFFUL;
  snprintf(offline.sid, sizeof(offline.sid), "o%08lx%06lx", parteAlta, parteBaixa);
  inicioOfflinePendente = false;
  seriesConfirmadas = 0;
  Serial.printf("P11: iniciando offline com a prescricao salva %s; novo sid %s.\n",
                ultimaPrescricao.sid, offline.sid);
  receberPrescricao(caneleira, offline);
  if (inicioOfflineAgora) apertarBotao(caneleira.sessao, caneleira.theta);  // o mesmo aperto já libera a série 1
  inicioOfflineAgora = false;
  anunciarSessao();
}

// Trata a prescrição recebida em cmd: valida, ignora repetições e começa a sessão.
void tratarCmd(JsonDocument& doc) {
  const int series = doc["series"] | 0;
  const int reps = doc["reps"] | 0;
  const int descanso = doc["descanso"] | -1;
  Prescricao p{};
  snprintf(p.sid, sizeof(p.sid), "%s", (const char*)(doc["sid"] | ""));
  p.series = (series >= 0 && series <= 255) ? series : 0;
  p.reps = (reps >= 0 && reps <= 255) ? reps : 0;
  p.admMin = doc["adm_min"] | 0.0f;
  p.velMax = doc["vel_max"] | 0.0f;
  p.descanso = (descanso >= 0 && descanso <= 600) ? descanso : 601;
  if (!prescricaoValida(p)) {
    Serial.println("MQTT: cmd ignorado (prescricao invalida).");
    return;
  }

  const StatusSessao st = caneleira.sessao.status;
  if (strcmp(p.sid, caneleira.sessao.p.sid) == 0 ||
      strcmp(p.sid, caneleira.prescricao.sid) == 0) {
    Serial.println("MQTT: cmd repetido da sessao atual; ignorado.");
    return;
  }
  if (st == StatusSessao::EM_EXECUCAO || st == StatusSessao::DESCANSO ||
      (st == StatusSessao::AGUARDANDO && caneleira.prescricao.series > 0)) {
    Serial.println("MQTT: cmd ignorado; ha uma sessao em andamento.");
    return;
  }
  if (limpezaPendente) limparResumoConfirmado();
  if (limpezaPendente || seriesConcluidas(caneleira.sessao) > seriesConfirmadas) {
    Serial.println("MQTT: cmd ignorado; aguarde o ACK da sessao anterior.");
    return;
  }

  seriesConfirmadas = 0;
  if (memoriaDisponivel && memoria.putBytes(CHAVE_PRESCRICAO, &p, sizeof(p)) == sizeof(p)) {
    ultimaPrescricao = p;
    Serial.printf("NVS: prescricao salva: %s.\n", p.sid);
    if (TESTAR_OFFLINE_P11 && strcmp(p.sid, SID_SEMENTE_P11) == 0) {
      Serial.println("P11 teste: reiniciando com o broker simulado indisponivel.");
      Serial.flush();
      ESP.restart();
    }
  } else {
    Serial.println("NVS: falha ao salvar a prescricao.");
  }
  receberPrescricao(caneleira, p);
  Serial.printf("MQTT: prescricao recebida (sessao %s).\n", p.sid);
  if (caneleira.sessao.status == StatusSessao::EM_EXECUCAO) anunciarSessao();
}

// Trata o ack do app: registra as séries salvas e, com a sessão concluída e toda
// confirmada, apaga a mensagem retida da sessão no broker.
void tratarAck(const char* sid, int recebidas) {
  if (strcmp(sid, caneleira.sessao.p.sid) != 0) return;  // ack de outra sessão
  const int concluidas = seriesConcluidas(caneleira.sessao);
  if (recebidas > concluidas) recebidas = concluidas;  // não confirma série que ainda não existe
  if (recebidas > seriesConfirmadas) {
    seriesConfirmadas = recebidas;
    salvarResumoPendente();
    limpezaPendente = caneleira.sessao.status == StatusSessao::CONCLUIDA &&
                      seriesConfirmadas == concluidas;
  }
  Serial.printf("MQTT: o app confirmou %u serie(s) salva(s).\n", (unsigned)seriesConfirmadas);

  if (limpezaPendente) limparResumoConfirmado();
}

// Recebe as mensagens assinadas (cmd e ack) e encaminha cada uma ao seu tratamento.
void aoReceberMensagem(char* topico, byte* payload, unsigned int tamanho) {
  Serial.printf("MQTT: mensagem recebida em %s (%u bytes).\n", topico, tamanho);
  marcarAtividade();
  JsonDocument doc;
  if (deserializeJson(doc, payload, tamanho)) {
    Serial.printf("MQTT: JSON invalido em %s; ignorado.\n", topico);
    return;
  }
  if (strcmp(topico, TOPICO_CMD) == 0) tratarCmd(doc);
  else if (strcmp(topico, TOPICO_ACK) == 0) tratarAck(doc["sid"] | "", doc["series_recebidas"] | 0);
}

// Conecta ao broker por TLS, deixando registrado o aviso de desconexão (last will).
bool conectarBroker() {
  Serial.println("MQTT: conectando ao broker...");
  if (!mqtt.connect(MQTT_CLIENT_ID, MQTT_USUARIO, MQTT_SENHA, TOPICO_ESTADO, 0, true, ESTADO_OFFLINE)) {
    Serial.printf("MQTT: falha na conexao (codigo %d).\n", mqtt.state());
    return false;
  }
  inicioOfflinePendente = false;
  Serial.println("MQTT: conectado.");
  // Reinscreve a cada conexão: não depende de sessão guardada no broker.
  const bool assinouCmd = mqtt.subscribe(TOPICO_CMD, 1);
  const bool assinouAck = mqtt.subscribe(TOPICO_ACK, 1);
  Serial.printf("MQTT: assinaturas cmd %s, ack %s.\n", assinouCmd ? "ok" : "FALHOU",
                assinouAck ? "ok" : "FALHOU");
  publicarEstado();
  if (limpezaPendente) limparResumoConfirmado();
  else if (seriesConcluidas(caneleira.sessao) > seriesConfirmadas) publicarResumo();
  return true;
}

// Mantém a conexão com o broker. A conexão TLS trava a ESP32 por 1 a 2 s, então só é
// tentada fora de uma série, com espera crescente entre tentativas (2 s a 30 s).
void atualizarComunicacao() {
  if (!USAR_REDE || simularBrokerIndisponivel || WiFi.status() != WL_CONNECTED) return;

  static bool estavaConectado = false;
  if (mqtt.connected()) {
    estavaConectado = true;
    mqtt.loop();
    if (estadoPendente || millis() - ultimoEstadoMs >= ESTADO_PERIODO_MS) publicarEstado();
    return;
  }
  if (estavaConectado) {  // -4: sem resposta do broker (keepalive); -3: conexão fechada
    Serial.printf("MQTT: conexao perdida (codigo %d).\n", mqtt.state());
    estavaConectado = false;
  }

  static uint32_t proximaTentativaMs = 0;
  static uint32_t esperaMs = RECONEXAO_MIN_MS;
  if ((int32_t)(millis() - proximaTentativaMs) < 0) return;
  if (caneleira.sessao.status == StatusSessao::EM_EXECUCAO) return;  // não interrompe a série

  if (conectarBroker()) {
    esperaMs = RECONEXAO_MIN_MS;
  } else {
    esperaMs = (esperaMs * 2 > RECONEXAO_MAX_MS) ? RECONEXAO_MAX_MS : esperaMs * 2;
  }
  proximaTentativaMs = millis() + esperaMs;
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
    if (s.status == StatusSessao::EM_EXECUCAO) anunciarSessao();
    else if (s.status == StatusSessao::AGUARDANDO) {
      Serial.println("Aguardando a prescricao do app (topico cmd)...");
    }
  }
  if (ev & EV_REP_SEM_USO) Serial.println("Movimento ignorado: caneleira fora da perna.");

  if (ev & (EV_REP_VALIDA | EV_REP_FORA)) {
    const ResumoSerie& r = s.resumos[s.serie];
    Serial.printf("Serie %u, rep %2u/%u | ADM %5.1f | vel %5.1f graus/s | sobe %.1f s | "
                  "desce %.1f s | %s\n",
                  (unsigned)r.n, (unsigned)r.total, (unsigned)s.p.reps, rep.adm, rep.velPico,
                  rep.tConc, rep.tExc, textoResultado(rep));
    if (rep.valida) sinalizar(Padrao::REP_VALIDA, "1 pulso curto - repeticao valida");
    else sinalizar(Padrao::REP_FORA, "2 pulsos curtos - repeticao fora do padrao");
  }
  if (ev & EV_FIM_SERIE) {
    sinalizar(Padrao::FIM_SERIE, "3 pulsos curtos - fim de serie");
    const bool salvo = salvarResumoPendente();
    if (salvo) Serial.printf("NVS: resumo salvo: %s, serie %u.\n", s.p.sid, (unsigned)(s.serie + 1));
    if (salvo && TESTAR_REINICIO_P11 && !TESTAR_OFFLINE_P11 && s.serie == 0) {
      Serial.println("P11: reiniciando antes de publicar o resumo.");
      Serial.flush();
      ESP.restart();
    }
    publicarResumo();
    if (!(ev & EV_SESSAO_CONCLUIDA)) Serial.printf("Descanso de %u s.\n", (unsigned)s.p.descanso);
  }
  if (ev & EV_FIM_DESCANSO) {
    sinalizar(Padrao::FIM_DESCANSO, "1 pulso longo - fim do descanso");
    if (s.esperaBotao) Serial.printf("Serie %u: aperte o botao para comecar.\n", (unsigned)(s.serie + 1));
    else if (!(ev & EV_SERIE_LIBERADA)) Serial.printf("Serie %u iniciada.\n", (unsigned)(s.serie + 1));
  }
  if (ev & EV_SERIE_LIBERADA) {
    Serial.printf("Serie %u iniciada pelo botao.\n", (unsigned)(s.serie + 1));
    // No descanso, o pulso longo já confirmou o aperto; na espera, confirma com um curto.
    if (!(ev & EV_FIM_DESCANSO)) sinalizar(Padrao::REP_VALIDA, "1 pulso curto - serie liberada");
  }
  if ((ev & EV_FIM_DESCANSO) && liberarSerieNaPartida) {  // acordou pelo botão no descanso
    liberarSerieNaPartida = false;
    const uint8_t liberada = apertarBotao(caneleira.sessao, caneleira.theta);
    if (liberada) tratarEventos(liberada, rep);
  }
  if (ev & EV_SESSAO_CONCLUIDA) {
    Serial.println("Sessao concluida.");
    if (simularBrokerIndisponivel) {
      simularBrokerIndisponivel = false;
      Serial.println("P11 teste: broker disponivel novamente.");
    }
  }
}

// Trata um aperto do botão: começa a série que aguarda, encerra o descanso ou antecipa
// a partida offline com a prescrição salva.
void tratarBotao() {
  marcarAtividade();
  Serial.println("Botao apertado.");
  Sessao& s = caneleira.sessao;
  if (!caneleira.calib.pronta) {
    Serial.println("  Ainda calibrando: mantenha a perna parada e aperte depois.");
    return;
  }
  if (s.status == StatusSessao::AGUARDANDO) {
    if (inicioOfflinePendente) {
      inicioOfflineAgora = true;
      Serial.println("  Partida offline pedida: comeca assim que a caneleira estiver presa.");
    } else {
      Serial.println("  Sem prescricao: aguardando o cmd do app.");
    }
    return;
  }
  Repeticao nenhuma{};
  const uint8_t ev = apertarBotao(s, caneleira.theta);
  if (ev) tratarEventos(ev, nenhuma);
}

// Lê o botão sem bloquear (debounce em botao.h) e trata cada aperto.
void atualizarBotaoFisico() {
  if (atualizarBotao(botao, digitalRead(PIN_BOTAO) == LOW, millis())) tratarBotao();
}

// Prepara os periféricos e entra em deep sleep. Acorda pelo botão e, no descanso, também
// pelo timer. A RAM se perde: a partida seguinte recupera a sessão da NVS (P11).
void dormir(const DecisaoSono& d) {
  const bool noDescanso = d.motivo == MotivoSono::DESCANSO;
  Serial.printf("P12: deep sleep (%s); acorda pelo botao%s.\n", noDescanso ? "descanso" : "ocioso",
                noDescanso ? " ou pelo timer" : "");
  if (noDescanso) Serial.printf("P12: timer de %.1f s.\n", d.dormirMs / 1000.0f);

  // Contrato: publicar offline antes de desligar normalmente.
  if (mqtt.connected()) {
    mqtt.publish(TOPICO_ESTADO, ESTADO_OFFLINE, true);
    mqtt.loop();
    mqtt.disconnect();
  }
  if (USAR_REDE) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }
  if (!USAR_SINAL_SIMULADO) imu.enableSleep(true);  // 3,8 mA -> 5 µA (docs/autonomia.md)

  // Motor desligado e base do transistor presa em LOW durante o sono.
  acionarMotor(false);
  gpio_hold_en((gpio_num_t)PIN_MOTOR);
  gpio_deep_sleep_hold_en();

  // Botão: GPIO 33 é pino RTC; o pull-up precisa continuar ligado durante o sono.
  rtc_gpio_pullup_en((gpio_num_t)PIN_BOTAO);
  rtc_gpio_pulldown_dis((gpio_num_t)PIN_BOTAO);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BOTAO, 0);
  if (noDescanso) esp_sleep_enable_timer_wakeup((uint64_t)d.dormirMs * 1000ULL);

  motivoUltimoSono = (uint8_t)d.motivo;
  Serial.flush();
  esp_deep_sleep_start();
}

// Confere, a cada amostra, se a ESP32 pode dormir agora (regras em energia.h).
void verificarSono() {
  if (!USAR_DEEP_SLEEP) return;
  const Sessao& s = caneleira.sessao;
  const uint32_t ociosoMs = millis() - ultimaAtividadeMs;
  const bool pendente = limpezaPendente || seriesConcluidas(s) > seriesConfirmadas;
  const bool esperandoAck = mqtt.connected() && pendente && ociosoMs < ESPERA_ACK_ANTES_DE_DORMIR_MS;
  const bool ocupada = !caneleira.calib.pronta || vibracao.tamanho > 0 || esperandoAck || botao.estavel;
  const DecisaoSono d = decidirSono(s, caneleira.tempo, ocupada, ociosoMs);
  if (d.motivo != MotivoSono::NENHUM) dormir(d);
}

// Inicializa o monitor serial, o sensor (no modo real) e o processamento.
void setup() {
  Serial.begin(115200);

  // P12: de onde a ESP32 está voltando e liberação dos pinos usados durante o sono.
  const esp_sleep_wakeup_cause_t causa = esp_sleep_get_wakeup_cause();
  const bool acordou = causa == ESP_SLEEP_WAKEUP_TIMER || causa == ESP_SLEEP_WAKEUP_EXT0;
  const MotivoSono dormiuPor = acordou ? (MotivoSono)motivoUltimoSono : MotivoSono::NENHUM;
  motivoUltimoSono = (uint8_t)MotivoSono::NENHUM;
  gpio_hold_dis((gpio_num_t)PIN_MOTOR);
  gpio_deep_sleep_hold_dis();
  rtc_gpio_deinit((gpio_num_t)PIN_BOTAO);
  pinMode(PIN_BOTAO, INPUT_PULLUP);
  iniciarBotao(botao, digitalRead(PIN_BOTAO) == LOW, millis());
  if (acordou) {
    Serial.printf("P12: acordou pelo %s.\n", causa == ESP_SLEEP_WAKEUP_TIMER ? "timer" : "botao");
  }
  memoriaDisponivel = memoria.begin("caneleira", false);
  if (!memoriaDisponivel) {
    Serial.println("NVS: falha ao abrir a memoria.");
  } else if (memoria.isKey(CHAVE_PRESCRICAO)) {
    Prescricao salva{};
    if (memoria.getBytes(CHAVE_PRESCRICAO, &salva, sizeof(salva)) == sizeof(salva) &&
        prescricaoValida(salva)) {
      ultimaPrescricao = salva;
      Serial.printf("NVS: prescricao recuperada: %s, %u series x %u reps.\n",
                    salva.sid, (unsigned)salva.series, (unsigned)salva.reps);
    } else {
      Serial.println("NVS: prescricao salva invalida.");
    }
  }
  iniciarMotor();
  iniciarWifi();
  clienteTls.setCACert(CERTIFICADO_CA_EMQX);
  mqtt.setServer(MQTT_HOST, MQTT_PORTA);
  mqtt.setBufferSize(1024);   // o resumo de três séries passa de 400 bytes
  mqtt.setSocketTimeout(5);   // limita a espera numa conexão que não responde
  mqtt.setKeepAlive(MQTT_KEEPALIVE_S);
  mqtt.setCallback(aoReceberMensagem);

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

  iniciarCaneleira(caneleira, ESPERAR_CMD ? Prescricao{} : PRESCRICAO_TESTE, INICIO_PELO_BOTAO);
  recuperarSessao();
  if (dormiuPor == MotivoSono::DESCANSO && caneleira.sessao.status == StatusSessao::DESCANSO) {
    // O descanso terminou durante o sono (ou o botão o encerrou): a próxima série começa
    // logo depois da calibração.
    caneleira.sessao.fimDescanso = 0;
    liberarSerieNaPartida = causa == ESP_SLEEP_WAKEUP_EXT0;
  }
  marcarAtividade();
  inicioEsperaBrokerMs = millis();
  inicioOfflinePendente = USAR_REDE && caneleira.sessao.status == StatusSessao::AGUARDANDO &&
                          prescricaoValida(ultimaPrescricao);
  simularBrokerIndisponivel = TESTAR_OFFLINE_P11 &&
                              strcmp(ultimaPrescricao.sid, SID_SEMENTE_P11) == 0 &&
                              caneleira.sessao.status != StatusSessao::CONCLUIDA;
  if (simularBrokerIndisponivel) Serial.println("P11 teste: broker simulado indisponivel.");
  Serial.println(USAR_SINAL_SIMULADO ? "Caneleira P12 - modo SIMULADO (repeticoes ficticias)."
                                     : "Caneleira P12 - MPU-6050 real.");
  Serial.println("Calibrando o giroscopio: mantenha a perna parada por 2 s...");
  proximaLeituraUs = micros() + PERIODO_US;
}

// Amostra a 100 Hz sem bloquear e entrega cada leitura ao processamento.
void loop() {
  atualizarMotor();
  atualizarBotaoFisico();
  acompanharWifi();
  atualizarComunicacao();

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
  const bool emUso = usoEfetivo();
  const uint8_t eventos = processarLeitura(caneleira, leitura, DT_S, emUso, rep);
  if (eventos) {
    marcarAtividade();
    tratarEventos(eventos, rep);
  }
  iniciarOfflineSeNecessario(emUso);
  verificarSono();

  amostras++;
  if (IMPRIMIR_ANGULO && amostras % 10 == 0) {
    Serial.printf("theta_acc %.1f | theta %.1f | omega %.1f | atrasos %lu\n", caneleira.thetaAcc,
                  caneleira.theta, caneleira.omega, (unsigned long)atrasos);
  }
}
