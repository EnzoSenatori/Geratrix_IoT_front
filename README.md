# Geratrix IoT — caneleira instrumentada

Lógica IoT da caneleira instrumentada para extensão de joelho, integrada ao aplicativo Geratrix. Projeto de extensão da disciplina Desenvolvimento IoT (Engenharia de Computação, Universidade Presbiteriana Mackenzie, 2026-2), em parceria com o IMREA.

**Grupo:** Enzo Senatori, Rafael de Oliveira Corrêa e Theo Casella Mendonça · **Banca:** 20/11/2026

> Protótipo didático. Não é dispositivo médico, não há teste com pacientes e não há dados reais: todos os valores do repositório são fictícios.

## Estado atual

O repositório reúne o pacote de continuidade de 13/10/2026 e, desde 01/11, a lógica de contagem e classificação das repetições (P5 a P7). A especificação completa está em `docs/guia-continuidade.md`.

| Etapa | Situação | Onde foi verificada |
|---|---|---|
| P1 — Contrato MQTT | Concluída | Teste manual no MQTTX, com mensagens digitadas |
| P2 — Compra de componentes | Aberta: só a ESP32 e o cabo estão em mãos | — |
| P3 — Leitura da IMU a 100 Hz | Concluída | Somente em simulação (Wokwi) |
| P4 — Leitura real, offset e eixos | Não iniciada; aguarda as peças | — |
| P5 a P7 — Filtro, contagem e classificação das repetições, resumo da série | Concluídas em simulação | Computador e Wokwi (01/11) |
| P8 a P11 — FSR, vibração, MQTT na ESP32 e operação sem rede | Não iniciadas | — |
| App — integração MQTT (`cmd`, `sessao`, `ack`, `estado`) | Não iniciada; as telas são HTML/CSS estático, com dados fixos | — |
| P12 a P16 — Autonomia, montagem, validação, documentação e relatório | Não iniciadas | — |

Nada foi testado ainda na placa real nem no aplicativo.

## Estrutura

```text
docs/
├── guia-continuidade.md     guia vivo: decisões, arquitetura e roteiro P1–P16
├── contrato-mqtt-v1.md      tópicos, payloads, QoS e retenção (aprovado em 12/10)
├── inventario-materiais.md  componentes em mãos e a comprar
├── repasse-rafael.md        repasse de 13/10, com o passo a passo do teste físico (P4)
└── referencias/             fotos da placa
firmware/
├── caneleira/               código da ESP32 e arquivos do projeto Wokwi
└── testes/                  teste da lógica no computador
app/                         telas do Geratrix (HTML/CSS), incluindo registro de exercício e histórico
evidencias/
└── p1-mqtt/                 capturas do teste manual do contrato
```

O enunciado da disciplina e a proposta do grupo não são versionados enquanto o repositório for público.

## Como executar

**Firmware no simulador.** Siga `firmware/caneleira/README.md`: abra o [projeto P3 no Wokwi](https://wokwi.com/projects/477822826125465601), troque o código pelo desta pasta e acrescente os dois arquivos `.h`. O projeto público ainda guarda a versão P3.

**Lógica no computador.** Em `firmware/testes/`, `g++ -std=c++17 -I../caneleira teste_processamento.cpp -o teste && ./teste` percorre o roteiro simulado e confere cada série.

**Firmware na placa.** Instale a Arduino IDE, o pacote de placas ESP32 da Espressif e a biblioteca Adafruit MPU6050. Ligue a MPU-6050 à ESP32 (VCC → 3V3, GND → GND, SDA → GPIO 21, SCL → GPIO 22), abra `firmware/caneleira/caneleira.ino`, grave e acompanhe o monitor serial em 115200 baud. O procedimento completo está na seção 3.3 de `docs/repasse-rafael.md`.

**Aplicativo.** Abra `app/index.html` no navegador para o fluxo completo, ou `app/checkin-exercicio.html` e `app/historico.html` para as duas telas da caneleira.

## Comunicação

Broker EMQX Cloud, prefixo `imrea/g01/caneleira` (`g01` é provisório). A especificação completa está em `docs/contrato-mqtt-v1.md`.

| Tópico | Direção | Conteúdo |
|---|---|---|
| `cmd` | Geratrix → caneleira | Prescrição do exercício |
| `sessao/<sid>` | Caneleira → Geratrix | Resumo acumulado das séries |
| `ack` | Geratrix → caneleira | Confirmação das séries salvas |
| `estado` | Caneleira → Geratrix | Conexão, bateria, sinal e uso |

## Regras do repositório

- Senhas do broker e do Wi-Fi nunca entram no Git. No firmware, elas ficarão em `firmware/caneleira/segredos.h`, já ignorado pelo `.gitignore`. Solicite-as ao Rafael por canal privado.
- Tópicos, campos, unidades, QoS e retenção só mudam com nova versão do contrato e aprovação do Rafael.
- Cada avanço é registrado em `docs/guia-continuidade.md`, com data, ambiente (simulação, bancada ou app), resultado e evidência.
- Resultado simulado ou com dados fictícios é sempre identificado como tal.
