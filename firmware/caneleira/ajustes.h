#pragma once
// AJUSTES DA CANELEIRA: todos os valores que se ajustam na placa e na apresentação ficam
// aqui, e só aqui. Para mudar o comportamento da caneleira, edite este arquivo e grave de
// novo; os demais arquivos não precisam ser abertos.
//
// Os valores abaixo são os propostos para a apresentação de 27/11/2026: médias que devem
// funcionar sem medição. Cada um diz como conferir em poucos minutos, se houver tempo.
// Ao trocar um valor depois de medir, anote no comentário o valor medido e a data.
//
// Não depende do Arduino: o teste do computador (firmware/testes) também usa este arquivo.

#include <stdint.h>

// ============================================================================
// 1. MODO DE OPERAÇÃO
// ============================================================================

// Leitura: false = MPU-6050 real. true = roteiro de repetições fictícias, sem sensor
// (plano B no Wokwi; nesse caso, NO_WOKWI também vai para true).
constexpr bool USAR_SINAL_SIMULADO = false;
constexpr bool NO_WOKWI = false;  // true só no Wokwi, que roda abaixo do tempo real

// Diagnóstico pelo monitor serial (115200). Deixe os dois em false na apresentação.
constexpr bool MODO_EIXOS = false;       // true: só mostra qual eixo da MPU aponta para cima (seção 2)
constexpr bool IMPRIMIR_ANGULO = false;  // true: ângulo a 10 Hz, no formato do Serial Plotter

constexpr bool USAR_FSR = true;           // false: considera a caneleira sempre presa (plano B do FSR)
constexpr bool INICIO_PELO_BOTAO = true;  // cada série começa quando o botão é apertado

// Deep sleep nos descansos longos e após 5 min ocioso. false na apresentação: muitos power
// banks se desligam sozinhos com o consumo baixo do sono, e a caneleira não acordaria.
// O modo continua implementado e testado em simulação; true depois de testar o power bank.
constexpr bool USAR_DEEP_SLEEP = false;

// Partida sem rede (P11): se o broker não responder em tanto tempo depois de ligar e a
// caneleira estiver presa, ela começa sozinha uma sessão com a última prescrição salva.
// 60 s dá folga ao roteamento do celular; na apresentação, espere "MQTT: conectado" no
// monitor (ou "Conectada" no app) antes de prender a caneleira na perna.
constexpr uint32_t ESPERA_BROKER_NA_PARTIDA_S = 60;

// O GPIO 35 fica sem ligação com o power bank (D15): sem medir, o estado publica "bat":null.
constexpr bool MEDIR_BATERIA = false;

// ============================================================================
// 2. EIXOS DA MPU-6050 (dependem de como a caixa está presa na caneleira)
// ============================================================================
// O acelerômetro mede cerca de +9,8 m/s² no eixo que aponta para CIMA. Informe qual eixo
// aponta para cima com a perna PENDENTE e qual aponta para cima com a perna ESTENDIDA na
// horizontal. O eixo e o sentido do giroscópio são deduzidos desses dois.
//
// Como conferir (2 min): MODO_EIXOS = true, gravar, prender a caneleira na perna, sentar
// e ler no monitor "para cima: ..." com a perna pendente e depois com ela estendida.
// Os dois eixos precisam ser diferentes (X, Y ou Z), cada um com o sinal lido.
//
// Valores propostos: os do projeto no Wokwi (Z para cima pendente, X para cima estendida).

enum class Eixo : uint8_t { X_POS, X_NEG, Y_POS, Y_NEG, Z_POS, Z_NEG };

constexpr Eixo EIXO_PENDENTE = Eixo::Z_POS;
constexpr Eixo EIXO_ESTENDIDO = Eixo::X_POS;
// Correção manual: -1 só se, com os eixos certos, omega sair negativo na subida da perna.
constexpr float SINAL_GIRO = 1.0f;

// ============================================================================
// 3. REPETIÇÃO (ângulos em graus acima do repouso medido antes de cada subida)
// ============================================================================
// Conferir: 5 extensões normais contam; 3 balanços pequenos não contam. Se balanço
// contar, suba THETA_INICIO para 20; se extensão incompleta não contar, desça para 12.
constexpr float THETA_INICIO_GRAUS = 15.0f;       // subiu mais que isto: a repetição começou
constexpr float THETA_FIM_GRAUS = 10.0f;          // voltou a menos que isto: a perna desceu
constexpr float DELTA_PICO_GRAUS = 5.0f;          // queda abaixo do pico que confirma a descida
constexpr float OMEGA_MOVIMENTO_GRAUS_S = 10.0f;  // |ω| acima disto: perna em movimento
constexpr float ALFA_FILTRO = 0.98f;              // peso do giroscópio no filtro complementar

// Cadência de cada repetição (limites técnicos de teste, não clínicos).
// Conferir: deixar a perna cair deve sair "FORA (cad)"; descer normal, "VALIDA".
constexpr float T_DESCIDA_MIN_S = 0.5f;    // descida mais rápida que isto: perna "solta"
constexpr float T_REPETICAO_MAX_S = 8.0f;  // repetição mais longa que isto: travou no meio

// ============================================================================
// 4. SENSOR DE PRESSÃO (FSR 402 em divisor com 10 kΩ, em mV no GPIO 34)
// ============================================================================
// Solto, o FSR fica perto de 0 a 150 mV; sob a tira, costuma ficar entre 500 e 2000 mV.
// Conferir: IMPRIMIR_ANGULO = true mostra "fsr" em mV; USO_LIGA abaixo do valor com a
// caneleira presa e parada; USO_DESLIGA acima do valor com ela solta.
constexpr float USO_LIGA_MV = 600.0f;      // acima disto: caneleira presa
constexpr float USO_DESLIGA_MV = 300.0f;   // abaixo disto por USO_TEMPO_SOLTA_S: solta
constexpr float USO_TEMPO_SOLTA_S = 0.5f;  // folga curta da tira durante a extensão não solta

// ============================================================================
// 5. MOTOR DE VIBRAÇÃO (biofeedback)
// ============================================================================
// Conferir: de olhos fechados, dá para distinguir 1 pulso de 2? Se não, aumente os tempos.
constexpr uint8_t INTENSIDADE_MOTOR = 255;  // 0 a 255 (motor de 3 V alimentado em 3,3 V)
constexpr uint16_t VIB_CURTO_MS = 200;      // pulso curto
constexpr uint16_t VIB_LONGO_MS = 700;      // pulso longo (fim do descanso)
constexpr uint16_t VIB_PAUSA_MS = 200;      // entre pulsos de um mesmo padrão
