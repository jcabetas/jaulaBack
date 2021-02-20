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
int16_t escribeStr50_C(uint16_t *sectorParam, const char *nombVar, char *valor);
int16_t leeStr50_C(uint16_t *sectorParam, const char *nombParam, const char *valorDefault, char *valor);
uint16_t sectorNumTelef;
/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palClearPad(GPIOC, GPIOC_LED);
    chThdSleepMilliseconds(1000);
    palSetPad(GPIOC, GPIOC_LED);
    chThdSleepMilliseconds(1000);
  }
}

/*
 * Application entry point.
 */
int main(void) {
    char numTlf[50];
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
  initW25q16();
  /*
   * Creates the blinker thread.
   */

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  sectorNumTelef = 1;
//  escribeStr50_C(&sectorNumTelef, "numTelef", "619262851");
  leeStr50_C(&sectorNumTelef, "numTelef", "000000", numTlf);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    if (!palReadPad(GPIOA, GPIOA_KEY)) {
        chThdSleepMilliseconds(500);
        pruebaADC();
    }
    chThdSleepMilliseconds(500);
  }
}
