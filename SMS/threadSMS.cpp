/*
 * threadSMS.cpp
 *
 *  Created on: 5 feb. 2021
 *      Author: joaquin
 */

#include "ch.hpp"
#include "hal.h"
using namespace chibios_rt;


#include "chprintf.h"
#include "tty.h"
#include "gets.h"
#include "string.h"
#include "sms.h"
#include "calendar.h"


thread_t *procesoSMS;
event_source_t enviarSMS_source;
extern sms *smsModem;
BaseChannel  *pSDSMS;

/*
 * Proceso SMS
 * Escucha para eventos de enviar SMS, y de vez en cuando busca SMS y la fecha del sistema
 * El SMS a enviar estara en variable global pendienteSMS al telefono tfnoSMS
 */

static THD_WORKING_AREA(waSMS, 2024);

static THD_FUNCTION(threadSMS, smsPtr) {
    sms *smsP;
    eventmask_t evt;
    eventflags_t flags;
    uint8_t huboTimeout;
    int16_t dsQuery; // contador para ver estados
    //char buffer[20];
    char bufferSendSMS[200];
    uint32_t posSMS;
    time_t ultAjusteHora = 0, ultQuerySMS = 0;

    smsP = (sms *) smsPtr;
    smsP->borraMsgRespuesta();
    dsQuery = -200; // los primeros 20+10 segundos dejo soltar mensajes a SIM800L, y no le hago preguntas

    uint8_t hayError, numParametros, *parametros[10];
    event_listener_t smsSend_listener, receivedData;
    chRegSetThreadName("SMS");

    // prueba dormir y despertar
    smsP->sleep();

    chEvtRegisterMask(&enviarSMS_source, &smsSend_listener,EVENT_MASK(0));
    chEvtRegisterMaskWithFlags (chnGetEventSource((SerialDriver *)smsP->pSD),&receivedData, EVENT_MASK (1),CHN_INPUT_AVAILABLE);

    while (true)
    {
        evt = chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(100));
        if (chThdShouldTerminateX())
        {
            chEvtUnregister(&enviarSMS_source, &smsSend_listener);
            chEvtUnregister(chnGetEventSource((SerialDriver *)smsP->pSD),&receivedData);//SerialDriver *
            smsP->callReady = 0;
            smsP->smsReadyVal = 0;
            smsP->pinReady = 0;
            smsP->estadoCREG = 0;
            smsP->proveedor[0] =0;
            chThdExit((msg_t) 1);
        }
        if (evt & EVENT_MASK(0)) // Evento enviar SMS
        {
            smsP->despierta();
            smsP->sendSMS();
            smsP->sleep();
            continue;
        }
        if (evt & EVENT_MASK(1)) // Algo ha entrado por la SIM
        {
            flags = chEvtGetAndClearFlags(&receivedData);
            if (flags & CHN_INPUT_AVAILABLE)
            {
                chgetsNoEchoTimeOut((BaseChannel *)smsP->pSD, (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS), TIME_MS2I(100),&huboTimeout);
                if (huboTimeout)
                    goto continuaRx;
                // si es una linea en blanco, lee la siguiente
                if (bufferSendSMS[0]==0)
                {
                    chgetsNoEchoTimeOut((BaseChannel *)smsP->pSD, (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS), TIME_MS2I(100),&huboTimeout);
                    if (huboTimeout)
                        goto continuaRx;
                }
                // +CMTI: "SM",5
                if (!strncmp(bufferSendSMS,"+CMTI: ",6))
                {
                    chEvtUnregister(chnGetEventSource((SerialDriver *)smsP->pSD), &receivedData);
                    Str2Int((uint8_t *)&bufferSendSMS[12], &posSMS);
                    smsP->despierta();
                    smsP->leoSmsCMTI();
                    smsP->sleep();
                    chEvtRegisterMaskWithFlags (chnGetEventSource ((SerialDriver *)smsP->pSD),&receivedData, EVENT_MASK (1),CHN_INPUT_AVAILABLE);
                }
                // +CMT: "+34619262851","19/12/05,21:12:30+04"
                else if (!strncmp(bufferSendSMS,"+CMT: ",6))
                {
                    chEvtUnregister(chnGetEventSource((SerialDriver *)smsP->pSD), &receivedData);
                    smsP->despierta();
                    smsP->leoSMS(bufferSendSMS,sizeof(bufferSendSMS));
                    smsP->sleep();
                    chEvtRegisterMaskWithFlags (chnGetEventSource ((SerialDriver *)smsP->pSD),&receivedData, EVENT_MASK (1),CHN_INPUT_AVAILABLE);
                }
                else if (!strncmp(bufferSendSMS,"Call Ready",sizeof(bufferSendSMS)))
                {
                    if (smsP->callReady != 1)
                        smsP->callReady = 1;
                }
                else if (!strncmp(bufferSendSMS,"SMS Ready",sizeof(bufferSendSMS)))
                {
                    if (smsP->smsReadyVal != 1)
                    {
                        smsP->smsReadyVal = 1;
                    }
                }
                //+CPIN: READY
                else if (!strncmp(bufferSendSMS,"+CPIN:",6))
                {
                    if (!strncmp(bufferSendSMS,"+CPIN: READY",12))
                    {
                        if (smsP->pinReady != 1)
                        {
                            smsP->pinReady = 1;
                        }
                    }
                    else
                        smsP->pinReady = 0;
                }
                else if (!strncmp(bufferSendSMS,"+CREG: ",7))
                {
                    uint8_t newEstadoCREG = bufferSendSMS[7]-'0';
                    if (smsP->estadoCREG != newEstadoCREG)
                    {
                        smsP->estadoCREG = newEstadoCREG;
                    }
                }
            }
            continuaRx:
//            chEvtRegisterMaskWithFlags (chnGetEventSource (&SD2),&receivedData, EVENT_MASK (1),CHN_INPUT_AVAILABLE);
            continue;
        }
        // Actualizo datos cada 20 segundos
        if (++dsQuery>=200)
        {
            chEvtUnregister(chnGetEventSource((SerialDriver *)smsP->pSD),&receivedData);
            smsP->despierta();
            // comprueba conexion
            hayError = modemParametros((BaseChannel *)smsP->pSD,"AT+CREG?\r\n","+CREG:", (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS),TIME_MS2I(5000),&numParametros, parametros);
            uint8_t newEstadoCREG = bufferSendSMS[7]-'0';
            // si ha cambiado el estado, actualiza Nextion
            if (newEstadoCREG != smsP->estadoCREG)
            {
                smsP->estadoCREG = newEstadoCREG;
                if (smsP->estadoCREG != 1)
                {
                    smsP->proveedor[0] = 0;
                }
            }
            time_t unix_time = calendar::GetTimeUnixSec();
            if (smsP->estadoCREG != 1 )
            {
                // si llevo sin conexi�n mas de 3 minutos, reseteo
                if (unix_time-smsP->tiempoLastConexion > 180)
                {
                    smsP->tiempoLastConexion = unix_time;
                    smsP->initSIM800SMS();
                }
            }
            else
            {
                // sigue habiendo conexion
                smsP->tiempoLastConexion = unix_time;
                // miro si hay mensajes cada 10s
                if (unix_time-ultQuerySMS > 10)
                {
                    ultQuerySMS = unix_time;
                    smsP->leoSmsCMTI();
                }
                // ajusta hora cada semana
                if (unix_time-ultAjusteHora > 604800)
                {
                    // leo hora
                    smsP->ponHoraConGprs();
                    ultAjusteHora = calendar::GetTimeUnixSec();
                }
                // leo proveedor, si no lo ten�a claro
                if (!smsP->proveedor[0])
                {
                    hayError = modemParametros((BaseChannel *)smsP->pSD,"AT+COPS?\r\n","+COPS:", (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS),TIME_MS2I(5000),&numParametros, parametros);
                    //+COPS: 0,0,"vodafone"
                    if (numParametros>=3 && strlen((char *)parametros[2])>3)
                    {
                        strncpy(smsP->proveedor,(char *)parametros[2],sizeof(smsP->proveedor)-1);
                    }
                }
                // Aprovecho para preguntar RSSI
                hayError = modemParametros((BaseChannel *)smsP->pSD,"AT+CSQ\r\n","+CSQ:", (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS),TIME_MS2I(5000),&numParametros, parametros);
                if (hayError==0 && numParametros>=1)
                {
                    smsP->rssiGPRS = lee2car(parametros[0],0,0,99,&hayError);
                }
                // miro si hay mensajes
//                modemOrden((BaseChannel *)smsP->pSD, "at+cnmi=1,2,0,0,0\r\n", (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS), TIME_MS2I(5000));
            }
            dsQuery = 0;
            smsP->sleep();
            chEvtRegisterMaskWithFlags (chnGetEventSource((SerialDriver *)smsP->pSD),&receivedData, EVENT_MASK (1),CHN_INPUT_AVAILABLE);
        }
    }
}


void sms::initThreadSMS()
{
    pSDSMS = pSD;
    if (!procesoSMS)
        procesoSMS = chThdCreateStatic(waSMS, sizeof(waSMS), NORMALPRIO, threadSMS, this);
    smsModem = this;
}



void sms::mataSMS(void)
{
    if (procesoSMS!=NULL)
    {
        chThdTerminate(procesoSMS);
        chThdWait(procesoSMS);
        procesoSMS = NULL;
        smsModem = NULL;
    }
}
