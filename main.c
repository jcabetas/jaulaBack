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
#include "alimCalle.h"

/*
 * Prueba de alimentador
 *
 * BUCLE
 * - Enciende led placa
 * - Abre tapa (servo=3000)
 * - Espera 3s y apaga el led y el servo
 * - Pone a dormir el STM32 por 30s, o hasta que se pulse KEY en la placa
 * - Enciende el led y el servo
 * - Cierra tapa (servo=6000)
 * - Apaga led y servo
 * - Pone a dormir el STM32 por 30s, o hasta que se pulse KEY en la placa
 *
 * HARDWARE
 * - Blackpill
 * - Pin B4 (GPIOB_MOSFET) unido con resistencia a gate del MOSFET-P, activo Low
 * - Pin B6 (GPIOB_PWMSERVO) salida PWM al servo
 * - Pin C13 (GPIOC_LED) es el led de la placa, activo Low
 * - Pin A0 (GPIOA_KEY) es pulsador KEY para despertar a la placa
 * - Mosfet-P Source a la batería
 * - Mosfet-P drenador al + del servo
 *
 */
int main(void) {

  halInit();
  chSysInit();

  // prueba sleep
  ports_set_normal();
  palClearPad(GPIOC, GPIOC_LED);    // enciende led placa
  chThdSleepMilliseconds(1000);
  palSetPad(GPIOC, GPIOC_LED);    // apaga led placa
  uint8_t pinA0 = palReadPad(GPIOA, GPIOA_KEY);
  //ports_set_lowpower();
  sleep_for_x_sec(10);
  palClearPad(GPIOC, GPIOC_LED);    // enciende led placa
  chThdSleepMilliseconds(1000);

  // Otra vez
  palSetPad(GPIOC, GPIOC_LED);    // apaga led placa
  pinA0 = palReadPad(GPIOA, GPIOA_KEY);
 // ports_set_lowpower();
  sleep_for_x_sec(10);
  palClearPad(GPIOC, GPIOC_LED);    // enciende led placa
  chThdSleepMilliseconds(1000);


  initServo();
  palSetPad(GPIOC, GPIOC_LED);    // apaga led placa
  palSetPad(GPIOB, GPIOB_MOSFET); // quita tension al servo
  chThdSleepMilliseconds(2000);

  while (1)
  {
    palClearPad(GPIOC, GPIOC_LED);    // enciende led placa
    palClearPad(GPIOB, GPIOB_MOSFET); // da tension al servo
    mueveServoAncho(3000);            // abre tapa

    chThdSleepMilliseconds(2000);
    palSetPad(GPIOC, GPIOC_LED);    // apaga led placa
    palSetPad(GPIOB, GPIOB_MOSFET); // quita tension al servo

    sleep_for_x_sec(5);
    //chThdSleepMilliseconds(3000);

    palClearPad(GPIOC, GPIOC_LED);    // enciende led placa
    palClearPad(GPIOB, GPIOB_MOSFET); // da tension al servo
    mueveServoAncho(6000);            // cierra tapa
    chThdSleepMilliseconds(3000);
    palSetPad(GPIOC, GPIOC_LED);    // apaga led placa
    palSetPad(GPIOB, GPIOB_MOSFET); // quita tension al servo

    sleep_for_x_sec(5);
    //chThdSleepMilliseconds(3000);
  }
}
