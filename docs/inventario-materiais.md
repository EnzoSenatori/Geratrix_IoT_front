# P2 — Inventário da caneleira — g01

**Lista de materiais, versão 1 (02/11/2026).** O grupo adotou um protótipo enxuto (decisão D15 do guia): alimentação por power bank, pelo cabo USB da ESP32, e montagem em mini protoboard, sem bateria LiPo nem circuito soldado. O motivo é a falta de tempo de todos para solda e para o circuito de bateria.

Em mãos: ESP32 DevKit V1 de 30 pinos e cabo USB-A ↔ Micro-USB, com o Rafael; power bank e caneleira. Falta confirmar que o cabo transmite dados e qual é o chip USB-serial da placa. Grande parte dos eletrônicos pode vir do laboratório da faculdade; o que faltar se compra numa loja nacional ou na Santa Ifigênia, de olho no prazo da banca (20/11). Esta lista não registra compras realizadas. Em 07/11, o Vitor tinha em mãos as peças da montagem (MPU-6050, FSR, motor, BC337, 1N4148, resistores, botão, protoboard e jumpers) e montou o circuito (`docs/montagem-protoboard.md`).

## Lista, versão 1

| # | Item | Qtd. | Para quê | Situação |
|---:|---|---:|---|---|
| 1 | ESP32 DevKit V1, 30 pinos | 1 | Processamento, decisão e Wi-Fi | Em mãos |
| 2 | Cabo USB-A ↔ Micro-USB de dados | 1 | Programação e alimentação | Em mãos |
| 3 | MPU-6050 (módulo GY-521) | 2 | Sensor de movimento; uma é reserva | A obter |
| 4 | FSR 402 | 1 | Uso efetivo, o segundo sinal exigido | A obter |
| 5 | Micro motor de vibração 1027, 3 V | 2 | Biofeedback; um é reserva | A obter |
| 6 | Transistor NPN BC337 | 2 | Aciona o motor; um é reserva | A obter |
| 7 | Diodo 1N4148 | 2 | Protege o transistor do pico do motor | A obter |
| 8 | Resistores de 1 kΩ e 10 kΩ | 1 de cada | Base do transistor e divisor do FSR | A obter |
| 9 | Push button 12 × 12 mm | 2 | Início da série e despertar; um é reserva | A obter |
| 10 | Mini protoboard de 170 pontos, com adesivo | 1 | Montagem sem solda, dentro da caixa | Substituída por uma de 400 pontos, já montada (08/11) |
| 11 | Jumpers de 20 cm, macho-fêmea e macho-macho | 40 de cada | Ligações; os macho-fêmea levam o FSR e o motor até a caixa | A obter |
| 12 | Power bank de 5000 mAh ou mais, saída USB-A | 1 | Alimentação autônoma pelo cabo USB | Em mãos |
| 13 | Medidor USB de tensão e corrente | 1 | Consumo para o cálculo de autonomia (P12) | A obter |
| 14 | Caneleira de peso, 0,5 a 1 kg, com velcro | 1 | O equipamento | Em mãos |
| 15 | Caixa Patola PB-114 (55 × 97 × 148 mm) ou similar | 1 | Abriga ESP32, protoboard, sensor e power bank | A obter |
| 16 | Fita de velcro dupla face, de amarração | 1 rolo | Prende a caixa à caneleira sem cola | A obter |
| 17 | Fita dupla face de espuma, 12 mm | 1 | Fixa ESP32, sensor e botão na caixa | A obter |
| 18 | Abraçadeiras de nylon 2,5 × 100 mm | 1 pacote | Organizam os fios | A obter |
| 19 | Transferidor de 180° | 1 | Referência de ângulo nos ensaios (P14) | A obter |

## Como montar sem solda

- **A ESP32 fica fora da protoboard.** Com 30 pinos, ela ocupa a mini protoboard inteira. Fica presa no fundo da caixa com dupla face e se liga à protoboard por jumpers macho-fêmea.
- **Na protoboard** ficam o transistor, o diodo, os resistores e as ligações de 3,3 V e GND.
- **A MPU-6050 fica firme no fundo da caixa**, com dupla face, ligada por jumpers. Se ela se mexer, o ângulo sai errado.
- **O FSR fica na tira, entre a tira e a perna, e o motor encostado na pele**, os dois ligados por jumpers macho-fêmea emendados até a caixa.
- **O botão fica do lado de fora da caixa**, preso com dupla face e ligado por jumpers, para poder ser apertado com a caixa fechada.
- **A barra de pinos da MPU-6050 costuma vir solta.** São oito pontos de solda, a única solda do projeto: uns cinco minutos no laboratório. Se alguém encontrar o módulo com os pinos já soldados, nem isso.

## Consequências do protótipo enxuto

1. **Bateria.** A ESP32 não lê a carga do power bank: o GPIO 35 fica sem ligação, e o campo `bat` do `estado` perde o significado. Falta decidir com o Rafael o que esse campo publica, porque ele faz parte do contrato.
2. **Modo de baixo consumo.** Muitos power banks se desligam sozinhos quando o consumo cai, como no *deep sleep*. Teste o modelo escolhido; se ele desligar, registre como limitação no relatório.
3. **Medição de corrente.** O medidor USB mostra os modos ativos (amostrando e transmitindo). O consumo em *deep sleep* fica abaixo da sua resolução e entra no cálculo pelo datasheet.
4. **Contatos.** Jumpers podem soltar com o movimento: use-os curtos dentro da caixa, prenda com abraçadeiras e confira tudo antes de cada ensaio.

## Fora da versão 1

Ficam como referência, caso o grupo volte à alimentação por bateria: LiPo 3,7 V de cerca de 1000 mAh com JST, módulo TP4056 com proteção DW01, regulador 3,3 V ME6211 ou AP2112K (≥ 500 mA) em módulo, capacitores de 10 µF e 100 µF, chave liga/desliga, dois resistores de 100 kΩ para ler a bateria, placa perfurada, barras de pinos, ferro de solda e multímetro com escala de mA. Os detalhes estão na seção 7 do guia de continuidade.

**Critério para encerrar P2:** todos os itens da lista em mãos, e o cabo testado com a placa no computador.

**Referências técnicas:** [placa DevKitC oficial da Espressif](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html) (exemplo de alimentação e USB, não identificação da placa do grupo); [MPU-6050 no Wokwi](https://docs.wokwi.com/parts/wokwi-mpu6050); [FSR 402 do fabricante](https://www.interlinkelectronics.com/fsr-402).
