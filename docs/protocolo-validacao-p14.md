# P14 — Protocolo de validação da caneleira

**Declarado em 06/11/2026, antes de qualquer ensaio.** O enunciado pede que o protocolo diga o que é medido, contra qual referência, quantas repetições e com qual erro aceitável, definidos antes dos testes. A data do commit deste arquivo no GitHub registra essa precedência. Mudanças posteriores entram como nova versão, com data e motivo, sem apagar a anterior.

O grupo optou por uma validação enxuta, centrada nos dois itens que o enunciado exige (exatidão do ângulo e da contagem de repetições), mais verificações de funcionamento feitas uma vez cada. Duração prevista: cerca de 40 minutos.

## Regras de ética e segurança

- **Só integrantes do grupo**, nunca pacientes. Nos registros, cada participante aparece por um código (P1, P2, P3), sem nome.
- **Nada ligado à tomada junto ao corpo.** Nos ensaios com pessoa, a caneleira é alimentada pelo power bank, e o notebook que lê o monitor serial fica **fora da tomada**, só na bateria.
- Caneleira de 0,5 a 1 kg. O participante interrompe o ensaio a qualquer desconforto.

## Montagem

- Firmware da `main` com `USAR_SINAL_SIMULADO = false`. No ensaio de ângulo, `IMPRIMIR_ANGULO = true`, para ler `theta` no monitor serial; na contagem, `false`.
- Convenção: **0° com a perna pendente (vertical)** e **90° com a perna estendida na horizontal**, a mesma do firmware depois do ajuste de eixos do P4.
- Prescrição da contagem, enviada pelo MQTTX: `{"sid":"v14-01","series":1,"reps":10,"adm_min":60,"vel_max":150,"descanso":60}`. Trocar o `sid` a cada participante (`v14-02`, `v14-03`), para não ser ignorado como repetido.

## Ensaios

| Ensaio | O que é medido | Referência | Repetições | Erro aceitável |
|---|---|---|---|---|
| **E1 — Ângulo em bancada** | `theta` parado em 0°, 30°, 60° e 90° | Transferidor (resolução de 1°; alinhamento de cerca de ±2°) | 5 leituras por ângulo (20) | Cada leitura dentro de **±5°** |
| **E2 — Contagem de repetições** | Repetições contadas pela caneleira | Contagem manual de um observador | 3 participantes × 10 repetições (30) | Pelo menos **90%** de acerto (até 3 erros em 30) |

**E1.** Prender a caneleira numa tábua ou régua larga que gire no plano vertical, com o transferidor alinhado no eixo de giro. Para cada ângulo, posicionar, esperar 3 s parado e anotar o `theta`. Tirar e recolocar a tábua na posição antes de cada uma das 5 leituras.

**E2.** Participante sentado, coxa apoiada, perna pendente, fazendo uma série de 10 extensões de joelho, voltando a perna até embaixo a cada repetição. O observador conta em voz alta sem olhar o monitor; outro integrante anota a contagem da caneleira ao fim da série.

## Verificações de funcionamento

Feitas uma vez cada, na mesma sessão. Resultado: passou ou não passou.

| Verificação | Como fazer | Passa quando |
|---|---|---|
| Uso efetivo | Balançar a caneleira na mão 10 vezes, como numa extensão de joelho | Nenhuma repetição é contada |
| Biofeedback | Durante a série do E2, o participante diz, de olhos fechados, se sentiu 1 ou 2 pulsos | Os padrões sentidos batem com o serial |
| Queda de conexão | Desligar a rede (roteador ou roteamento do celular) no meio de uma série e religar no descanso | O resumo chega ao MQTTX com todas as séries feitas |
| Aquecimento | Tocar o motor e a caixa depois de 10 minutos de uso | Nenhum aquecimento perceptível; se houver, os ensaios com pessoa param |

## Registro

| Ensaio | Participante | Referência | Caneleira | Erro ou acerto |
|---|---|---|---|---|
| E1 | bancada | 30° | | |
| E2 | P1 | 10 repetições | | |

No E1, informar para cada ângulo o erro médio e o maior erro. Prints do serial e do MQTTX e fotos da montagem (sem rosto) vão para `evidencias/p14/`.

## Critério geral

Cada ensaio é aprovado ou reprovado pelo seu erro aceitável. Um ensaio reprovado não é repetido até passar: registra-se o resultado e a causa provável (fixação, eixos, limites) e, se houver ajuste, o ensaio é refeito e as duas rodadas entram no relatório.
