/*
 * sms.c
 *
 *  Created on: 2/8/2017
 *      Author: joaquin
 */

#include "ch.hpp"
#include "hal.h"
using namespace chibios_rt;


#include "chprintf.h"
#include "string.h"
#include "calendar.h"
#include "sms.h"
#include "varsFlash.h"

extern "C"
{
    void initSMS(void);
}



sms *smsModem = NULL;

extern event_source_t registraLog_source;
extern struct queu_t colaLog;




const char *sms::diMsgRespuesta(void)
{
    return msgRespuesta;
}

void sms::startSMS(void)
{
    uint8_t error;
    mataSMS();

    error = initSIM800SMS((BaseChannel *)&SD2);
    if (error)
    {
        hayError = 1;
        return;
    }
    hayError = 0;
    initThreadSMS((BaseChannel *)&SD2);
}

void haCambiadoGPRS(void)
{
    smsModem->startSMS();
}


// SMS tlfAdmin logTxt rssiTxt proveedorTxt smsReady
// SMS [tlfAdmin.sms 619262851] [pin.sms 3421] logSms.sms.txt rssi.sms.txt proveedor.sms.txt smsReady.sms.pic.31 creg.sms.txt
sms::sms(const char *numTelefAdminDefault, const char *pinDefault)
{
    //  escribeStr50_C(&sectorNumTelef, "numTelef", "619262851");
    uint16_t sectorNumTelef = SECTORNUMTELEF;
    uint16_t sectorPin = SECTORPIN;
    leeStr50(&sectorNumTelef, "numTelef", numTelefAdminDefault, telefonoAdmin);
    leeStr50(&sectorPin, "numPin", pinDefault, pin);
    borraMsgRespuesta();
    chMtxObjectInit(&MtxEspSim800SMS);
}

sms::~sms()
{
    mataSMS();
}


int8_t sms::init(void)
{
    startSMS();
    return 0;
}



void sms::borraMsgRespuesta(void)
{
    msgRespuesta[0] = '\0';
    telefonoRecibido[0] = 0;
    telefonoEnvio[0] = 0;
}

void sms::addMsgRespuesta(const char *texto)
{
    if (msgRespuesta[0])
        strncat(msgRespuesta,".",strlen(msgRespuesta)-1);
    strncat(msgRespuesta,texto,strlen(msgRespuesta)-1);
}

void initSMS(void)
{
    smsModem = new sms("619262851","");
    smsModem->init();
}
