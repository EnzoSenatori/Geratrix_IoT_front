# P2 — Inventário da caneleira — g01

**Situação em 12/10/2026:** fotos enviadas por Rafael mostram uma placa marcada `ESP32 DEVKITV1`, de 30 pinos, com conector Micro-USB e um cabo USB-A ↔ Micro-USB. A marcação do chip USB-serial não está legível nas fotos; confirmar no Gerenciador de Dispositivos após conectar. Os demais itens não estão em mãos. Esta lista organiza o que verificar antes da compra; não registra compras realizadas.

## Para começar na bancada

| Item | Quantidade prevista | Situação |
|---|---:|---|
| ESP32 DevKit V1, 30 pinos | 1 | Em mãos; chip USB-serial ainda não identificado |
| MPU-6050 (GY-521) | 2, incluindo 1 reserva | A confirmar |
| Protoboard e jumpers | 1 conjunto | A confirmar |
| Botão tátil | 1 | A confirmar |
| Cabo USB-A ↔ Micro-USB | 1 | Em mãos; verificar se transmite dados ao conectar a placa |

## Para completar o protótipo

| Item | Quantidade prevista | Situação |
|---|---:|---|
| FSR 402 | 1 | A confirmar |
| Motor de vibração tipo moeda, 3 V | 1 | A confirmar |
| Transistor BC337 ou 2N2222 e diodo 1N4148 | 1 de cada | A confirmar |
| Resistores: 10 kΩ, 1 kΩ e 100 kΩ | 1, 1 e 2 | A confirmar |
| LiPo 3,7 V, cerca de 1000 mAh, com JST | 1 | A confirmar |
| TP4056 com proteção DW01, USB-C | 1 | A confirmar |
| Regulador 3,3 V ME6211 ou AP2112K, ≥500 mA | 1 | A confirmar |
| Capacitores 10 µF e 100 µF; chave liga/desliga | 1 de cada | A confirmar |
| Placa perfurada, barras de pinos, caixa e velcro | 1 conjunto | A confirmar |
| Caneleira ajustável com velcro, 0,5–1 kg | 1 | A confirmar |
| Multímetro com escala de mA, ferro de solda e estanho | 1 conjunto | A confirmar |
| Goniômetro ou transferidor | 1 | A confirmar |

**Critério para encerrar P2:** itens da bancada em mãos e restantes comprados ou com pedido confirmado. Antes de qualquer pedido, confrontar itens já disponíveis, especificações elétricas e fornecedores com o grupo.

**Decisão de compra ainda pendente:** o cabo e o modelo da placa estão identificados; verificar o chip USB-serial no computador antes de instalar driver. A lista do guia prevê um pedido único, mas o circuito de bateria e regulador deve ser conferido contra o modelo real da placa antes da compra. Enquanto isso, o P3 pode começar no Wokwi sem peças físicas.

**Referências técnicas:** [placa DevKitC oficial da Espressif](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html) (exemplo de alimentação e USB, não identificação da placa do grupo); [MPU-6050 no Wokwi](https://docs.wokwi.com/parts/wokwi-mpu6050); [FSR 402 do fabricante](https://www.interlinkelectronics.com/fsr-402).
