# P12 — Autonomia da caneleira (estimativa pelos datasheets)

**Situação em 06/11/2026.** Estimativa calculada com as correntes dos datasheets e com o power bank que o grupo já tem. Falta confirmar com o medidor USB quando a placa chegar (tabela no fim). O enunciado aceita autonomia "medida ou calculada".

## Fonte de energia

Power bank i2GO PRO Series, modelo PROBAT018 (decisão D15): **20 000 mAh / 74 Wh**, saída USB-A de 5 V e até 3 A. A caneleira se liga a ele pelo cabo micro-USB da ESP32.

A etiqueta dá a capacidade na tensão das células (cerca de 3,7 V), mas a caneleira consome em 5 V. Por isso a conta parte da energia:

- capacidade em 5 V = 74 Wh ÷ 5 V = **14 800 mAh**;
- capacidade utilizável = 14 800 mAh × 0,8 = **11 840 mAh**. O fator 0,8 é o do guia (P12) e cobre as perdas de conversão do power bank e do cabo.

Na placa, o regulador AMS1117 é linear: a corrente que entra em 5 V é praticamente a mesma que sai em 3,3 V, mais a corrente quiescente dele. Assim, as correntes abaixo valem na entrada USB.

## Corrente por estado

| Componente | Ativo (sessão) | Dormindo (*deep sleep*) | Fonte |
|---|---:|---:|---|
| ESP32 com Wi-Fi conectado | 100 mA | — | Datasheet ESP32 v5.3, tab. 5-4: RX 95 a 100 mA. Valor conservador: com o Wi-Fi em *modem sleep*, a média real é menor |
| ESP32 em *deep sleep* | — | 0,01 mA | Datasheet ESP32 v5.3, tab. 4-2: RTC timer + memória RTC, 10 µA |
| Regulador AMS1117 da placa | 5 mA | 5 mA | Corrente quiescente, sempre ligado (guia Hubble) |
| Chip USB-serial da placa | 3,5 mA | 3,5 mA | 2 a 5 mA mesmo sem comunicação (guia Hubble); confirmar se a placa usa CP2102 ou CH340 |
| LED de energia da placa | 3 mA | 3 mA | 2 a 3 mA (guia Hubble) |
| MPU-6050 | 3,8 mA | 0,005 mA | PS-MPU-6000A rev. 3.3, seção 6.3: 3,8 mA em operação, 5 µA em *idle*. O firmware precisa pôr o sensor em *sleep* antes de dormir (P12) |
| LED e regulador do módulo GY-521 | 2 mA | 2 mA | Estimativa: o módulo não desliga o próprio LED |
| FSR e divisor de 10 kΩ | 0,3 mA | 0 | 3,3 V sobre 10 kΩ mais o FSR pressionado; solto, o FSR não conduz |
| Motor de vibração 1027 | 3 mA | 0 | 60 mA no máximo (especificação do 1027) em pulsos de 150 ms a cada repetição, cerca de 5% do tempo |
| **Total** | **120,6 mA** | **13,5 mA** | |

Os picos de transmissão do Wi-Fi (até 240 mA, tab. 5-4) duram milissegundos a cada publicação e não mudam a média.

## Perfil de uso declarado

Duas sessões por dia, de 10 minutos cada: três séries de 10 repetições, os descansos e a espera pela prescrição. No resto do dia, a ESP32 fica em *deep sleep*.

| Estado | Corrente | Tempo por dia | Carga por dia |
|---|---:|---:|---:|
| Ativo | 120,6 mA | 0,33 h | 40,2 mAh |
| Dormindo | 13,5 mA | 23,67 h | 319,9 mAh |
| **Total** | | 24 h | **360,1 mAh** |

I_média = Σ(I_i · t_i) / T = 360,1 mAh ÷ 24 h = **15,0 mA**

## Resultado

| Cenário | Conta | Autonomia |
|---|---|---|
| **A. O power bank fica ligado durante o *deep sleep*** | 11 840 mAh ÷ 15,0 mA | **≈ 789 h, cerca de 33 dias** |
| **B. O power bank se desliga sozinho no *deep sleep*** | 11 840 mAh ÷ 20,1 mAh por sessão | **≈ 589 sessões, cerca de 295 dias** a duas por dia. Mas cada sessão exige religar o power bank no botão dele, e o despertar pelo botão da caneleira não funciona |
| Uso contínuo, como nos ensaios e na banca | 11 840 mAh ÷ 120,6 mA | **≈ 98 h** |

**Leitura.** Com 74 Wh, a autonomia não é problema para o protótipo. O que pesa é outra coisa: no cenário A, 89% da energia do dia vai para os componentes da placa de desenvolvimento (regulador, chip USB-serial e LEDs), que continuam ligados enquanto a ESP32 dorme. Com um módulo ESP32 sem esses componentes, o mesmo uso chegaria a cerca de 290 dias. Não é necessário para o protótipo, mas é a melhoria natural de uma versão de produto e vale citar no relatório.

## Riscos e pendências

1. **Desligamento automático.** Muitos power banks se desligam quando o consumo cai a poucas dezenas de mA, e a etiqueta deste modelo não informa o limiar. Os 13,5 mA do *deep sleep* provavelmente estão abaixo dele. Teste com a placa: pôr a ESP32 em *deep sleep* e ver se o power bank continua ligado. Isso define se vale o cenário A ou o B, e se o despertar pelo botão do P12 funciona com ele.
2. **Peso e tamanho.** Um power bank de 20 000 mAh é grande e pesado. Confira se ele cabe na caixa Patola PB-114 (55 × 97 × 148 mm por fora); se não couber, prenda-o na caneleira com uma segunda tira de velcro. Pese a caneleira montada e declare a massa no relatório, porque ela soma à carga do exercício.
3. **Chip USB-serial.** Confirmar se a placa do grupo usa CP2102 ou CH340; o consumo dos dois é diferente.

## Medição (a preencher no P12, com a placa)

Medidor USB entre o power bank e o cabo da ESP32. Ele mede bem os estados ativos; o *deep sleep* pode ficar abaixo da resolução dele, e nesse caso vale o valor estimado.

| Estado | Estimado | Medido | Observação |
|---|---:|---:|---|
| Ativo, amostrando com Wi-Fi conectado | 120,6 mA | | |
| Transmitindo (pico) | até 240 mA | | |
| *Deep sleep* | 13,5 mA | | O power bank continua ligado? |

## Fontes

- [ESP32 Series Datasheet v5.3, Espressif](https://documentation.espressif.com/esp32_datasheet_en.html), tabelas 4-2 (modos de energia) e 5-4 (consumo do RF).
- [MPU-6000/MPU-6050 Product Specification, PS-MPU-6000A rev. 3.3](https://datasheetq.com//pdf-html/140215/Unspecified/14page/MPU6000.html), seção 6.3.
- [ESP32 Deep Sleep Current: What the Datasheet Says vs What You'll Actually Measure, Hubble](https://hubble.com/community/guides/esp32-deep-sleep-current-what-the-datasheet-says-vs-what-you-ll-actually-measure/): consumo do regulador, do chip USB-serial e do LED em placas de desenvolvimento.
- [Motor de vibração 1027, 3 V, especificação](https://ifuturetech.org/product/1027-3v-flat-mobile-phone-vibration-motor): corrente nominal de até 60 mA.
- Etiqueta do power bank i2GO PRO Series PROBAT018: 20 000 mAh / 74 Wh.
