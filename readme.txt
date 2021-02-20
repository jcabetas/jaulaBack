*****************************************************************************
** ChibiOS/RT port for ARM-Cortex-M4 STM32F411.                            **
*****************************************************************************

Datos en https://stm32-base.org/boards/STM32F411CEU6-WeAct-Black-Pill-V2.0.html

Conexiones:
- A7: ADC in (medida tensión batería)
- A2: T2TX (a RX SIM800L)
- A3: T2RX (a TX SIM800L)

w25q16
     * CS -   PA4
     * SCK -  PA5    SPI1_SCK
     * MISO - PA6    SPI1_MISO
     * MOSI - PA7    SPI1_MOSI
