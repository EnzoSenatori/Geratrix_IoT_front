# Evidências da integração com o app (Geratrix em Flutter)

**Data:** 07/11/2026 · **Ambiente:** Wokwi (rede Wokwi-GUEST) com o firmware da `main` (P11), broker EMQX Cloud do grupo (`z2f0d126`) e o app Geratrix no Chrome, com o usuário `geratrix_g01` · **Responsável:** Enzo

O app fez o papel que, nos testes do P10 e do P11, era do MQTTX. Ele enviou a prescrição pelo `cmd`, mostrou o `estado` da caneleira, recebeu as séries em `sessao/<sid>` e confirmou cada uma com o `ack`, sem nenhuma ação manual. O código está no repositório do Geratrix, na branch `es/caneleira-mqtt`: a tela fica em `lib/screens/exercicio_screen.dart`, o serviço MQTT em `lib/services/caneleira_mqtt.dart` e a entrada de teste em `lib/main_caneleira.dart`.

| Arquivo | O que mostra |
|---|---|
| `app-caneleira-desligada.png` | O app conectado ao broker com a caneleira desligada. O card mostra "Desconectada" e "Desligada ou fora da rede", e o botão de envio fica desabilitado. |
| `app-sessao-concluida.png` | O fim da sessão `smw6yvs8l`: a prescrição enviada às 19:20 e as duas séries, cada uma com 7/10 válidas, 2 desvios de ADM e 1 de velocidade. O botão volta a ficar liberado para uma nova sessão. |
| `wokwi-app-sessao-ack.txt` | O monitor serial resumido da mesma sessão. Mostra o `cmd` do app recebido e salvo na memória, as duas séries publicadas e os dois `ack` do app; o último apaga a mensagem retida. |

**Como ler.** No início da sessão, as repetições foram ignoradas porque o potenciômetro do FSR estava no mínimo, ou seja, a caneleira estava "fora da perna" (P8). A contagem começou quando ele foi levantado. Os dois `ack` chegaram dentro do tempo esperado: o da série 1 durante o descanso de 10 s e o da série 2 logo após `Sessao concluida.`

A prescrição de teste é fixa no app: 2 séries × 10 repetições, ADM mínima de 60°, velocidade máxima de 150 °/s e descanso de 10 s. Os dados são fictícios. O campo `bat` sai 0 porque o potenciômetro da bateria estava no mínimo.

**Cobertura do item "Integração com o app".** A telemetria corresponde às séries na tela, o estado ao card da caneleira e os comandos à prescrição pelo `cmd` e às confirmações pelo `ack`. Falta o teste de queda de conexão, com a caneleira e depois o app perdendo a conexão no meio da sessão.
