# Evidências do P1 — teste manual do contrato MQTT

**Data:** 12/10/2026 · **Ambiente:** MQTTX Web conectado ao EMQX Cloud por WebSocket seguro · **Responsável:** Rafael

Mensagens fictícias, digitadas à mão. As capturas comprovam o broker e o formato das mensagens; não envolvem a ESP32 nem o Geratrix.

| Arquivo | O que mostra |
|---|---|
| `1-comando.png` | Publicação e recebimento de `cmd` com a prescrição |
| `2-estado.png` | Publicação de `estado` com retenção ativada |
| `3-retain.png` | Nova assinatura recebe o `estado` marcado como retido |
| `4-sessao.png` | Publicação e recebimento de `sessao/s7f3a` |
| `5-ack.png` | Publicação e recebimento de `ack`, após `estado` offline retido |
| `6-limpeza-retained.png` | Painel do EMQX: só `estado` permanece retido, após a remoção da sessão de teste |
