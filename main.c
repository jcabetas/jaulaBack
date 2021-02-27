/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"
#include "varsFlash.h"

//#include "portab.h"
//#include "usbcfg.h"

//#define ttyUSB (BaseSequentialStream *)&PORTAB_SDU1
//void initSerialUSB(void);
void pruebaADC(void);
void initW25q16(void);
void initSMS(void);
void initSensores(void);
void initAdc(void);

enum estado_t {esperoApertura=1, abiertoTemporizando, esperoGato, cayendoPuerta, atascada, gatoEnJaula, avisado};
extern enum estado_t estado;

/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {
  (void)arg;
  // numPulsos, interv. Por ejemplo 2,700 seria 100on,200off,100on,700off
  //enum estado_t {esperoApertura, abiertoTemporizando, esperoGato, cayendoPuerta, atascada, gatoEnJaula, avisado};
  //                  1,100             2,500            1,2900       1,100        3,500       2,2600      3,2300
  uint16_t numPulsos[] =     {  1,  1,  2,  1,   1,  3,   2,  3};
  uint16_t msEntrePulsos[] = {100,100,500,2900,100,500,2900,2900};
  chRegSetThreadName("blinker");
  while (true) {
      uint8_t estJaula = estado;
      if (estJaula>7)
          estJaula = 0;
      uint16_t numPuls = numPulsos[estJaula];
      uint16_t msEntrePuls = msEntrePulsos[estJaula];
      for (uint8_t numP=0;numP<numPuls;numP++)
      {
          palClearPad(GPIOC, GPIOC_LED); // enciende
          chThdSleepMilliseconds(100);   // mantiene 100 ms
          palSetPad(GPIOC, GPIOC_LED);   // apagado
          if (numP<numPuls-1)            // si no es el ultimo, deja apagado 200 ms
              chThdSleepMilliseconds(200);
      }
      chThdSleepMilliseconds(msEntrePuls);
  }
}

/*
 * Application entry point.
 */
int main(void) {
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();


  /*
   * Activates the serial driver 2 using the driver default configuration.
   */
  /*
   * Initializes serial-over-USB CDC drivers.
   */
//  sduObjectInit(&SDU1);
//  sduStart(&SDU1, &serusbcfg);

  /*
   * Activo USB
   */
//  initSerialUSB();
  initAdc();
  initSensores();
  initW25q16();
  initSMS();
  /*
   * Creates the blinker thread.
   */

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

//  sectorNumTelef = 1;
//  escribeStr50_C(&sectorNumTelef, "numTelef", "619262851");
//  leeStr50_C(&sectorNumTelef, "numTelef", "000000", numTlf);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    if (!palReadPad(GPIOA, GPIOA_KEY)) {
        chThdSleepMilliseconds(500);
        //pruebaADC();
    }
    chThdSleepMilliseconds(500);
  }
}
