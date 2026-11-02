# Evidências do P9 — biofeedback

**Data:** 02/11/2026 · **Ambiente:** Wokwi, firmware em modo simulado, LED no GPIO 26 no lugar do motor de vibração · **Responsável:** Enzo

| Arquivo | O que mostra |
|---|---|
| `wokwi-sessao-completa.txt` | Monitor serial da sessão inteira: cada repetição seguida do padrão de vibração pedido, o fim de cada série (`3 pulsos curtos`) e o fim de cada descanso (`1 pulso longo`); três séries com 7 válidas, 2 desvios de ADM e 1 de velocidade, e nenhuma repetição espúria depois dos descansos |

O LED piscou nos quatro padrões, conferido visualmente por Enzo. Não valida a sensação na perna nem a intensidade, que dependem do motor real.
