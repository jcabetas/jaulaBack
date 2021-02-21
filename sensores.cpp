/*
 * sensores.cpp
 *
 *  Created on: 21 feb. 2021
 *      Author: joaquin
 */


#include "ch.hpp"
#include "hal.h"
using namespace chibios_rt;

extern "C"
{
    void initSensores(void);
}

thread_t *procesoSensor;
event_source_t jaulaint_event;
event_listener_t elJaula;
uint8_t estadoJaula, estadoEnableEventJaula;

static virtual_timer_t antiRebote_vt;
static void jaula_cb(void *arg);



// restaura eventos Jaula
static void activaEventosJaula_cb(void *arg) {
    (void)arg;
   uint8_t estado = palReadPad(GPIOA, GPIOA_JAULA);
   if (estadoJaula != estado)
   {
       estadoJaula = estado;
   }
   if (estadoEnableEventJaula==0)
   {
       estadoEnableEventJaula = 1;
       palEnableLineEventI(LINE_A8_JAULA, PAL_EVENT_MODE_BOTH_EDGES);
       palSetLineCallbackI(LINE_A8_JAULA, jaula_cb, NULL);
   }
}

/*
 * Interrupciones sensores
 */
static void jaula_cb(void *arg) {
  (void)arg;
  uint8_t estado = palReadPad(GPIOA, GPIOA_JAULA);
  chSysLockFromISR();
  if (estadoJaula != estado)
  {
      estado = estadoJaula;
  }
  if (estadoEnableEventJaula==1)
  {
      estadoEnableEventJaula = 0;
      palDisableLineEventI(LINE_A8_JAULA);
      chVTSetI(&antiRebote_vt, OSAL_MS2I(50), activaEventosJaula_cb, NULL);
  }
  chEvtBroadcastI(&jaulaint_event);
  chSysUnlockFromISR();
}



/*
 * sensores thread.
 * gestiona interrupciones de los sensores
 */
static THD_WORKING_AREA(sensorint_wa, 128);
static THD_FUNCTION(sensorint, p) {
  (void)p;
  uint8_t estadoThr;
  eventmask_t evt;
  chRegSetThreadName("sensorint");
  estadoEnableEventJaula = 1;
  palEnableLineEvent(LINE_A8_JAULA, PAL_EVENT_MODE_BOTH_EDGES);
  palSetLineCallback(LINE_A8_JAULA, jaula_cb, NULL);
  chEvtRegister(&jaulaint_event, &elJaula, 0);
  while(!chThdShouldTerminateX()) {
    evt = chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(1000));
    if (evt==0) // timeout, chequea estados
    {
        if (estadoJaula!=palReadPad(GPIOA, GPIOA_JAULA))
        {
            estadoJaula = palReadPad(GPIOA, GPIOA_JAULA);
            // comprueba si hay que enviar mensaje
        }
    }
    if (evt & EVENT_MASK(0)) // jaula
    {
        estadoThr = estadoJaula;
        // ha cambiado de estado.
        // Para evitar rebotes desactiva eventos 100 ms y lee al final el nuevo estado
        // comprueba si hay que enviar mensaje
    }
  }
  chEvtUnregister(&jaulaint_event, &elJaula);
  procesoSensor = NULL;
  chThdExit((msg_t) 1);
}



/*
 * PA8  - GPIOA_JAULA               (input pullup).
 * PA9  - GPIOA_CONFIG              (input pullup).
 */
void initSensores(void)
{
    palSetPadMode(GPIOA, GPIOA_JAULA, PAL_MODE_INPUT);
    estadoJaula = palReadPad(GPIOA, GPIOA_JAULA);
    chVTObjectInit(&antiRebote_vt);

    palSetPadMode(GPIOA, GPIOA_CONFIG, PAL_MODE_INPUT);
    palEnablePadEvent(GPIOA, GPIOA_CONFIG, PAL_EVENT_MODE_BOTH_EDGES);

    if (!procesoSensor)
        procesoSensor = chThdCreateStatic(sensorint_wa, sizeof(sensorint_wa), NORMALPRIO + 7,  sensorint, NULL);

}
