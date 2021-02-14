#include "ch.hpp"
#include "hal.h"
#include "string.h"
#include "chprintf.h"


using namespace chibios_rt;

extern "C"
{
    void pruebaADC(void);
}

/*
 * La tension se lee en PA7 (ADC7), a traves de divisor con factor 0,69894
 *   El fondo de escala es 4096 corresponde a 3.3 V
 *   Por tanto la tension es ADC*3.314/(0,69894*4096) = ADC*0,001157584
 * La temperatura  es Vtemp = 0,76 + 0.0025*(T-25)
 *   => T = 25 + (V - 0,76)/0.025
 */

#define ADC_GRP1_NUM_CHANNELS   2
#define ADC_GRP1_BUF_DEPTH      8

// Create buffer to store ADC results. This is
// one-dimensional interleaved array

adcsample_t samples_buf[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH]; // results array

static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {

  (void)adcp;
  (void)err;
}

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 8 samples of 1 channel, SW triggered.
 * Channels:    IN11.
 */
static const ADCConversionGroup adcgrpcfg1 = {
  FALSE,
  ADC_GRP1_NUM_CHANNELS,
  NULL,
  adcerrorcallback,
  0,                        /* CR1 */
  ADC_CR2_SWSTART,          /* CR2 */
  0,                        /* SMPR1 */
  ADC_SMPR2_SMP_AN7(ADC_SAMPLE_480), /* SMPR2 */
  0,                        /* HTR */
  0,                        /* LTR */
  0,                        /* SQR1 */
  0,  /* SQR2 */
  ADC_SQR3_SQ1_N(ADC_CHANNEL_IN7) | ADC_SQR3_SQ2_N(ADC_CHANNEL_SENSOR)
};




/* Lee tensiones */
void leeTensiones(float *vBat, float *T)
{
	adcConvert(&ADCD1, &adcgrpcfg1, &samples_buf[0], ADC_GRP1_BUF_DEPTH);
	*vBat = samples_buf[0]*0.001157584f;
	uint16_t TS_CAL1 = *(volatile uint16_t*)0x1FFF7A2C; //  943 @30C
    uint16_t TS_CAL2 = *(volatile uint16_t*)0x1FFF7A2E; // 1209 @110C
    *T = 30.0f + 80.0f*(samples_buf[1]-TS_CAL1)/(TS_CAL2 - TS_CAL1);
}



void pruebaADC(void)
{
    char buff[30];
    float vBat,temp;
    palSetPadMode(GPIOA, 7, PAL_MODE_INPUT_ANALOG); // this is 7th channel
    adcStart(&ADCD1,NULL);
    adcSTM32EnableTSVREFE();
    leeTensiones(&vBat,&temp);
    adcStop(&ADCD1);
    adcSTM32DisableTSVREFE();
    chsnprintf(buff,sizeof(buff),"Vbat:%.3f T:%.1f C\r\n",vBat,temp);
}



