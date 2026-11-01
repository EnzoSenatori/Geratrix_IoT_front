# Contrato MQTT — caneleira e Geratrix — g01 · v1.1

**Aprovado por Rafael em 12/10/2026**, responsável pela integração MQTT no Geratrix. Integração e testes somente com dados fictícios. Prefixo: `imrea/g01/caneleira`. MQTT 3.1.1, mensagens UTF-8/JSON, sem nome ou identificação de paciente. O grupo `g01` continua provisório até receber o número oficial.

**Conexão (v1.1):** EMQX Cloud Serverless na conta do Enzo, `z2f0d126.ala.eu-central-1.emqxsl.com`. ESP32: MQTT/TLS, porta `8883`, usuário `caneleira_g01`. Geratrix: WSS `wss://z2f0d126.ala.eu-central-1.emqxsl.com:8084/mqtt`, usuário `geratrix_g01`. Testes manuais (MQTTX e Wokwi): usuário `teste_g01`. Senhas guardadas fora do contrato, sob a guarda do Enzo, e repassadas só por canal privado. Clientes MQTT com IDs distintos. Reconexão automática em 2–30 s; reinscrever após reconectar, sem depender de sessão persistente do broker.

| Tópico completo | Quem publica → quem lê | QoS / retain | Conteúdo e momento |
|---|---|---|---|
| `imrea/g01/caneleira/cmd` | Geratrix → ESP32 | 1 / não | Ao iniciar ou alterar a prescrição: `{"sid":"s7f3a","series":3,"reps":10,"adm_min":60,"vel_max":150,"descanso":60}`. O app reenvia comando pendente após reconexão com o mesmo `sid`; o ESP32 não reinicia uma sessão duplicada. |
| `imrea/g01/caneleira/sessao/<sid>` | ESP32 → Geratrix | 0 / sim | Ao fim de cada série e após reconectar, publica o resumo acumulado das séries da sessão até a confirmação. Exemplo abaixo. Cada sessão tem tópico próprio para não sobrescrever outra pendente. |
| `imrea/g01/caneleira/ack` | Geratrix → ESP32 | 1 / não | Só depois de salvar: `{"sid":"s7f3a","series_recebidas":1}`. O número indica quantas séries foram salvas. Após o `ack` final da sessão concluída, o ESP32 apaga a mensagem retida com publicação vazia no mesmo tópico e então remove a cópia local. |
| `imrea/g01/caneleira/estado` | ESP32 → Geratrix | 0 / sim | Na conexão, quando mudar e a cada 60 s: `{"online":true,"bat":84,"rssi":-60,"uso":true,"sid":"s7f3a"}`. *Last will* retido `{"online":false}`; publicar `offline` também antes de desligar normalmente. |

Exemplo de `sessao/s7f3a` após a primeira série:

```json
{"sid":"s7f3a","status":"em_execucao","series":[{"n":1,"validas":8,"total":10,"adm_media":63.0,"vel_pico":128.0,"desvios":{"adm":2,"vel":0},"t_conc":1.4,"t_exc":1.7}],"ts":1791806400}
```

`sid` é único e anônimo, sem `/` ou `+` no identificador. `status`: `aguardando`, `em_execucao`, `descanso`, `concluida`. ADM em graus; velocidade em graus/s; descanso e tempos das fases concêntrica/excêntrica em segundos; bateria em %, sinal em dBm; `ts` em segundos Unix UTC ou `null` sem hora confiável. O app identifica séries por `sid` + `n`, evita duplicatas, salva antes do `ack` e associa o `sid` ao residente somente dentro do app. O ESP32 guarda resumos pendentes em memória não volátil e continua o exercício e o biofeedback mesmo sem rede.

**Validação P1:** em 12/10/2026, no broker anterior (`z71f6421`, conta do Rafael), `cmd`, `sessao`, `ack` e `estado` fictícios foram publicados e recebidos no MQTTX Web via WSS. A retenção de `estado` foi comprovada; a sessão de teste retida foi apagada. Não houve teste do ESP32, do Geratrix ou do *last will*. Os valores de amplitude e velocidade são exemplos para desenvolvimento, sujeitos a calibração; não são limites clínicos. A credencial MQTT usada diretamente no navegador não é secreta para quem acessa o app; usar somente dados fictícios no protótipo e renovar essa credencial após a demonstração.

## Histórico de versões

| Versão | Data | Alteração |
|---|---|---|
| v1 | 12/10/2026 | Versão aprovada por Rafael: tópicos, payloads, unidades, QoS, *retain* e *last will* |
| v1.1 | 01/11/2026 | Broker migrado para a conta do Enzo (`z2f0d126`) e criado o usuário `teste_g01`. Tópicos, payloads, unidades, QoS e retenção inalterados |
