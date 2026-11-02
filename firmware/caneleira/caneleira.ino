// Caneleira instrumentada — firmware da ESP32 (P3 a P10).
// Lê a IMU a 100 Hz, estima o ângulo da perna, conta e classifica cada repetição e,
// ao fim de cada série, publica o resumo no tópico sessao/<sid> do contrato MQTT.
// Com USAR_SINAL_SIMULADO = true, roda sem sensor, sobre um roteiro de repetições
// fictícias cujo resultado esperado é conhecido (ver sinal_simulado.h).

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

#include "biofeedback.h"
#include "certificado_ca.h"
#include "processamento.h"
#include "segredos.h"
#include "sinal_simulado.h"

// ===== Configuração =====
constexpr bool USAR_SINAL_SIMULADO = true;  // false para usar a MPU-6050 real
constexpr bool IMPRIMIR_ANGULO = false;     // true: ângulos a 10 Hz, útil no P4 e no P5
constexpr bool USAR_REDE = true;            // false: funciona sem Wi-Fi nem broker
constexpr bool ESPERAR_CMD = USAR_REDE;     // com rede, a sessão só começa com a prescrição do app
constexpr bool NO_WOKWI = true;             // o simulador roda abaixo do tempo real (ver ESTADO_PERIODO_MS)
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

Adafruit_MPU6050 imu;
Caneleira caneleira;
DetectorUso detectorUso;
Biofeedback vibracao;
WiFiClientSecure clienteTls;
PubSubClient mqtt(clienteTls);
bool estadoPendente = false;  // algo mudou: publicar estado na próxima volta
uint32_t ultimoEstadoMs = 0;
uint8_t seriesConfirmadas = 0;  // séries que o app confirmou ter salvo (ack)
GeradorSimulado gerador = {12345u, 0.0f};
uint32_t proximaLeituraUs = 0;
uint32_t amostras = 0;
uint32_t atrasos = 0;

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
  Serial.println("Serie 1 iniciada.");
  estadoPendente = true;
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
  if (st != StatusSessao::AGUARDANDO && strcmp(p.sid, caneleira.sessao.p.sid) == 0) {
    Serial.println("MQTT: cmd repetido da sessao atual; ignorado.");
    return;
  }
  if (st == StatusSessao::EM_EXECUCAO || st == StatusSessao::DESCANSO) {
    Serial.println("MQTT: cmd ignorado; ha uma sessao em andamento.");
    return;
  }

  seriesConfirmadas = 0;
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
  if (recebidas > seriesConfirmadas) seriesConfirmadas = recebidas;
  Serial.printf("MQTT: o app confirmou %u serie(s) salva(s).\n", (unsigned)seriesConfirmadas);

  if (caneleira.sessao.status == StatusSessao::CONCLUIDA &&
      seriesConfirmadas >= seriesConcluidas(caneleira.sessao)) {
    char topico[64];
    topicoSessao(topico, sizeof(topico));
    mqtt.publish(topico, (const uint8_t*)"", 0, true);  // publicação vazia apaga a retida
    Serial.println("MQTT: sessao toda confirmada; mensagem retida apagada.");
  }
}

// Recebe as mensagens assinadas (cmd e ack) e encaminha cada uma ao seu tratamento.
void aoReceberMensagem(char* topico, byte* payload, unsigned int tamanho) {
  Serial.printf("MQTT: mensagem recebida em %s (%u bytes).\n", topico, tamanho);
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
  Serial.println("MQTT: conectado.");
  // Reinscreve a cada conexão: não depende de sessão guardada no broker.
  const bool assinouCmd = mqtt.subscribe(TOPICO_CMD, 1);
  const bool assinouAck = mqtt.subscribe(TOPICO_ACK, 1);
  Serial.printf("MQTT: assinaturas cmd %s, ack %s.\n", assinouCmd ? "ok" : "FALHOU",
                assinouAck ? "ok" : "FALHOU");
  publicarEstado();
  if (seriesConcluidas(caneleira.sessao) > seriesConfirmadas) publicarResumo();  // pendente
  return true;
}

// Mantém a conexão com o broker. A conexão TLS trava a ESP32 por 1 a 2 s, então só é
// tentada fora de uma série, com espera crescente entre tentativas (2 s a 30 s).
void atualizarComunicacao() {
  if (!USAR_REDE || WiFi.status() != WL_CONNECTED) return;

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
    else Serial.println("Aguardando a prescricao do app (topico cmd)...");
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
    publicarResumo();
    if (!(ev & EV_SESSAO_CONCLUIDA)) Serial.printf("Descanso de %u s.\n", (unsigned)s.p.descanso);
  }
  if (ev & EV_FIM_DESCANSO) {
    sinalizar(Padrao::FIM_DESCANSO, "1 pulso longo - fim do descanso");
    Serial.printf("Serie %u iniciada.\n", (unsigned)(s.serie + 1));
  }
  if (ev & EV_SESSAO_CONCLUIDA) Serial.println("Sessao concluida.");
}

// Inicializa o monitor serial, o sensor (no modo real) e o processamento.
void setup() {
  Serial.begin(115200);
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

  iniciarCaneleira(caneleira, ESPERAR_CMD ? Prescricao{} : PRESCRICAO_TESTE);
  Serial.println(USAR_SINAL_SIMULADO ? "Caneleira P10 - modo SIMULADO (repeticoes ficticias)."
                                     : "Caneleira P10 - MPU-6050 real.");
  Serial.println("Calibrando o giroscopio: mantenha a perna parada por 2 s...");
  proximaLeituraUs = micros() + PERIODO_US;
}

// Amostra a 100 Hz sem bloquear e entrega cada leitura ao processamento.
void loop() {
  atualizarMotor();
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
  const uint8_t eventos = processarLeitura(caneleira, leitura, DT_S, usoEfetivo(), rep);
  if (eventos) tratarEventos(eventos, rep);

  amostras++;
  if (IMPRIMIR_ANGULO && amostras % 10 == 0) {
    Serial.printf("theta_acc %.1f | theta %.1f | omega %.1f | atrasos %lu\n", caneleira.thetaAcc,
                  caneleira.theta, caneleira.omega, (unsigned long)atrasos);
  }
}
