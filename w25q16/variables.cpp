/*
 * variables.cpp
 *
 *  Created on: 11 mar 2023
 *      Author: joaquin
 */

#include "ch.hpp"
#include "hal.h"

using namespace chibios_rt;

#include <w25q16/w25q16.h>
#include "variables.h"
#include "string.h"

/*
    #define POS_ADDAMANECER     2
    #define POS_ADDATARDECER    4
    #define POS_AUTOPUERTA      6
    #define POS_MARGENADAPT     8
    #define POS_DSADDPORDIA    10
    #define POS_POSABIERTO     12
    #define POS_POSCERRADO     14
 */

int16_t addAmanecer;
int16_t addAtardecer;
uint16_t autoPuerta;  // 0:cerrada, 1:abierta, 2: automatico, 3: autoConMargen
int16_t dsAddPordia;
uint16_t posAbierto;
uint16_t posCerrado;

extern "C" {
    void leeVariablesC(void);
}

static const SPIConfig spicfg = {
    .circular         = false,
    .slave            = false,
    .data_cb          = NULL,
    .error_cb         = NULL,
    .ssport           = GPIOA,
    .sspad            = GPIOA_W25Q16_CS,
    .cr1              = SPI_CR1_BR_1 | SPI_CR1_CPOL | SPI_CR1_CPHA,
    .cr2              = 0U
};


/*
 * Si la FLASH es valida, los dos primeros bytes seran 0x7851
 *
 */
void reseteaEEprom(void)
{
  W25Q16_sectorErase(0);
  W25Q16_write_i16(0, POS_ADDAMANECER, 30);
  W25Q16_write_i16(0, POS_ADDATARDECER, 30);
  W25Q16_write_u16(0, POS_AUTOPUERTA, 2);
  W25Q16_write_i16(0, POS_DSADDPORDIA, 0);
  W25Q16_write_u16(0, POS_POSABIERTO, 95);
  W25Q16_write_u16(0, POS_POSCERRADO, 0);
  W25Q16_write_u16(0, 0, 0x7851);
}


void leeVariables(void)
{
  initW25q16();
  uint16_t claveEEprom = W25Q16_read_u16(0, 0);
  if (claveEEprom != 0x7851)
      reseteaEEprom();
  addAmanecer = W25Q16_read_i16(0, POS_ADDAMANECER);
  addAtardecer = W25Q16_read_i16(0, POS_ADDATARDECER);
  autoPuerta = W25Q16_read_u16(0, POS_AUTOPUERTA);
  dsAddPordia = W25Q16_read_i16(0, POS_DSADDPORDIA);
  posAbierto = W25Q16_read_u16(0, POS_POSABIERTO);
  posCerrado = W25Q16_read_u16(0, POS_POSCERRADO);
  sleepW25q16();
}

void escribeVariables(void)
{
  initW25q16();
  W25Q16_sectorErase(0);
  W25Q16_write_i16(0, POS_ADDAMANECER, addAmanecer);
  W25Q16_write_i16(0, POS_ADDATARDECER, addAtardecer);
  W25Q16_write_u16(0, POS_AUTOPUERTA, autoPuerta);
  W25Q16_write_i16(0, POS_DSADDPORDIA, dsAddPordia);
  W25Q16_write_u16(0, POS_POSABIERTO, posAbierto);
  W25Q16_write_u16(0, POS_POSCERRADO, posCerrado);
  W25Q16_write_u16(0, 0, 0x7851);
  sleepW25q16();
  leeVariables();
}

void leeVariablesC(void)
{
  leeVariables();
}

void sleepW25q16(void)
{
    W25Q16_powerDown();
    spiStop(&SPID1);
    palSetPadMode(GPIOA, GPIOA_SPI1_SCK, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOA, GPIOA_SPI1_MOSI, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOA, GPIOA_SPI1_MISO, PAL_MODE_INPUT_ANALOG);
}

void initW25q16(void)
{
    // defino los pines
    /*
     * CS -   PA4
     * SCK -  PA5    SPI1_SCK
     * MISO - PA6    SPI1_MISO
     * MOSI - PA7    SPI1_MOSI
     */
    palSetPad(GPIOA, GPIOA_W25Q16_CS);
    palSetPadMode(GPIOA, GPIOA_W25Q16_CS, PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(GPIOA, GPIOA_SPI1_SCK);
    palClearPad(GPIOA, GPIOA_SPI1_MOSI);
    palClearPad(GPIOA, GPIOA_SPI1_MISO);

    palSetLineMode(LINE_A5_W25Q16_CLK,
                     PAL_MODE_ALTERNATE(5) |
                     PAL_STM32_OSPEED_HIGHEST);         /* SPI SCK.             */
    palSetLineMode(LINE_A6_W25Q16_MISO,
                     PAL_MODE_ALTERNATE(5) |
                     PAL_STM32_OSPEED_HIGHEST);         /* MISO.                */
    palSetLineMode(LINE_A7_W25Q16_MOSI,
                     PAL_MODE_ALTERNATE(5) |
                     PAL_STM32_OSPEED_HIGHEST);         /* MOSI.                */
    spiStart(&SPID1, &spicfg);
    W25Q16_init();
}
