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

/*
 * Sensores: jaulaOpened y jaulaClosed (1 estado activo)
 * Maquina de estados:
 * - esperoApertura: estado inicial, debo esperar a jaulaOpened && !jaulaClosed
 * - abiertoTemporizando: durante 5s, si hay cambios vuelvo a esperoApertura
 * - esperoGato: la jaula esta abierta, espero durmiendo a !jaulaOpened
 * - cayendoPuerta: lanzo temporizador 5s y espero a jaulaClosed.
 *   Si no se ha activado jaulaClosed, lanzar alarma y pasa a atascada
 *   Si jaulaOpen & !jaulaClosed, debio ser un mal contacto y volver a esperoGato
 *   Si jaulaClosed, pasa a gatoEnJaula
 * - atascada: si jaulaClosed, lanza alarma de mal cierre y pasa a avisando
 * - gatoEnJaula: lanza aviso de cierre y pasa a avisado
 * - avisado: duerme
 */


enum estado_t {esperoApertura=1, abiertoTemporizando, esperoGato, cayendoPuerta, atascada, gatoEnJaula, avisado};

thread_t *procesoSensor;
event_source_t jaulaint_event;
event_listener_t elJaulaChanged;
uint8_t estadoJaulaClosed, estadoJaulaOpened, estadoEnableEventJaulaClosed, estadoEnableEventJaulaOpened;
enum estado_t estado;


static virtual_timer_t antiReboteClosed_vt, antiReboteOpened_vt, tempApertura_vt, tempCayendo_vt;
static void jaulaChangedClosed_cb(void *arg);
static void jaulaChangedOpened_cb(void *arg);


uint8_t actualizaEstados(void)
{
    uint8_t estOpen, estClose, hayCambios;
    hayCambios = 0;
    estClose = palReadPad(GPIOA, GPIOA_JAULACLOSED);
    if (estClose!=estadoJaulaClosed)
    {
        estadoJaulaClosed = estClose;
        hayCambios = 1;
    }
    estOpen = palReadPad(GPIOA, GPIOA_JAULAOPENED);
    if (estOpen!=estadoJaulaOpened)
    {
        estadoJaulaOpened = estOpen;
        hayCambios = 1;
    }
    return hayCambios;
}

static void tempOffCayendo_cb(void *arg)
{
    (void)arg;
    chSysLockFromISR();
    chEvtBroadcastI(&jaulaint_event);
    chSysUnlockFromISR();
}

static void tempOffApertura_cb(void *arg)
{
    (void)arg;
    chSysLockFromISR();
    chEvtBroadcastI(&jaulaint_event);
    chSysUnlockFromISR();
}

void trataCambios(void)
{
    switch (estado)
    {
        case esperoApertura:
            if (estadoJaulaOpened && !estadoJaulaClosed)
            {
                estado = abiertoTemporizando;
                chVTSet(&tempApertura_vt, OSAL_MS2I(5000), tempOffApertura_cb, NULL);
            }
            break;
        case abiertoTemporizando:
            if (estadoJaulaOpened && !estadoJaulaClosed)
                estado = esperoGato;
            else
                estado = esperoApertura;
            break;
        case esperoGato:
            if (!estadoJaulaOpened)
            {
                if (estadoJaulaClosed)
                    estado = gatoEnJaula;
                else
                {
                    estado = cayendoPuerta;
                    chVTSet(&tempCayendo_vt, OSAL_MS2I(5000), tempOffCayendo_cb, NULL);
                }
            }
            break;
        case cayendoPuerta:
            if (!estadoJaulaOpened && estadoJaulaClosed)
            {
                estado = gatoEnJaula;
                chVTReset(&tempCayendo_vt); // ya no hace falta esperar a que caiga
            }
            break;
        case atascada: // a buenas horas cierras o haces algún cambio
            break;
        case gatoEnJaula:
            break;
        case avisado:
            break;
      }
}



// restaura eventos Jaula
static void activaEventosJaulaClosed_cb(void *arg) {
    (void)arg;
   if (estadoEnableEventJaulaClosed==0)
   {
       estadoEnableEventJaulaClosed = 1;
       palEnableLineEventI(LINE_A8_JAULACLOSED, PAL_EVENT_MODE_BOTH_EDGES);
       palSetLineCallbackI(LINE_A8_JAULACLOSED, jaulaChangedClosed_cb, (void *)1);
   }
   chSysLockFromISR();
   chEvtBroadcastI(&jaulaint_event);
   chSysUnlockFromISR();
}
static void activaEventosJaulaOpened_cb(void *arg) {
    (void)arg;
   if (estadoEnableEventJaulaOpened==0)
   {
       estadoEnableEventJaulaOpened = 1;
       palEnableLineEventI(LINE_A9_JAULAOPENED, PAL_EVENT_MODE_BOTH_EDGES);
       palSetLineCallbackI(LINE_A9_JAULAOPENED, jaulaChangedOpened_cb, (void *)2);
   }
   chSysLockFromISR();
   chEvtBroadcastI(&jaulaint_event);
   chSysUnlockFromISR();
}


/*
 * Interrupciones sensores
 */
static void jaulaChangedClosed_cb(void *)
{
//    int sensorChanged = (int) arg;
    chSysLockFromISR();
    if (estadoEnableEventJaulaClosed == 1)
    {
        estadoEnableEventJaulaClosed = 0;
        palDisableLineEventI (LINE_A8_JAULACLOSED);
        chVTSetI(&antiReboteClosed_vt, OSAL_MS2I(50), activaEventosJaulaClosed_cb, NULL);
    }
    chSysUnlockFromISR();
}
static void jaulaChangedOpened_cb(void *)
{
    chSysLockFromISR();
    if (estadoEnableEventJaulaOpened == 1)
    {
        estadoEnableEventJaulaOpened = 0;
        palDisableLineEventI (LINE_A9_JAULAOPENED);
        chVTSetI(&antiReboteOpened_vt, OSAL_MS2I(50), activaEventosJaulaOpened_cb, NULL);
    }
    chSysUnlockFromISR();
}




/*
 * sensores thread.
 * gestiona interrupciones de los sensores
 */
static THD_WORKING_AREA(sensorint_wa, 128);
static THD_FUNCTION(sensorint, p) {
  (void)p;
  eventmask_t evt;
  chRegSetThreadName("sensorint");
  estadoEnableEventJaulaClosed = 1;
  estadoEnableEventJaulaOpened = 1;
  estado = esperoApertura;
  // habilito gestion de cambios en sensores
  palEnableLineEvent(LINE_A8_JAULACLOSED, PAL_EVENT_MODE_BOTH_EDGES);
  palSetLineCallback(LINE_A8_JAULACLOSED, jaulaChangedClosed_cb, (void *)1);
  palEnableLineEvent(LINE_A9_JAULAOPENED, PAL_EVENT_MODE_BOTH_EDGES);
  palSetLineCallback(LINE_A9_JAULAOPENED, jaulaChangedOpened_cb, (void *)2);
  // notificacion de que ha habido cambios
  chEvtRegister(&jaulaint_event, &elJaulaChanged, 1);
  while(!chThdShouldTerminateX()) {
    evt = chEvtWaitAnyTimeout(ALL_EVENTS, TIME_S2I(100));
    uint8_t hayCambios = actualizaEstados();
    if (evt==0 && hayCambios) // timeout, chequea estados
        trataCambios();
    if (evt & EVENT_MASK(1)) // jaula
        trataCambios();
    if (estado==gatoEnJaula)
    {
        // envia mensaje
        estado = avisado;
    }
    if (estado==atascada)
    {
        // envia mensaje
        estado = avisado;
    }
  }
  chEvtUnregister(&jaulaint_event, &elJaulaChanged);
  procesoSensor = NULL;
  chThdExit((msg_t) 1);
}



/*
 * PA8  - GPIOA_JAULA               (input pullup).
 * PA9  - GPIOA_CONFIG              (input pullup).
 */
void initSensores(void)
{
    chVTObjectInit(&antiReboteClosed_vt);
    chVTObjectInit(&antiReboteOpened_vt);
    chVTObjectInit(&tempApertura_vt);
    chVTObjectInit(&tempCayendo_vt);
    chEvtObjectInit(&jaulaint_event);

    palSetPadMode(GPIOA, GPIOA_JAULACLOSED, PAL_MODE_INPUT);
    estadoJaulaClosed = palReadPad(GPIOA, GPIOA_JAULACLOSED);
    palSetPadMode(GPIOA, GPIOA_JAULAOPENED, PAL_MODE_INPUT);
    estadoJaulaOpened = palReadPad(GPIOA, GPIOA_JAULAOPENED);

    if (!procesoSensor)
        procesoSensor = chThdCreateStatic(sensorint_wa, sizeof(sensorint_wa), NORMALPRIO + 7,  sensorint, NULL);

}
