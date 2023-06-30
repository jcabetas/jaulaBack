/*
 * gps.c
 */


#include "hal.h"

#include "tty.h"
#include "gets.h"
#include "string.h"
#include "math.h"
#include <stdlib.h>

#include "calendarUTC/calendarUTC.h"
#include "chprintf.h"
#include "variables.h"


extern "C"
{
    void leeGPS(void);
}

void printSerialCPP(const char *msg);
void calibraConGPS(uint8_t soloCheck);

/*
 * Para leer hora:
 * - Arranca un proceso para conectar el GPS
 * - Cuando se recibe la hora, pone el reloj y
 *   termina el proceso
 * Verde GPS = TXGPS -> A10 (RX1)
 */

static const SerialConfig ser_cfg = {9600, 0, 0, 0, };

extern int16_t dsAddPordia;
uint8_t hayGps;
uint8_t llamadasAGPS = 0;
uint8_t estadoAjusteGps = 0;
struct fechaHora fechHoraOldGps;


/*
 *   struct tm
    int tm_sec       seconds after minute [0-61] (61 allows for 2 leap-seconds)
    int tm_min       minutes after hour [0-59]
    int tm_hour      hours after midnight [0-23]
    int tm_mday      day of the month [1-31]
    int tm_mon       month of year [0-11]
    int tm_year      current year-1900
    int tm_wday      days since Sunday [0-6]
    int tm_yday      days since January 1st [0-365]
    int tm_isdst     daylight savings indicator (1 = yes, 0 = no, -1 = unknown)
 *
 */

// $GPRMC,082126.00,A,4023.11591,N,00421.89349,W,0.047,,061022,,,A*69
// => 08:21:26 Z  06/10/22
// 0=>fijo hora inicial, 1=>ajustoDesfase, 2=>compruebo desfase y reajusto, 3=> ajuste periodico
/*
 * extern int16_t dsAddPordia;
uint8_t hayGps;
uint8_t estadoAjusteGps = 0;
time_t secLecInicial;
 *
 */

uint8_t ajustaHora(char *horaStr,char *fechaStr)
{
    struct tm tmGPS;
    if (strlen(horaStr)<6 || strlen(fechaStr)<6)
        return 0;
    tmGPS.tm_year = atoi(&fechaStr[4])+100;
    fechaStr[4] = 0;
    tmGPS.tm_mon = atoi(&fechaStr[2])-1;
    fechaStr[2] = 0;
    tmGPS.tm_mday = atoi(fechaStr);
    // 082126.56
    uint16_t cs = atoi(&horaStr[7]);
    uint16_t ds = cs/10;
    horaStr[6] = 0;
    tmGPS.tm_sec = atoi(&horaStr[4]);
    horaStr[4] = 0;
    tmGPS.tm_min = atoi(&horaStr[2]);
    horaStr[2] = 0;
    tmGPS.tm_hour = atoi(horaStr);
    tmGPS.tm_isdst = 0;
    calendar::completeYdayWday(&tmGPS);
    // tengo la fecha actual, procesala
    calendar::rtcSetFecha(&tmGPS, ds);
    calendar::getFechaHora(&fechHoraOldGps);
    printSerialCPP("En ajustaHora, he ajustado RTC\n\r");
    calendar::vuelcaFecha();
    printSerialCPP("\n\r");
    if (estadoAjusteGps==0)
        estadoAjusteGps = 1;
    return 1;
}


uint8_t ajustaHoraDetallada(uint16_t ano, uint8_t mes, uint8_t dia, uint8_t hora, uint8_t min, uint8_t sec)
{
    struct tm fecha;
    fecha.tm_year = ano - 1900;
    fecha.tm_mon = mes-1;
    fecha.tm_mday = dia;
    fecha.tm_sec = sec;
    fecha.tm_min = min;
    fecha.tm_hour = hora;
    fecha.tm_isdst = 0;
    uint16_t ds = 0;
    calendar::completeYdayWday(&fecha);
    rtcSetTM(&RTCD1, &fecha, ds);
    // horaStr = "082126.00"
    // fechaStr = "061022"
    // => 08:21:26 Z  06/10/22getFecha
    return 1;
}

// latStr  = "4026.73829", N
// longStr = "00359.88675", W
void setPos(char *latStr, char *NS, char *longStr, char *EW)
{
    if (strlen(latStr)<8 || strlen(longStr)<8)
        return;
    float latMin = atof(&latStr[2]);
    latStr[2] = 0;
    float latDeg = atof(latStr);
    float latDegree = (latDeg+latMin/60.0f)* M_PI/180.0f;
    if (!strcmp(NS,"S"))
        latDegree = -latDegree;
    float longMin = atof(&longStr[3]);
    longStr[3] = 0;
    float longDeg = atof(longStr);
    float longDegree = (longDeg+longMin/60.0f)* M_PI/180.0f;
    if (!strcmp(EW,"W"))
        longDegree = -longDegree;
    calendar::setLatLong(latDegree,longDegree);
}

uint8_t startGPS(void)
{
    char buffer[100];
    uint8_t huboTimeout;
    printSerialCPP("Conecto GPS...\n\r");
    palSetPadMode(GPIOB, GPIOB_ONGPS, PAL_MODE_OUTPUT_PUSHPULL);
    ACTIVAPAD(GPIOB, GPIOB_ONGPS);
    palSetPad(GPIOA, GPIOA_RX1);
    palSetPadMode(GPIOA, GPIOA_RX1, PAL_MODE_ALTERNATE(7));
    sdStart(&SD1, &ser_cfg);
    chgetsNoEchoTimeOut((BaseChannel *)&SD1, (uint8_t *) buffer,sizeof(buffer), TIME_MS2I(2000), &huboTimeout);
    if (huboTimeout)
    {
        hayGps = 0;
        printSerialCPP("No hay GPS\n\r");
        return 0;
    }
    return 1;
}

void stopGPS(void)
{
    QUITAPAD(GPIOB, GPIOB_ONGPS);
    sdStop(&SD1);
    palSetPadMode(GPIOA, GPIOA_RX1, PAL_MODE_INPUT_ANALOG);
}

// devuelve 1 si hay GPS y consigue hora
void leeGPS(void)
{
    char buffer[100];
    uint8_t huboTimeout;
    char *parametros[30];
    uint16_t dsEsperandoHora = 0;
    uint16_t numParam;

    if (!hayGps)
        return;
    // procede leer GPS si:
    // - No se ha leido nunca (estadoAjusteGps==0)
    // - Ha pasado una semana sin leerse
    // - no hay calibracion RTC (dsAddPordia==0)
    calendar::rtcGetFecha();
    uint32_t sDiffInit = calendar::sDiff(&fechHoraOldGps);
    if (estadoAjusteGps!=0 && sDiffInit<604800L && dsAddPordia!=0)
        return;
    if (!startGPS())
    {
        stopGPS();
        return;
    }
    //
    while (true)
    {
        if (++dsEsperandoHora>900)
        {
            hayGps = 0;
            printSerialCPP("Demasiado tiempo esperando la hora en GPS\n\r");
            break;
        }
        chgetsNoEchoTimeOut((BaseChannel *)&SD1, (uint8_t *) buffer,sizeof(buffer), TIME_MS2I(100), &huboTimeout);
        if (huboTimeout)
            continue;
        parseStr(buffer,parametros, ",",&numParam);
        if (numParam<5)
            continue;
        // $GPRMC,082126.00,A,4023.11591,N,00421.89349,W,0.047,,061022,,,A*69
        // => 08:21:26 Z  06/10/22
        // aunque no tengamos ubicacion, compruebo la hora
        if (numParam>10 && !strncmp("$GPRMC",parametros[0],10))
        {
            if (ajustaHora(parametros[1],parametros[9]))
            {
                hayGps = 1;
                break;
            }
        }
    }
    if (hayGps && dsAddPordia==0)
    {
        calibraConGPS(0);
    }
    stopGPS();
}


/*

$GPGSA,A,3,19,14,01,17,,,,,,,,,4.46,2.58,3.63*07
$GPGSV,3,1,12,01,67,058,20,04,33,171,15,08,17,159,08,09,06,195,14*7C
$GPGSV,3,2,12,14,18,254,31,17,45,307,16,19,16,318,27,21,50,094,13*7B
$GPGSV,3,3,12,22,27,050,15,28,20,283,28,31,03,082,,32,06,035,*7F
$GPGLL,4023.11590,N,00421.89340,W,082125.00,A,A*7F
$GPRMC,082126.00,A,4023.11591,N,00421.89349,W,0.047,,061022,,,A*69
$GPVTG,,T,,M,0.047,N,0.086,K,A*2E
$GPGGA,082126.00,4023.11591,N,00421.89349,W,1,04,2.58,675.8,M,50.1,M,,*42
$GPGSA,A,3,19,14,01,17,,,,,,,,,4.45,2.58,3.63*04

*/



