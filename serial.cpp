/*
 * serial.cpp
 *
 *  Created on: 9 dic. 2022
 *      Author: joaquin
 */

#include "ch.hpp"
#include "hal.h"
using namespace chibios_rt;

#include "tty.h"
#include "string.h"
#include <stdlib.h>
#include "chprintf.h"

extern "C" {
void initSerial(void);
}
void ajustaP(uint16_t porcP);

extern uint16_t pObjetivo;
extern uint16_t hayTension;

static const SerialConfig ser_cfg = {115200, 0, 0, 0, };

thread_t *comProcess;

/*
 * Escucha para eventos de cambio de potencia (p.e. P=30). Devuelve el valor para confirmar recepcion
 * Envia estado de deteccion de impulsos (Z=1) cada segundo
 */

static THD_WORKING_AREA(wathreadCom, 3500);

static THD_FUNCTION(threadCom, arg) {
    (void)arg;
    eventmask_t evt;
    eventflags_t flags;
    uint8_t huboTimeout;
    uint16_t ms;
    event_listener_t receivedData;
    char buffer[20];
    chRegSetThreadName("nextionCom");

    ms = 0;
    chEvtRegisterMaskWithFlags (chnGetEventSource(&SD1),&receivedData, EVENT_MASK (1),CHN_INPUT_AVAILABLE);
    while (true)
    {
        evt = chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(100));
        if (chThdShouldTerminateX())
        {
            chEvtUnregister(chnGetEventSource(&SD1), &receivedData);
            chThdExit((msg_t) 1);
        }
        if (evt & EVENT_MASK(1)) // Algo ha entrado desde SD1
        {
            flags = chEvtGetAndClearFlags(&receivedData);
            if (flags & CHN_INPUT_AVAILABLE)
            {
                chgetsNoEchoTimeOut((BaseChannel *)&SD1, (uint8_t *) buffer, sizeof(buffer), TIME_MS2I(100),&huboTimeout);
                if (huboTimeout)
                    continue;
                if (buffer[0]=='P' && buffer[1]==':' && strlen(buffer)<=6)
                {
                    uint16_t Penviado = atoi(&buffer[2]);
                    if (Penviado<=100)
                    {
                        ajustaP(Penviado);
                        chprintf((BaseSequentialStream *)&SD1,"P:%d\n\r",pObjetivo);
                    }
                }
            }
        }
        ms += 100;
        if (ms>=2000)
        {
            ms =0;
            chprintf((BaseSequentialStream *)&SD1,"V:%d\n",hayTension);
        }
    }
}

void initSerial(void) {
    palClearPad(GPIOA, GPIOA_TX1);
    palSetPad(GPIOA, GPIOA_RX1);
    palSetPadMode(GPIOA, GPIOA_TX1, PAL_MODE_ALTERNATE(7));
    palSetPadMode(GPIOA, GPIOA_TX1, PAL_MODE_ALTERNATE(7));
    sdStart(&SD1, &ser_cfg);
    if (!comProcess)
        comProcess = chThdCreateStatic(wathreadCom, sizeof(wathreadCom),
                                       NORMALPRIO, threadCom, NULL);
}
