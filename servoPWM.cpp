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
    void cierraPuertaC(void);
    void abrePuertaC(void);
}

extern uint16_t posAbierto;
extern uint16_t posCerrado;

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

bool tensionCritica(void);


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

void mueveServoAncho(uint16_t ancho, uint16_t ms)
{
  initServo();
  palClearPad(GPIOB, GPIOB_ONSERVO);
  // minimo 2980, maximo 6068, medio 4529
  if (ancho<2700) ancho=2700;
  if (ancho>6500) ancho=6500;
  if (PWMD4.state==PWM_STOP)
      pwmStart(&PWMD4, &pwmcfg);
  pwmEnableChannel(&PWMD4, 2, ancho);
  chThdSleepMilliseconds(ms);
  closeServo();
}

void mueveServoPos(uint16_t porcPosicion, uint16_t ms)
{
  uint16_t ancho;
  if (porcPosicion>100) porcPosicion=100;
  ancho = (uint16_t) (2700.0f+3800.0f*porcPosicion/100.0f);
  mueveServoAncho(ancho, ms);
}

void mueveServoPosC(uint16_t porcPosicion)
{
    mueveServoPos(porcPosicion,1500);
}

void cierraPuertaC(void)
{
    if (tensionCritica())
        return;
    uint16_t posIntermedia = posAbierto + ((posCerrado - posAbierto)>>2);
    mueveServoPos(posIntermedia,1000);
    chThdSleepMilliseconds(1500);
    mueveServoPos(posCerrado,1500);
}

void abrePuertaC(void)
{
    mueveServoPos(posAbierto,1500);
}
