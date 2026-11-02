# Evidências do P10 — MQTT na ESP32 (parte 1)

**Data:** 02/11/2026 · **Ambiente:** Wokwi (rede Wokwi-GUEST), broker EMQX Cloud do grupo (`z2f0d126`), MQTTX Web com o usuário `teste_g01` · **Responsável:** Enzo

| Arquivo | O que mostra |
|---|---|
| `wokwi-conexao-estado.txt` | Monitor serial de uma sessão simulada inteira: Wi-Fi conectado durante a série 1; conexão ao broker só no primeiro descanso, pela regra de não interromper a série; uma única conexão até o fim da sessão, sem quedas; `estado` publicado a cada 20 s simulados (`NO_WOKWI`) |

No MQTTX, o tópico `imrea/g01/caneleira/estado` recebeu as mensagens `{"online":true,...}` publicadas pela ESP32 e, após a simulação ser interrompida, o aviso `{"online":false}` publicado pelo próprio broker (*last will*). O campo `bat` saiu 0 porque o potenciômetro de bateria estava no mínimo. Dados fictícios.
