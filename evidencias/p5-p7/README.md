# Evidências do P5 a P7 — contagem e classificação em simulação

**Data:** 01/11/2026 · **Ambiente:** Wokwi (ESP32 DevKit V1 virtual), firmware em modo simulado · **Responsável:** Enzo

| Arquivo | O que mostra |
|---|---|
| `wokwi-sessao-concluida.png` | Fim da sessão no monitor serial após 3 min 22 s de simulação: na 3ª série, a 8ª repetição sai fora por ADM (49,9°), a 9ª fora por velocidade (220,0 °/s) e a 10ª válida; o resumo final traz 7 válidas, 2 desvios de ADM e 1 de velocidade em cada uma das três séries, com `status` `concluida` |

Os valores coincidem com os do teste no computador (`firmware/testes/teste_processamento.cpp`), o que confirma que a lógica se comporta igual no compilador da ESP32. Dados fictícios, gerados pelo roteiro de `firmware/caneleira/sinal_simulado.h`; não validam a placa real.
