/*
 * procesaOrden.c
 *
 *  Created on: 7/8/2017
 *      Author: joaquin
 */

/*
 * procesaOrden.c
 *
 *  Created on: 6/8/2017
 *      Author: joaquin
 */

/**
 * @file stm32/sms/parsing.c
 * @brief Ejecuta un SMS recibido
 * @addtogroup SMSs
 * @{
 */
#include "ch.hpp"
#include "hal.h"

using namespace chibios_rt;

#include "string.h"
#include "gets.h"
#include "chprintf.h"
#include "ctype.h"
#include "sms.h"
#include "varsFlash.h"

void leeTension(float *vBat);
float hallaCapBat(float *vBat);

extern event_source_t enviarSMS_source;
extern uint8_t estadoJaulaClosed, estadoJaulaOpened;

void parseStr(char *cadena,char **parametros, const char *tokens,uint16_t *numParam)
{
    char *puntOut,*puntStr;
    *numParam=0;
    puntOut = cadena;
    do
    {
        puntStr = strsep(&puntOut,tokens);
        if (puntStr)
        {
            parametros[*numParam] = puntStr;
            (*numParam)++;
        }
    } while (puntStr);
}



/**
 * @brief Quita espacios de delante y de atras de una cadena
 *
 * @param str Texto a modificar
 * @note Modifica el texto original
 */
char *trimall(char *str)
{
    char *posInicio, *posFinal;

    posInicio = str;
    while (*posInicio==' ') posInicio++;
    posFinal = posInicio + strlen(posInicio)-1;
    while (*posFinal==' ' && posFinal>=posInicio)
    {
        *posFinal = (char) 0;
        posFinal--;
    }
    return posInicio;
}



void sms::procesaOrdenAsignacion(char *orden, char *puntSimbIgual)
{
    char buff[50], *puntValor;
    int32_t err;
    // orden de asignacion
    *puntSimbIgual = (char) 0;
    trimall(orden);
    puntValor = trimall(puntSimbIgual+1);
    //
    if (!strcasecmp(orden,"telefono"))
    {
        ponEstado();
        uint16_t sectorNumTelef = SECTORNUMTELEF;
        err = escribeStr50(&sectorNumTelef, "numTelef", puntValor);
        if (err==0)
        {
            chsnprintf(buff,sizeof(buff),"Telefono envio:%s\n",puntValor);
            addMsgRespuesta(buff);
        }
        else
            addMsgRespuesta("Error al grabar num. telef.!!");
        return;
    }
    if (!strcasecmp(orden,"pin"))
    {
        ponEstado();
        uint16_t sectorPin = SECTORPIN;
        err = escribeStr50(&sectorPin, "numPin", puntValor);
        if (err==0)
        {
            chsnprintf(buff,sizeof(buff),"pin:%s\n",puntValor);
            addMsgRespuesta(buff);
        }
        else
            addMsgRespuesta("Error al grabar pin!!");
        return;
    }
}



/**
 * @brief Procesa status
 *
 */
void sms::procesaStatusYAlgo(char *algo)
{
    (void) algo;
//   // char buff[50],valor[30];
//    // me piden un parametroFlash?
//    if (!strncasecmp(algo,"historico",strlen(algo)))
//    {
//        //ponStatusHistorico();
//        return;
//    }
//    if (!strncasecmp(algo,"tensiones",strlen(algo)))
//    {
//        //        ponStatusSMSAlimentacion();
//        return;
//    }
}



/**
 * @brief Procesa status general
 *
 */

void sms::procesaStatus(void)
{

//    char binStr[10];
//    extern uint8_t estadoAbusones;
//    extern uint8_t estadoLlamaciones, estadoActivos;
////    extern uint8_t petBomba;
//
//    struct datosPozoGuardados *datos = (struct datosPozoGuardados *) BKPSRAM_BASE;
//    chsnprintf(statusResu,sizeof(statusResu),"Bomba:%d\n",petBomba);
//    enviaTexto(statusResu);
//    int2str(estadoLlamaciones, binStr);
//    chsnprintf(statusResu,sizeof(statusResu),"Piden:%s\n",binStr);
//    enviaTexto(statusResu);
//    int2str(estadoActivos, binStr);
//    chsnprintf(statusResu,sizeof(statusResu),"Activos:%s\n",binStr);
//    enviaTexto(statusResu);
//    if (modopozo.getValorNum()==2)
//    {
//        chsnprintf(statusResu,sizeof(statusResu),"bloqueoAbusones:%d\n",bloqueoAbusones.getValorNum());
//        enviaTexto(statusResu);
//    }
//    if (estadoAbusones)
//    {
//        int2str(estadoAbusones, binStr);
//        chsnprintf(statusResu,sizeof(statusResu),"Abusones:%s\n",binStr);
//        enviaTexto(statusResu);
//    }
//    if (hayModbus.getValorNum() && idModbusEnergia.getValorNum()) // hay medida de energia
//    {
//        if (errorEnergia2)
//            enviaTexto("Error medidor\n");
//        else
//        {
//            chMtxLock(&MtxMedidas);
//            chsnprintf(statusResu,sizeof(statusResu),"P:%.1f kWhP:%.1f V:%.1f\n",kW,datos->kWhPunta, datos->kWhValle);
//            chMtxUnlock(&MtxMedidas);
//            enviaTexto(statusResu);
//        }
//    }
//    if (hayModbus.getValorNum() && idModbusVariador.getValorNum()) // hay medida de variador
//    {
//        if (errorVariador2)
//            enviaTexto("Error comunicacion variador\n");
//        else
//        {
//            chMtxLock(&MtxMedidas);
//            chsnprintf(statusResu,sizeof(statusResu),"Presion:%.1f frecuencia:%.2f\n",presion, frecuencia);
//            enviaTexto(statusResu);
//            chMtxUnlock(&MtxMedidas);
//            switch (estadoVariador)
//            {
//            case 0:
//                enviaTexto("Inv. parado");
//                break;
//            case 1:
//                enviaTexto("Inv. en marcha");
//                break;
//            case 3:
//                enviaTexto("Inv. durmiendo");
//                break;
//            case 4:
//                enviaTexto("Inv. en banda");
//                break;
//            default:
//                chsnprintf(statusResu,sizeof(statusResu),"Estado raro inversor (%d)",estadoVariador);
//                enviaTexto(statusResu);
//            }
//        }
//    }
}



void sms::ponEstado(void)
{
    char buff[30];
    float vBat, capBat;
    if (estadoPuesto==1)
        return;
    if (estadoJaulaClosed && !estadoJaulaOpened)
        addMsgRespuesta("Jaula cerrada");
    else if (!estadoJaulaClosed && estadoJaulaOpened)
        addMsgRespuesta("Jaula abierta");
    else if (!estadoJaulaClosed && !estadoJaulaOpened)
        addMsgRespuesta("Estado jaula desconocido");
    else
        addMsgRespuesta("Error en estado sensores de jaula");

    leeTension(&vBat);
    capBat = hallaCapBat(&vBat);
    chsnprintf(buff,sizeof(buff),"Vbat:%.3fV (%d%%)",vBat,(int16_t)capBat);
    addMsgRespuesta(buff);
    estadoPuesto = 1;
}


/**
 * @brief Procesa una orden
 *
 * @param orden Orden a ejecutar
 *
 */
char bufferOrden[40];
void sms::procesaOrden(char *orden, uint8_t *error)
{
    char *puntSimb, *puntValor, *puntOrden;

    puntOrden = trimall(orden);
    strncpy(bufferOrden,puntOrden,sizeof(bufferOrden));
    puntSimb = strchr(bufferOrden,' ');
    if (puntSimb != NULL)
        *puntSimb = (char) 0;

    if (!strncasecmp(bufferOrden,"status",strlen(bufferOrden)))
    {
        ponEstado();
        if (puntSimb != NULL)
        {
            puntValor = trimall(puntSimb+1);
            procesaStatusYAlgo(puntValor);
            return;
        }
        procesaStatus();
        return;
    }
    // contiene = : ?
    puntSimb = strchr(orden,'=');
    if (puntSimb==NULL)
        puntSimb = strchr(orden,':');
    if (puntSimb != NULL)
    {
        // es una orden de asignacion
        procesaOrdenAsignacion(puntOrden, puntSimb);
        return;
    }
    // si llego aqui, es que no he interpretado la orden
    *error = 1;
}


/**
 * @brief interpreta las ordenes de un SMS
 *
 * @param textoSMS Texto a ejecutar
 * @param enviaRespuestaPorSMS Indica si tiene que enviar respuesta por SMS
 * @note divide el SMS en trozos, y procesa orden a orden
 * origenSMS = +CMT: \"+34619262851\",\"\",\"17/08/06,15:34:39+08\"\r\n
 */
void sms::interpretaSMS(uint8_t *textoSMS)
{
    char *ordenes[15];
    uint16_t numOrdenes, i;
    uint8_t error;

    msgRespuesta[0] = 0;
    // compruebo que no hay caracteres raros
    for (i=0;i<strlen((char *)textoSMS);i++)
        if (!isprint((int) textoSMS[i]))
        {
            return;
        }
    // divido ordenes

    error = 0;
    parseStr((char *)textoSMS,ordenes,",.;",&numOrdenes);
    for (i=0;i<numOrdenes;i++)
        procesaOrden(ordenes[i],&error);
    if (error)
    {
        chsnprintf(bufferOrden,sizeof(bufferOrden),"No entiendo '%s'",textoSMS);
        addMsgRespuesta(bufferOrden);
    }
    ponEstado();
}
