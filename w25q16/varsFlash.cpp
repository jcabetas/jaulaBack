/*
 * testFlash.c
 *
 *  Created on: 12/9/2019
 *      Author: jcabe
 */

/*
 *      Author: joaquin
 */

#include "ch.hpp"
#include "hal.h"

using namespace chibios_rt;

#include "w25q16.h"
#include "varsFlash.h"
#include "chprintf.h"
//#include "tty.h"


/*
 * Formato opcional:
 * - Cada sector (16 paginas) tiene una sola variable. Se indexa con la "posicion"
 *
 * Cada versi�n PLC
 * - Los primeros dos sectores guardan dos versiones PLC
 * - Comienzan con una variable uint16_t con valor 12345
 * - Otra variable uint16_t con longitud de datos
 * - Los datos empiezan en posici�n 4 (0-base)
 *
 * Cada sector siguiente:
 * - Tipo de variable: 0/FF (no ocupada) 1 (No/Si)  2 (No/Si/Auto) 3 (Baud) 4 (ModoPozo)
 *                                       5 (uint8) 6 (int8) 7 (uint16) 8 (int16) 9 (int32)
 *                                      10 (str50)
 * - Checksum (sum) del nombre uint16_t
 * - Nombre de variable (str16)
 * - 1 byte: posvalida 'J' o no (0x00)
 * - N bytes con el valor
 *   Si no es valido, continuar leyendo hasta 100 veces.
 *   A partir de ahi hay que borrar el sector entero y volver a grabar
 *
 * - La ventaja es que puedo ver todas las variables y comprobar valores
 */

//uint8_t longParam[] = {1,1,1,1,1,1,2,2,4,8,50};

uint8_t plcFlashOk(void)
{
  uint16_t varCheck, page, longPLC;
  uint8_t  sector;
  for (sector=0;sector<=1;sector++)
  {
      page = 16*sector;
      varCheck = W25Q16_read_u16(page, 0);
      if (varCheck==0xFFFF || varCheck!=0x1234)
          return 0;
      longPLC = W25Q16_read_u16(page, 2);
      if (longPLC==0xFFFF)
          return 0; // no hay nada grabado
  }
  return 1;
}




void initFlash(void)
{
  W25Q16_chipErase();
  // escribo datos de PLC
  for (uint8_t plcSector=0;plcSector<=1;plcSector++)
  {
    W25Q16_write_u16(16*plcSector, 0, 0x1234); // variable de control
    W25Q16_write_u16(16*plcSector, 2, 0);      // longitud del testo
  }
}




// devuelve 0 si no hay problemas
uint8_t flashOk(void)
{
    if (!plcFlashOk())
        return 0;
    for (uint16_t sector=2;sector<512;sector++)
    {
    }
    return 0;
}


void escriboFlash(uint16_t page, uint8_t pageAddress, uint16_t numero)
{
  uint16_t i;
  W25Q16_initStreamWrite(page, pageAddress);
  for (i=0;i<numero;i++)
  {
    W25Q16_streamWrite(&page, &pageAddress, 20+(i%50));
  }
  W25Q16_closeStreamWrite();
}
