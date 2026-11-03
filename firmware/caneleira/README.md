# Firmware da caneleira

**Estado em 02/11/2026.** O P3 (leitura da IMU a 100 Hz) foi verificado no Wokwi em 13/10. Os passos P5 a P7 (filtro, contagem e classificação das repetições, resumo da série) estão implementados, verificados em simulação no computador e executados no Wokwi em 01/11/2026, do início ao fim da sessão. O P8 (uso efetivo) e o P9 (biofeedback) foram verificados no Wokwi em 02/11/2026, com um potenciômetro no lugar do FSR e um LED no lugar do motor. O P10 foi verificado no Wokwi e no MQTTX em 02/11/2026: conexão segura ao broker, aviso de desconexão, `estado`, prescrição recebida pelo `cmd`, séries publicadas em `sessao/<sid>` e `ack` que apaga a mensagem retida. Falta testar uma queda de conexão no meio da sessão e guardar prescrição e resumos na memória não volátil, o que segue com o P11. Nada foi testado ainda na placa real.

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `caneleira.ino` | Configuração, leitura (simulada ou real), eventos, motor e comunicação MQTT (`estado`, `cmd`, `sessao/<sid>` e `ack`) |
| `processamento.h` | Lógica sem dependência do Arduino: calibração do giroscópio, filtro complementar, máquina de estados, classificação, séries e JSON |
| `sinal_simulado.h` | Gerador de repetições fictícias com resultado conhecido |
| `biofeedback.h` | Padrões de vibração tocados sem bloquear a amostragem (lógica pura) |
| `certificado_ca.h` | Certificado raiz público do broker (DigiCert Global Root G2, válido até 2038), para a conexão TLS |
| `segredos.exemplo.h` | Modelo das credenciais. Copie como `segredos.h`, que o Git ignora, e preencha as senhas |
| `diagram.json`, `libraries.txt` | Circuito e bibliotecas do projeto Wokwi |

## Circuito no Wokwi

O `diagram.json` traz o circuito completo da caneleira. Cada peça provisória está no pino definitivo do mapa do guia (seção 6.2): na placa real, troca-se a peça, não o código.

| Peça no Wokwi | Representa | Pino | Na placa real |
|---|---|---|---|
| MPU-6050 | Sensor de movimento | GPIO 21 (SDA) e 22 (SCL) | MPU-6050 (GY-521) |
| Potenciômetro `fsr` | Sensor de pressão na tira | GPIO 34 | FSR 402 com divisor de 10 kΩ |
| Potenciômetro `bateria` | Tensão da bateria | GPIO 35 | Divisor de 2 × 100 kΩ ligado à LiPo |
| LED `motor` + resistor de 220 Ω | Motor de vibração | GPIO 26 | Transistor NPN, diodo 1N4148 e motor de 3 V |
| Botão `botao` | Início de série e despertar | GPIO 33 | Botão tátil de 12 mm ligado ao GND |

Os GPIOs 34 e 35 estão no ADC1, o único utilizável com o Wi-Fi ligado; o 33 pode acordar a ESP32 do *deep sleep*.

## Como rodar no Wokwi

1. Abra o [projeto P3 no Wokwi](https://wokwi.com/projects/477822826125465601), use **Save a copy** para ter a sua cópia e substitua o `diagram.json` pelo desta pasta.
2. Substitua todo o conteúdo de `sketch.ino` pelo de `caneleira.ino`.
3. Crie os arquivos `processamento.h`, `sinal_simulado.h`, `biofeedback.h` e `certificado_ca.h` (seta ▾ ao lado das abas → **New file...**) e cole neles o conteúdo dos arquivos desta pasta, com os mesmos nomes. Substitua também o `libraries.txt`.
4. Crie o `segredos.h` com o conteúdo de `segredos.exemplo.h`. Salve o projeto ainda com o marcador no lugar da senha; só então digite a senha do `teste_g01`, rode e **não salve mais**: o projeto do Wokwi é público.
5. Clique em ▶. As alterações não precisam ser salvas para rodar. Com `USAR_REDE = false`, o firmware funciona sem Wi-Fi nem broker.

Com `USAR_SINAL_SIMULADO = true`, o sensor virtual é ignorado e o firmware processa o roteiro fictício. A primeira série termina em cerca de 45 s, e a sessão completa, em menos de 3 min.

Com `USAR_REDE = true`, a sessão só começa quando a prescrição chega pelo `cmd` (seção seguinte). Com `USAR_REDE = false`, começa logo após a calibração, com `PRESCRICAO_TESTE`: sessão `sim001`, 3 séries de 10 repetições.

**Resultado esperado.** Em cada série: 10 repetições, 7 válidas, 2 fora do padrão por ADM e 1 por velocidade (perto de 220 °/s). A posição de cada uma muda de série para série, porque o roteiro segue de onde parou. Ao fim de cada série, uma linha `sessao/<sid> -> {...}` com o resumo no formato do contrato:

```text
Serie 1, rep  9/10 | ADM  69.8 | vel 220.1 graus/s | sobe 0.5 s | desce 0.9 s | FORA (vel)
  [vibracao] 2 pulsos curtos - repeticao fora do padrao
Serie 1, rep 10/10 | ADM  69.0 | vel  77.7 graus/s | sobe 1.3 s | desce 1.5 s | VALIDA
  [vibracao] 1 pulso curto - repeticao valida
  [vibracao] 3 pulsos curtos - fim de serie
sessao/sim001 -> {"sid":"sim001","status":"descanso","series":[{"n":1,"validas":7,"total":10,"adm_media":65.3,"vel_pico":220.1,"desvios":{"adm":2,"vel":1},"t_conc":1.2,"t_exc":1.5}],"ts":null}
```

## Teste com o MQTTX: `cmd`, `sessao` e `ack`

O MQTTX Web faz o papel do app. Conexão: `wss://z2f0d126.ala.eu-central-1.emqxsl.com`, porta 8084, caminho `/mqtt`, usuário `teste_g01`. Assine `imrea/g01/caneleira/#` com QoS 1.

1. Feche outras abas do Wokwi com este projeto: duas caneleiras com o mesmo client ID (`caneleira-01`) derrubam uma à outra. Deixe o Wokwi e o MQTTX lado a lado, visíveis: o navegador desacelera abas em segundo plano.
2. Rode e espere `MQTT: assinaturas cmd ok, ack ok.`
3. Publique em `imrea/g01/caneleira/cmd`, QoS 1, sem *retain*, só o JSON:
   `{"sid":"s7f3a","series":2,"reps":10,"adm_min":60,"vel_max":150,"descanso":10}`
   O monitor mostra `MQTT: prescricao recebida (sessao s7f3a).` e a série 1 começa.
4. Reenvie o mesmo `cmd` durante a série: `MQTT: cmd repetido da sessao atual; ignorado.`
5. No fim da série 1, chega `sessao/s7f3a` no MQTTX. Publique em `imrea/g01/caneleira/ack`: `{"sid":"s7f3a","series_recebidas":1}`.
6. Depois de `Sessao concluida.`, publique `{"sid":"s7f3a","series_recebidas":2}`: o monitor mostra `MQTT: sessao toda confirmada; mensagem retida apagada.`, e o MQTTX recebe uma mensagem vazia em `sessao/s7f3a`.

**Queda de conexão (pendente: primeiro teste do P11).** Com uma sessão em andamento, abra no MQTTX uma segunda conexão com o Client ID `caneleira-01`, o mesmo da ESP32, durante uma série. O broker derruba a ESP32, e o monitor mostra `MQTT: conexao perdida`. Desconecte essa segunda conexão antes do fim da série. Esperado: no fim da série, `MQTT: sem conexao; o resumo sera enviado ao reconectar.`; no descanso, a ESP32 reconecta e o resumo acumulado chega em `sessao/<sid>` na conexão principal do MQTTX. A contagem não pode parar em nenhum momento.

Se o `cmd` não fizer efeito, o monitor diz onde parou: sem `MQTT: mensagem recebida`, a mensagem não chegou à ESP32 (confira se o MQTTX recebe o próprio envio; se não recebe, desconecte e conecte o MQTTX de novo); com ela, mas sem `prescricao recebida`, o conteúdo foi recusado e a linha seguinte diz o motivo; `MQTT: conexao perdida (codigo -4)` indica falta de resposta do broker, e `-3`, conexão fechada, por exemplo por outro cliente com o mesmo ID.

## Roteiro simulado (uma série, repetida sem fim)

| Rep. | Caso | Amplitude | Subida | Resultado esperado |
|---:|---|---:|---:|---|
| 1, 2, 6, 7, 10 | Normal | 66–71° | 1,4–1,5 s | Válida |
| 3 | Tremor de ±1,5° no topo | 72° | 1,4 s | Válida, contada uma vez |
| 4 | Incompleta | 45° | 1,2 s | Fora (ADM) |
| 5 | Pausa de 2 s no topo | 70° | 1,4 s | Válida, contada uma vez |
| 8 | Incompleta | 50° | 1,3 s | Fora (ADM) |
| 9 | Rápida | 70° | 0,5 s | Fora (velocidade) |

O gerador acrescenta ruído ao acelerômetro e ao giroscópio e um offset de 1,7 °/s no giroscópio, que a calibração inicial precisa remover.

## Definições adotadas

- **Ângulo θ:** 0° com a perna pendente e cerca de 90° estendida, pelo filtro complementar `θ = α·(θ + ω·Δt) + (1 − α)·θ_acc`, com α = 0,98.
- **Repetição:** contada quando a perna sobe acima de 15°, desce 5° abaixo do pico e volta abaixo de 10°, no fim da descida. Se o ângulo voltar a passar do pico, a subida é retomada, sem nova contagem.
- **ADM:** pico menos o ângulo de repouso anterior à repetição.
- **Velocidade de pico:** maior velocidade de subida medida pelo giroscópio.
- **Fases:** concêntrica é o tempo com ω acima de +10 °/s; excêntrica, o tempo abaixo de −10 °/s. Pausas no topo ficam fora das duas.
- **Classificação:** válida se ADM ≥ `adm_min` e velocidade de pico ≤ `vel_max`. Uma repetição pode ter os dois desvios. Sem uso efetivo durante toda a repetição, o movimento não é contado.
- **Uso efetivo:** tensão do divisor do FSR no GPIO 34, com histerese: presa a partir de 1,2 V, solta abaixo de 0,8 V; entre os dois, mantém o estado. Valores provisórios. Com `USAR_FSR = false`, a caneleira é considerada sempre presa.
- **Reinício do detector:** depois de um descanso, uma nova repetição só é aceita quando a perna é vista em repouso (abaixo de 10°); assim, a descida em curso no fim do descanso não vira repetição.
- **Biofeedback:** 1 pulso curto para repetição válida, 2 para fora do padrão, 3 no fim da série e 1 longo no fim do descanso; pulsos de 150 ms (longo, 600 ms), 150 ms entre pulsos e 500 ms entre padrões. Os padrões entram numa fila e tocam pelo PWM do GPIO 26, consultando `millis()` a cada volta do `loop()`, sem `delay()`.
- **Comunicação:** o Wi-Fi conecta sem bloquear. A conexão segura ao broker trava a ESP32 por 1 a 2 s, por isso só é tentada fora de uma série, com espera crescente de 2 s a 30 s entre tentativas. Na conexão, a caneleira registra o aviso de desconexão `{"online":false}`, retido, e publica `estado` na conexão, quando o uso muda e a cada 60 s. O buffer da PubSubClient foi ampliado de 256 para 1024 bytes: o maior resumo possível, com 6 séries, tem cerca de 810 bytes.
- **Prescrição (`cmd`):** aceita só se o `sid` tiver de 1 a 15 caracteres, sem `/`, `+` ou `#`, e se séries (1 a 6, o limite de memória `MAX_SERIES`), repetições (1 a 50), `adm_min` (até 120°), `vel_max` (até 500 °/s) e descanso (até 600 s) forem coerentes. São limites técnicos de coerência dos dados, não clínicos. Antes da calibração terminar, a prescrição fica guardada e a sessão começa logo depois dela. O mesmo `sid` reenviado é ignorado, como pede o contrato, e um `cmd` com outro `sid` também é ignorado enquanto houver sessão em andamento.
- **Resumo e confirmação (`sessao/<sid>` e `ack`):** ao fim de cada série, o resumo acumulado é publicado retido. Sem conexão, sai assim que a caneleira reconectar, enquanto o app não confirmar todas as séries. O `ack` nunca confirma mais séries do que as já concluídas. Com a sessão concluída e toda confirmada, uma publicação vazia e retida apaga a mensagem no broker.
- **Wokwi e tempo real:** o simulador roda a cerca de 50% do tempo real, e o broker conta o tempo real. Com `NO_WOKWI = true`, o `estado` sai a cada 20 s e o keepalive sobe para 60 s, para o broker não dar a caneleira por desconectada. Na placa real, `NO_WOKWI = false` volta aos 60 s do contrato e ao keepalive de 15 s.
- **Bateria:** o GPIO 35 lê metade da tensão da LiPo; 3,3 V vira 0% e 4,2 V vira 100%, em aproximação linear provisória.
- **Resumo da série (a validar com o Rafael):** `adm_media`, `t_conc` e `t_exc` são médias de todas as repetições; `vel_pico` é o máximo da série; `status` vale `descanso` entre séries e `concluida` ao final; `ts` é `null` até haver hora de rede.

## Constantes a calibrar em bancada

Ficam no topo de `processamento.h`: `ALFA_FILTRO`, `THETA_INICIO_GRAUS`, `DELTA_PICO_GRAUS`, `THETA_FIM_GRAUS`, `OMEGA_MOVIMENTO_GRAUS_S`, `AMOSTRAS_CALIBRACAO`, `SINAL_GIRO`, `USO_LIGA_MV` e `USO_DESLIGA_MV`; no topo de `biofeedback.h`, as constantes `VIB_...`; no `caneleira.ino`, `INTENSIDADE_MOTOR`. Os eixos de `anguloAcelerometro()` e o sinal do giroscópio são provisórios e devem ser confirmados no P4, com a placa presa à caneleira.

## Teste no computador

A lógica pode ser testada sem Arduino nem Wokwi:

```bash
cd firmware/testes
g++ -std=c++17 -Wall -Wextra -I../caneleira teste_processamento.cpp -o teste && ./teste
```

O teste percorre o roteiro inteiro e confere o offset removido, o erro do ângulo filtrado (máximo observado: 1,7°) e o resultado de cada série. Confere também a histerese do uso, o reinício do detector após o descanso, a validação da prescrição e a espera pelo `cmd` depois da calibração.

## Limites desta versão

- A simulação não reproduz a aceleração linear do movimento, que existirá na placa real e pode exigir outro α.
- Prescrição e resumos ficam só na RAM: um reinício da ESP32 antes do `ack` perde as séries não confirmadas, e, com rede ligada mas sem broker, a sessão não começa. O P11 vai guardar os dois em Preferences e rodar a sessão com a última prescrição salva.
- Uma sessão nova substitui a anterior na memória. Se o app ainda não confirmou a anterior, ela continua retida no broker, mas a ESP32 deixa de reenviá-la.
- A queda de conexão no meio de uma sessão foi verificada só no computador; falta repeti-la no Wokwi (roteiro na seção "Teste com o MQTTX").
- `ts` sai `null`: a ESP32 ainda não obtém a hora da rede (NTP).
- A vibração foi vista num LED; a distinção dos padrões na perna e a intensidade dependem do motor real.
- O FSR é simulado por um potenciômetro; os limiares de uso ainda não foram medidos.
- O roteiro simulado se repete sem fim, para que repetições descartadas num teste não esgotem o movimento.
- O giroscópio está configurado até 250 °/s; acima disso, a leitura satura.

## Histórico

**P3, 13/10/2026.** Primeiro projeto Wokwi: ESP32 DevKit V1 e MPU-6050 por I²C (SDA no GPIO 21, SCL no GPIO 22, 3,3 V e GND), leitura a 100 Hz. Com `accelX = accelZ = 0,7 g`, o ângulo foi 45,0°; com `rotationY = 60 °/s`, a velocidade foi 60,0 °/s; `dt_ms` ficou entre 9,99 e 10,01 ms. O projeto é público porque o link não listado exige plano pago; não contém credenciais nem dados de pacientes.
