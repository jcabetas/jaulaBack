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
#include "alimCalle.h"
#include "gets.h"
#include "chprintf.h"
#include "tty.h"
#include "w25q16/variables.h"
#include "w25q16.h"

void initSensores(void);
void initSerial(void);
void closeSerial(void);
void leeGPS(void);
void estadoDeseadoPuertaC(uint8_t *estDes, uint32_t *sec2change);
void abrePuertaC(void);
void cierraPuertaC(void);
int chprintf(BaseSequentialStream *chp, const char *fmt, ...);
void pruebaADC(void);
//void opciones(void);
//void sleepW25q16(void);
void leeVariablesC(void);
void printFechaC(char *buff, uint16_t longBuff);
void addDsC(int16_t dsAdd);
void printSerial(char *msg);
void opciones(void);
void iniciaSecAdaptacionC(void);
extern uint8_t enHora, hayUbicacion;
extern uint8_t GL_Flag_External_WakeUp;

/*
 * Alimentador de gatosvoid addDsC(int16_t dsAdd)
 *
 * BUCLE
 * Si es el primer arranque, pon RTC en hora con GPS
 * Para cualquier arranque
 * - leer variables
 * - lee la hora del RTC
 * - bucle:
 *      - Situar la puerta con la logica definida
 *      - Analizar segundos que faltan para cambiar de estado
 *      - Si el dia del año difiere, actualizar hora de amanecer
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
 * - Pin C13 (GPIOC_LED) es el led de la placa, activo Lowvoid opciones(void)
 * - Pin A0 (GPIOA_KEY) es pulsador KEY para despertar a la placa
 */

// Consumo: usando regulador HT7333 consigo 12 uA en placa

volatile uint8_t msDelayLed = 1;
volatile uint16_t numCuentas;
extern uint16_t autoPuerta; // 0:cerrada, 1:abierta, 2: automatico, 3: autoConMargen
extern int16_t dsAddPordia;
extern uint16_t posAbierto;
extern uint16_t posCerrado;
extern uint16_t autoPuerta; // 0:cerrada, 1:abierta, 2: automatico, 3: autoConMargen
extern uint32_t secAdaptacion;

void parpadear(uint8_t numVeces, uint16_t ms) {
    palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_OUTPUT_PUSHPULL);
    for (uint8_t i = 0; i < numVeces; i++) {
        palClearPad(GPIOC, GPIOC_LED); // enciende led placa
        chThdSleepMilliseconds(10);
        palSetPad(GPIOC, GPIOC_LED); // apaga led placa
        chThdSleepMilliseconds(ms);
    }
    palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_INPUT_ANALOG);
}

int main(void) {
    uint8_t estDes;
    uint32_t sec2change;
    char buffer[90], buff[50];

    halInit();
    chSysInit();
    initSerial();
    parpadear(2, 250);
    leeVariablesC();
    if (autoPuerta != 3)
        secAdaptacion = 0;
 //   leeGPS();
    printSerial("Inicializado\n\r");
    while (1 == 1) {
        leeVariablesC();
        estadoDeseadoPuertaC(&estDes, &sec2change);
        printFechaC(buff, sizeof(buff));
        chsnprintf(buffer, sizeof(buffer), "Fecha actual UTC: %s\n\r", buff);
        printSerial(buffer);
        chsnprintf(buffer, sizeof(buffer),
                   "Main, auto puerta:%d estado puerta:%d, cambio en %d s\n\r",
                   autoPuerta, estDes, sec2change);
        printSerial(buffer);
        if (estDes == 1)
            abrePuertaC();
        else
            cierraPuertaC();
        chsnprintf(buffer, sizeof(buffer), "A dormir %d s\n\r", sec2change);
        printSerial(buffer);
        ports_set_lowpower();
        stop(sec2change);
        addDsC(dsAddPordia);
        if (GL_Flag_External_WakeUp == 0) {
            parpadear(1, 100);
            printSerial("Timeout desde stop\n\r");
        }
        else
            opciones();
    }
    //  leeGPS(); // leer GPS
    //  while (!enHora) // espero a que termine
    //  {
    //      chThdSleepMilliseconds(200);
    //  }
}
