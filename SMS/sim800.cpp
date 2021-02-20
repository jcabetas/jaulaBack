/*
 * sim900.c
 *
 *  Created on: 5/1/2017
 *      Author: jcabe
 */

#include "ch.hpp"
#include "hal.h"
using namespace chibios_rt;


#include "chprintf.h"
#include "tty.h"
#include "gets.h"
#include "string.h"
#include "stdlib.h"
#include "sms.h"
#include "time.h"
#include "calendar.h"

//mutex_t MtxEspSim800SMS;

//extern uint8_t hayGPRS, hayCAN, prefWifi, prefMovil, mqttMedCoche, mqttMedTotales, mqttEstado, smsFinCarga, smsEstado, smsBotonParo;
//extern uint16_t mqqtTiempo;
//extern uint8_t esp8266AP[21], esp8266Psswd[26];

//extern uint32_t secsUTM;
//extern struct sdate fechaLocal;
//extern mutex_t MtxSecs;
//extern uint32_t segundosUTMCambioInv2Ver, segundosUTMCambioVer2Inv;

//extern uint8_t mqqtOk, smsOk, gprsOk;

//extern uint8_t telefonoEnvio[16];
//extern uint8_t telefonoRecibido[16];
//extern char bufferSendSMS[200];
extern event_source_t enviarSMS_source;
//char mensajeRecibido[200];
//extern char bufferSendSMS[200];
//uint8_t bufferSMS[200];

//void SetTimeUnixSec(time_t unix_time);
time_t GetTimeUnixSec(void);
//uint8_t hayGPRSValor(void);
void limpiaSerie(BaseChannel  *pSD, uint8_t *buffer, uint16_t bufferSize);
void chgetsNoEchoTimeOut(BaseChannel  *pSD, uint8_t *buffer, uint16_t bufferSize,systime_t timeout, uint8_t *huboTimeout);
void buscaParametros(uint8_t *posCadena, uint8_t *numParametros,uint8_t *parametros[]);
void procesaOrden(char *orden, uint8_t *error);


uint8_t bufferGetsGPRS[200];

static const SerialConfig ser_cfg = {
    115200, 0, 0, 0,
};

uint16_t lee2car(uint8_t *buffer, uint16_t posIni,uint16_t minValor, uint16_t maxValor, uint8_t *hayError)
{
    int32_t res;
    uint32_t numero;
    buffer[posIni+2] = 0;
    res = Str2Int(&buffer[posIni], &numero);
    if (res<0) *hayError=1;
    if (numero<minValor || numero>maxValor) *hayError=1;
    return (uint16_t) numero;
}


int8_t sms::sendSMS(BaseChannel  *pSD, char *msg, char *numTelefono)
{
    uint8_t hayError, huboTimeout, ch, bufferSMS[40];
    char bufferSendSMS[50];
    if (msg[0]==0 || numTelefono[0]==0) return -2;
    chsnprintf(bufferSendSMS,sizeof(bufferSendSMS),"AT+CMGS=\"%s\"\r\n", numTelefono);
    chMtxLock(&MtxEspSim800SMS);
    chnWrite(pSD, (const uint8_t *)bufferSendSMS, strlen(bufferSendSMS));
    while (1==1)
    {
        ch = chgetchTimeOut(pSD, TIME_MS2I(5000), &huboTimeout);
        if (huboTimeout) // timeout
        {
            chMtxUnlock(&MtxEspSim800SMS);
            return -1;
        }
        if (ch=='>') break;
    }

    while (1==1)
    {
        ch = chgetchTimeOut(pSD, TIME_MS2I(1000), &huboTimeout);
        if (huboTimeout) // timeout
        {
            chMtxUnlock(&MtxEspSim800SMS);
            return -1;
        }
        if (ch==' ') break;
    }
    // envio texto
    limpiaSerie(pSD, (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS));
    chprintf((BaseSequentialStream  *) pSD, msg);
    // termino con ^Z
    chsnprintf(bufferSendSMS,sizeof(bufferSendSMS),"%c",(char) 26);  //26 es CTRL-Z
//    antes chsnprintf(bufferSendSMS,sizeof(bufferSendSMS),"%s%c", msg,(char) 26);  //26 es CTRL-Z
    hayError = modemOrden(pSD,bufferSendSMS, bufferSMS, sizeof(bufferSMS),TIME_MS2I(8000));
    chMtxUnlock(&MtxEspSim800SMS);
    return hayError;
}

void sms::sendSMS(void)
{
    if (!msgRespuesta[0])
        return;
    if (telefonoRecibido[0])
        sendSMS((BaseChannel  *)&SD2, msgRespuesta, telefonoRecibido);
    else
        sendSMS((BaseChannel  *)&SD2, msgRespuesta, telefonoAdmin);
    borraMsgRespuesta();
}

uint8_t sms::initSIM800SMS(BaseChannel  *pSD)
{
    uint8_t hayError, numIntentos;
    uint8_t numParametros, *parametros[10];
    char buffer[30], bufferSendSMS[150];

    // soft init
    callReady = 0;
    smsReadyVal = 0;
    pinReady = 0;
    estadoCREG = 99;
    rssiGPRS = 0;
    telefonoEnvio[0] = 0;
    proveedor[0] = 0;
    tiempoLastConexion = calendar::GetTimeUnixSec();

    // hardware init
    palClearPad(GPIOA, GPIOA_TX2);
    palSetPad(GPIOA, GPIOA_RX2);
    palSetPadMode(GPIOA, GPIOA_TX2, PAL_MODE_ALTERNATE(7));
    palSetPadMode(GPIOA, GPIOA_RX2, PAL_MODE_ALTERNATE(7));
    palClearPad(GPIOB, GPIOB_RESETSIM800);
    chThdSleepMilliseconds(150);
    palSetPad(GPIOB, GPIOB_RESETSIM800);
    sdStart(&SD2,&ser_cfg);

    // miro a ver si me responde
    numIntentos = 0;
    do
    {
        hayError = modemOrden(pSD, "AT\r\n", bufferGetsGPRS, sizeof(bufferGetsGPRS), TIME_MS2I(500));
        if (!hayError) break;
        hayError = modemOrden(pSD,"AT\r\n", bufferGetsGPRS, sizeof(bufferGetsGPRS), TIME_MS2I(500));
        if (!hayError) break;
        osalThreadSleepMilliseconds(1300);
        modemOrden(pSD,"+++", bufferGetsGPRS, sizeof(bufferGetsGPRS), TIME_MS2I(1000));
        osalThreadSleepMilliseconds(1300);
    } while (hayError && numIntentos++<3);
    if (hayError==1)
    {
        return 1;
    }
    // si me costo que me entendieran, fijo los baudios
    if (numIntentos>0)
    {
        hayError = modemOrden(pSD,"AT+IPR=115200\r\n", bufferGetsGPRS, sizeof(bufferGetsGPRS), TIME_MS2I(1000));
        hayError = modemOrden(pSD,"AT&W\r\n", bufferGetsGPRS, sizeof(bufferGetsGPRS), TIME_MS2I(1000));
    }
    // SIM insertada?
    hayError = modemParametros((BaseChannel  *)&SD2,"AT+CSMINS?\r\n","+CSMINS:", (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS),TIME_MS2I(200),&numParametros, parametros);
    if (numParametros==2 && atoi((char *) parametros[1])==0)
    {
        return 1;
    }
    // estado pin
    hayError = modemParametros((BaseChannel  *)&SD2,"AT+CPIN?\r\n","+CPIN:", (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS),TIME_MS2I(200),&numParametros, parametros);
    if (numParametros==1 && !strncmp((char *) parametros[0],"SIM_PIN",7)) // la SIM esta esperando el PIN
    {
        hayError = modemParametros((BaseChannel  *)&SD2,"AT+SPIC\r\n","+SPIC:", (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS),TIME_MS2I(200),&numParametros, parametros);
        chsnprintf(buffer,sizeof(buffer),"Quedan %d intentos PIN\n\r",atoi((char *) parametros[0]));
        chsnprintf(buffer,sizeof(buffer),"AT+CPIN=%s\r\n",pin);
        hayError = modemOrden(pSD,buffer, bufferGetsGPRS, sizeof(bufferGetsGPRS), TIME_MS2I(1000));
        if (hayError)
        {
            return 1;
        }
    }
    // Aprovecho para preguntar RSSI
    hayError = modemParametros((BaseChannel  *)&SD2,"AT+CSQ\r\n","+CSQ:", (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS),TIME_MS2I(200),&numParametros, parametros);
    // Registrar en red
    modemOrden(pSD, "AT+CREG=1\r\n", bufferGetsGPRS, sizeof(bufferGetsGPRS), TIME_MS2I(1000));
    // SMS modo texto
    modemOrden(pSD, "AT+CMGF=1\r\n", bufferGetsGPRS, sizeof(bufferGetsGPRS), TIME_MS2I(500));
    // que me digan todos los mensajes
    modemOrden((BaseChannel  *)&SD2, "at+cmgl=\"ALL\"\r\n", (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS), TIME_MS2I(5000));
    // SMS notification: memorizo aunque me llegaran notificaciones CMTI
    modemOrden((BaseChannel  *)&SD2, "at+cnmi=2,1,0,0,0\r\n", (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS), TIME_MS2I(5000));
    return 0;
}


/*
+CMTI: "SM",1

+CMTI: "SM",2     => envia dos veces porque hab�a en dos memorias diferentes. Si no enredamos solo debe llegar uno
at+cmgl="ALL"
+CMGL: 1,"REC UNREAD","+34619262851","","21/01/02,19:43:14+04"
Vamos a ver hoy

+CMGL: 2,"REC UNREAD","+34619262851","","21/01/02,20:37:26+04"
Vamos a ver

OK

 */
void sms::leoSmsCMTI(BaseSequentialStream  *pSD)
{
    uint8_t huboTimeout, hayError, hayMensaje;
    uint8_t bufferGetsGPRS[20], numSMSaBorrar;
    char bufferCMGL[120], bufferNxt[100], *posCadena, *posSMS; //bufferSMS[120],
    uint8_t numParametros, *parametros[10];
    struct tm fechaSMS;
    numSMSaBorrar = 0;

    do
    {
        hayMensaje = 0;
        // envio orden de volcar mensajes
        chprintf(pSD, "at+cmgl=\"ALL\"\r\n");
        // no hago caso del eco inicial
        chgetsNoEchoTimeOut((BaseChannel *) pSD, (uint8_t *) bufferCMGL, sizeof(bufferCMGL), TIME_MS2I(500),&huboTimeout);
        // solo nos vamos a fijar en el primer mensaje. Lo leemos, saltamos el resto, procesamos y borramos
        chgetsNoEchoTimeOut((BaseChannel *) pSD, (uint8_t *) bufferCMGL, sizeof(bufferCMGL), TIME_MS2I(500),&huboTimeout);
        if (huboTimeout==1)
            return;
        // puede llegar o una cabecera, o OK
        if (!strncmp((char *) bufferCMGL,"OK",2))
            return;
        // cabecera del tipo
        // +CMGL: 1,"REC UNREAD","+34619262851","","21/01/02,19:43:14+04"
        posCadena = strstr(bufferCMGL, (char *)"+CMGL:");
        if (posCadena==NULL)
            break;
        posCadena += 6;  // avanzo "+CMGL:"
        buscaParametros((uint8_t *) posCadena, &numParametros, parametros);
        if (numParametros<5)
            break;
        numSMSaBorrar = atoi((char *) parametros[0]);
        horaSMStoTM((uint8_t *)parametros[4], &fechaSMS);
        // leo mensaje, pueden venir en varias lineas hasta que llegue OK o +CMGL. Voy acumulando
        posSMS = mensajeRecibido;
        do
        {
            chgetsNoEchoTimeOut((BaseChannel *) pSD, (uint8_t *) posSMS, sizeof(mensajeRecibido)-(posSMS - mensajeRecibido), TIME_MS2I(500),&huboTimeout);
            if (huboTimeout==1 || !strncmp((char *) posSMS,"OK",2) || !strncmp((char *) posSMS,"+CMGL",5))
                break;
            hayMensaje = 1;
            posSMS += strlen((char *)posSMS);
        } while (1==1);
        // marco el final del mensaje (no incorporo el OK o +CMGL)
        *posSMS = 0;
        // salto el resto del envio
        limpiaBuffer((BaseChannel *) pSD);

        if (hayMensaje)
        {
            strncpy(telefonoRecibido,(char *)parametros[2],sizeof(telefonoRecibido));
            chsnprintf(bufferNxt,sizeof(bufferNxt),"SMS de %s, enviado el %d/%d %d:%02d:%02d",parametros[2],
                       fechaSMS.tm_mday,fechaSMS.tm_mon+1,fechaSMS.tm_hour,fechaSMS.tm_min,fechaSMS.tm_sec);
            chsnprintf(bufferNxt,sizeof(bufferNxt),"Mensaje: %s",mensajeRecibido);
            // borro el mensaje
            chsnprintf(bufferNxt,sizeof(bufferNxt),"at+cmgd=%d\r\n",numSMSaBorrar);
            hayError = modemOrden((BaseChannel *) pSD, bufferNxt, bufferGetsGPRS, sizeof(bufferGetsGPRS), TIME_MS2I(500));
            if (!hayError)
                procesaOrden(mensajeRecibido, &hayError);
        }
    } while (hayMensaje);
//
//
//
//    // +CMT: "+34619262851","","20/03/07,08:40:04+04"
//    for (i=7;i<sizeof(bufferSendSMS) && i<strlen(bufferSendSMS);i++)
//    {
//        if (bufferSendSMS[i]=='\"')
//            break;
//        telefonoRecibido[i-7] = bufferSendSMS[i];
//    }
//    telefonoRecibido[i-7] = 0;
//    // decodifico fecha
//    horaSMStoTM((uint8_t *)&bufferSendSMS[25], &fechaSMS);
//    chsnprintf(buffer,sizeof(buffer),"SMS de %s",telefonoRecibido);
//    msgParaLCD(buffer, 1000);
//    chsnprintf(buffer,sizeof(buffer),"%d/%d/%d %d:%d:%d",
//                    fechaSMS.tm_mday, fechaSMS.tm_mon+1, fechaSMS.tm_year+1900,
//                    fechaSMS.tm_hour, fechaSMS.tm_min, fechaSMS.tm_sec);
//    msgParaLCD(buffer, 1000);
//    // mensaje (puede haber varias lineas)
//    mensajeRecibido[0] = 0;
//    // reaprovecho bufferSendSMS
//    do
//    {
//        chgetsNoEchoTimeOut((BaseChannel  *)&SD2, (uint8_t *) bufferSendSMS, sizeof(bufferSendSMS), TIME_MS2I(100),&huboTimeout);
//        if (!huboTimeout)
//        {
//            strncat(mensajeRecibido,";",2);
//            strncat(mensajeRecibido,bufferSendSMS,sizeof(mensajeRecibido));
//        }
//        else
//            break;
//    } while (TRUE);
//    msgParaLCD(mensajeRecibido,1000);
//    // escribe en log de SD
//    chsnprintf(buffer,sizeof(buffer),"SMS de %s: %s",telefonoRecibido, mensajeRecibido);
//    registraEnLog(buffer);
//    // interpreta SMS y envia mensaje correspondiente
//    interpretaSMS((uint8_t *)mensajeRecibido);
//    chEvtBroadcast(&enviarSMS_source);
}


void sms::leoSMS(char *bufferSendSMS, uint16_t buffSize)
{
    uint8_t huboTimeout,i;
    char buffer[81];
    struct tm fechaSMS;
    // +CMT: "+34619262851","","20/03/07,08:40:04+04"
    for (i=0;i<sizeof(telefonoRecibido) && i<buffSize-7;i++)
    {
        if (bufferSendSMS[i+7]=='\"')
            break;
        telefonoRecibido[i] = bufferSendSMS[i+7];
    }
    telefonoRecibido[i] = 0;
    // decodifico fecha
    horaSMStoTM((uint8_t *)&bufferSendSMS[25], &fechaSMS);
    chsnprintf(buffer,sizeof(buffer),"SMS de %s",telefonoRecibido);
    //msgParaLCD(buffer, 1000);
    chsnprintf(buffer,sizeof(buffer),"%d/%d/%d %d:%d:%d",
                    fechaSMS.tm_mday, fechaSMS.tm_mon+1, fechaSMS.tm_year+1900,
                    fechaSMS.tm_hour, fechaSMS.tm_min, fechaSMS.tm_sec);
    //msgParaLCD(buffer, 1000);
    // mensaje (puede haber varias lineas)
    mensajeRecibido[0] = 0;
    // reaprovecho bufferSendSMS
    do
    {
        chgetsNoEchoTimeOut((BaseChannel  *)&SD2, (uint8_t *) bufferSendSMS, buffSize, TIME_MS2I(500),&huboTimeout);
        if (bufferSendSMS[0])
        {
            if (mensajeRecibido[0])
                strncat(mensajeRecibido,";",2);
            strncat(mensajeRecibido,bufferSendSMS,sizeof(mensajeRecibido));
        }
        else
            break;
    } while (TRUE);
    //msgParaLCD(mensajeRecibido,1000);
    // escribe en log de SD
    //chsnprintf(buffer,sizeof(buffer),"SMS de %s: %s",telefonoRecibido, mensajeRecibido);
    // interpreta SMS y envia mensaje correspondiente
    interpretaSMS((uint8_t *)mensajeRecibido);
    chEvtBroadcast(&enviarSMS_source);
}

/*
 * Convierto fecha SMS a struct tm
 * Formato: 19/12/05,21:12:30+04
 * Retorna 1 si hay error
 */
uint8_t sms::horaSMStoTM(uint8_t *cadena, struct tm *fecha)
{
    uint8_t hayError;
    int16_t difGMT;
    // 04/01/01,00:10:57+04
    hayError = 0;
    difGMT = lee2car(cadena,19,0,96,&hayError);
    if (cadena[18]=='-') difGMT = -difGMT;
    fecha->tm_year = lee2car(cadena,0,17,99,&hayError)+2000-1900;
    fecha->tm_mon = lee2car(cadena,3,1,12,&hayError)-1;
    fecha->tm_mday = lee2car(cadena,6,1,31,&hayError);
    fecha->tm_hour = lee2car(cadena,9,0,23,&hayError);
    fecha->tm_min = lee2car(cadena,12,0,59,&hayError);
    fecha->tm_sec = lee2car(cadena,15,0,59,&hayError);
    uint8_t numCuartos = lee2car(cadena,18,0,8,&hayError);
    if (numCuartos==4) fecha->tm_isdst = 0; // horario de invierno
    if (numCuartos==8) fecha->tm_isdst = 1; // horario de verano
    return hayError;
}


uint8_t sms::ponHoraConGprs(BaseChannel  *pSD)
{
    uint8_t hayError, numParametros, *parametros[10];
    char buffer[40];
    struct tm fechaGPRS, fechaUTM;
    time_t secs;
    RTCDateTime timespec;
    /*
     *  bufferGetsGPRS = "+CCLK: \"17/08/03,20:39:29+08",
     *   parametros[0] = "17/08/04,11:18:30+08", // el 8 es la diferencia con PST en unidades de 15 minutos!
     *                    01234567890123456789
     */
    /*
     * Miro si está activado leer hora
     */
    chMtxLock(&MtxEspSim800SMS);
    hayError = modemParametros(pSD,"AT+CLTS?\r\n","+CLTS:", bufferGetsGPRS, sizeof(bufferGetsGPRS),TIME_MS2I(1000),&numParametros, parametros);
    chMtxUnlock(&MtxEspSim800SMS);
    if (hayError==0 && numParametros==1 && strcmp((char *)parametros[0],"0")==0)
    {
        chMtxLock(&MtxEspSim800SMS);
        hayError = modemOrden(pSD, "AT+CLTS=1;&W\r\n", bufferGetsGPRS, sizeof(bufferGetsGPRS), TIME_MS2I(1000));
        chMtxUnlock(&MtxEspSim800SMS);
        if (hayError!=0) return 1;
        osalThreadSleepMilliseconds(100);
        // resetea modem
//        msgParaLCD("Reseteo modem GPRS",400);
//        //ponEnLCD(2,"Reseteo modem GPRS");
//        chMtxLock(&MtxEspSim800SMS);
//        hayError = modemOrden(pSD, "AT+CFUN=1,1\r\n", bufferGetsGPRS, sizeof(bufferGetsGPRS), TIME_MS2I(1000));
//        chMtxUnlock(&MtxEspSim800SMS);
//        msgParaLCD("Activada hora de red",500);
    }
    chMtxLock(&MtxEspSim800SMS);
    hayError = modemParametros(pSD,"AT+CCLK?\r\n","+CCLK:", bufferGetsGPRS, sizeof(bufferGetsGPRS),TIME_MS2I(1000),&numParametros, parametros);
    chMtxUnlock(&MtxEspSim800SMS);
    // +CCLK: "21/02/01,21:25:04+04"
    if (hayError==1)
        return 1;
    hayError = horaSMStoTM(&bufferGetsGPRS[8], &fechaGPRS);
    if (hayError) // fecha mal, no funciona CCLK. Podria auto-enviarme un SMS
    {
        return 1;
    }
    calendar::fechaLocal2UTM(&fechaGPRS, &fechaUTM, &secs);
    rtcConvertStructTmToDateTime(&fechaUTM, 0, &timespec);
    rtcSetTime(&RTCD1, &timespec);
    chsnprintf(buffer,sizeof(buffer),"Fecha GPRS:%d/%d/%d %d:%d:%d",
                    fechaGPRS.tm_mday, fechaGPRS.tm_mon+1, fechaGPRS.tm_year+1900,
                    fechaGPRS.tm_hour, fechaGPRS.tm_min, fechaGPRS.tm_sec);
    return 0;
}












