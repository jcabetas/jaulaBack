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
#include "gets.h"
#include "string.h"
#include <stdlib.h>
#include "chprintf.h"
#include "calendarUTC.h"
#include <w25q16/w25q16.h>
#include "variables.h"

extern "C" {
    void initSerial(void);
    void closeSerial(void);
    void opciones(void);
    void printSerial(const char *msg);
}

void ajustaP(uint16_t porcP);
uint8_t ajustaHoraDetallada(uint16_t ano, uint8_t mes, uint8_t dia, uint8_t hora, uint8_t min, uint8_t sec);
void diSecAmanecerAnochecer(uint16_t *secActual, uint16_t *secAmanecer, uint16_t *secAnochecer);
void estadoDeseadoPuertaC(uint8_t *estDes, uint16_t *sec2change);
void mueveServoPos(uint16_t porcPosicion, uint16_t ms);
void leeTension(float *vBat);
bool tensionCritica(void);
float hallaCapBat(float *vBat);
void ajustaCALMP(int16_t dsDia);

extern int16_t addAmanecer;
extern int16_t addAtardecer;
extern uint16_t autoPuerta;  // 0:cerrada, 1:abierta, 2: automatico, 3: autoConMargen
extern uint32_t secAdaptacion;
extern uint16_t diaAdaptado;
extern int16_t dsAddPordia;
extern uint16_t posAbierto;
extern uint16_t posCerrado;

extern uint16_t pObjetivo;
extern uint16_t hayTension;
extern int16_t incAdPormil;
uint8_t puertoAbierto = 0;

const char *estPuertaAutoStr[] = {"cerrada", "abierta","abierta de noche","abierta adaptando gatos"};
const char *estPuertaStr[] = {"cerrada", "abierta"};
static const SerialConfig ser_cfg = {115200, 0, 0, 0, };

void initSerial(void) {
    if (puertoAbierto==1)
        return;
    palClearPad(GPIOA, GPIOA_TX2);
    palSetPad(GPIOA, GPIOA_RX2);
    palSetPadMode(GPIOA, GPIOA_RX2, PAL_MODE_ALTERNATE(7));
    palSetPadMode(GPIOA, GPIOA_TX2, PAL_MODE_ALTERNATE(7));
    sdStart(&SD2, &ser_cfg);
    puertoAbierto = 1;
}

void closeSerial(void) {
    if (puertoAbierto==0)
        return;
    palSetPadMode(GPIOA, GPIOA_RX2,PAL_MODE_INPUT_ANALOG );
    palSetPadMode(GPIOA, GPIOA_TX2,PAL_MODE_INPUT_ANALOG );
    sdStop(&SD2);
    puertoAbierto = 0;
}

void printSerial(const char *msg)
{
    initSerial();
    chprintf((BaseSequentialStream *)&SD2,msg);
    chThdSleepMilliseconds(10);
    closeSerial();
}

void printSerialCPP(const char *msg)
{
  printSerial(msg);
}

void ajustaPuerta(void)
{
    int16_t result;
    int16_t opcion;
    uint8_t estDes;
    uint32_t sec2change;
    chprintf((BaseSequentialStream *)&SD2,"Estado de la puerta: %s\n\r",estPuertaAutoStr[autoPuerta]);
    uint16_t autoPuertaOld = autoPuerta;
    while (1==1)
    {
        opcion = autoPuerta;
        chprintf((BaseSequentialStream *)&SD2,"0 Cerrada siempre (OJO)\n\r");
        chprintf((BaseSequentialStream *)&SD2,"1 Abierta siempre\n\r");
        chprintf((BaseSequentialStream *)&SD2,"2 Abrir y cerrar con el sol\n\r");
        chprintf((BaseSequentialStream *)&SD2,"3 Idem con adaptacion\n\r");
        chprintf((BaseSequentialStream *)&SD2,"4 Salir\n\r");
        result = preguntaNumero((BaseChannel *)&SD2, "Dime opcion", &opcion, 0, 4);
        if (result==2 || (result==0 && opcion==4))
            return;
        if (result==0 && opcion!=autoPuertaOld && opcion>=0 && opcion<=3)
        {
          if (autoPuerta!=3 && opcion==3) // si cambio a adaptacion, reinicializa
              calendar::iniciaSecAdaptacion();
          autoPuerta = opcion;
          if (autoPuerta!=3)
            secAdaptacion = 0;
          escribeVariables();
          calendar::estadoDeseadoPuerta(&estDes, &sec2change);
          if (estDes == 1)
              mueveServoPos(posAbierto,1500);
          else
              mueveServoPos(posCerrado, 1500);
          break;
        }
    }
}


void ajustaPosiciones(void)
{
    int16_t result;
    int16_t opcion;
    int16_t posicion;
    uint8_t estDes;
    uint32_t sec2change;
    while (1==1)
    {
        chprintf((BaseSequentialStream *)&SD2,"Posicion servo abierto: %d, cerrado: %d\n\r",posAbierto, posCerrado);
        chprintf((BaseSequentialStream *)&SD2,"1 Posicion abierto\n\r");
        chprintf((BaseSequentialStream *)&SD2,"2 Posicion cerrado\n\r");
        chprintf((BaseSequentialStream *)&SD2,"3 Salir\n\r");
        result = preguntaNumero((BaseChannel *)&SD2, "Dime opcion", &opcion, 1, 3);
        chprintf((BaseSequentialStream *)&SD2,"\n\r");
        if (result==2 || (result==0 && opcion==3))
            return;
        if (result==0 && opcion==1)
        {
            posicion = posAbierto;
            result = preguntaNumero((BaseChannel *)&SD2, "Posicion abierto", &posicion, 0, 100);
            if (result == 0)
            {
                posAbierto = posicion;
                escribeVariables();
            }
        }
        if (result==0 && opcion==2)
        {
            posicion = posCerrado;
            result = preguntaNumero((BaseChannel *)&SD2, "Posicion cerrado", &posicion, 0, 100);
            if (result == 0)
            {
                posCerrado = posicion;
                escribeVariables();
            }
        }
        calendar::estadoDeseadoPuerta(&estDes, &sec2change);
        if (estDes == 1)
            mueveServoPos(posAbierto, 1500);
        else
            mueveServoPos(posCerrado, 1500);
    }
}

void ajustaAddMinutos(void)
{
    int16_t result;
    int16_t opcion;
    int16_t addMin;
    while (1==1)
    {
        chprintf((BaseSequentialStream *)&SD2,"Minutos adcionales amanecer: %d atardecer: %d\n\r",addAmanecer, addAtardecer);
        chprintf((BaseSequentialStream *)&SD2,"1 Adicionales al amanecer\n\r");
        chprintf((BaseSequentialStream *)&SD2,"2 Adicionales al atardecer\n\r");
        chprintf((BaseSequentialStream *)&SD2,"3 Salir\n\r");
        result = preguntaNumero((BaseChannel *)&SD2, "Dime opcion", &opcion, 1, 3);
        chprintf((BaseSequentialStream *)&SD2,"\n\r");
        if (result==2 || (result==0 && opcion==3))
            return;
        if (result==0 && opcion==1)
        {
            addMin = addAmanecer;
            result = preguntaNumero((BaseChannel *)&SD2, "Minutos adicionales al amanecer", &addMin, -300, 300);
            if (result == 0)
            {
                addAmanecer = addMin;
                escribeVariables();
            }
        }
        if (result==0 && opcion==2)
        {
            addMin = addAtardecer;
            result = preguntaNumero((BaseChannel *)&SD2, "Minutos adicionales al atardecer", &addMin, -300, 300);
            if (result == 0)
            {
                addAtardecer = addMin;
                escribeVariables();
            }
        }
    }
}

void ajustaHora(void)
{
    int16_t ano;
    int16_t mes, dia, hora, min, sec;
    char buff[50];
    struct tm tim;
    int16_t result;
    calendar::rtcGetFecha();
    calendar::gettm(&tim);
    ano = tim.tm_year+100;
    mes = tim.tm_mon+1;
    dia = tim.tm_mday;
    hora = tim.tm_hour;
    min = tim.tm_min;
    sec = tim.tm_sec;
    if (preguntaNumero((BaseChannel *)&SD2, "Anyo", &ano, 2023, 2060) == 2)
        return;
    preguntaNumero((BaseChannel *)&SD2, "Mes", &mes, 1, 12);
    preguntaNumero((BaseChannel *)&SD2, "Dia", &dia, 1, 31);
    preguntaNumero((BaseChannel *)&SD2, "Hora", &hora, 0, 23);
    preguntaNumero((BaseChannel *)&SD2, "Minutos", &min, 0, 59);
    result = preguntaNumero((BaseChannel *)&SD2, "Segundos", &sec, 0, 59);
    if (result==2)
        return;
    ajustaHoraDetallada(ano, mes, dia, hora, min, sec);
    calendar::printFecha(buff,sizeof(buff));
    chprintf((BaseSequentialStream *)&SD2,"Fecha actual UTC: %s\n\r",buff);
}

void ajustaDsAdd(void)
{
    int16_t result;
    int16_t opcion;
    int16_t dsAdd;
    while (1==1)
    {
        chprintf((BaseSequentialStream *)&SD2,"Decimas de segundo dia adicionales: %d\n\r",dsAddPordia);
        chprintf((BaseSequentialStream *)&SD2,"1 Nuevo valor\n\r");
        chprintf((BaseSequentialStream *)&SD2,"2 Salir\n\r");
        result = preguntaNumero((BaseChannel *)&SD2, "Dime opcion", &opcion, 1, 2);
        chprintf((BaseSequentialStream *)&SD2,"\n\r");
        if (result==2 || (result==0 && opcion==2))
            return;
        if (result==0 && opcion==1)
        {
            dsAdd = dsAddPordia;
            result = preguntaNumero((BaseChannel *)&SD2, "Decimas de segundo adicionales/dia", &dsAdd, -400, 400);
            if (result == 0)
            {
                dsAddPordia = dsAdd;
                escribeVariables();
                ajustaCALMP(dsAddPordia);
            }
        }
    }
}


void ajustaIncAD(void)
{
    int16_t result;
    int16_t opcion;
    int16_t incAD;
    while (1==1)
    {
        chprintf((BaseSequentialStream *)&SD2,"Add a A/D %d o/oo\n\r",incAdPormil);
        chprintf((BaseSequentialStream *)&SD2,"1 Nuevo valor\n\r");
        chprintf((BaseSequentialStream *)&SD2,"2 Salir\n\r");
        result = preguntaNumero((BaseChannel *)&SD2, "Dime opcion", &opcion, 1, 2);
        chprintf((BaseSequentialStream *)&SD2,"\n\r");
        if (result==2 || (result==0 && opcion==2))
            return;
        if (result==0 && opcion==1)
        {
            incAD = incAdPormil;
            result = preguntaNumero((BaseChannel *)&SD2, "Nuevo valor o/oo", &incAD, -600, 600);
            if (result == 0)
            {
                incAdPormil = incAD;
                escribeVariables();
            }
        }
    }
}

void opciones(void)
{
    int16_t result;
    int16_t opcion;
    uint8_t estDes;
    uint32_t sec2change;
    uint32_t secActual, secAmanecer, secAnochecer;
    float vBat;
    char buff[50];
    initSerial();
    while (1==1)
    {
        leeVariables();
        calendar::diSecAmanecerAnochecer(&secActual, &secAmanecer, &secAnochecer);
        calendar::estadoDeseadoPuerta(&estDes, &sec2change);
        leeTension(&vBat);
        float porcBat = hallaCapBat(&vBat);
        calendar::rtcGetFecha();
        chprintf((BaseSequentialStream *)&SD2,"\n\r");
        calendar::printFecha(buff,sizeof(buff));
        chprintf((BaseSequentialStream *)&SD2,"Fecha actual UTC: %s\n\r",buff);
        calendar::printHoras(buff,sizeof(buff));
        chprintf((BaseSequentialStream *)&SD2,"Tension bateria:%.2f (%d o/o) compAD:%d o/oo\n\r",vBat,(uint16_t) porcBat,incAdPormil);
        if (tensionCritica())
            chprintf((BaseSequentialStream *)&SD2,"OJO: Tension bateria criticamente baja\n\r");
        chprintf((BaseSequentialStream *)&SD2,"Hora amanecer y atardecer UTC: %s\n\r",buff);
        chprintf((BaseSequentialStream *)&SD2,"sec. amanecer:%d. anochecer:%d\n\r",secAmanecer, secAnochecer);
        chprintf((BaseSequentialStream *)&SD2,"sec. actual:%d, prox. cambio:%d, estado puerta:%d:%s\n\r",secActual, sec2change, estDes,estPuertaStr[estDes]);
        chprintf((BaseSequentialStream *)&SD2,"Minutos adicionales amanecer: %d atardecer: %d  secAdaptacion:%d (diaAdapt:%d)\n\r",addAmanecer, addAtardecer, secAdaptacion, diaAdaptado);
        chprintf((BaseSequentialStream *)&SD2,"Automatizacion de puerta: %d:%s\n\r",autoPuerta, estPuertaAutoStr[autoPuerta]);
        chprintf((BaseSequentialStream *)&SD2,"Correccion diaria de hora: %d ds/dia\n\r",dsAddPordia);
        chprintf((BaseSequentialStream *)&SD2,"Posicion servo abierto: %d, cerrado: %d\n\r\n\r",posAbierto, posCerrado);

        chprintf((BaseSequentialStream *)&SD2,"1 Ajusta fecha y hora\n\r");
        chprintf((BaseSequentialStream *)&SD2,"2 Automatizacion puerta\n\r");
        chprintf((BaseSequentialStream *)&SD2,"3 Posiciones de servos\n\r");
        chprintf((BaseSequentialStream *)&SD2,"4 Minutos adicionales\n\r");
        chprintf((BaseSequentialStream *)&SD2,"5 Correccion de hora\n\r");
        chprintf((BaseSequentialStream *)&SD2,"6 Correccion A/D\n\r");
        chprintf((BaseSequentialStream *)&SD2,"7 Salir\n\r");
        result = preguntaNumero((BaseChannel *)&SD2, "Dime opcion", &opcion, 1, 7);
        chprintf((BaseSequentialStream *)&SD2,"\n\r");
        if (result==2 || (result==0 && opcion==7))
        {
            closeSerial();
            return;
        }
        if (result==0 && opcion==1)
            ajustaHora();
        if (result==0 && opcion==2)
            ajustaPuerta();
        if (result==0 && opcion==3)
            ajustaPosiciones();
        if (result==0 && opcion==4)
            ajustaAddMinutos();
        if (result==0 && opcion==5)
            ajustaDsAdd();
        if (result==0 && opcion==6)
            ajustaIncAD();
    }
}
