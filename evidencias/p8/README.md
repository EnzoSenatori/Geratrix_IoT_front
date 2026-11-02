# Evidências do P8 — uso efetivo

**Data:** 02/11/2026 · **Ambiente:** Wokwi, firmware em modo simulado, potenciômetro `fsr` no GPIO 34 no lugar do FSR · **Responsável:** Enzo

| Arquivo | O que mostra |
|---|---|
| `wokwi-uso-efetivo.png` | Monitor serial no início da 3ª série: com o potenciômetro girado para "solta", três movimentos são ignorados (`Movimento ignorado: caneleira fora da perna.`); ao voltar para "presa", a contagem retoma na repetição 1/10 |

Limiares provisórios (presa a partir de 1,2 V, solta abaixo de 0,8 V); não validam o FSR real.
