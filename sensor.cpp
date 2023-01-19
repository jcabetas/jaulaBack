/*
 * sensores.cpp
 *
 *  Created on: 21 feb. 2021
 *      Author: joaquin
 */


#include "ch.hpp"
#include "hal.h"
#include "alimCalle.h"
using namespace chibios_rt;
#include "sms.h"

extern "C"
{
    void initSensores(void);
}

static virtual_timer_t activaGate_vt;
static void activaGate_cb(void *arg);

static virtual_timer_t apagaGate_vt;
static void apagaGate_cb(void *arg);

// apaga la puerta
static void apagaGate_cb(void *arg) {
    (void)arg;

}

// llega el tiempo de activar puerta
static void activaGate_cb(void *arg) {
    (void)arg;
//    palSetPad(GPIOA, GPIOA_GATETRIAC);     // enciende gate Triac
//    chSysLockFromISR();
//    chVTSetI(&apagaGate_vt, OSAL_US2I(500), apagaGate_cb, NULL);
//    chSysUnlockFromISR();
}

uint16_t usRetraso;
/*
 * Interrupcion ZC
 */
extern volatile uint8_t msDelayLed;
extern volatile uint16_t numCuentas;
static void zc_cb(void *)
{
//    palClearPad(GPIOA, GPIOA_GATETRIAC);  // apaga gate Triac
//    if (palReadPad(GPIOA, GPIOA_ZC))
//    {
//        palSetPad(GPIOB, GPIOB_LEDROJO);    // apaga led rojo
////        palClearPad(GPIOA, GPIOA_GATETRIAC);  // apaga gate Triac
//    }
//    else
//    {
//      palClearPad(GPIOB, GPIOB_LEDROJO);    // enciende led rojo
//    }
    numCuentas++;
    chSysLockFromISR();
    chVTSetI(&activaGate_vt, OSAL_US2I(usRetraso), activaGate_cb, NULL);
    chSysUnlockFromISR();
}



/*
 * PA8  - GPIOA_GATETRIAC        (output push-pull) Salida puerta triac
 * PA9  - GPIOA_ZC               (input pullup).
 * PB12 - GPIOB_LEDROJO          (output push-pull)
 */
void initSensores(void)
{
    chVTObjectInit(&activaGate_vt);
    chVTObjectInit(&apagaGate_vt);

    usRetraso = 1000;

//    palSetPadMode(GPIOA, GPIOA_ZC, PAL_MODE_INPUT);
//    uint8_t estadoPin = palReadPad(GPIOA, GPIOA_ZC);
//
//    palSetPadMode(GPIOB, GPIOB_LEDROJO, PAL_MODE_OUTPUT_PUSHPULL);
//    palSetPad(GPIOB, GPIOB_LEDROJO);    // apaga led rojo
//
//    palSetPadMode(GPIOA, GPIOA_GATETRIAC, PAL_MODE_OUTPUT_PUSHPULL);
//    palSetPad(GPIOA, GPIOA_GATETRIAC);    // apaga triac

    // habilito gestion de cambios en sensores
    // con rising, el intervalo es 0,0097s
    //palEnableLineEvent(LINE_A9_ZC, PAL_EVENT_MODE_BOTH_EDGES);//PAL_EVENT_MODE_RISING_EDGE);
    //palSetLineCallback(LINE_A9_ZC, zc_cb, (void *)1);
}
