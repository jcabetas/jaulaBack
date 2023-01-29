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
void leeGPS(void);
extern uint8_t enHora, hayUbicacion;

/*
 * Alimentador de gatos
 *
 * BUCLE
 * Si es el primer arranque, pon RTC en hora con GPS
 * Para cualquier arranque
 * - lee la hora del RTC
 * - Ver estado de sensores
 * - Leer mensajes del GPRS y si procede, enviar alarmas
 * - Actualizar estado del servo (si es de noche o no)
 * - Analizar segundos que faltan para cambiar de estado
 * - Lanzar stop con tiempo restante
 *
 * HARDWARE
 * - Blackpill
 * - Pin A0 Sensor 1
 * - Pin A1 Sensor 2
 * - Pin A2 (GPIOB_TX2) TX para GPRS
 * - Pin A3 (GPIOB_RX2) RX para GPRS
 * - Pin A9 (GPIOB_TX1) TX para GPS
 * - Pin A10 (GPIOB_RX1) RX para GPS
 * - Pin B0 (GPIOB_ONSERVO) low activa tension al servo
 * - Pin B1 (GPIOB_ONGPS) low activa GPS
 * - Pin B2 (GPIOB_ONGPRS) low activa GPRS
 * - Pin B8 (GPIOB_PWMSERVO) salida PWM al servo en TIM4CH2
 * - Pin C13 (GPIOC_LED) es el led de la placa, activo Low
 * - Pin A0 (GPIOA_KEY) es pulsador KEY para despertar a la placa
 */

// Consumo: 107 uA. Si quito la pila, baja a 76 uA (consumo por alimentacion a mosfet). Alimentando en 3.3V, baja a 22 uA

volatile uint8_t msDelayLed = 1;
volatile uint16_t numCuentas;


void parpadear(uint8_t numVeces,uint16_t ms)
{
    palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_OUTPUT_PUSHPULL);
    for (uint8_t i=0;i<numVeces;i++)
    {
      palClearPad(GPIOC, GPIOC_LED);    // enciende led placa
      chThdSleepMilliseconds(10);
      palSetPad(GPIOC, GPIOC_LED);    // apaga led placa
      chThdSleepMilliseconds(ms);
    }
    palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_INPUT_ANALOG);
}

int main(void) {
    uint8_t estDes;
    uint16_t sec2change;
  halInit();
  chSysInit();

  palSetPad(GPIOC, GPIOC_LED);
  parpadear(5,200);
  ports_set_lowpower();
  initServo();
  leeGPS(); // leer GPS
  while (!enHora) // espero a que termine
  {
      chThdSleepMilliseconds(200);
  }
  // vienes de despertar de un standby?
//  if ((PWR->CSR) && PWR_CSR_SBF)
//  {
//      parpadear(3,1000);
//  }
//  else
//  {
//      parpadear(4,1000);
//      leeGPS(); // lanza proceso para leer GPS
//      while (!enHora) // espero a que termine
//      {
//          chThdSleepMilliseconds(200);
//      }
//  }
  while (1)
  {
//      palClearPad(GPIOC, GPIOC_LED);    // enciende led placa
//      palClearPad(GPIOB, GPIOB_ONSERVO); // da tension al servo
//      mueveServoAncho(3000);            // abre tapa
//      chThdSleepMilliseconds(2000);
//      palSetPad(GPIOC, GPIOC_LED);    // apaga led placa
//      palSetPad(GPIOB, GPIOB_ONSERVO); // quita tension al servo
      parpadear(2,250);
      estadoDeseadoPuerta(&estDes, &sec2change);
      palClearPad(GPIOB, GPIOB_ONSERVO); // da tension al servo
      if (estDes==0)
          mueveServoPos(0);
      else
          mueveServoPos(100);
      // dale tiempo para posicionarse
      chThdSleepMilliseconds(2000);
      palSetPad(GPIOB, GPIOB_ONSERVO); // quita tension al servo
      estadoDeseadoPuerta(&estDes, &sec2change);
      stop(sec2change);
  }
}
