# Evidências do P10 — MQTT na ESP32

**Data:** 02/11/2026 · **Ambiente:** Wokwi (rede Wokwi-GUEST), broker EMQX Cloud do grupo (`z2f0d126`), MQTTX Web com o usuário `teste_g01` · **Responsável:** Enzo

| Arquivo | O que mostra |
|---|---|
| `wokwi-conexao-estado.txt` | Parte 1. Monitor serial de uma sessão simulada inteira: Wi-Fi conectado durante a série 1; conexão ao broker só no primeiro descanso, pela regra de não interromper a série; uma única conexão até o fim da sessão, sem quedas; `estado` publicado a cada 20 s simulados (`NO_WOKWI`) |
| `wokwi-cmd-sessao-ack.txt` | Parte 2. Monitor serial: a sessão espera o `cmd` depois da calibração, começa ao recebê-lo e ignora o mesmo `cmd` reenviado no meio da série; cada série sai em `sessao/s7f3a` com 7 válidas, 2 desvios de ADM e 1 de velocidade; o `ack` de 1 série é registrado; o `ack` de 2 séries, com a sessão concluída, apaga a mensagem retida. Uma única conexão do início ao fim |
| `mqttx-cmd-sessao-ack.txt` | Parte 2, vista do MQTTX, que fez o papel do app: `cmd` e `ack` enviados com QoS 1, os resumos recebidos em `sessao/s7f3a` e, às 20:28:41, a publicação vazia que apaga a mensagem retida |

**Parte 1.** No MQTTX, o tópico `imrea/g01/caneleira/estado` recebeu as mensagens `{"online":true,...}` publicadas pela ESP32 e, após a simulação ser interrompida, o aviso `{"online":false}` publicado pelo próprio broker (*last will*).

**Parte 2.** O MQTTX assinava `imrea/g01/caneleira/#` e, por isso, cada `cmd` e cada `ack` aparecem duas vezes no seu registro, com cerca de 10 ms de diferença: o envio e o recebimento da própria mensagem, que confirma que o broker a aceitou. Prescrição de teste com 2 séries e 10 s de descanso, para encurtar a sessão.

Em ambas, o campo `bat` saiu 0 porque o potenciômetro de bateria estava no mínimo, e `ts` sai `null` porque a ESP32 ainda não obtém a hora da rede. Dados fictícios.
