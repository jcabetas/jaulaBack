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


uint8_t quitarAbuso(uint8_t numEstacion);

extern float  kW;
extern float frecuencia, presion;
extern uint16_t estadoVariador;
extern mutex_t MtxMedidas;
extern int16_t errorEnergia2, errorVariador2;

extern uint8_t hayQueLeerID;
extern int16_t errorID;
extern uint16_t numID;
extern uint16_t valorID;


extern uint8_t fs_ready;
extern uint8_t numTelef[16];
extern event_source_t enviarSMS_source;

extern char pendienteSMS[200];
extern char telefonoEnvio[16];

static uint8_t enviarSMS; // 0: no hay nada, 1: hay mensaje para enviar, 2: bloqueado porque habia sin enviar



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


/**
 * @brief Anexa respuestas a enviar
 *
 * @param mensaje Mensaje a anexar
 *
 */

//void enviaTexto(const char *mensaje)
//{
//    if (mensaje[0]==0) return;
//    if (strlen(pendienteSMS)+strlen(mensaje)>sizeof(pendienteSMS)) return;
//    if (!mensaje[0] && strlen(pendienteSMS)<sizeof(pendienteSMS)-3) strncat(pendienteSMS,"; ",sizeof(pendienteSMS)-1);
//    strncat(pendienteSMS,mensaje,sizeof(pendienteSMS)-1);
//    if (enviarSMS!=2)
//        enviarSMS = 1;
//}

void sms::procesaOrdenAsignacion(char *orden, char *puntSimbIgual)
{
    char buff[50], *puntAjuste, *puntValor;
    int32_t error;
    uint32_t valor;
    // orden de asignacion
    *puntSimbIgual = (char) 0;
    puntAjuste = trimall(orden);
    puntValor = trimall(puntSimbIgual+1);
    //
    if (!strcasecmp(puntValor,"si")||!strcasecmp(puntValor,"on"))
        valor = 1;
    else if  (!strcasecmp(puntValor,"1") || puntValor[0]=='s' || puntValor[0]=='S')
        valor = 1;
    else if  (!strcasecmp(puntValor,"0") || !strcasecmp(puntValor,"no")||!strcasecmp(puntValor,"off"))
        valor = 0;
    else if  (!strcasecmp(puntValor,"2") || !strcasecmp(puntValor,"auto"))
        valor = 2;
    else
    {
        error = Str2Int((uint8_t *) (puntValor), &valor);
        if (error!=0)
        {
            chsnprintf(buff,sizeof(buff),"Valor incorrecto (%s)",puntValor);
            addMsgRespuesta(buff);
            return;
        }
    }

    if (!strncasecmp(puntAjuste,"bloqueoAbusones",strlen(puntAjuste)))
    {
        if (valor>1)
        {
            chsnprintf(buff,sizeof(buff),"Valor incorrecto bloqueoAbusones (%s)",puntValor);
            addMsgRespuesta(buff);
            return;
        }
        //bloqueoAbusones.setVal(valor);
        //bloqueoAbusones.escribeFlash();
    }
    if (!strncasecmp(puntAjuste,"desbloquea",strlen(puntAjuste)))
    {
//        if (modopozo.getValorNum()!=2)
//        {
//            chsnprintf(buff,sizeof(buff),"No soy el pozo. No puedo desbloquear\n",puntValor);
//            addMsgRespuesta(buff);
//            return;
//        }
        if (valor==0 || valor>8)
        {
            chsnprintf(buff,sizeof(buff),"Numero incorrecto de estacion en desbloquea",puntValor);
            addMsgRespuesta(buff);
            return;
        }
 // OJO CORREGIR       radio::quitarAbuso(valor);
    }
}


void sms::ponStatusHistorico(void)
{
}


/**
 * @brief Procesa status
 *
 */
void sms::procesaStatusYAlgo(char *algo)
{
   // char buff[50],valor[30];
    // me piden un parametroFlash?
    if (!strncasecmp(algo,"historico",strlen(algo)))
    {
        ponStatusHistorico();
        return;
    }
    if (!strncasecmp(algo,"tensiones",strlen(algo)))
    {
        //        ponStatusSMSAlimentacion();
        return;
    }
}


/*
 *         if (hayQueLeerID)
            valorID = leeSDMHex(idModbusVariador.getValorNum(), 4,  numID, &errorID);
            hayQueLeerID = 0;
 */

void sms::procesaID(char *)
{

}

/**
 * @brief Procesa status general
 *
 */


char statusResu[90];

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


void procesaStatusOld(void)
{
    //	float PVE;
    //	char *strErr;
    //	uint8_t errorVE;
    //  chsnprintf(statusResu,sizeof(statusResu),"%s",estadoCoche[statusCarga]);
    //  enviaTexto(statusResu);
    //  if (statusCarga==CARGADO)
    //  {
    //      chsnprintf(statusResu,sizeof(statusResu),"\nFin de carga %02d:%02d\nkWh punta %5.2f kWh valle %5.2f",
    //                 horaFinCarga, minHoraCarga, kWhVEp, kWhVEv);
    //      enviaTexto(statusResu);
    //  }
    //  if (statusCarga==CARGANDO_BULK || statusCarga==CARGANDO_BULK_BAJITO)
    //  {
    //    // medidas[WB*5+0] => PVE[WB]
    //    PVE = getValorMedControl(MED_PVE_MAS1(numCargador), &errorVE);
    //    strErr=" ";
    //    if (errorVE)
    //    	strErr = "(Error) ";
    //    chsnprintf(statusResu,sizeof(statusResu),"\n%5d W %s%5.1f kWh",PVE, strErr, kWhVEt);
    //    enviaTexto(statusResu);
    //  }
    //  if (statusCarga==ESPERANDOVALLE)
    //  {
    //    chsnprintf(statusResu,sizeof(statusResu),"\nCargara a las %2dh",horaValle);
    //    enviaTexto(statusResu);
    //  }
}

void procesaAlarmas(uint8_t reconoce)
{
    (void) reconoce;
    //  uint8_t i, primeraAlarma = 1, esActiva;
    //  for (i=0;i<sizeof(alarmaActiva);i++)
    //  {
    //    chMtxLock(&MtxHayAlarma);
    //    if (alarmaActiva[i] && !alarmaReconocida[i])
    //      esActiva = 1;
    //    else
    //      esActiva = 0;
    //    chMtxUnlock(&MtxHayAlarma);
    //    if (!esActiva)
    //      continue;
    //    if (primeraAlarma)
    //      enviaTexto("Alarmas:");
    //    primeraAlarma = 0;
    //    enviaTexto(textoAlarmas[i]);
    //    if (reconoce)
    //    {
    //      chMtxLock(&MtxHayAlarma);
    //      alarmaReconocida[i] = 1;
    //      chMtxUnlock(&MtxHayAlarma);
    //    }
    //  }
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
        if (puntSimb != NULL)
        {
            puntValor = trimall(puntSimb+1);
            procesaStatusYAlgo(puntValor);
            return;
        }
        procesaAlarmas(0);
        procesaStatus();
        return;
    }
    if (!strncasecmp(bufferOrden,"id",strlen(bufferOrden)))
    {
        if (puntSimb == NULL)
        {
            addMsgRespuesta("pon numero ID\n");
            return;
        }
        puntValor = trimall(puntSimb+1);
        procesaID(puntValor);
        procesaAlarmas(0);
        procesaStatus();
        return;
    }

    if (!strncasecmp(puntOrden,"reconoce",strlen(puntOrden)))
    {
        procesaAlarmas(1);
        return;
    }
    // �contiene = � : ?
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

    enviarSMS = 0;
    borraMsgRespuesta();
//    if (msgRespuesta[0])
//        enviarSMS = 2; // si hay mensajes pendientes, bloqueo envio

    // compruebo que no hay caracteres raros
    for (i=0;i<strlen((char *)textoSMS);i++)
        if (!isprint((int) textoSMS[i]))
        {
            return;
        }
    // divido ordenes
    enviarSMS = 0;
    error = 0;
    parseStr((char *)textoSMS,ordenes,",.;",&numOrdenes);
    for (i=0;i<numOrdenes;i++)
        procesaOrden(ordenes[i],&error);
    if (error)
    {
        chsnprintf(bufferOrden,sizeof(bufferOrden),"No entiendo '%s'",textoSMS);
        addMsgRespuesta(bufferOrden);
    }
}
