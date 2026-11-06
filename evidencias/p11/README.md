# Evidências do P11 — Operação sem rede

Testes feitos por Rafael em 06/11/2026 com dados fictícios no Wokwi, broker EMQX do grupo e MQTTX Web no papel do aplicativo.

| Cenário | Resultado observado | Evidência |
| --- | --- | --- |
| Queda de MQTT durante a série | O MQTT caiu após a repetição 2/10. A caneleira contou até 10/10 e entregou o resumo da série 1 ao MQTTX após reconectar no descanso. | [Serial](queda-reconexao-serial.png) · [MQTTX](queda-reconexao-mqttx.png) |
| Prescrição após reinício | A ESP32 gravou a prescrição `p11-nvs-01` e recuperou da memória a mesma configuração de duas séries de 10 repetições antes de iniciar o Wi-Fi. | [Serial](prescricao-reinicio-serial.png) |
| Resumo pendente após reinício | A ESP32 reiniciou antes de publicar a série 1. Depois, recuperou o resumo com zero séries confirmadas, entregou ao MQTTX e retomou na série 2. | [Serial](resumo-reinicio-serial.png) · [MQTTX](resumo-reinicio-mqttx.jpg) |
| Partida sem acesso ao broker | A ESP32 carregou `p11-offline-01` da memória, executou duas séries de 5 repetições com um novo `sid` e guardou os resultados enquanto a comunicação MQTT estava indisponível. | [Serial após o reinício](offline-partida-serial.png) · [Conclusão no serial](offline-ack-serial.png) |
| Sincronização e ACK após sessão offline | O MQTTX recebeu a sessão `o5ba5e14db45c4f` como concluída. A ESP32 recebeu o ACK das duas séries e registrou a limpeza da mensagem retida. | [MQTTX](offline-sincronizacao-mqttx.jpeg) · [ACK no serial](offline-ack-serial.png) |

## Como reproduzir

Use o [roteiro de conexão do firmware](../../firmware/caneleira/README.md#teste-com-o-mqttx-cmd-sessao-e-ack). Publique `cmd` e `ack` com QoS 1 e Retain desligado.

1. **Queda durante a série.** Siga o roteiro de queda do firmware com os dois clientes MQTTX. Compare a contagem do serial com a série recebida depois da reconexão.
2. **Reinício com resumo pendente.** Ative apenas `TESTAR_REINICIO_P11`. Envie uma prescrição com duas séries. A ESP32 reinicia após gravar a primeira série e antes de publicá-la. Confira as linhas de recuperação no serial e a série recebida no MQTTX antes da segunda.
3. **Partida offline.** Ative apenas `TESTAR_OFFLINE_P11` e publique no tópico `imrea/g01/caneleira/cmd`:

   ```json
   {"sid":"p11-offline-01","series":2,"reps":5,"adm_min":60,"vel_max":150,"descanso":5}
   ```

   Após a gravação, o firmware reinicia e simula o broker indisponível até a conclusão. Confira o início offline, a reconexão e as duas séries publicadas sob o novo `sid`. Envie o ACK em `imrea/g01/caneleira/ack` com esse novo `sid` e `"series_recebidas":2`.

Ao terminar, deixe as duas flags de teste em `false` e restaure o marcador da senha em `segredos.h` antes de salvar o projeto Wokwi.

A simulação de indisponibilidade no terceiro cenário suspende a conexão MQTT do firmware. A validação com interrupção real de rede na placa física e com o app integrado segue pendente.
