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

extern "C" {
    void initSerial(void);
    void opciones(void);
}
void ajustaP(uint16_t porcP);
uint8_t ajustaHoraDetallada(uint16_t ano, uint8_t mes, uint8_t dia, uint8_t hora, uint8_t min, uint8_t sec);

extern uint16_t pObjetivo;
extern uint16_t hayTension;

static const SerialConfig ser_cfg = {115200, 0, 0, 0, };

void initSerial(void) {
    palClearPad(GPIOA, GPIOA_TX2);
    palSetPad(GPIOA, GPIOA_RX2);
    palSetPadMode(GPIOA, GPIOA_RX2, PAL_MODE_ALTERNATE(7));
    palSetPadMode(GPIOA, GPIOA_TX2, PAL_MODE_ALTERNATE(7));
    sdStart(&SD2, &ser_cfg);
}



void ajustaHora(void)
{
    int16_t ano;
    int16_t mes, dia, hora, min, sec;
    char buff[50];
    struct tm tim;
    calendar::init();
    calendar::getFecha(&tim);
    ano = tim.tm_year+100;
    mes = tim.tm_mon+1;
    dia = tim.tm_mday;
    hora = tim.tm_hour;
    min = tim.tm_min;
    sec = tim.tm_sec;
    preguntaNumero((BaseChannel *)&SD2, "Anyo", &ano, 2023, 2060);
    preguntaNumero((BaseChannel *)&SD2, "Mes", &mes, 1, 12);
    preguntaNumero((BaseChannel *)&SD2, "Dia", &dia, 1, 31);
    preguntaNumero((BaseChannel *)&SD2, "Hora", &hora, 0, 23);
    preguntaNumero((BaseChannel *)&SD2, "Minutos", &min, 0, 59);
    preguntaNumero((BaseChannel *)&SD2, "Segundos", &sec, 0, 59);
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
    calendar::printFecha(buff,sizeof(buff));
    chprintf((BaseSequentialStream *)&SD2,"Fecha actual UTC: %s\n\r",buff);
    calendar::printHoras(buff,sizeof(buff));
    chprintf((BaseSequentialStream *)&SD2,"Hora amanecer y atardecer UTC: %s\n\r",buff);
    while (1==1)
    {
        chprintf((BaseSequentialStream *)&SD2,"1 Ajusta fecha y hora\n\r");
        chprintf((BaseSequentialStream *)&SD2,"2 Automatizacion puerta\n\r");
        chprintf((BaseSequentialStream *)&SD2,"3 Desfases\n\r");
        chprintf((BaseSequentialStream *)&SD2,"4 Salir\n\r");
        result = preguntaNumero((BaseChannel *)&SD2, "Dime opcion", &opcion, 1, 3);
        chprintf((BaseSequentialStream *)&SD2,"\n\r");
        if (result==0 && opcion==1)
            ajustaHora();
        if (result==0 && opcion==4)
            return;
        if (result==0)
            break;
    }
    chprintf((BaseSequentialStream *)&SD2,"Opcion elegida: %d\n\r",opcion);
    chprintf((BaseSequentialStream *)&SD2,"\n\r");
}
