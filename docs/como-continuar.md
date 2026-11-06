# Como continuar o projeto da caneleira

02/11/2026 · Enzo Senatori

## Para que serve este guia

Este guia permite que qualquer integrante, com ou sem IA, retome o projeto a partir do estado registrado aqui sem depender de quem o conduzia. Todo o material está no repositório [Geratrix_IoT_front](https://github.com/EnzoSenatori/Geratrix_IoT_front).

Ordem de leitura antes de mexer em qualquer coisa:

1. Este guia, inteiro.
2. `README.md` do repositório: estado atual e como rodar cada parte.
3. `docs/guia-continuidade.md`: decisões fechadas, arquitetura e roteiro P1 a P16.
4. `docs/contrato-mqtt-v1.md`: obrigatório antes de mexer em comunicação.
5. O `README.md` da pasta em que for trabalhar, por exemplo `firmware/caneleira/README.md`.

Quem for trabalhar com uma IA entrega a ela este guia e os arquivos acima e cola o prompt da seção "Como trabalhar com uma IA".

## O projeto em poucas linhas

O grupo constrói uma caneleira instrumentada para o exercício de extensão de joelho, sentado e com a coxa apoiada. É o projeto de extensão de Desenvolvimento IoT (Mackenzie, 2026-2), em parceria com o IMREA, e a banca é em 20/11/2026.

Uma ESP32 com sensor de movimento (MPU-6050) conta e qualifica cada repetição, compara-a com a prescrição, avisa o usuário por vibração e envia o resumo de cada série, via broker MQTT, ao app Geratrix. O app envia a prescrição, mostra o estado da caneleira e guarda o histórico.

O protótipo é didático: não é dispositivo médico, não é testado em pacientes e usa apenas dados fictícios.

## Onde estamos

Em 06/11/2026, o firmware conta e classifica as repetições, ignora movimento sem uso, sinaliza por vibração e sincroniza os resumos com o broker. A operação offline e a recuperação após reinício foram validadas em simulação. A validação na placa real e no app segue pendente.

| Etapa | Situação | Onde foi verificada |
| --- | --- | --- |
| P1 — Contrato MQTT | Concluída; v1.1 desde 01/11, com o broker novo | MQTTX: em 12/10 no broker anterior; em 02/11 no novo |
| Broker próprio | Criado em 01/11 na conta do Enzo, com três usuários | Painel do EMQX |
| P2 — Compra de componentes | Aberta: lista de materiais versão 1 (02/11), em `docs/inventario-materiais.md`; em mãos, ESP32, cabo, power bank e caneleira | — |
| P3 — Leitura do sensor a 100 Hz | Concluída | Wokwi, em 13/10 |
| P4 — Leitura real, offset e eixos | Não iniciada; aguarda as peças | — |
| P5 a P7 — Filtro, contagem, classificação e resumo da série | Concluídas em simulação | Computador e Wokwi, em 01/11 |
| P8 — Uso efetivo pelo sensor de pressão | Concluída em simulação | Wokwi, com potenciômetro no lugar do FSR, em 02/11 |
| P9 — Biofeedback por vibração | Concluída em simulação | Wokwi, com LED no lugar do motor, em 02/11 |
| P10 — MQTT na ESP32 | Concluída em simulação. `estado`, aviso de desconexão, `cmd`, `sessao/<sid>` e `ack` verificados | Wokwi e MQTTX, em 02/11 |
| P11 — Operação sem rede | Concluída em simulação. Queda durante a série, recuperação após reinício e sessão iniciada offline verificadas | [Wokwi e MQTTX, em 06/11](../evidencias/p11/) |
| App — integração MQTT no Geratrix | Não iniciada; as telas são só visuais | — |
| P12 a P16 — Autonomia, montagem, validação, documentação, relatório | Não iniciadas | — |

O `README.md` do repositório traz sempre a versão mais recente desta tabela.

## O que fazer a seguir

Enquanto as peças não chegam, o grupo trabalha no que não depende da placa, nesta ordem. O item 1 foi feito pelo Rafael em 06/11/2026; os itens 2 e 3 estão com o Felipe. Antes de começar um item, avise no grupo para ninguém fazer o mesmo em paralelo.

1. **Operação sem rede (P11).** Validada no Wokwi e no broker do grupo, com o MQTTX representando o app. O roteiro e os resultados estão nas [evidências do P11](../evidencias/p11/), e as limitações aceitas, no README do firmware.
2. **Botão e baixo consumo (P12), a parte de código.** Início da série pelo botão; *deep sleep* com despertar pelo botão ou pelo timer do descanso, sem perder o estado da sessão; publicar `{"online":false}` antes de dormir. Tudo testável no Wokwi; só a medição de corrente depende da placa.
3. **Documentação técnica.** Esquema elétrico a partir do circuito completo do Wokwi, lista de componentes com dados de datasheet, autonomia estimada pelas correntes do datasheet, protocolo de validação (P14) e as seções do relatório que não dependem de medições.
4. **Integração MQTT no app**, feita pelo grupo depois da refatoração do Geratrix para Flutter, com um pequeno back-end só para a tela que liga os dois projetos. Pronto quando a tela de registro de exercício envia o `cmd`, mostra o estado da caneleira (inclusive desconectada), preenche as séries conforme chegam, grava antes de publicar o `ack`, e o histórico exibe as sessões gravadas. O app assina `sessao/+`, e não só o `sid` que criou, e confirma toda sessão recebida: a sessão iniciada sem o app (P11) tem `sid` criado pela caneleira, e, sem esse `ack`, a caneleira recusa prescrições novas. O contrato não muda, e um app instalado pode usar a porta 8883, como a ESP32.

Ficam para quando as peças chegarem: eixos e offset reais (P4), ajuste dos limites com movimento real (P6), faixa real do sensor de pressão, corrente medida, montagem (P13) e ensaios (P14).

## Como o repositório está organizado

Cada pasta tem um papel fixo; código novo entra na pasta da parte a que pertence.

```text
README.md                  estado atual e como rodar cada parte
docs/
├── como-continuar.md      este guia
├── guia-continuidade.md   guia vivo: decisões, arquitetura, roteiro P1–P16
├── contrato-mqtt-v1.md    tópicos, mensagens, QoS e retenção (v1.1)
├── inventario-materiais.md  componentes em mãos e a comprar
├── repasse-rafael.md      repasse de 13/10, com o passo a passo do P4
└── referencias/           fotos da placa
firmware/
├── caneleira/             código da ESP32 e arquivos do Wokwi
└── testes/                teste da lógica no computador
app/                       telas do Geratrix (HTML/CSS)
evidencias/                capturas e registros de cada etapa concluída
```

No firmware, a lógica fica em `processamento.h`, sem dependência do Arduino, para poder ser testada no computador. O `caneleira.ino` só lê o sensor, imprime e aciona. Os valores a calibrar ficam reunidos no topo de `processamento.h`.

## Como trabalhar no repositório

A `main` guarda só o que foi testado e revisado; todo trabalho novo nasce numa branch.

1. **Branch por funcionalidade.** Atualize a `main` e crie uma branch com nome curto: `p10-mqtt-esp32`, `app-mqtt`, `p9-biofeedback`.
2. **Commits pequenos, em português.** A primeira linha diz o que mudou, no imperativo ("Implementa a reconexão MQTT"). O corpo diz o que foi testado e onde: computador, Wokwi, bancada ou app.
3. **Revisão antes da `main`.** Abra um pull request; outro integrante revisa e aprova antes de juntar. Mudanças no contrato MQTT passam também pelo Rafael.
4. **Ao concluir uma etapa**, no mesmo pull request:
    - atualize a tabela "Estado atual" do `README.md`;
    - registre no `docs/guia-continuidade.md` a data, o ambiente, o resultado e o que ficou pendente;
    - salve a evidência (print, foto, tabela) em `evidencias/<etapa>/`, com um `README.md` dizendo o que cada arquivo mostra.
5. **Nunca versione** senhas, dados de Wi-Fi, arquivos compilados ou qualquer dado de pessoa real. O `.gitignore` já bloqueia `segredos.h`, `*.local.js` e os binários do teste.

Resultado obtido em simulação ou com dados fictícios é sempre identificado como tal, no commit e no guia.

## Como testar cada parte

Toda mudança é testada no ambiente mais próximo possível do real antes de ir para revisão.

| Parte | Onde testar | Como |
| --- | --- | --- |
| Lógica do firmware (`processamento.h`) | Computador | Em `firmware/testes/`: `g++ -std=c++17 -I../caneleira teste_processamento.cpp -o teste && ./teste`. Só siga se terminar com "todas as verificacoes passaram". |
| Firmware completo | Wokwi | Abra o [projeto P3](https://wokwi.com/projects/477822826125465601), troque o `sketch.ino` pelo `caneleira.ino` e crie os arquivos `.h` da pasta com os mesmos nomes. O resultado esperado está em `firmware/caneleira/README.md`. |
| Comunicação MQTT | [MQTTX Web](https://mqttx.app/web-client) | Host `wss://z2f0d126.ala.eu-central-1.emqxsl.com`, porta 8084, caminho `/mqtt`, usuário `teste_g01`. Assine `imrea/g01/caneleira/#`. Para fazer o papel da caneleira ou do app, publique os exemplos do contrato. |
| Telas do app | Navegador | Abra `app/index.html` (fluxo completo) ou `app/checkin-exercicio.html`. |

O projeto do Wokwi é público: com senha no código, rode sem salvar.

## Credenciais e segurança

O broker fica na conta EMQX Cloud do Enzo, e as senhas só circulam por mensagem privada. Ninguém além dele precisa de acesso ao painel do EMQX.

| Usuário | Quem usa | Onde a senha fica |
| --- | --- | --- |
| `caneleira_g01` | ESP32 | `firmware/caneleira/segredos.h`, ignorado pelo Git |
| `geratrix_g01` | App no navegador | Arquivo `*.local.js`, ignorado pelo Git |
| `teste_g01` | MQTTX e Wokwi, nos testes | Digitada na hora, nunca salva em projeto público |

- **Peça a senha ao Enzo no privado**, nunca no grupo nem em issue do GitHub.
- **Cada conexão usa um Client ID diferente** (`caneleira-01`, `geratrix-web`, `mqttx-<seu nome>`). IDs repetidos derrubam uma das conexões.
- **A senha do `geratrix_g01` fica visível** para quem abrir o app: só dados fictícios no protótipo, e essa senha é trocada após a banca.
- **O certificado em `certificado_ca.h` é público** e pode ficar no repositório; segredo são as senhas.
- **Nenhum dado de pessoa real** entra no código, no broker, nos prints ou no relatório. A caneleira é anônima: o vínculo com o residente existe só dentro do app (LGPD).

## Regras que não mudam sem o grupo

Estas regras protegem a integração entre as partes e a nota; quem precisar mudá-las propõe no grupo antes.

- **Contrato MQTT:** tópicos, campos, unidades, QoS e retenção só mudam com nova versão do contrato e aval do Rafael. Caneleira e app quebram se um lado mudar sozinho.
- **Decisões fechadas D1 a D15** (seção 4 do guia vivo): exercício sentado com uma única IMU, resumo por série e não por repetição, caneleira anônima, prescrição definida pelo gestor, protótipo enxuto com power bank e sem solda, entre outras. Não se reabrem sem motivo técnico concreto.
- **Decisão local:** toda a lógica de contagem e classificação roda na ESP32 e funciona com o celular desligado. O app exibe, configura e comanda; não decide.
- **Linguagem:** o protótipo nunca é apresentado como dispositivo médico. Nada de "diagnóstico", "monitoramento clínico" ou "risco de queda", e os limites de teste (ADM 60°, 150 °/s) não são valores clínicos.
- **Evidência:** um resultado só vale com data, ambiente (simulação, bancada ou app) e registro (print, foto ou tabela). Mensagem digitada no MQTTX não é dado medido pela ESP32, e tela estática não é integração.

## Como trabalhar com uma IA

A IA acompanha o trabalho, mas quem entende, testa e aprova é o integrante. Por isso o método é de passo a passo, não de entrega pronta.

- **Dê contexto antes de pedir qualquer coisa:** anexe ou aponte os arquivos da ordem de leitura e cole o prompt abaixo no início da conversa.
- **Código em blocos:** peça um bloco por vez, com o arquivo e o ponto onde colar e a explicação do que ele faz. Cole, teste e só então peça o próximo. Assim você consegue explicar o código na banca.
- **Nada sobe sem você:** a IA mostra o que vai para o repositório, e o commit só acontece com a sua autorização.
- **Desconfie de "funcionou" sem evidência:** peça sempre o teste que comprova e onde ele rodou.

Prompt para colar no início da conversa:

```text
Você vai me acompanhar na continuação de um projeto acadêmico de IoT: a caneleira instrumentada do grupo g01 (Desenvolvimento IoT, Mackenzie, parceria IMREA, banca em 20/11/2026). Leia integralmente, nesta ordem: docs/como-continuar.md, README.md, docs/guia-continuidade.md, docs/contrato-mqtt-v1.md e o README da pasta em que vamos trabalhar.

Regras de trabalho:
1. Comece confirmando, em até cinco linhas, o estado atual e o próximo item da seção "O que fazer a seguir", e pergunte só o que bloquear o início.
2. Avance um passo por vez. Quando houver código, entregue-o em blocos pequenos, dizendo em qual arquivo e onde colar e explicando o que cada bloco faz; espere eu testar antes do próximo.
3. Siga os padrões do projeto: comentário curto acima da assinatura de cada função, constantes de calibração no topo do arquivo, laço principal sem delay() na amostragem.
4. Não altere o contrato MQTT nem as decisões fechadas; se houver motivo técnico para isso, aponte e sugira consultar o grupo.
5. Distinga sempre o que foi testado (e onde: computador, Wokwi, bancada ou app) do que é hipótese. Nunca peça nem exiba senhas.
6. Antes de qualquer commit ou push, mostre o que vai mudar e espere minha autorização explícita.
7. Ao fim de cada passo, liste em até três linhas o que guardar para o relatório (decisão, valor medido, evidência).
```

## Quem faz o quê

Divisão proposta ao grupo em 01/11/2026 e atualizada em 02/11/2026, com a entrada do Felipe.

| Integrante | Frente | Procure para |
| --- | --- | --- |
| Enzo Senatori | Coordenação, lógica do firmware, broker e relatório | Senhas do broker, prioridades, revisão do firmware |
| Rafael de Oliveira Corrêa | Hardware: compras, montagem e testes físicos; aval técnico do MQTT; operação sem rede (P11, 06/11) | Placa e peças, mudanças no contrato MQTT |
| Theo Casella Mendonça | App: integração MQTT no Geratrix e telas | Telas e código do app |
| Felipe (GitHub `felpszada`) | Firmware sem placa e documentação técnica: código do P12, esquema, lista de componentes, autonomia estimada e protocolo P14 | Operação sem rede, baixo consumo, documentação técnica |

Se alguém sair de uma frente, o substituto começa por este guia e pelo `README.md` da pasta correspondente; o histórico do que foi feito está nos commits e no guia vivo.
