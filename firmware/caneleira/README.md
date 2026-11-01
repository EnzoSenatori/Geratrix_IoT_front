# P3 — primeiro projeto Wokwi da caneleira

**P3 verificado em 13/10/2026.** [Abrir a simulação no Wokwi](https://wokwi.com/projects/477822826125465601). O projeto foi salvo como público porque a opção de link não listado exige plano pago. Não contém credenciais nem dados de pacientes.

Este projeto contém ESP32 DevKit V1 e MPU-6050 ligada por I²C: SDA no GPIO 21, SCL no GPIO 22, alimentação 3,3 V e GND. O programa agenda leituras a 100 Hz e imprime uma linha a cada 10 leituras. A leitura inicial estima o ângulo por acelerômetro; o plano/eixo real será confirmado no P4. Contagem de repetições, vibração e MQTT são funções reservadas para os próximos passos.

## Verificação no Wokwi

1. Criar um projeto ESP32, substituir `diagram.json` e `libraries.txt` pelos arquivos desta pasta e colar o conteúdo de `caneleira.ino` no `sketch.ino` do Wokwi. Na Arduino IDE, abrir `caneleira.ino` diretamente: a pasta já tem o nome que a IDE exige.
2. Iniciar a simulação. O monitor serial deve mostrar `P3 iniciado` e linhas com `dt_ms` próximo de `10`.
3. Abrir o controle da MPU-6050 virtual. Para simular aproximadamente 45°, ajustar `accelX=0.707`, `accelZ=0.707` (em g); para 90°, `accelX=1`, `accelZ=0`. O campo `angulo_acc_graus` deve mudar nessa direção.
4. Alterar `rotationY` para observar `vel_y_graus_s`. Esses valores simulados verificam o software, ainda não calibram a placa física.

**Resultado observado:** a simulação compilou e executou. Em repouso, `dt_ms` oscilou aproximadamente de 9,99 a 10,01 ms. Com `accelX=0,7 g` e `accelZ=0,7 g`, o monitor mostrou `45,0°`; com `rotationY=60°/s`, mostrou `60,0°/s`, sem atrasos registrados nesse ensaio curto. Assim, o critério de P3 foi cumprido em simulação. Esses números ainda não validam a precisão da caneleira física nem substituem a calibração do P4.

**Para o relatório:** guardar o link, o código e uma captura do monitor serial com os valores acima. O projeto público contém uma versão funcional equivalente do programa; os arquivos desta pasta incluem comentários adicionais para facilitar a continuidade.

Referências: [MPU-6050 no Wokwi](https://docs.wokwi.com/parts/wokwi-mpu6050) e [formato de `diagram.json`](https://docs.wokwi.com/diagram-format).
