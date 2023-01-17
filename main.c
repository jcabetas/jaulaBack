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
#include "lcd.h"


void initSensores(void);
void leeAlGPS(void);
extern uint8_t enHora, hayUbicacion;

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
 * - Si hay interrupcion sensor, activa led rojo (B5) por 2 ms
 *
 * HARDWARE
 * - Blackpill
 * - Pin B4 (GPIOB_MOSFET) unido con resistencia a gate del MOSFET-P, activo Low
 * - Pin B6 (GPIOB_PWMSERVO) salida PWM al servo
 * - Pin PB8 (GPIOB_I2C1SCL) para LCD, ALT04
 * - Pin PB9 (GPIOB_I2C1SDA) para LCD, ALT04
 * - Pin C13 (GPIOC_LED) es el led de la placa, activo Low
 * - Pin A0 (GPIOA_KEY) es pulsador KEY para despertar a la placa
 * - Mosfet-P Source a la batería
 * - Mosfet-P drenador al + del servo
 * - Pin A8 (GPIOA_GATETRIAC) TIM1CH1 a puerta de triac
 * - Pin A9 (GPIOA_ZC) Sensor paso por cero, activo bajo
 */

volatile uint8_t msDelayLed = 1;
volatile uint16_t numCuentas;
extern uint16_t usRetraso;
uint16_t p2us[21] = {9200, 8564, 7951, 7468, 7048, 6666, 6309, 5969, 5640, 5318, 5000, 4681, 4359, 4030, 3690, 3333, 2951, 2531, 2048, 1435, 100 };


///*
// * This is a periodic thread that does absolutely nothing except flashing
// * a LED.
// */
//static THD_WORKING_AREA(waThread1, 128);
//static THD_FUNCTION(Thread1, arg) {
//  (void)arg;
//  uint16_t msDelay = 200;
//  chRegSetThreadName("blinker");
//  while (true) {
//      if (enHora==1)
//          msDelay = 4700;
//      else
//          msDelay = 500;
//    palSetPad(GPIOC, GPIOC_LED);
//    chThdSleepMilliseconds(msDelay);
//    palClearPad(GPIOC, GPIOC_LED);
//    chThdSleepMilliseconds(1);
//    if (hayUbicacion)
//    {
//        palSetPad(GPIOC, GPIOC_LED);
//        chThdSleepMilliseconds(299);
//        palClearPad(GPIOC, GPIOC_LED);
//        chThdSleepMilliseconds(1);
//    }
//    else
//        chThdSleepMilliseconds(300);
//  }
//}

int main(void) {
  halInit();
  chSysInit();

  // chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
//  ports_set_lowpower();
  leeAlGPS(); // leer GPS
  while (!enHora) // espero a que termine
  {
      chThdSleepMilliseconds(200);
  }
  initServo();
  // vienes de despertar de un standby?
  if ((PWR->CSR) && PWR_CSR_SBF)
  {
    palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_OUTPUT_PUSHPULL);
    for (uint8_t i=0;i<10;i++)
    {
      palClearPad(GPIOC, GPIOC_LED);    // enciende led placa
      chThdSleepMilliseconds(200);
      palSetPad(GPIOC, GPIOC_LED);    // apaga led placa
      chThdSleepMilliseconds(200);
    }
    palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_INPUT_ANALOG);
  }
  else
  {
      leeAlGPS(); // lanza proceso para leer GPS
      while (!enHora) // espero a que termine
      {
          chThdSleepMilliseconds(200);
      }
  }
  while (1)
  {
      palClearPad(GPIOC, GPIOC_LED);    // enciende led placa
      palClearPad(GPIOB, GPIOB_ONSERVO); // da tension al servo
      mueveServoAncho(3000);            // abre tapa
      chThdSleepMilliseconds(2000);
      palSetPad(GPIOC, GPIOC_LED);    // apaga led placa
      palSetPad(GPIOB, GPIOB_ONSERVO); // quita tension al servo
      stop(8);
      initServo();

      palClearPad(GPIOC, GPIOC_LED);    // enciende led placa
      palClearPad(GPIOB, GPIOB_ONSERVO); // da tension al servo
      mueveServoAncho(5000);            // cierra tapa
      chThdSleepMilliseconds(2000);
      palSetPad(GPIOC, GPIOC_LED);    // apaga led placa
      palSetPad(GPIOB, GPIOB_ONSERVO); // quita tension al servo
      stop(8);
      initServo();
  }
}
