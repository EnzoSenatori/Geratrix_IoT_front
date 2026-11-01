# Guia de continuidade - Caneleira IoT e Geratrix

**Pacote de repasse ao colega | 13/10/2026 | grupo g01 (provisório)**

> Documento de repasse preparado pelo Rafael em 13/10/2026. Em 01/11/2026 os caminhos foram atualizados para a estrutura deste repositório; o conteúdo técnico não foi alterado.

Este guia permite continuar o projeto em outro computador. Comece pela seção 1, mantenha o contrato MQTT aprovado e registre toda evidência nova no guia vivo (`docs/guia-continuidade.md`). A solução é um **protótipo acadêmico** para extensão de joelho sentado, com a coxa apoiada. **Não há ensaio com pacientes nem uso de dados reais.**

## 1. Leia isto primeiro

1. Clone o repositório (ou baixe-o como ZIP), preservando as pastas.
2. Abra este arquivo. O arquivo `docs/guia-continuidade.md` é a memória completa do projeto; leia-o antes de alterar decisões.
3. Consulte `docs/contrato-mqtt-v1.md` para tópicos, mensagens e regras. Esta é a versão aprovada, em 12/10/2026, por Rafael, responsável técnico pela integração MQTT.
4. Abra `firmware/caneleira/README.md` e o [projeto público no Wokwi](https://wokwi.com/projects/477822826125465601). A pasta contém também `caneleira.ino`, `diagram.json` e `libraries.txt` para reconstruí-lo.
5. Abra `app/README.md` e, no navegador, `index.html` ou `checkin-exercicio.html`. Trata-se de um protótipo **HTML/CSS estático**: ainda não existe integração funcional com MQTT, persistência nem aplicativo instalado.

### Onde está cada material

| Pasta | Conteúdo | Uso |
|---|---|---|
| `docs/` | Guia vivo, inventário de peças e este repasse | Decisões, tarefas P1-P16 e compras pendentes |
| `docs/contrato-mqtt-v1.md` | Contrato MQTT v1 | Implementação e teste de interoperabilidade |
| `firmware/caneleira/` | Projeto Wokwi P3 | Código, circuito virtual e ensaio de IMU |
| `app/` | Telas e estilos originais | Base visual para futura integração |
| `evidencias/p1-mqtt/` | Capturas dos testes P1 | Comprovação do teste manual MQTTX |
| `docs/referencias/` | Fotos da placa | Requisitos acadêmicos e identificação física |

## 2. O que está pronto e o que falta

**P1 - concluído no MQTTX Web:** foram publicados e recebidos exemplos fictícios de comando, sessão, confirmação e estado no EMQX Cloud. Foi verificada a retenção de `estado` e apagada a sessão de ensaio retida. A conexão usou MQTT por WebSocket seguro. Isso prova o contrato e o broker em um teste manual; **não prova o firmware nem o Geratrix**.

**P2 - aberto:** as fotos mostram ESP32 DevKit V1 de 30 pinos e cabo USB-A/Micro-USB. O cabo ainda precisa ser confirmado como cabo de dados; o chip USB-serial, a porta COM e seu driver não foram identificados. MPU-6050, protoboard e jumpers ainda não foram mostrados como disponíveis.

**P3 - concluído no Wokwi:** ESP32 e MPU-6050 virtuais por I2C, com leituras a 100 Hz, ângulo provisório por acelerômetro e velocidade angular. No ensaio, o intervalo de leitura ficou em torno de 9,99-10,01 ms; inclinação simulada de 45 graus e rotação de 60 graus/s foram reproduzidas. **Eixos, offset, precisão e repetição não foram validados no corpo ou na placa real.**

**Próximo marco - P4:** ligar uma MPU-6050 real ao ESP32, confirmar a leitura serial, medir o offset do giroscópio e identificar os eixos do movimento. Enquanto faltarem peças, avance somente em simulação ou nas telas com dados fictícios, marcando cada resultado como simulado.

## 3. Primeiro trabalho no computador do colega

### 3.1 Conferir a placa que Rafael tem

1. Com a ESP32 sem nada conectado aos pinos, ligue o cabo Micro-USB a uma porta USB do computador.
2. No Windows, abra **Gerenciador de Dispositivos > Portas (COM e LPT)**. Anote a porta que aparece ao conectar e desaparece ao retirar a placa. Veja também se há dispositivo com alerta em **Outros dispositivos**.
3. Se a porta não aparecer, experimente um cabo Micro-USB sabidamente de dados e outra porta USB. Identifique o chip USB-serial por inscrição legível na placa ou nas propriedades do dispositivo antes de instalar um driver compatível.
4. Registre: porta COM, chip, cabo testado, estado do LED e foto legível. Este passo ainda não equivale a teste da MPU-6050.

### 3.2 Reabrir a simulação P3

1. Acesse o link Wokwi da seção 1. Se o projeto público não abrir, crie um projeto **ESP32** em [wokwi.com/projects/new/esp32](https://wokwi.com/projects/new/esp32) e substitua `diagram.json` e `libraries.txt` pelos da pasta `firmware/caneleira/` e cole o conteúdo de `caneleira.ino` no `sketch.ino` do Wokwi.
2. Inicie a simulação. No monitor serial, procure `P3 iniciado` e a coluna `dt_ms` perto de `10`.
3. Na MPU-6050 virtual, ajuste `accelX=0.707` e `accelZ=0.707` (g). O campo `angulo_acc_graus` deve ficar perto de 45. Ajuste `rotationY=60` (graus/s); `vel_y_graus_s` deve ficar perto de 60.
4. Guarde uma captura da tela e registre data, valores e eventuais erros. Essa reprodução confirma só a simulação.

### 3.3 Preparar o teste físico P4 quando as peças chegarem

Peças mínimas: **MPU-6050 (módulo GY-521), protoboard, quatro jumpers, ESP32 e cabo USB de dados**. Não use a bateria nesta etapa; alimente a placa pela USB do computador.

1. **Desconecte a USB antes de montar.** Faça as quatro ligações: `MPU VCC -> ESP32 3V3`; `MPU GND -> ESP32 GND`; `MPU SDA -> ESP32 GPIO 21`; `MPU SCL -> ESP32 GPIO 22`. Confira na serigrafia da placa real. Não ligue VCC ao pino de 5 V.
2. Instale a Arduino IDE e o pacote de placas ESP32 da Espressif. Na IDE, selecione um perfil compatível com a DevKit V1 e a porta COM identificada. Instale a biblioteca **Adafruit MPU6050** e suas dependências pelo gerenciador de bibliotecas.
3. Abra `firmware/caneleira/caneleira.ino`, compile e envie para a ESP32. Se a gravação não iniciar, confira a porta e o cabo; em algumas DevKit V1 é preciso manter **BOOT** pressionado no início da gravação e soltá-lo quando ela começar.
4. Abra o monitor serial em **115200 baud**. O esperado é `P3 iniciado` e linhas `amostra,dt_ms,angulo_acc_graus,vel_y_graus_s,atrasos`. Se aparecer `MPU-6050 nao encontrado`, desligue a USB e revise os quatro fios, principalmente 3V3/GND e SDA/SCL.
5. Com o sensor parado por 10-20 segundos, anote o valor médio da velocidade angular no eixo usado; isto será o **offset** a subtrair no código. Depois mova o sensor lentamente em um plano, identifique o eixo e o sinal correspondentes à extensão do joelho e compare posições com transferidor/goniômetro.
6. Registre duração, média e faixa de `dt_ms`, atrasos, offset, orientação e foto das ligações. Só então avance para filtro complementar (P5). O código atual imprime leituras provisórias e **ainda não calcula offset nem faz fusão**.

Se não houver MPU-6050, encerre a etapa no registro da pendência; não marque P4 como concluído.

## 4. Comunicação combinada com o Geratrix

Prefixo MQTT: `imrea/g01/caneleira`. `g01` é provisório. Broker EMQX Cloud Serverless: `z71f6421.ala.eu-central-1.emqxsl.com`. Para ESP32, conexão MQTT/TLS na porta `8883`, usuário `caneleira_g01`. Para o protótipo Web, `wss://z71f6421.ala.eu-central-1.emqxsl.com:8084/mqtt`, usuário `geratrix_g01`. **As senhas não acompanham este pacote**; solicite-as a Rafael por canal privado, quando necessário. Cada cliente deve ter um ID diferente.

> **Atualização de 01/11/2026:** o broker passou para a conta do Enzo (`z2f0d126.ala.eu-central-1.emqxsl.com`), e as senhas agora ficam com ele. A conexão vigente está em `docs/contrato-mqtt-v1.md` (v1.1).

| Tópico | Direção | Significado |
|---|---|---|
| `imrea/g01/caneleira/cmd` | Geratrix -> caneleira | Prescrição do exercício; QoS 1; sem retenção |
| `imrea/g01/caneleira/sessao/<sid>` | Caneleira -> Geratrix | Resumo acumulado de cada sessão; QoS 0; retido até confirmação final |
| `imrea/g01/caneleira/ack` | Geratrix -> caneleira | Quantas séries foram **salvas**; QoS 1; sem retenção |
| `imrea/g01/caneleira/estado` | Caneleira -> Geratrix | Online, bateria, sinal, uso, sessão ativa; QoS 0; retido |

Fluxo: o Geratrix envia `cmd` com um `sid` anônimo; a caneleira executa localmente e publica `sessao/<sid>`; o Geratrix salva sem duplicar séries (`sid` + `n`) e só então publica `ack`; a caneleira limpa a mensagem retida da sessão depois do `ack` final. A caneleira publica `estado` ao conectar, em mudanças e a cada 60 s; sua mensagem de última vontade é `{"online":false}`. `estado` é informado pela **caneleira**, e o app o exibe. Detalhes de JSON, unidades, reconexão e persistência estão no contrato v1. Os valores do teste são exemplos técnicos, não limites clínicos.

## 5. Ordem sugerida depois de P4

1. **P5 - filtro complementar:** combinar acelerômetro e giroscópio; medir erro e deriva em posições conhecidas.
2. **P6-P7 - repetições e métricas:** máquina de estados com histerese, ADM, velocidade, tempos de fase e desvios. Provar primeiro em bancada e em dados simulados.
3. **P8-P9 - uso e vibração:** adicionar FSR e biofeedback; verificar cada resposta com limites configuráveis pelo app.
4. **P10-P11 - MQTT no ESP32:** aplicar o contrato v1, reconectar, guardar resultados pendentes e continuar a decisão local sem rede/celular.
5. **Geratrix:** manter o layout das telas, implementar conexão e persistência com dados fictícios, mostrar estado, enviar prescrição e confirmar somente o que foi salvo. O ZIP fornecido é apenas a base visual.
6. **P12-P16 - autonomia, montagem, validação e entrega:** concluir alimentação de bateria apenas após revisão elétrica; realizar medidas de bancada e relatório acadêmico. Não planejar demonstração com paciente.

## 6. Regras de continuidade

- Registre cada avanço com data, ambiente (**simulação**, **bancada** ou **app real**), procedimento, resultado, captura/foto e arquivo alterado. Atualize `docs/guia-continuidade.md` quando uma pendência mudar.
- Não confunda a tela estática do Geratrix com integração implementada. Não confunda mensagens digitadas no MQTTX com dados medidos pela ESP32.
- Não publique senhas, Wi-Fi, tokens nem identificadores de pacientes em Wokwi, Git, capturas ou neste repositório. O projeto Wokwi é público.
- Não altere tópicos, campos, unidades, QoS ou política de retenção sem versionar o contrato e alinhar com Rafael e o grupo. Rafael dá a aprovação técnica final da integração MQTT.
- Antes de P14, o grupo deve fixar erro aceitável e número de medições/repetições. A proposta de ±5 graus é provisória. Não use os valores fictícios de `adm_min` e `vel_max` como prescrição clínica.

## 7. Mensagem para retomar com outra IA

> Estou continuando o projeto acadêmico de caneleira IoT do grupo g01. Leia integralmente `docs/guia-continuidade.md`, este guia de repasse (`docs/repasse-rafael.md`), `docs/contrato-mqtt-v1.md` e `firmware/caneleira/README.md`. P1 foi testado manualmente no MQTTX; P3 foi testado no Wokwi; P2 e P4 estão pendentes; o Geratrix fornecido é estático. Use dados fictícios e não peça nem exponha senhas. Confirme o estado e conduza a próxima etapa com instruções verificáveis, mantendo o guia vivo atualizado. Diga claramente o que foi demonstrado e o que continua pendente.

**Ponto de retomada recomendado:** peça ao colega o resultado da seção 3.1 (porta COM/cabo/driver) e se o kit MPU-6050 + protoboard + jumpers já chegou. Com isso, execute P4. Se o kit não chegou, reproduza P3 e avance apenas com um protótipo fictício do Geratrix.
