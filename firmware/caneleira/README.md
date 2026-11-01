# Firmware da caneleira

**Estado em 01/11/2026.** O P3 (leitura da IMU a 100 Hz) foi verificado no Wokwi em 13/10. Os passos P5 a P7 (filtro, contagem e classificação das repetições, resumo da série) estão implementados, verificados em simulação no computador e executados no Wokwi em 01/11/2026, do início ao fim da sessão. Nada foi testado ainda na placa real.

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `caneleira.ino` | Configuração, leitura (simulada ou real), impressão dos eventos e biofeedback provisório |
| `processamento.h` | Lógica sem dependência do Arduino: calibração do giroscópio, filtro complementar, máquina de estados, classificação, séries e JSON |
| `sinal_simulado.h` | Gerador de repetições fictícias com resultado conhecido |
| `certificado_ca.h` | Certificado raiz público do broker (DigiCert Global Root G2, válido até 2038), para a conexão TLS no P10 |
| `diagram.json`, `libraries.txt` | Circuito e bibliotecas do projeto Wokwi |

## Como rodar no Wokwi

1. Abra o [projeto P3 no Wokwi](https://wokwi.com/projects/477822826125465601). Ele já tem o circuito e as bibliotecas.
2. Substitua todo o conteúdo de `sketch.ino` pelo de `caneleira.ino`.
3. Crie os arquivos `processamento.h` e `sinal_simulado.h` (seta ▾ ao lado das abas → **New file...**) e cole neles o conteúdo dos arquivos desta pasta, com os mesmos nomes.
4. Clique em ▶. As alterações não precisam ser salvas para rodar.

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

## Roteiro simulado (uma série, repetida três vezes)

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
- **Classificação:** válida se ADM ≥ `adm_min` e velocidade de pico ≤ `vel_max`. Uma repetição pode ter os dois desvios. Sem uso efetivo, o movimento não é contado; até o P8, o uso é sempre verdadeiro.
- **Resumo da série (a validar com o Rafael):** `adm_media`, `t_conc` e `t_exc` são médias de todas as repetições; `vel_pico` é o máximo da série; `status` vale `descanso` entre séries e `concluida` ao final; `ts` é `null` até haver hora de rede.

## Constantes a calibrar em bancada

Ficam no topo de `processamento.h`: `ALFA_FILTRO`, `THETA_INICIO_GRAUS`, `DELTA_PICO_GRAUS`, `THETA_FIM_GRAUS`, `OMEGA_MOVIMENTO_GRAUS_S`, `AMOSTRAS_CALIBRACAO` e `SINAL_GIRO`. Os eixos de `anguloAcelerometro()` e o sinal do giroscópio são provisórios e devem ser confirmados no P4, com a placa presa à caneleira.

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
- A vibração é apenas impressa (P9), o FSR ainda não existe (P8) e nada é publicado no broker (P10).
- O resumo de três séries tem 412 bytes, acima do buffer padrão de 256 bytes da PubSubClient; o P10 deve ampliá-lo.
- O giroscópio está configurado até 250 °/s; acima disso, a leitura satura.

## Histórico

**P3, 13/10/2026.** Primeiro projeto Wokwi: ESP32 DevKit V1 e MPU-6050 por I²C (SDA no GPIO 21, SCL no GPIO 22, 3,3 V e GND), leitura a 100 Hz. Com `accelX = accelZ = 0,7 g`, o ângulo foi 45,0°; com `rotationY = 60 °/s`, a velocidade foi 60,0 °/s; `dt_ms` ficou entre 9,99 e 10,01 ms. O projeto é público porque o link não listado exige plano pago; não contém credenciais nem dados de pacientes.
