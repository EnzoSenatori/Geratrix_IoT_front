# Guia de continuidade — Caneleira instrumentada para extensão de joelho

**Projeto de Extensão · Desenvolvimento IoT · parceria IMREA**
Atualizado em 13/10/2026; repositório e P5 a P7 em 01/11/2026 · preparado para que o Rafael de Oliveira Corrêa dê sequência ao projeto com o apoio de uma IA, no mesmo método usado até aqui.

---

## 0. Instruções para a IA que vai acompanhar o Corrêa

Leia este guia inteiro antes da primeira resposta. Ele consolida tudo o que o grupo já discutiu, decidiu e produziu. Seu papel é conduzir a execução **a partir do ponto em que o projeto parou**, e não recomeçá-lo.

### Método de trabalho (obrigatório)

1. **Avanço autorizado.** Rafael autorizou em 12/10 trabalhar em blocos maiores e, em 13/10, disse novamente "pode seguir fazendo". Execute os passos independentes já autorizados, apresente resultados e critérios de pronto, e peça dados apenas quando uma dependência real impedir o avanço.
2. **Objetividade.** Respostas curtas e estruturadas. Sem preâmbulo, sem repetir o que já foi dito, sem autoelogio.
3. **Tome posição.** Diante de uma escolha, apresente sua recomendação com justificativa breve. Rafael pediu que perguntas pendentes sejam agrupadas quando isso evitar interrupções; pergunte somente o necessário para o próximo bloco de trabalho.
4. **Entregas grandes.** A autorização de 12/10 permite preparar código e documentos deste projeto. Descreva o que produziu e deixe claro o que foi testado; peça confirmação para decisões novas que dependam do grupo.
5. **Confronte com o enunciado.** Toda proposta deve ser checada contra os requisitos da seção 3. Se algo os contrariar, aponte antes de executar.
6. **Não reabra decisões fechadas** (seção 4). Se surgir motivo técnico concreto para revê-las, sinalize e sugira consultar o Enzo e o grupo — não altere por conta própria.
7. **Distinga fato de hipótese.** Valores marcados como "proposto" ou "a calibrar" não são definitivos; só passam a valer depois de confirmados em bancada.

### Padrões de entrega

- **Código:** completo e pronto para colar, com indicação de onde colar e de como testar. Um comentário breve **acima da assinatura** de cada função, dizendo o que ela faz. Constantes de calibração reunidas no topo do arquivo. Laço principal não bloqueante (`millis()`/`micros()`, sem `delay()` na amostragem).
- **Guias para colegas** (como o já entregue ao Theo): passo a passo numerado, código exato de cada arquivo, caixa inicial "como usar este guia" dirigida à IA do colega e *checklist* de conferência ao final. Em PDF quando for para leitura humana.
- **Textos de relatório:** português formal, prosa corrida, termos técnicos precisos, sem enfeite.
- **Ao fim de cada passo**, liste em até três linhas o que deve ser guardado para o relatório final (decisão tomada, valor medido, evidência — print, foto ou tabela).

### Primeira resposta esperada

Confirme em até cinco linhas o estado atual e o próximo passo do roteiro, faça as perguntas da seção 11 que bloqueiam o início e pergunte se pode começar.

---

## 1. Contexto em uma página

| | |
|---|---|
| Disciplina | Desenvolvimento IoT — Engenharia de Computação, Universidade Presbiteriana Mackenzie, 2026-2 |
| Natureza | Projeto de extensão em parceria com o IMREA (Instituto de Medicina Física e Reabilitação) |
| Grupo | Enzo Senatori, Rafael de Oliveira Corrêa e Theo Casella Mendonça |
| Tema do enunciado | **Tema B — equipamento de fisioterapia instrumentado** (o Tema A era prótese instrumentada) |
| Equipamento | Caneleira instrumentada para **extensão de joelho na posição sentada, com a coxa apoiada** |
| Entrega final | **Banca em 20/11/2026** — demonstração, documentação e relatório de impacto |

**Problema (demanda do IMREA).** Parte expressiva da reabilitação acontece fora da sessão supervisionada, sem que o fisioterapeuta veja a execução. Perde-se a informação que orienta a conduta: se o exercício foi feito, quantas vezes e se foi feito corretamente. O IMREA aponta como erros recorrentes na execução não supervisionada: amplitude insuficiente, velocidade excessiva, compensações posturais, carga inadequada e descanso não respeitado.

**Solução.** Uma caneleira com ESP32 e MPU-6050 que conta e qualifica cada repetição, compara-a com a prescrição, sinaliza desvios por vibração e publica os resultados via MQTT para um aplicativo.

**Por que uma única IMU basta.** Com a coxa apoiada na horizontal, a caneleira gira junto com a perna, e a inclinação da perna em relação à gravidade é o próprio ângulo de extensão do joelho. Essa premissa — postura sentada, coxa apoiada — é a delimitação do projeto e precisa constar no relatório como tal.

**Público e cenário.** A pessoa idosa, para quem o fortalecimento do quadríceps sustenta marcha, capacidade de levantar-se e autonomia. A ILPI (Instituição de Longa Permanência para Idosos) é o **cenário de aplicação**: ali o exercício é acompanhado por um cuidador que não tem formação para avaliar a execução. O texto ancora sempre no problema do IMREA, e a ILPI entra como cenário, não como protagonista, porque "aderência à demanda" tem peso 2 na avaliação.

**Relação com o Geratrix.** O Geratrix é o aplicativo de registro do cuidado em ILPIs que o mesmo grupo desenvolve em Projetos de Eng. Comp I e em Desenvolvimento Mobile (repositório `github.com/EnzoSenatori/Geratrix`, branch `developer`). Hoje o cuidador só consegue atestar *que* o exercício ocorreu; a caneleira acrescenta *como* ele ocorreu. O app é o cliente MQTT da caneleira.

**Caráter do protótipo.** Didático. Não é dispositivo médico, não será testado em pacientes, e a validação ocorre em bancada e com os próprios integrantes.

---

## 2. Estado atual (13/10/2026)

| Item | Situação | Onde está |
|---|---|---|
| Tema, equipamento e proposta | Decididos; nenhuma validação com pacientes | Seções 1, 3 e 4 |
| MQTT, quatro tópicos e exemplos fictícios | P1 concluído; Rafael aprovou e os quatro tipos de mensagem circularam no MQTTX Web | `docs/contrato-mqtt-v1.md` |
| Broker | EMQX Cloud Serverless criado; credenciais ficam fora dos documentos | Contrato MQTT |
| ESP32 | DevKit V1 de 30 pinos e cabo Micro-USB em mãos; cabo de dados e chip USB-serial ainda sem teste | `docs/inventario-materiais.md` |
| Outros componentes | Não foram mostrados como disponíveis; compra ainda pendente | Inventário P2 |
| Geratrix recebido | ZIP contém telas HTML/CSS estáticas; `checkin-exercicio.html` e `historico.html` ainda não têm MQTT nem dados dinâmicos | Cópia recebida em 12/10 |
| Firmware P3 | Simulação Wokwi concluída: ESP32 + MPU-6050, 100 Hz, ângulo e velocidade coerentes | [Projeto Wokwi P3](https://wokwi.com/projects/477822826125465601) e `firmware/caneleira/` |
| Firmware P5–P7 | Implementado em 01/11/2026: filtro complementar, máquina de estados, métricas, classificação e resumo da série; verificado em simulação no computador sobre 30 repetições de resultado conhecido; executado no Wokwi em 01/11/2026 até o fim da sessão | `firmware/caneleira/` e `firmware/testes/` |
| Teste físico | P4 ainda não iniciado: falta MPU-6050, protoboard, jumpers e conferência USB da placa | Seção 8 |
| Repositório | Criado em 01/11/2026 com o conteúdo do pacote de 13/10; nenhum código novo desde então | `github.com/EnzoSenatori/Geratrix_IoT_front` |

**Ponto de retomada.** P1 e P3 estão concluídos em ambiente de simulação. P2 continua aberto. O próximo marco físico é P4; em paralelo pode-se preparar a interface do Geratrix com o contrato MQTT aprovado. Nenhum dado de paciente nem teste real com paciente será usado.

**Limite da evidência.** O teste MQTTX confirmou o broker e o formato das mensagens; não testou ESP32 nem Geratrix. O Wokwi confirmou a execução do esqueleto do firmware; não calibrou ângulo, deriva ou contagem na placa real.

---

## 3. Requisitos do enunciado (*checklist* permanente)

Levantados do enunciado ao longo das conversas. Em caso de divergência com o enunciado da disciplina, ele prevalece sobre esta lista.

- [ ] Sensor **no equipamento de terapia** (Tema B) — não no corpo, nem em móveis.
- [ ] **Segundo sinal** além da IMU → FSR na tira de fixação (uso efetivo).
- [ ] **Decisão tomada localmente no ESP32**, funcionando com o celular desligado.
- [ ] **Atuador** com efeito físico → biofeedback por vibração.
- [ ] Comunicação via **broker MQTT** com payload JSON — nada de chamada direta a uma API.
- [ ] Integração com o app cobrindo **telemetria, estado, comandos e tratamento de queda de conexão** (critério de nota máxima).
- [ ] **Registros anonimizados**: sem nome, prontuário ou qualquer identificação pessoal (LGPD).
- [ ] **Alimentação autônoma de baixa tensão**, com modo de baixo consumo e **cálculo de autonomia**.
- [ ] **Montagem reversível** no equipamento — sem colagem nem perfuração.
- [ ] Taxa de amostragem **justificada**.
- [ ] Validação de bancada com **número de ensaios e erro aceitável declarados antes** dos testes.
- [ ] Componentes justificados por **datasheet** (faixa de medição, sensibilidade, tensão, consumo).
- [ ] Documentação: esquema elétrico, código comentado, lista de componentes e resultados.
- [ ] **Relatório de impacto** para o parceiro.
- [ ] Protótipo **não é dispositivo médico**: nenhum teste em pacientes e nenhuma promessa clínica (nada de "risco de queda", "diagnóstico" ou "monitoramento clínico").
- [ ] **Aderência à demanda** do IMREA (peso 2).

---

## 4. Decisões fechadas (não reabrir sem consultar o grupo)

| # | Decisão | Motivo |
|---|---|---|
| D1 | Tema B, com a caneleira como equipamento. Halter e faixa elástica descartados | Mesmo hardware e firmware do halter, mas narrativa mais forte com o público idoso e com o Geratrix; a faixa elástica tem fixação e calibração difíceis |
| D2 | Exercício: extensão de joelho sentado, coxa apoiada; **uma** MPU-6050 | A geometria torna a inclinação da perna igual ao ângulo do joelho |
| D3 | FSR na tira como segundo sinal (uso efetivo) | Atende ao requisito e impede "fabricar" repetições balançando a caneleira na mão |
| D4 | Biofeedback por motor de vibração | Quem se exercita não olha para o celular |
| D5 | O ESP32 decide localmente; parâmetros persistidos na memória; opera com o celular desligado | Requisito do enunciado |
| D6 | MQTT com JSON; o app assina o broker direto, via MQTT sobre WebSocket — **sem backend nem API** | O Geratrix não tem backend |
| D7 | Telemetria **por série** (um resumo ao fim de cada série), não por repetição | O "ao vivo" por repetição foi julgado complexo demais; o retorno imediato já existe na perna, pela vibração |
| D8 | A caneleira guarda os resumos até receber confirmação do app; mensagens com *retain* | Tratamento de queda sem o custo do ao vivo |
| D9 | Caneleira **anônima**; o vínculo com o residente é feito **só no app**, que envia um identificador de sessão anônimo | Concilia a LGPD do enunciado com a necessidade do Geratrix de saber de quem é o exercício |
| D10 | Prescrição definida pelo **gestor**: pré-preenchida na tela, editável só por ele | O Geratrix não tem perfil de fisioterapeuta |
| D11 | Duas telas no Geratrix: **Registro de exercício** (nova) e **Histórico** (modificada) | Segue a lógica registrar → histórico do app e dá dois momentos claros na banca |
| D12 | Esforço percebido em três níveis (leve, moderado, intenso), informado pelo cuidador | O sensor não o mede; simples para quem registra |
| D13 | Validação em bancada e com integrantes: goniômetro ou transferidor para o ângulo, contagem manual para as repetições | Limite ético do enunciado |
| D14 | *Deep sleep* com despertar pelo botão; eletrônica em caixa presa à caneleira por velcro | Autonomia e montagem reversível |

---

## 5. Objetivos da proposta (referência para o relatório)

**Objetivo geral.** Desenvolver um protótipo de caneleira instrumentada, baseado em ESP32, capaz de detectar e qualificar autonomamente as repetições do exercício de extensão de joelho na posição sentada, sinalizar desvios em relação aos parâmetros prescritos por meio de biofeedback e publicar os resultados via MQTT para um aplicativo móvel de acompanhamento, atendendo à demanda do IMREA por registro objetivo dos exercícios realizados fora da sessão supervisionada.

**Objetivos específicos** (entre colchetes, o passo do roteiro que o cumpre):

1. Levantar, junto à demanda do IMREA e à literatura, os parâmetros que caracterizam a execução correta da extensão de joelho e os desvios mais frequentes na execução não supervisionada. [relatório]
2. Identificar os sensores, o atuador e a fonte de alimentação adequados, a partir de faixa de medição, sensibilidade, tensão e consumo informados em datasheet. [P2, P15]
3. Determinar a taxa de amostragem da IMU e o método de estimativa do ângulo da perna, com fusão de acelerômetro e giroscópio por filtro complementar. [P4, P5]
4. Caracterizar cada repetição por métricas de qualidade — amplitude de movimento (ADM), velocidade angular e duração das fases concêntrica e excêntrica —, a partir de um algoritmo de detecção de eventos executado no próprio ESP32. [P6, P7]
5. Descrever o contrato de interface com o aplicativo: broker, tópicos, payload JSON, unidades, periodicidade, QoS e mensagem de *last will*. [P1]
6. Implementar o biofeedback vibratório para repetição válida, repetição fora do padrão, fim de série e fim do intervalo de descanso, com parâmetros configuráveis pelo aplicativo. [P9]
7. Verificar a exatidão do ângulo medido e da contagem de repetições em bancada, contra goniômetro ou transferidor e contagem manual, com erro aceitável previamente declarado. [P14]
8. Avaliar a autonomia do dispositivo em bateria, com uso de modo de baixo consumo, por estimativa calculada ou medida. [P12]
9. Analisar o benefício esperado para o parceiro e para as pessoas atendidas, consolidado no relatório de impacto. [P16]

---

## 6. Arquitetura técnica

### 6.1 Componentes

| Componente | Especificação | Função | Grandeza / sinal |
|---|---|---|---|
| ESP32 | DevKit V1 (ESP32-WROOM-32) | Aquisição, processamento, decisão e Wi-Fi | — |
| IMU | MPU-6050, módulo GY-521 (I²C) | Sensor de movimento | Aceleração e velocidade angular → ângulo, ADM, repetições |
| FSR | FSR 402 na tira de fixação | Uso efetivo | Pressão de contato com a perna |
| Botão | Tátil 12 mm | Início/fim de série; despertar do *deep sleep* | Digital |
| Motor de vibração | Tipo moeda, 3 V, acionado por NPN (BC337 ou 2N2222) com diodo 1N4148 | Biofeedback | Padrões de vibração por evento |
| Alimentação | LiPo 3,7 V ~1000 mAh + TP4056 com proteção + LDO 3,3 V (ME6211 ou AP2112K) | Alimentação autônoma de baixa tensão | Nível de bateria (%) por divisor resistivo |

### 6.2 Mapa de pinos — **proposto, confirmar na bancada**

| Sinal | GPIO | Motivo |
|---|---|---|
| MPU-6050 SDA | 21 | I²C padrão do ESP32 |
| MPU-6050 SCL | 22 | I²C padrão do ESP32 |
| Botão | 33 | Pino RTC: permite despertar do *deep sleep* (ext0) e tem *pull* interno |
| FSR (divisor com 10 kΩ) | 34 | ADC1 — o ADC2 não funciona com o Wi-Fi ativo |
| Bateria (divisor 2 × 100 kΩ) | 35 | ADC1; a LiPo cheia (4,2 V) chega ao pino como ~2,1 V |
| Motor (base do transistor via 1 kΩ) | 26 | Saída PWM por LEDC (conteúdo da Aula 5 — saídas PWM) |

GPIOs 34 a 39 são apenas entrada e não têm *pull-up* interno; o divisor resistivo resolve isso para o FSR e para a bateria.

### 6.3 Convenção de grandezas

- **Ângulo θ:** 0° com a perna pendente (vertical) e ≈ 90° com a perna estendida (horizontal). É nessa convenção que a prescrição "ADM mínima de 60°" faz sentido.
- **ADM** de uma repetição: θ_máx − θ_mín do ciclo.
- **Velocidade angular:** em °/s, lida do giroscópio. A biblioteca Adafruit MPU6050 entrega o giroscópio em rad/s e o acelerômetro em m/s² — converter.
- **Tempos:** fase concêntrica (subida) e excêntrica (descida), em s; descanso entre séries, em s.

### 6.4 Processamento local no ESP32

1. **Amostragem a 100 Hz** (proposta). Justificativa: a extensão de joelho é lenta, da ordem de uma repetição a cada poucos segundos; 50 a 100 Hz dá folga ampla para filtragem e detecção de fases. Confirmar o Δt real em bancada.
2. **Calibração do *offset* do giroscópio** na inicialização: média de algumas centenas de amostras com a perna parada.
3. **Ângulo pelo acelerômetro:** `θ_acc = atan2(...)` sobre os dois eixos do plano do movimento. Eixos e sinais dependem de como a IMU for fixada; validar com transferidor em 0° e 90°.
4. **Filtro complementar:** `θ_k = α·(θ_(k−1) + ω·Δt) + (1 − α)·θ_acc`, com α ≈ 0,98 como ponto de partida e Δt = 0,01 s. O giroscópio dá suavidade no movimento; o acelerômetro corrige a deriva.
5. **Máquina de estados com histerese** (o mesmo princípio de variável de estado já usado no relatório do LM35 desta disciplina):
   - `REPOUSO` → `SUBINDO` quando θ > θ_início (≈ 15°, a calibrar);
   - `SUBINDO` → `DESCENDO` quando θ cai δ abaixo do pico registrado (δ ≈ 5°, a calibrar), o que evita que tremor no topo gere dois picos;
   - `DESCENDO` → `REPOUSO` quando θ < θ_fim (≈ 10°, a calibrar), e **a repetição é contabilizada**.
6. **Métricas por repetição:** ADM, velocidade de pico na subida, duração da fase concêntrica (início → pico) e da excêntrica (pico → retorno). *Implementação de 01/11:* as fases são medidas pelo giroscópio — concêntrica é o tempo com ω acima de +10 °/s; excêntrica, abaixo de −10 °/s —, o que exclui pausas no topo e dispensa localizar o instante exato do pico.
7. **Classificação:** válida se ADM ≥ `adm_min` **e** velocidade de pico ≤ `vel_max` **e** uso efetivo confirmado pelo FSR. Caso contrário, fora do padrão, com o motivo (`adm` ou `vel`). Movimento sem uso efetivo não é contado.
8. **Descanso:** cronometrado pelo próprio ESP32, com aviso vibratório ao final.
9. **Biofeedback — padrões propostos** (a confirmar no Passo 9):

| Evento | Padrão |
|---|---|
| Repetição válida | 1 pulso curto |
| Repetição fora do padrão | 2 pulsos curtos |
| Fim de série | 3 pulsos curtos |
| Fim do descanso | 1 pulso longo |

### 6.5 Contrato MQTT

**Aprovado por Rafael em 12/10/2026**, responsável pela integração MQTT no Geratrix. Integração e testes somente com dados fictícios. Prefixo: `imrea/g01/caneleira`. MQTT 3.1.1, mensagens UTF-8/JSON, sem nome ou identificação de paciente. O grupo `g01` continua provisório até receber o número oficial.

**Conexão:** EMQX Cloud Serverless `z71f6421.ala.eu-central-1.emqxsl.com`. ESP32: MQTT/TLS, porta `8883`, usuário `caneleira_g01`. Geratrix: WSS `wss://z71f6421.ala.eu-central-1.emqxsl.com:8084/mqtt`, usuário `geratrix_g01`. Senhas guardadas fora do contrato. Clientes MQTT com IDs distintos. Reconexão automática em 2–30 s; reinscrever após reconectar, sem depender de sessão persistente do broker.

| Tópico completo | Quem publica → quem lê | QoS / retain | Conteúdo e momento |
|---|---|---|---|
| `imrea/g01/caneleira/cmd` | Geratrix → ESP32 | 1 / não | Ao iniciar ou alterar a prescrição: `{"sid":"s7f3a","series":3,"reps":10,"adm_min":60,"vel_max":150,"descanso":60}`. O app reenvia comando pendente após reconexão com o mesmo `sid`; o ESP32 não reinicia uma sessão duplicada. |
| `imrea/g01/caneleira/sessao/<sid>` | ESP32 → Geratrix | 0 / sim | Ao fim de cada série e após reconectar, publica o resumo acumulado das séries da sessão até a confirmação. Exemplo abaixo. Cada sessão tem tópico próprio para não sobrescrever outra pendente. |
| `imrea/g01/caneleira/ack` | Geratrix → ESP32 | 1 / não | Só depois de salvar: `{"sid":"s7f3a","series_recebidas":1}`. O número indica quantas séries foram salvas. Após o `ack` final da sessão concluída, o ESP32 apaga a mensagem retida com publicação vazia no mesmo tópico e então remove a cópia local. |
| `imrea/g01/caneleira/estado` | ESP32 → Geratrix | 0 / sim | Na conexão, quando mudar e a cada 60 s: `{"online":true,"bat":84,"rssi":-60,"uso":true,"sid":"s7f3a"}`. *Last will* retido `{"online":false}`; publicar `offline` também antes de desligar normalmente. |

Exemplo de `sessao/s7f3a` após a primeira série:

```json
{"sid":"s7f3a","status":"em_execucao","series":[{"n":1,"validas":8,"total":10,"adm_media":63.0,"vel_pico":128.0,"desvios":{"adm":2,"vel":0},"t_conc":1.4,"t_exc":1.7}],"ts":1791806400}
```

`sid` é único e anônimo, sem `/` ou `+` no identificador. `status`: `aguardando`, `em_execucao`, `descanso`, `concluida`. ADM em graus; velocidade em graus/s; descanso e tempos das fases concêntrica/excêntrica em segundos; bateria em %, sinal em dBm; `ts` em segundos Unix UTC ou `null` sem hora confiável. O app identifica séries por `sid` + `n`, evita duplicatas, salva antes do `ack` e associa o `sid` ao residente somente dentro do app. O ESP32 guarda resumos pendentes em memória não volátil e continua o exercício e o biofeedback mesmo sem rede.

**Validação P1:** em 12/10/2026, `cmd`, `sessao`, `ack` e `estado` fictícios foram publicados e recebidos no MQTTX Web via WSS. A retenção de `estado` foi comprovada; a sessão de teste retida foi apagada. Não houve teste do ESP32, do Geratrix ou do *last will*. Os valores de amplitude e velocidade são exemplos para desenvolvimento, sujeitos a calibração; não são limites clínicos. A credencial MQTT usada diretamente no navegador não é secreta para quem acessa o app; usar somente dados fictícios no protótipo e renovar essa credencial após a demonstração.

### 6.6 Integração com o Geratrix (lado do app)

**Tela 1 — Registro de exercício** (`checkin-exercicio.html`, nova, aberta pelo item "Atividade física" da lista):

1. O cuidador abre o exercício do residente, confere a prescrição e toca em "Iniciar". O app publica o `cmd` com a prescrição e o `sid`.
2. A tela mostra "Em execução — série 1 de 3" e o estado da caneleira (online, bateria, sinal), alimentado por `estado` e pelo *last will*.
3. A cada série concluída, chega a `sessao` atualizada e a linha correspondente da tabela é preenchida.
4. Ao final, o cuidador informa o esforço (leve, moderado, intenso) e uma observação, e registra.
5. **Caneleira offline:** ponto vermelho, "Desconectada", e só o registro manual fica disponível.

**Tela 2 — Histórico** (`historico.html`, modificada): gráfico de ADM média por sessão com a linha da ADM mínima prescrita, repetições válidas por data e selo azul "Dispositivo" nos registros vindos da caneleira — o que distingue dado medido de dado declarado.

A cópia recebida em 12/10 é HTML e CSS estáticos, sem JavaScript, backend ou cliente MQTT. As telas existem visualmente, mas os botões, indicadores, tabela e histórico ainda exibem dados fixos. O próximo trabalho do app é implementar a troca `cmd`/`sessao`/`ack`/`estado` por WSS com dados fictícios e substituir esses valores fixos. A credencial MQTT do navegador não é segredo de produção.

---

## 7. Materiais

### 7.1 Lista parcial — suficiente para começar

Critério: o marco das duas primeiras semanas — ângulo estimado, repetições contadas e resumo da série publicado no broker.

| Hardware | Software |
|---|---|
| ESP32 DevKit V1 | Arduino IDE 2.x + pacote de placas ESP32 (Espressif) + driver USB-serial (CP210x ou CH340, conforme a placa) |
| 2× MPU-6050 (GY-521) — a segunda é reserva, clones costumam falhar | Bibliotecas Adafruit MPU6050, PubSubClient e ArduinoJson |
| Botão tátil | Broker MQTT público |
| Cabo micro-USB **de dados** | MQTT Explorer (ou MQTTX) |
| Protoboard e jumpers | Wokwi (simula ESP32 com MPU-6050) |

### 7.2 Itens da segunda fase (lista completa)

- **Sensor e atuador:** FSR 402; motor de vibração tipo moeda 3 V; BC337 ou 2N2222; 1N4148; resistores de 10 kΩ, 1 kΩ e 2 × 100 kΩ.
- **Alimentação:** LiPo 3,7 V ~1000 mAh com JST; TP4056 com proteção (DW01), USB-C; LDO 3,3 V ME6211 ou AP2112K (≥ 500 mA); capacitores de 10 µF e 100 µF; chave liga/desliga.
- **Montagem e validação:** placa perfurada ilhada (~5 × 7 cm) e barras de pinos; ferro de solda e estanho; multímetro com escala de mA; caneleira com velcro (0,5–1 kg); caixa plástica pequena e tira de velcro; goniômetro universal.
- **Software:** Preferences, WiFi e esp_sleep (já no pacote ESP32); MQTT.js e Chart.js (app); Wokwi ou Fritzing (esquema elétrico).

Recomendação feita ao grupo: comprar a lista completa num único pedido, porque os itens são baratos e o frete costuma pesar mais que eles. O risco real é o prazo de entrega.

### 7.3 Substitutos de pior caso (só se a compra falhar; conferir contra o enunciado antes)

| Item | Substituto | Ressalva |
|---|---|---|
| FSR | Pino de toque capacitivo do ESP32 (ex.: GPIO 4) com placa de fita de cobre em contato com a pele | Mesma função, custo quase nulo |
| Motor | Buzzer (melhor que LED, pois o usuário não olha para a perna) | Biofeedback mais pobre, mas atende |
| LiPo + circuito | *Power bank* USB | Muitos se desligam sozinhos com consumo baixo, como no *deep sleep*; testar e, se ocorrer, declarar como limitação |
| Caixa | Tira de velcro e qualquer caixinha plástica | — |

---

## 8. Roteiro passo a passo (a partir de 12/10)

### Calendário

| Semana | Fase | Passos |
|---|---|---|
| Já (em paralelo) | 1 — Pendências de definição | P1 contrato MQTT · P2 compra |
| 12/10 e 19/10 | 2 — Bancada: sensoriamento | P3 a P8 |
| 26/10 | 3 — Atuação e conectividade | P9 a P11 |
| 02/11 | 4 — Autonomia e montagem | P12 e P13 |
| 09/11 a 19/11 | 5 — Validação e entrega | P14 a P16 |
| **20/11** | **Banca** | — |

**Caminho crítico atualizado:** P1 foi concluído. Restam P2 e P4 a P6 para validar o sensoriamento físico e a contagem de repetições; a implementação MQTT do Geratrix pode avançar em paralelo com dados fictícios.

---

### Fase 1 — Pendências de definição

**P1 — Fechar o contrato MQTT**
- **Objetivo:** congelar tópicos, payloads, unidades, periodicidade, QoS, *retain* e *last will* com quem implementa o app.
- **Fazer:** partir da v1 (seção 6.5); resolver os pontos em aberto; redigir o contrato em uma página, com uma tabela por tópico; publicar à mão, no MQTT Explorer, um exemplo de cada mensagem.
- **Pronto quando:** o responsável pelo app aprova, e cada mensagem de exemplo é publicada e lida no MQTT Explorer.
- **Para o relatório:** a tabela do contrato aprovado e a justificativa da telemetria por série e do *retain* acumulado. **Situação:** concluído em 12/10 com exemplos fictícios publicados e lidos no MQTTX Web.

**P2 — Confirmar a compra**
- **Fazer:** verificar com o grupo o que já foi comprado; se nada, um pedido único com a lista completa. Identificar o chip USB-serial da placa para instalar o driver certo.
- **Pronto quando:** os itens da lista parcial estão em mãos, e os demais, a caminho. **Situação:** pendente; apenas ESP32 e cabo foram identificados nas fotos.

### Fase 2 — Bancada: sensoriamento

**P3 — Esqueleto do firmware no Wokwi** *(concluído em 13/10; não depende de hardware)*
- **Fazer:** projeto Wokwi com ESP32 e MPU-6050 em I²C (21/22). Ler acelerômetro e giroscópio com a Adafruit MPU6050 a 100 Hz, com temporização por `micros()`. Imprimir θ_acc e ω (°/s) no monitor serial. Estruturar o código desde já em funções separadas — leitura da IMU, estimativa do ângulo, máquina de repetição, biofeedback e comunicação —, para que os passos seguintes apenas preencham cada uma.
- **Pronto quando:** ao mover os controles de inclinação da MPU-6050 virtual, o ângulo responde de forma coerente, e o Δt medido do laço fica estável em 10 ms.
- **Para o relatório:** a estrutura do firmware e o [link do projeto Wokwi](https://wokwi.com/projects/477822826125465601). Em simulação, `dt_ms` ficou perto de 10 ms; 0,7 g em X e Z resultou em 45,0°; giro Y de 60°/s resultou em 60,0°/s.

**P4 — Leitura real, *offset* e taxa de amostragem**
- **Fazer:** montar ESP32 e MPU-6050 na protoboard; calibrar o *offset* do giroscópio; confirmar 100 Hz (ou justificar 50 Hz); fixar a IMU provisoriamente na caneleira e definir os eixos do plano do movimento.
- **Pronto quando:** com a perna parada, θ_acc concorda com o transferidor em 0°, 45° e 90°, e a deriva do giroscópio parado está medida.
- **Para o relatório:** *offset* medido, taxa escolhida e justificativa, orientação dos eixos (foto).

**P5 — Filtro complementar**
- **Fazer:** implementar a fórmula da seção 6.4; ajustar α a partir de 0,98; comparar no Serial Plotter θ_acc, o ângulo integrado só pelo giroscópio e θ filtrado, durante o movimento.
- **Pronto quando:** em movimento, θ filtrado não tem os picos do acelerômetro; parado, não deriva.
- **Para o relatório:** α escolhido e a figura comparativa das três curvas.
- **Situação:** implementado em 01/11/2026, com α = 0,98. Em simulação no computador, o offset de 1,7 °/s foi removido e o erro máximo do ângulo filtrado foi de 1,7° em todo o roteiro. Executado também no Wokwi em 01/11. Falta ajustar α com a placa real, cuja aceleração linear a simulação não reproduz.

**P6 — Máquina de estados das repetições** *(marco crítico)*
- **Fazer:** implementar os estados e limiares com histerese da seção 6.4; calibrar θ_início, δ e θ_fim; testar 20 repetições contra contagem manual e os casos-limite: repetição incompleta, tremor no topo, pausa no topo.
- **Pronto quando:** 20 de 20 repetições contadas, sem dupla contagem, e repetição incompleta não aparece como válida.
- **Para o relatório:** diagrama de estados e limiares finais.
- **Situação:** implementado em 01/11/2026, com um estado adicional, FINALIZANDO: a repetição só é contada quando a descida termina. Se o ângulo voltar a passar do pico, a subida é retomada sem nova contagem. Em simulação, 30 de 30 repetições foram contadas, sem dupla contagem com tremor ou pausa no topo, e as incompletas saíram como fora do padrão. Limiares ainda a calibrar em bancada.

**P7 — Métricas e classificação**
- **Fazer:** calcular ADM, velocidade de pico e durações das fases; classificar contra uma prescrição fixa provisória (3 × 10, ADM mínima de 60°, velocidade máxima de 150 °/s, descanso de 60 s); ao fim de cada série, imprimir o resumo em JSON no formato exato do contrato.
- **Pronto quando:** o JSON do serial é idêntico em estrutura à mensagem `sessao` do contrato.
- **Situação:** implementado em 01/11/2026 e verificado em simulação: 7 válidas, 2 desvios de ADM e 1 de velocidade por série, como esperado, e JSON no formato do contrato. Definições adotadas no resumo, a validar com o Rafael: `adm_media`, `t_conc` e `t_exc` são médias de todas as repetições da série; `vel_pico` é o máximo da série; `status` vale `descanso` entre séries. O resumo de três séries tem 412 bytes, acima do buffer padrão de 256 bytes da PubSubClient, a ajustar no P10.

**P8 — FSR e uso efetivo**
- **Fazer:** divisor com 10 kΩ no GPIO 34; levantar a faixa de leitura com a caneleira presa e solta; definir o limiar com histerese; impedir a contagem sem uso efetivo.
- **Pronto quando:** balançar a caneleira na mão não gera repetições.
- **Para o relatório:** faixa de leitura e limiar adotado.

### Fase 3 — Atuação e conectividade

**P9 — Biofeedback**
- **Fazer:** motor pelo NPN, com base em 1 kΩ, diodo 1N4148 em antiparalelo e GPIO 26 em PWM (LEDC); implementar os quatro padrões da seção 6.4 de forma **não bloqueante**.
- **Pronto quando:** os quatro padrões são distinguíveis de olhos fechados, e a amostragem não perde ciclos durante a vibração.

**P10 — MQTT**
- **Fazer:** Wi-Fi e PubSubClient com *last will* em `estado`; receber o `cmd`, validá-lo e gravá-lo em Preferences; publicar `sessao` acumulada ao fim de cada série; retentar até receber o `ack`; reconexão não bloqueante.
- **Pronto quando**, simulando o app no MQTT Explorer: o `cmd` enviado é aplicado; a série executada aparece em `sessao`; derrubar o Wi-Fi no meio da sessão e religá-lo entrega tudo; desligar o ESP32 faz `estado` virar `{"online":false}`.

**P11 — Operação com o celular desligado**
- **Fazer:** sem rede ou sem broker, a sessão roda com a última prescrição salva; os resumos ficam na memória não volátil até o `ack`.
- **Pronto quando:** uma sessão completa roda com o roteador desligado e, ao religá-lo, o app recebe tudo.

### Fase 4 — Autonomia e montagem

**P12 — *Deep sleep* e cálculo de autonomia**
- **Fazer:** *deep sleep* com despertar pelo botão (ext0, GPIO 33) **e por timer no fim do descanso** — sem o timer, o aviso de fim de descanso não acontece com o ESP32 dormindo. Guardar o estado da sessão em `RTC_DATA_ATTR` ou na memória não volátil, pois o *deep sleep* apaga a RAM. Medir com o multímetro a corrente em cada modo: dormindo, amostrando sem Wi-Fi, transmitindo.
- **Cálculo:** I_média = Σ(I_i · t_i) / T; autonomia (h) = C (mAh) × 0,8 / I_média (mA), para um uso típico declarado (por exemplo, duas sessões por dia).
- **Pronto quando:** a tabela de consumo medido e a autonomia calculada estão prontas.

**P13 — Montagem**
- **Fazer:** transferir o circuito para a placa perfurada; alimentar pela cadeia LiPo → TP4056 → chave → LDO 3,3 V → pino 3V3, sem passar pelo AMS1117 da placa; capacitores junto ao ESP32; caixa presa por velcro; FSR na tira.
- **Pronto quando:** o equipamento é vestível, liga pela chave, funciona sem cabo e sai da caneleira sem dano.

### Fase 5 — Validação e entrega

**P14 — Validação**
- **Fazer:** declarar o protocolo **antes** dos ensaios — ângulos de referência (0°, 30°, 60°, 90°), número de medições por ângulo (por exemplo, 10), erro aceitável (proposta: ±5°) e número de séries com contagem manual; executar com os três integrantes; registrar tudo em planilha.
- **Pronto quando:** há tabela de erro médio e máximo por ângulo e taxa de acerto da contagem, comparadas ao erro declarado.

**P15 — Documentação técnica**
- **Fazer:** esquema elétrico (Wokwi ou Fritzing), código comentado, lista de componentes com dados de datasheet (faixa, sensibilidade, tensão, consumo), contrato MQTT e resultados.

**P16 — Relatório de impacto e demonstração**
- **Fazer:** relatório de impacto (benefício ao IMREA e às pessoas atendidas — objetivo específico 9); roteiro da demonstração: o cuidador abre o exercício → `cmd` → séries com vibração → resumo no app → queda de conexão simulada e recuperação → histórico.
- **Pronto quando:** a demonstração foi ensaiada do início ao fim e cronometrada.

---

## 9. Pontos de atenção técnicos

- **Cabo micro-USB de dados.** Cabos só de carga são a causa mais comum de "placa não reconhecida".
- **ADC2 inutilizável com Wi-Fi ativo.** Sinais analógicos só nos GPIOs 32 a 39; ler com `analogReadMilliVolts()`.
- **Botão de despertar em pino RTC** (GPIO 33).
- **AMS1117 da placa** derruba tensão demais com LiPo: usar LDO de baixa queda no pino 3V3. Não alimentar pela bateria e pelo USB ao mesmo tempo.
- **Picos de corrente do Wi-Fi:** capacitores de 10 µF e 100 µF junto à alimentação do ESP32.
- **Unidades da Adafruit MPU6050:** rad/s e m/s².
- **Core ESP32 3.x** mudou a API do LEDC (`ledcAttach` por pino, em vez de `ledcSetup` + `ledcAttachPin`): conferir a versão instalada antes de escrever o PWM.
- **PubSubClient:** publica só em QoS 0 e tem buffer de 256 bytes por padrão (seção 6.5).
- ***Retain*** guarda apenas a última mensagem por tópico (seção 6.5).
- **Nada de `delay()`** no laço de amostragem nem nas rotinas de vibração e reconexão.
- ***Deep sleep*** apaga a RAM; a reconexão ao Wi-Fi ao despertar leva alguns segundos.
- **Linguagem do relatório e da banca:** nunca apresentar o protótipo como dispositivo médico; qualquer uso real exigiria Comitê de Ética em Pesquisa e enquadramento regulatório.

---

## 10. Referências

- **MDPI Sensors (2022)** — *Motion Sensors for Knee Angle Recognition in Muscle Rehabilitation Solutions*: https://www.mdpi.com/1424-8220/22/19/7605 — usa ESP32 e MPU-6050 para o ângulo do joelho, com voluntários sentados elevando a perna e app móvel. Usou duas IMUs (coxa e perna); a premissa da coxa apoiada permite uma só. Vale ler a seção de protocolo experimental.
- **Frontiers in Neurorobotics (2022)** — sistema inercial vestível para reabilitação domiciliar após artroplastia de joelho: https://www.frontiersin.org/journals/neurorobotics/articles/10.3389/fnbot.2022.836184/full — sustenta a viabilidade de uma única IMU para o ângulo do joelho.
- **YouTube — ESP32 Step Tracker com MPU6050 e React Native:** https://www.youtube.com/watch?v=_QS4kDK6_ho — detecção de passo por limiar é análoga à detecção de repetição.
- **YouTube — ligação física e código ESP32 + MPU6050:** https://www.youtube.com/watch?v=H9e1Up7xHjc
- **IBGE, Censo 2022** — população com 60 anos ou mais: 32.113.490 pessoas (15,6%), aumento de 56,0% em relação a 2010.
- **Repositório do Geratrix:** https://github.com/EnzoSenatori/Geratrix (branch `developer`).
- **Material da disciplina** (no Projeto do Enzo): Aula 3 — Memórias e Arduino; Aula 4 — Programação C e entradas (conversão A/D); Aula 5 — Saídas PWM.
- **Documentos do grupo:** `Proposta_Caneleira_Instrumentada_IMREA.pdf`, `Guia_Telas_IoT_Geratrix.pdf`, `Caneleira_Lista_de_Materiais.pdf`.

---

## 11. Pendências a confirmar com o grupo

1. **Compra:** decidir e registrar o pedido de MPU-6050, protoboard/jumpers e demais componentes; até agora só ESP32 e cabo foram mostrados.
2. **Placa:** ligar ao computador para confirmar que o cabo transmite dados, qual porta aparece e qual chip USB-serial/driver é necessário.
3. **Número oficial do grupo:** `g01` é provisório. Migrar o prefixo MQTT somente após decisão do grupo, sem perder registros do teste.
4. **Geratrix:** confirmar se o ZIP recebido é a base atual ou apenas o protótipo visual; o código contém telas estáticas e não a integração MQTT.
5. **Validação P14:** definir antes dos ensaios o erro aceitável (proposta inicial ±5°) e o número de medições/repetições.
6. **Escopo acadêmico:** alinhar com Enzo e Theo como a integração IoT aparece no projeto Geratrix e na apresentação. Não haverá teste com pacientes.
7. **Broker próprio (Enzo, 02/11):** o grupo passará a usar um broker EMQX Cloud Serverless sob gestão do Enzo, com os usuários `caneleira_g01`, `geratrix_g01` e `teste_g01`. Depois da criação, o contrato sobe para a v1.1, alterando só a seção de conexão, e o Rafael é avisado da troca.

**Já respondido:** Rafael é o responsável pela integração MQTT no Geratrix e pela aprovação técnica; o contrato P1 está aprovado. O enunciado e a proposta foram disponibilizados à IA.

---

## 12. Prompt inicial sugerido

Cole junto com este arquivo:

```
Você vai me acompanhar na continuidade de um projeto acadêmico de IoT. O guia anexo consolida tudo o que meu grupo já decidiu e produziu até aqui. Leia-o integralmente e siga o método da seção 0: um passo por vez, respostas objetivas, posição tomada e avanço apenas quando eu autorizar. Comece confirmando, em até cinco linhas, o estado atual e o próximo passo, e me pergunte o que for preciso da seção 11.
```
