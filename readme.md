# Alimentador de gatos

## Sistema operativo
ChibiOS/RT port for ARM-Cortex-M4 STM32F411.

## uControlador
Datos en https://stm32-base.org/boards/STM32F411CEU6-WeAct-Black-Pill-V2.0.html

## Conexiones
- A1: Sensor 1
- A2: Sensor 2
- A2: TX2 (a RX Configuracion)
- A3: RX2 (a TX Configuracion)
- A9: TX1 (a GPS)
- A10: RX1 (a GPS)
- B0: ADC in divide Ubat por 220/(220+91)
- B8: PWM para Servo
- B13: ONGPS
- B14: ONSERVO
- B15: ON-AD


w25q16
- CS -   PA4
- SCK -  PA5    SPI1_SCK
- MISO - PA6    SPI1_MISO
- MOSI - PA7    SPI1_MOSI

## Parametros
* Las configuraciones se guardan en flash