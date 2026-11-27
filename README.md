# Geratrix IoT — caneleira instrumentada

Lógica IoT da caneleira instrumentada para extensão de joelho, integrada ao aplicativo Geratrix. Projeto de extensão da disciplina Desenvolvimento IoT (Engenharia de Computação, Universidade Presbiteriana Mackenzie, 2026-2), em parceria com o IMREA.

**Grupo:** Enzo Senatori, Rafael de Oliveira Corrêa, Theo Casella Mendonça e Felipe · **Banca:** 27/11/2026

> Protótipo didático. Não é dispositivo médico, não há teste com pacientes e não há dados reais: todos os valores do repositório são fictícios.

**Para continuar o projeto, comece por [`docs/como-continuar.md`](docs/como-continuar.md).**

## Estado atual

O repositório reúne o pacote de continuidade de 13/10/2026 e, desde 01/11, a lógica de contagem e classificação das repetições (P5 a P7), o uso efetivo (P8), o biofeedback (P9) e o MQTT na ESP32 (P10). A especificação completa está em `docs/guia-continuidade.md`.

| Etapa | Situação | Onde foi verificada |
|---|---|---|
| P1 — Contrato MQTT | Concluída | Teste manual no MQTTX, com mensagens digitadas |
| P2 — Compra de componentes | Aberta: lista de materiais versão 1 (02/11), sem solda nem bateria, em `docs/inventario-materiais.md`; em mãos, ESP32, cabo, power bank e caneleira | — |
| P3 — Leitura da IMU a 100 Hz | Concluída | Somente em simulação (Wokwi) |
| P4 — Leitura real, offset e eixos | Eixos configuráveis em `firmware/caneleira/ajustes.h`, com o `MODO_EIXOS` para identificá-los na caneleira (27/11); valores medidos pendentes | Computador (27/11) |
| P5 a P7 — Filtro, contagem e classificação das repetições, resumo da série | Concluídas em simulação | Computador e Wokwi (01/11) |
| P8 — Uso efetivo pelo sensor de pressão | Concluída em simulação | Wokwi, com potenciômetro no lugar do FSR (02/11) |
| P9 — Biofeedback por vibração | Concluída em simulação | Wokwi, com LED no lugar do motor (02/11) |
| P10 — MQTT na ESP32 | Concluída em simulação. `estado`, aviso de desconexão, `cmd`, `sessao/<sid>` e `ack` verificados | Wokwi e MQTTX, no broker novo (02/11) |
| P11 — Operação sem rede | Concluída em simulação. Queda durante a série, recuperação após reinício e início offline com a última prescrição, seguido de sincronização e ACK | Wokwi e MQTTX (06/11), com [evidências](evidencias/p11/) |
| App — integração MQTT (`cmd`, `sessao`, `ack`, `estado`) | Feita no app Flutter do repositório Geratrix (tela de registro de exercício) | Wokwi e app (07/11), com [evidências](evidencias/app/) |
| P12 — Baixo consumo e autonomia | Autonomia estimada pelos datasheets (06/11, `docs/autonomia.md`); botão e *deep sleep* implementados (08/11). Na apresentação, `USAR_DEEP_SLEEP = false`, até testar se o power bank continua ligado com o consumo do sono | Computador (08/11) |
| P13 — Montagem | Protoboard de 400 pontos montada, com a ESP32 ligada por jumpers, conforme `docs/montagem-protoboard.md` e `docs/diagrama-protoboard.png` | Bancada |
| P14 a P16 — Validação, documentação e relatório | Protocolo do P14 declarado (06/11); ensaios não iniciados | — |

Firmware v1.2 (27/11): todos os valores ajustáveis em `firmware/caneleira/ajustes.h`; limiares de repetição relativos ao repouso; uso efetivo com histerese de tempo; cadência (`desvios.cad`); `bat` nulo com o power bank; MPU-6050 lida sem biblioteca. Testado no computador e compilado para a ESP32 (core 3.3.13); não testado na placa.

## Estrutura

```text
docs/
├── guia-continuidade.md     guia vivo: decisões, arquitetura e roteiro P1–P16
├── contrato-mqtt-v1.md      tópicos, payloads, QoS e retenção (aprovado em 12/10)
├── inventario-materiais.md  componentes em mãos e a comprar
├── repasse-rafael.md        repasse de 13/10, com o passo a passo do teste físico (P4)
├── montagem-protoboard.md   esquema, posição de cada peça e roteiro antes de ligar (P13)
└── referencias/             fotos da placa
firmware/
├── caneleira/               código da ESP32 e arquivos do projeto Wokwi
└── testes/                  teste da lógica no computador e teste de bancada da placa
app/                         telas do Geratrix (HTML/CSS), incluindo registro de exercício e histórico
evidencias/
└── p1-mqtt/                 capturas do teste manual do contrato
```

O enunciado da disciplina e a proposta do grupo não são versionados enquanto o repositório for público.

## Como executar

**Firmware no simulador.** Siga `firmware/caneleira/README.md`: abra o [projeto P3 no Wokwi](https://wokwi.com/projects/477822826125465601), troque o código pelo desta pasta e acrescente os dois arquivos `.h`. O projeto público ainda guarda a versão P3.

**Lógica no computador.** Em `firmware/testes/`, `g++ -std=c++17 -I../caneleira teste_processamento.cpp -o teste && ./teste` percorre o roteiro simulado e confere cada série.

**Firmware na placa.** Monte o circuito como em `docs/montagem-protoboard.md` e grave primeiro `firmware/testes/teste_bancada_esp32/`, que confere cada peça. Depois, instale a Arduino IDE, o pacote de placas ESP32 da Espressif e as bibliotecas PubSubClient e ArduinoJson; copie `segredos.exemplo.h` como `segredos.h` e preencha o Wi-Fi e a senha do broker. Abra `firmware/caneleira/caneleira.ino`, grave e acompanhe o monitor serial em 115200 baud. Tudo o que se ajusta na placa (eixos, limiares, FSR, motor e modos) fica em `firmware/caneleira/ajustes.h`.

**Aplicativo.** Abra `app/index.html` no navegador para o fluxo completo, ou `app/checkin-exercicio.html` e `app/historico.html` para as duas telas da caneleira.

## Comunicação

Broker EMQX Cloud Serverless na conta do Enzo (`z2f0d126.ala.eu-central-1.emqxsl.com`; TLS na porta 8883, WebSocket seguro na 8084), prefixo `imrea/g01/caneleira` (`g01` é provisório). A especificação completa está em `docs/contrato-mqtt-v1.md` (v1.1).

| Tópico | Direção | Conteúdo |
|---|---|---|
| `cmd` | Geratrix → caneleira | Prescrição do exercício |
| `sessao/<sid>` | Caneleira → Geratrix | Resumo acumulado das séries |
| `ack` | Geratrix → caneleira | Confirmação das séries salvas |
| `estado` | Caneleira → Geratrix | Conexão, bateria, sinal e uso |

## Regras do repositório

- Senhas do broker e do Wi-Fi nunca entram no Git. No firmware, elas ficarão em `firmware/caneleira/segredos.h`, já ignorado pelo `.gitignore`. Solicite-as ao Enzo por canal privado.
- Tópicos, campos, unidades, QoS e retenção só mudam com nova versão do contrato e aprovação do Rafael.
- Cada avanço é registrado em `docs/guia-continuidade.md`, com data, ambiente (simulação, bancada ou app), resultado e evidência.
- Resultado simulado ou com dados fictícios é sempre identificado como tal.
