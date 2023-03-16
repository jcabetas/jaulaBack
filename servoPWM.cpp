/*
 * pwmInts.c
 *
 *  Created on: 03/06/2012, adaptado 15/11/2015 a Flip32,
 *  adaptado y muy simplificado para BlackPill 20/9/2022
 *      Author: joaquin
 *  Salida servo 1  (PWM1, PB6, TIM4_CH1, AF02)
 */

#include "hal.h"
#include "chprintf.h"
#include "alimCalle.h"

extern "C" {
    void mueveServoPosC(uint16_t porcPosicion);
}


static PWMConfig pwmcfg = {
  3125000, /* 48 MHz PWM clock frequency */
    62500,   /* PWM period 20 millisecond */
  NULL,  /* No callback */
  /* Channel 4 enabled */
  {
    {PWM_OUTPUT_DISABLED, NULL},
    {PWM_OUTPUT_DISABLED, NULL},
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
    {PWM_OUTPUT_DISABLED, NULL},
  },
  0,
  0,
  0
};

void initServo(void)
{
/*
 *   Salida servo: TIM4CH3 (PB8)
 */
    palSetPadMode(GPIOB, GPIOB_PWMSERVO,PAL_MODE_ALTERNATE(2) | PAL_STM32_OSPEED_HIGHEST);
//    palSetPadMode(GPIOB, GPIOB_ONSERVO, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOB, GPIOB_ONSERVO, PAL_MODE_OUTPUT_OPENDRAIN);
    palSetPad(GPIOB, GPIOB_ONSERVO);
	pwmStart(&PWMD4, &pwmcfg);
}

void closeServo(void)
{
    palSetPadMode(GPIOB, GPIOB_PWMSERVO,PAL_MODE_INPUT_ANALOG );
    palSetPad(GPIOB, GPIOB_ONSERVO);
    pwmStop(&PWMD4);
}

void mueveServoAncho(uint16_t ancho)
{
  initServo();
  palClearPad(GPIOB, GPIOB_ONSERVO);
  // minimo 2980, maximo 6068, medio 4529
  if (ancho<2980) ancho=2980;
  if (ancho>6068) ancho=6068;
  if (PWMD4.state==PWM_STOP)
      pwmStart(&PWMD4, &pwmcfg);
  pwmEnableChannel(&PWMD4, 2, ancho);
  chThdSleepMilliseconds(2000);
  closeServo();
}

void mueveServoPos(uint16_t porcPosicion)
{
  uint16_t ancho;
  if (porcPosicion>100) porcPosicion=100;
  ancho = (uint16_t) (2980.0f+3088.0f*porcPosicion/100.0f);
  mueveServoAncho(ancho);
}

void mueveServoPosC(uint16_t porcPosicion)
{
    mueveServoPos(porcPosicion);
}

