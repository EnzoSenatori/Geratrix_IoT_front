# P13 — Montagem na protoboard

**Situação em 08/11/2026.** Circuito montado pelo Vitor numa protoboard de 400 pontos, sem a ESP32, que ainda não estava com ele: o lugar dela ficou marcado e todos os jumpers que vão aos pinos dela já estão nos furos vizinhos. Falta conferir a montagem por foto, encaixar a ESP32, rodar o teste de bancada e só então gravar o `caneleira.ino`. Nada foi energizado ainda.

A protoboard usada não é a de 170 pontos da lista de materiais (`docs/inventario-materiais.md`, item 10): é uma de 400 pontos, com linhas 1 a 30, colunas a–e e f–j e trilhos de alimentação dos dois lados. Com ela, a ESP32 cabe na própria protoboard e as ligações ficam curtas.

## Esquema elétrico (ligações)

Mesmo mapa de pinos do guia (seção 6.2) e do `firmware/caneleira/caneleira.ino`. Tudo em 3,3 V: **nada vai ao pino VIN (5 V)**.

| Peça | Pino da peça | Liga em | GPIO da ESP32 |
|---|---|---|---|
| MPU-6050 (GY-521) | VCC | 3V3 | — |
| | GND | GND | — |
| | SCL | — | 22 |
| | SDA | — | 21 |
| | XDA, XCL, AD0, INT | sem ligação (AD0 solto = endereço 0x68) | — |
| FSR 402 | uma perna | 3V3 | — |
| | outra perna | resistor de 10 kΩ para o GND | 34 (ponto entre o FSR e o 10 kΩ) |
| Motor 1027, 3 V | fio vermelho | 3V3 | — |
| | fio preto | coletor do BC337 | — |
| Diodo 1N4148 | anodo (sem listra) | coletor do BC337 (fio preto do motor) | — |
| | catodo (listra) | 3V3 (fio vermelho do motor) | — |
| BC337 | base | resistor de 1 kΩ | 26 (pelo 1 kΩ) |
| | emissor | GND | — |
| Botão 12 mm | uma perna | — | 33 (pull-up interno) |
| | perna oposta, na diagonal | GND | — |

O motor é acionado pelo lado do GND (*low-side*): o GPIO 26 liga o BC337 pela base, e o diodo em antiparalelo com o motor absorve o pico de tensão quando ele desliga. O FSR forma um divisor com o 10 kΩ: solto, o GPIO 34 fica perto de 0 V; com a tira apertada, a tensão sobe e passa dos limiares do firmware (presa a partir de 1,2 V, solta abaixo de 0,8 V). O GPIO 35 (bateria) fica sem ligação, porque a alimentação é pelo power bank (D15).

**Conta do motor.** Com a base em 3,3 V pelo 1 kΩ, a corrente de base é de cerca de (3,3 − 0,7) / 1000 ≈ 2,6 mA. Para os até 60 mA do motor, o BC337 precisa de ganho de só ~23, bem abaixo do mínimo dele, então satura com folga. A corrente sai do 3V3 da placa, cujo regulador fornece algumas centenas de mA, suficiente para a ESP32 com Wi-Fi e o motor.

## Posição de cada peça na protoboard

Lembrete: cada linha de 5 furos (a–e ou f–j) é ligada por dentro. Trilhos: do lado esquerdo, + na fileira de fora e − na de dentro; do lado direito, o contrário (+ por dentro, − por fora).

**ESP32 (lugar marcado):** linhas 1 a 15, pinos nas colunas **b** e **i**, USB virado para a linha 1. Conferência ao encaixar: 3V3 em b1 e VIN em i1. Os furos livres a1–a15 e j1–j15 recebem os jumpers. As linhas 16 a 20 ficam vazias, sob a antena. Nunca ligar nada em j1 (VIN, 5 V).

| # | Ligação | De | Para |
|---:|---|---|---|
| 1 | MPU-6050 em pé, na coluna a | VCC a22, GND a23, SCL a24, SDA a25 | XDA a26, XCL a27, AD0 a28, INT a29, sem nada |
| 2 | VCC da MPU | b22 | trilho + esquerdo |
| 3 | GND da MPU | b23 | trilho − esquerdo |
| 4 | SCL | b24 | a14 (GPIO 22) |
| 5 | SDA | b25 | a11 (GPIO 21) |
| 6 | 3V3 da ESP32 | a1 | trilho + esquerdo |
| 7 | GND da ESP32, lado esquerdo | a2 | trilho − esquerdo |
| 8 | GND da ESP32, lado direito | j2 | trilho − direito |
| 9 | Ponte do 3,3 V | trilho + esquerdo | trilho + direito (perto da linha 29) |
| 10 | BC337, face chata para o lado a–e | C h21, B h22, E h23 | — |
| 11 | Diodo 1N4148 | j21 (sem listra) | trilho + direito (listra) |
| 12 | Resistor de 1 kΩ (marrom, preto, vermelho) | f22 | f25 |
| 13 | Sinal do motor | h25 | j7 (GPIO 26) |
| 14 | Emissor no GND | j23 | trilho − direito |
| 15 | Motor, fio vermelho | trilho + direito | — |
| 16 | Motor, fio preto | f21 | — |
| 17 | GND do divisor do FSR | j27 | trilho − direito |
| 18 | Resistor de 10 kΩ (marrom, preto, laranja) | h27 | h30 |
| 19 | FSR, primeira perna (jumper macho-fêmea) | trilho + direito | — |
| 20 | FSR, segunda perna (jumper macho-fêmea) | f30 | — |
| 21 | Sinal do FSR | j30 | j12 (GPIO 34) |
| 22 | Botão, primeira perna (macho-fêmea) | j9 (GPIO 33) | — |
| 23 | Botão, perna oposta na diagonal (macho-fêmea) | trilho − direito | — |

## Antes de ligar a ESP32

1. Confira a montagem contra a tabela acima, de preferência por foto de cima.
2. Sem nada energizado, use o multímetro em continuidade: o trilho + (3,3 V) **não** pode ter continuidade com o trilho −. Se tiver, há curto; não ligue a placa.
3. Encaixe a ESP32 com o USB para a linha 1 e confira que o 3V3 caiu em b1.
4. Grave primeiro o teste de bancada (`firmware/testes/teste_bancada_esp32/`), não o firmware completo. Ele não precisa de biblioteca e diz, peça por peça, o que está funcionando:
   - na partida, `MPU-6050 ok (WHO_AM_I = 0x68)` e três pulsos no motor;
   - a cada 250 ms, o ângulo mudando ao inclinar a MPU, o FSR perto de 0 mV solto e acima de 1200 mV (`PRESA`) apertado;
   - ao apertar o botão, `botao` e um pulso de 300 ms no motor.
5. Anote no monitor serial a tensão do FSR com a tira solta e presa na perna: são os valores para ajustar `USO_LIGA_MV` e `USO_DESLIGA_MV` (P8).
6. Só então grave o `caneleira.ino` com `USAR_SINAL_SIMULADO = false` e siga o P4 (`docs/repasse-rafael.md`, seção 3.3).

## Da protoboard para a caneleira

Na caixa, a protoboard vai inteira, com a ESP32 encaixada. O FSR (na tira, entre a tira e a perna), o motor (encostado na pele) e o botão (do lado de fora da caixa) ficam ligados por jumpers macho-fêmea. A MPU-6050 precisa ficar firme: se ela balançar na protoboard, o ângulo sai errado. Nesse caso, prenda-a com fita dupla face ou passe-a para o fundo da caixa, ligada por jumpers.
