# Firmware da caneleira

**Estado em 01/11/2026.** O P3 (leitura da IMU a 100 Hz) foi verificado no Wokwi em 13/10. Os passos P5 a P7 (filtro, contagem e classificação das repetições, resumo da série) estão implementados, verificados em simulação no computador e executados no Wokwi em 01/11/2026, do início ao fim da sessão. O P8 (uso efetivo) e o P9 (biofeedback) foram verificados no Wokwi em 02/11/2026, com um potenciômetro no lugar do FSR e um LED no lugar do motor. Do P10, a conexão Wi-Fi, a conexão segura ao broker, o aviso de desconexão e a publicação de `estado` foram verificados no Wokwi e no MQTTX em 02/11/2026; faltam receber o `cmd` e publicar as séries. Nada foi testado ainda na placa real.

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `caneleira.ino` | Configuração, leitura (simulada ou real), impressão dos eventos e biofeedback provisório |
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

**Resultado esperado.** Em cada uma das três séries: 10 repetições, 7 válidas, 2 fora do padrão por ADM (4ª e 8ª) e 1 por velocidade (9ª, perto de 220 °/s). Ao fim de cada série, uma linha `sessao/sim001 -> {...}` com o resumo no formato do contrato:

```text
Serie 1, rep  9/10 | ADM  69.8 | vel 220.1 graus/s | sobe 0.5 s | desce 0.9 s | FORA (vel)
  [vibracao] 2 pulsos curtos - repeticao fora do padrao
Serie 1, rep 10/10 | ADM  69.0 | vel  77.7 graus/s | sobe 1.3 s | desce 1.5 s | VALIDA
  [vibracao] 1 pulso curto - repeticao valida
  [vibracao] 3 pulsos curtos - fim de serie
sessao/sim001 -> {"sid":"sim001","status":"descanso","series":[{"n":1,"validas":7,"total":10,"adm_media":65.3,"vel_pico":220.1,"desvios":{"adm":2,"vel":1},"t_conc":1.2,"t_exc":1.5}],"ts":null}
```

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
- **Comunicação:** o Wi-Fi conecta sem bloquear. A conexão segura ao broker trava a ESP32 por 1 a 2 s, por isso só é tentada fora de uma série, com espera crescente de 2 s a 30 s entre tentativas. Na conexão, a caneleira registra o aviso de desconexão `{"online":false}`, retido, e publica `estado` na conexão, quando o uso muda e a cada 60 s.
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

O teste percorre o roteiro inteiro e confere o offset removido, o erro do ângulo filtrado (máximo observado: 1,7°) e o resultado de cada série.

## Limites desta versão

- A simulação não reproduz a aceleração linear do movimento, que existirá na placa real e pode exigir outro α.
- A prescrição é fixa (`PRESCRICAO_TESTE`) até o P10 recebê-la pelo tópico `cmd`; no modo simulado, o descanso é de 10 s.
- O `cmd` ainda não é recebido, e o resumo das séries ainda não sai em `sessao/<sid>` (P10, parte 2): a sessão começa sozinha com `PRESCRICAO_TESTE`.
- A vibração foi vista num LED; a distinção dos padrões na perna e a intensidade dependem do motor real.
- O FSR é simulado por um potenciômetro; os limiares de uso ainda não foram medidos.
- O roteiro simulado se repete sem fim, para que repetições descartadas num teste não esgotem o movimento.
- O resumo de três séries tem 412 bytes, acima do buffer padrão de 256 bytes da PubSubClient; o P10 deve ampliá-lo.
- O giroscópio está configurado até 250 °/s; acima disso, a leitura satura.

## Histórico

**P3, 13/10/2026.** Primeiro projeto Wokwi: ESP32 DevKit V1 e MPU-6050 por I²C (SDA no GPIO 21, SCL no GPIO 22, 3,3 V e GND), leitura a 100 Hz. Com `accelX = accelZ = 0,7 g`, o ângulo foi 45,0°; com `rotationY = 60 °/s`, a velocidade foi 60,0 °/s; `dt_ms` ficou entre 9,99 e 10,01 ms. O projeto é público porque o link não listado exige plano pago; não contém credenciais nem dados de pacientes.
