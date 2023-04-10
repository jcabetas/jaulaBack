/*
 * timer.cpp
 *
 *  Created on: 8 abr 2023
 *      Author: joaquin
 */


/*
 * qei.c
 *
 *  Created on: 27 jul. 2019
 *      Author: joaquin
 */

#include "hal.h"
#include "chprintf.h"
#include "variables.h"

#define NUMPULSOS 1000

uint16_t numPulsos;
uint32_t contadorIni, contadorFin, contadorOld;
float secPorDia;
extern int16_t dsAddPordia;
bool yaEsta;

void printSerialCPP(const char *msg);


static void gps_pulse(void *)
{
    uint32_t contActual = TIM2->CNT;
    if (numPulsos == 0)
    {
        TIM2->CNT = 0;
        contadorIni = TIM2->CNT;
        contadorOld = contadorIni;
        numPulsos++;
        return;
    }
    if (numPulsos++ == NUMPULSOS)
    {
        contadorFin = contActual;
        yaEsta = true;
    }
    else
    {
        uint32_t pulso = contActual - contadorOld;
        if (pulso<510 || pulso>514)
        {
            // pulso raro, reinicio
            numPulsos = 0;
            TIM2->CNT = 0;
            contadorIni = TIM2->CNT;
            contadorOld = contadorIni;
        }
        else
            contadorOld = contActual;
    }
}

void calibraConGPS(void)
{
    uint16_t secsEspera;
    char buff[60];
    // configuro RTC para que saque oscilador por RTC_OUT (PC13)
    // conectar RTC_OUT (PC13) a trigger TIM2 TIM2_ETR (PA15)
    // conectar pulsos GPS a PA1
    // Segun https://ucilnica.fri.uni-lj.si/pluginfile.php/182326/mod_resource/content/0/Using%20the%20hardware%20real-time%20clock%20%28RTC%29.pdf
    // PREDIV_A[5] must be set to 1 to enable the RTC_CALIB output signal generation. If PREDIV_A[5] = 0, no signal is output on RTC_CALIB
    // Por tanto, en RTCv2 cambio STM32_RTC_PRESA_VALUE a 128 y STM32_RTC_PRESS_VALUE a 256
    palSetPadMode(GPIOA, GPIOA_PIN15, PAL_MODE_ALTERNATE(1));
    palSetPadMode(GPIOA, GPIOA_SENS2,  PAL_MODE_INPUT);
    RTCD1.rtc->WPR = 0xCA;       // Disable write protection
    RTCD1.rtc->WPR = 0x53;
    RTCD1.rtc->CR = RTC_CR_COE;  // Calibration output enable
    RTCD1.rtc->WPR = 0xFF;
    // configuro TIM2 como contador
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    rccEnableTIM2(TRUE);
    rccResetTIM2();
    TIM2->PSC = 1;
    TIM2->SMCR |= TIM_SMCR_ECE; // entrada por TIM2_ETR
    TIM2->SMCR |= ((0b111)<< TIM_SMCR_TS_Pos); // trigger: External Trigger input (ETRF)
    TIM2->CR1 |= TIM_CR1_CEN; // activa contador (upcounter)
    // cuento durante varios segundos
    yaEsta = false;
    numPulsos = 0;
    secsEspera = 0;
    palEnableLineEvent(LINE_A1_SENS1, PAL_EVENT_MODE_RISING_EDGE);//PAL_EVENT_MODE_RISING_EDGE);
    palSetLineCallback(LINE_A1_SENS1, gps_pulse, NULL);
    while(!yaEsta && secsEspera<60)
    {
        if (numPulsos==0)
            secsEspera++;
        chThdSleepMilliseconds(1000);
        if (numPulsos>10 && (numPulsos%10)==0)
        {
            secPorDia = (contadorOld/((float) (numPulsos-1.0f))/512.0f-1.0f)*86400.0f;
            chsnprintf(buff,sizeof(buff),"Con %d pulsos, secDia:%.1f\n\r", numPulsos, secPorDia);
            printSerialCPP(buff);
        }
    }
    if (yaEsta)
    {
        secPorDia = (contadorFin/((float) (numPulsos-1.0f))/512.0f-1.0f)*86400.0f;
        chsnprintf(buff,sizeof(buff),"Resultado secDia:%.1f\n\r", secPorDia);
        printSerialCPP(buff);
        dsAddPordia = -10.0f*secPorDia;
        escribeVariables();
    }
    else
        printSerialCPP("No detecto pulsos de GPS, no puedo ajustar desvio de RTC\n\r");
    palDisableLineEvent(LINE_A1_SENS1);
    RTCD1.rtc->WPR = 0xCA;       // Disable write protection
    RTCD1.rtc->WPR = 0x53;
    RTCD1.rtc->CR &= ~RTC_CR_COE;  // Calibration output disable
    TIM2->CR1 &= ~TIM_CR1_CEN;
    RTCD1.rtc->WPR = 0xFF;
    rccEnableTIM2(FALSE);
    RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
    rccResetTIM2();
    palSetPadMode(GPIOA, GPIOA_PIN15, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOA, GPIOA_SENS2, PAL_MODE_INPUT_ANALOG);
}
