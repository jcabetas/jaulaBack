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
}
void ajustaP(uint16_t porcP);
uint8_t ajustaHoraDetallada(uint16_t ano, uint8_t mes, uint8_t dia, uint8_t hora, uint8_t min, uint8_t sec);

extern int16_t addAmanecer;
extern int16_t addAtardecer;
extern uint16_t autoPuerta;  // 0:cerrada, 1:abierta, 2: automatico, 3: autoConMargen
extern uint16_t margenAdaptacionInicial, margenAdaptacion;

extern uint16_t pObjetivo;
extern uint16_t hayTension;

const char *estPuertaStr[] = {"cerrada", "abierta","abierta de noche","abierta adaptando gatos"};
static const SerialConfig ser_cfg = {115200, 0, 0, 0, };

void initSerial(void) {
    palClearPad(GPIOA, GPIOA_TX2);
    palSetPad(GPIOA, GPIOA_RX2);
    palSetPadMode(GPIOA, GPIOA_RX2, PAL_MODE_ALTERNATE(7));
    palSetPadMode(GPIOA, GPIOA_TX2, PAL_MODE_ALTERNATE(7));
    sdStart(&SD2, &ser_cfg);
}

void closeSerial(void) {
    palSetPadMode(GPIOA, GPIOA_RX2,PAL_MODE_INPUT_ANALOG );
    palSetPadMode(GPIOA, GPIOA_TX2,PAL_MODE_INPUT_ANALOG );
    sdStop(&SD2);
}

void ajustaPuerta(void)
{
    int16_t result;
    int16_t opcion;
    chprintf((BaseSequentialStream *)&SD2,"Estado de la puerta: %s\n\r",estPuertaStr[autoPuerta]);
    uint16_t autoPuertaOld = autoPuerta;
    while (1==1)
    {
        opcion = autoPuerta;
        chprintf((BaseSequentialStream *)&SD2,"0 Cerrada siempre (OJO)\n\r");
        chprintf((BaseSequentialStream *)&SD2,"1 Abierta siempre\n\r");
        chprintf((BaseSequentialStream *)&SD2,"2 Abrir y cerrar con el sol\n\r");
        chprintf((BaseSequentialStream *)&SD2,"3 Abierta y cada dia 1 hora menos\n\r");
        chprintf((BaseSequentialStream *)&SD2,"4 Salir\n\r");
        result = preguntaNumero((BaseChannel *)&SD2, "Dime opcion", &opcion, 0, 4);
        //chprintf((BaseSequentialStream *)&SD2,"\n\r");
        if (result==2 || (result==0 && opcion==4))
        {
            closeSerial();
            return;
        }
        if (result==0 && opcion!=autoPuertaOld && opcion>=0 && opcion<=3)
        {
          autoPuerta = opcion;
          if (autoPuerta==3)
          {
              margenAdaptacion = margenAdaptacionInicial;
              chprintf((BaseSequentialStream *)&SD2,"=> margen inicial establecido a %d minutos\n\r",margenAdaptacion);
          }
          escribeVariables();
          break;
        }
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
            addAmanecer = result;
            result = preguntaNumero((BaseChannel *)&SD2, "Minutos adicionales al amanecer", &addMin, -300, 300);
            if (result == 0)
            {
                addAmanecer = addMin;
                escribeVariables();
            }
        }
        if (result==0 && opcion==2)
        {
            addAtardecer = result;
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
    calendar::init();
    calendar::getFecha(&tim);
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



void opciones(void)
{
    int16_t result;
    int16_t opcion;
    char buff[50];
    initSerial();
    while (1==1)
    {
        leeVariables();
        calendar::printFecha(buff,sizeof(buff));
        chprintf((BaseSequentialStream *)&SD2,"\n\r");
        chprintf((BaseSequentialStream *)&SD2,"Fecha actual UTC: %s\n\r",buff);
        chprintf((BaseSequentialStream *)&SD2,"Hora amanecer y atardecer UTC: %s\n\r",buff);
        chprintf((BaseSequentialStream *)&SD2,"Minutos adicionales amanecer: %d atardecer: %d\n\r",addAmanecer, addAtardecer);
        chprintf((BaseSequentialStream *)&SD2,"Estado de la puerta: %s\n\r",estPuertaStr[autoPuerta]);
        chprintf((BaseSequentialStream *)&SD2,"1 Ajusta fecha y hora\n\r");
        chprintf((BaseSequentialStream *)&SD2,"2 Automatizacion puerta\n\r");
        chprintf((BaseSequentialStream *)&SD2,"3 Minutos adicionales\n\r");
        chprintf((BaseSequentialStream *)&SD2,"4 Salir\n\r");
        result = preguntaNumero((BaseChannel *)&SD2, "Dime opcion", &opcion, 1, 4);
        chprintf((BaseSequentialStream *)&SD2,"\n\r");
        if (result==2 || (result==0 && opcion==4))
        {
            closeSerial();
            return;
        }
        if (result==0 && opcion==1)
            ajustaHora();
        if (result==0 && opcion==2)
            ajustaPuerta();
        if (result==0 && opcion==3)
            ajustaAddMinutos();
    }
}
