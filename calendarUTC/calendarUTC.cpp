#include "ch.hpp"
#include "hal.h"
using namespace chibios_rt;
#include "string.h"
#include "chprintf.h"
#include "../calendarUTC/calendarUTC.h"
#include "gets.h"
#include "stdlib.h"
#include "math.h"

void rtcSetTM(RTCDriver *rtcp, struct tm *tim, uint16_t ds, uint8_t esHoraVerano);
void rtcGetTM(RTCDriver *rtcp, struct tm *tim, uint16_t *ds);
void completeYdayWday(struct tm *tim);
uint8_t dayofweek(uint16_t y, uint16_t m, uint16_t d);
void printSerial(char *msg);
void printSerialCPP(char *msg);


extern int16_t addAmanecer;
extern int16_t addAtardecer;
extern uint16_t autoPuerta;  // 0:cerrada, 1:abierta, 2: automatico, 3: autoConMargen

extern struct queu_t colaMsgTxCan;
extern event_source_t sendMsgCAN_source;

uint16_t calendar::minAmanecer = 370;
uint16_t calendar::minAnochecer = 1194;
uint16_t calendar::diaCalculado = 9999;
float calendar::longitudRad = -0.0697766927f; //  -3.99791
float calendar::latitudRad = 0.7058975433f;   //  40.44495
time_t calendar::fechaCambioVer2Inv = 0;
time_t calendar::fechaCambioInv2Ver = 0;
struct fechaHora calendar::fechaHoraNow = {0,0};
struct tm calendar::fechaNow = {0,0,0,0,0,0,0,0,0};

extern "C"
{
    void leeHora(struct tm *tmAhora);
    void actualizaAmanAnoch(void);
    void estadoDeseadoPuertaC(uint8_t *estDes, uint16_t *sec2change);
}


// segundos Unix en UTC
time_t calendar::getSecUnix(struct tm *tm)
{
    // Month-to-day offset for non-leap-years.
    static const int month_day[12] =
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    // Most of the calculation is easy; leap years are the main difficulty.
    int16_t month = tm->tm_mon % 12;
    uint16_t year = tm->tm_year + tm->tm_mon / 12;
    if (month < 0) {   // Negative values % 12 are still negative.
        month += 12;
        --year;
    }

    // This is the number of Februaries since 1900.
    const uint16_t year_for_leap = (month > 1) ? year + 1 : year;

    time_t rt = tm->tm_sec                          // Seconds
        + 60 * (tm->tm_min                          // Minute = 60 seconds
        + 60 * (tm->tm_hour                         // Hour = 60 minutes
        + 24 * (month_day[month] + tm->tm_mday - 1  // Day = 24 hours
        + 365 * (year - 70)                         // Year = 365 days
        + (year_for_leap - 69) / 4                  // Every 4 years is     leap...
        - (year_for_leap - 1) / 100                 // Except centuries...
        + (year_for_leap + 299) / 400)));           // Except 400s.

//    // añade desvio respecto UTM
//    if (tm->tm_isdst==0)
//        rt -= 3600;
//    else
//        rt -= 7200;

    return rt < 0 ? -1 : rt;
}


time_t calendar::getSecUnix(void) {
    return fechaHoraNow.secsUnix;
}

void calendar::getFecha(struct tm *fecha)
{
    memcpy(fecha, &fechaNow, sizeof(fechaNow));
}

void calendar::getFechaHora(struct fechaHora *fechHora)
{
    fechHora->secsUnix = fechaHoraNow.secsUnix;
    fechHora->dsUnix = fechaHoraNow.dsUnix;
}

uint8_t calendar::getDOW(void)
{
    return fechaNow.tm_wday;
}

//Franjas horarias Tarifa de acceso 2.0TD (Península, Baleares y Canarias)
// Punta: 10:00 - 14:00 y 18:00 - 22:00
// Llano: 08:00 - 10:00, 14:00 - 18:0 y  22:00 - 00:00
// Valle: 00:00 - 08:00 y findes
enum getPeriodoTarifa calendar::getPeriodoTarifa(void)
{
    struct tm fecha;
    getFecha(&fecha);
    if (fecha.tm_wday==0 || fecha.tm_wday==6 || fecha.tm_hour<8)
        return valle;
    if ((fecha.tm_hour>=10 && fecha.tm_hour<14) || (fecha.tm_hour>=18 && fecha.tm_hour<22))
        return punta;
    return llano;
}



uint32_t calendar::dsDiff(struct fechaHora *fechHoraOld)
{
    if (fechaHoraNow.secsUnix < fechHoraOld->secsUnix)
    {
        // se ha debido cambiar la hora, machacamos la hora antigua con la actual
        fechHoraOld->dsUnix =  fechaHoraNow.dsUnix;
        fechHoraOld->secsUnix =  fechaHoraNow.secsUnix;
    }
    uint32_t ds = 10*(fechaHoraNow.secsUnix - fechHoraOld->secsUnix) + fechaHoraNow.dsUnix - fechHoraOld->dsUnix;
    return ds;
}

uint32_t calendar::sDiff(time_t *timetOld)
{
    if (fechaHoraNow.secsUnix < *timetOld)
    {
        // se ha debido cambiar la hora, machacamos la hora antigua con la actual
        *timetOld = fechaHoraNow.secsUnix;
    }
    return (fechaHoraNow.secsUnix - *timetOld);
}


uint32_t calendar::sDiff(struct fechaHora *fechHoraOld)
{
    if (fechaHoraNow.secsUnix < fechHoraOld->secsUnix)
    {
        // se ha debido cambiar la hora, machacamos la hora antigua con la actual
        fechHoraOld->dsUnix =  fechaHoraNow.dsUnix;
        fechHoraOld->secsUnix =  fechaHoraNow.secsUnix;
    }
    return (uint32_t) (fechaHoraNow.secsUnix - fechHoraOld->secsUnix);
}

void calendar::cambiaFecha(uint16_t *anyo, uint8_t *mes, uint8_t *dia, uint8_t *hora, uint8_t *min, uint8_t *seg, uint8_t *dsPar)
{
    struct tm fechaUTC;
    uint16_t ds;

    rtcGetTM(&RTCD1, &fechaUTC, &ds);         // leo hora local
    if (anyo!=NULL && *anyo>2020 && *anyo<3000) // actualizo datos con lo que hayan pasado
        fechaUTC.tm_year = *anyo-1900;
    if (mes!=NULL && *mes>=1 && *mes<=12)
        fechaUTC.tm_mon = *mes-1;
    if (dia!=NULL && *dia>=1 && *dia<=31)
        fechaUTC.tm_mday = *dia;
    if (hora!=NULL && *hora<=23)
        fechaUTC.tm_hour = *hora;
    if (min!=NULL && *min<=59)
        fechaUTC.tm_min = *min;
    if (seg!=NULL && *seg<=59)
        fechaUTC.tm_sec = *seg;
    if (dsPar!=NULL && *dsPar<=9)
        ds = *dsPar;
    rtcSetFecha(&fechaUTC,ds);
    init();
}

void calendar::cambiaFechaTM(uint8_t anyo, uint8_t mes, uint8_t dia, uint8_t hora, uint8_t min, uint8_t seg, uint8_t dsPar)
{
    struct tm fechaUTC;
    uint16_t anyoReal, ds;

    rtcGetTM(&RTCD1, &fechaUTC, &ds);         // leo hora local
    anyoReal = 1900 + anyo;
    mes += 1;
    if (anyoReal>2020 && anyoReal<3000) // actualizo datos con lo que hayan pasado
        fechaUTC.tm_year = anyo;
    if (mes>=1 && mes<=12)
        fechaUTC.tm_mon = mes-1;
    if (dia>=1 && dia<=31)
        fechaUTC.tm_mday = dia;
    if (hora<=23)
        fechaUTC.tm_hour = hora;
    if (min<=59)
        fechaUTC.tm_min = min;
    if (seg<=59)
        fechaUTC.tm_sec = seg;
    if (dsPar<=9)
        ds = dsPar;
    rtcSetFecha(&fechaUTC,ds);
}


void calendar::rtcSetFecha(struct tm *fecha, uint16_t ds)
{
    fechaHoraNow.secsUnix = calendar::getSecUnix(fecha);
    fechaHoraNow.dsUnix = ds;
    rtcSetTM(&RTCD1, fecha, ds);
    init();
    //fecha::actualizaCuandoPuedas();
}

/*
 * ver https://www.esrl.noaa.gov/gmd/grad/solcalc/solareqns.PDF
 * Altura solar ajustada a 93 grados (en 96 es demasiado oscuro)
 */
void calendar::ajustaHorasLuz(void)
{
    float diaAno, fraccAno;
    float decl, ha;

    if (diaCalculado == fechaNow.tm_yday)
        return; // ya estaba calculado
    diaAno = (float) (fechaNow.tm_yday);
    fraccAno = 2.0*M_PI/365.0*(diaAno-0.5f);
//    eqTime = +229.18f*(0.000075f+0.001868f*cos(fraccAno)-0.032077f*sin(fraccAno)-0.014615f*cos(2.0f*fraccAno)-0.040849f*sin(2.0f*fraccAno));
    decl = 0.006918f-0.399912f*cosf(fraccAno) + 0.070257f*sinf(fraccAno)-0.006758f*cosf(2.0f*fraccAno)+0.000907f*sinf(2.0f*fraccAno)-0.002697f*cosf(3.0f*fraccAno) + 0.00148f*sinf(3.0f*fraccAno);
    ha = -acosf(cosf(93.0f*M_PI/180.0f)/cosf(latitudRad)/cosf(decl)-tanf(latitudRad)*tan(decl));
    minAmanecer = 60.0f*(12.0f+ha*12.0f/M_PI-longitudRad*12.0f/M_PI);
    minAnochecer = 60.0*(12.0f-ha*12.0f/M_PI-longitudRad*12.0f/M_PI);
    diaCalculado = fechaNow.tm_yday;
}


// debe ser llamado al inicio, en cambio de fecha, o cuando cambia el año
void calendar::ajustaFechasCambHorario(void)
{
    struct tm fecha;
    uint8_t dow;

    fecha.tm_year = fechaNow.tm_year;
    fecha.tm_mon = 2; // enero = 0
    fecha.tm_mday = 31;
    fecha.tm_hour = 1;// son las 2am hora local
    fecha.tm_min = 0;
    fecha.tm_sec = 0;
    fecha.tm_isdst = 0;
    dow = dayofweek(fecha.tm_year+1900, fecha.tm_mon+1, fecha.tm_mday);
    fecha.tm_mday -= dow;
    completeYdayWday(&fecha);
    // segundos Unix en UTC
    fechaCambioInv2Ver = calendar::getSecUnix(&fecha);

    // repetimos para el cambio de octubre
    fecha.tm_year = fechaNow.tm_year;
    fecha.tm_mon = 9; // enero = 0
    fecha.tm_mday = 31;
    fecha.tm_hour = 1; // son las 3am hora local
    fecha.tm_min = 0;
    fecha.tm_sec = 0;
    fecha.tm_isdst = 1;
    dow = dayofweek(fecha.tm_year+1900, fecha.tm_mon+1, fecha.tm_mday);
    fecha.tm_mday -= dow;
    fechaCambioVer2Inv = calendar::getSecUnix(&fecha);
    
}

uint8_t calendar::esHoraVerano(void)
{
    if (fechaHoraNow.secsUnix>fechaCambioInv2Ver && fechaHoraNow.secsUnix<fechaCambioVer2Inv)
        return 1;
    else
        return 0;

}

// deberia llamarse cada hora
// no hace falta al estar todo en UTC
//void calendar::checkDstFlagVerano(void)
//{
//    if (esHoraVerano())
//    {
//        // estamos en hora de verano. Comprueba si hay que actualizar hora y dstflag
//        if (fechaNow.tm_isdst==0)
//        {
//            fechaNow.tm_hour += 1;
//            rtcSetTM(&RTCD1, &fechaNow, 0, 1);
//        }
//    }
//    else
//    {
//        // estamos en hora de invierno. Comprueba si hay que actualizar dstflag
//        if (fechaNow.tm_isdst==1)
//        {
//            fechaNow.tm_hour -= 1;
//            rtcSetTM(&RTCD1, &fechaNow, 0, 0);
//        }
//    }
//}

uint8_t calendar::esDeNoche(void)
{
    uint16_t minutosNow;
    minutosNow = 60*fechaNow.tm_hour + fechaNow.tm_min;
    if (minutosNow<minAmanecer || minutosNow>minAnochecer)
    	return 1;
    else
    	return 0;
}

void calendar::printHoras(char *buff, uint16_t longBuff)
{
    uint8_t hAma,minAma,hNoche,minNoche;
    hAma = minAmanecer/60;
    minAma = minAmanecer - 60*hAma;
    hNoche = minAnochecer/60;
    minNoche = minAnochecer - 60*hNoche;
    chsnprintf(buff,longBuff,"%d:%02d-%d:%02d",hAma,minAma,hNoche,minNoche);
}

void calendar::printFecha(char *buff, uint16_t longBuff)
{
    struct tm tim;
    calendar::init();
    calendar::getFecha(&tim);
    chsnprintf(buff,longBuff,"%d/%d/%d %d:%d:%d",tim.tm_mday,tim.tm_mon+1,tim.tm_year-100,tim.tm_hour,tim.tm_min,tim.tm_sec);
}


void calendar::setLatLong(float latitudRadNoche, float longitudRadNoche)
{
    latitudRad = latitudRadNoche;
    longitudRad = longitudRadNoche;
}


void calendar::init(void)
{
    struct tm tim;
    uint16_t ds;
    rtcGetTM(&RTCD1, &tim, &ds);
    memcpy(&fechaNow, &tim, sizeof(fechaNow));
    fechaHoraNow.secsUnix = getSecUnix(&tim);
    fechaHoraNow.dsUnix = ds;
    //ajustaFechasCambHorario(); no se usa, hay que ponerlo cuando se lee la hora si se quiere hora local
    ajustaHorasLuz();
}


// habría que llamarle cada ds
void calendar::updateEveryDs(void)
{
    struct tm tim;
    uint16_t ds;
    rtcGetTM(&RTCD1, &tim, &ds);
    memcpy(&fechaNow, &tim, sizeof(fechaNow));
    fechaHoraNow.secsUnix = getSecUnix(&tim);
    fechaHoraNow.dsUnix = ds;
    if (fechaNow.tm_sec==0 && fechaNow.tm_min==0)
    {
        // cada hora
        if (fechaNow.tm_hour==0)
        {
            // cada dia
            ajustaHorasLuz();
            if (fechaNow.tm_mday==1 && fechaNow.tm_mon==0)
            {
                // cada anyo
                ajustaFechasCambHorario();
            }
        }
    }
}

void actualizaAmanAnoch(void)
{
    calendar::ajustaHorasLuz();
}

void leeHora(struct tm *tmAhora)
{
    calendar::init();
    calendar::getFecha(tmAhora);
}

void calendar::diSecAmanecerAnochecer(uint32_t *secActual, uint32_t *secAmanecer, uint32_t *secAnochecer)
{
    struct tm tim;
    calendar::init();
    calendar::getFecha(&tim);
    *secActual = 3600L*tim.tm_hour+60L*tim.tm_min + tim.tm_sec;
    *secAmanecer = 60*minAmanecer;
    *secAnochecer = 60*minAnochecer;
}

void calendar::estadoDeseadoPuerta(uint8_t *estDes, uint16_t *sec2change)
{
    uint32_t secActual, secAmanecer, secAnochecer, secsProxCambio;
    char buffer[100];
    calendar::diSecAmanecerAnochecer(&secActual, &secAmanecer, &secAnochecer);
    if (secActual<secAmanecer)  // antes de amanecer
        secsProxCambio = secAmanecer - secActual + 45;
    else if (secActual<secAnochecer) // de dia
        secsProxCambio = secAnochecer - secActual + 45;
    else // ya de noche, prox cambio a las 1am del dia siguiente
        secsProxCambio = (uint16_t)(86400L-secActual + 3600L + 45L);
    *sec2change = secsProxCambio;
    // ponemos estado de puerta
    if (autoPuerta==0)
        *estDes = 0;
    else if (autoPuerta==1)
        *estDes = 1;
    else if (autoPuerta==2)
    {
        if (secActual>=secAmanecer+addAmanecer && secActual<=secAnochecer+addAtardecer)
            *estDes = 0;
        else
            *estDes = 1;
    }
    else if (autoPuerta==3)
    {
      if (secActual>=secAmanecer && secActual<=secAnochecer)
          *estDes = 1;
      else
          *estDes = 0;
    }
//    chsnprintf(buffer,sizeof(buffer),"Calendar, auto puerta:%d estado puerta:%d\n\r",autoPuerta, *estDes);
//    printSerialCPP(buffer);
}

void estadoDeseadoPuertaC(uint8_t *estDes, uint16_t *sec2change)
{
    calendar::estadoDeseadoPuerta(estDes, sec2change);
}

