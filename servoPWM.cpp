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

extern "C" {
    void mueveServoPosC(uint16_t porcPosicion);
    void cierraPuertaC(void);
    void abrePuertaC(void);
    void abrePuertaHastaA1C(void);
}
void printSerialCPP(const char *msg);

extern uint16_t posAbierto;
extern uint16_t posCerrado;
uint16_t porcPosAnterior = 50;

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
    palSetPadMode(GPIOB, GPIOB_ONSERVO, PAL_MODE_OUTPUT_PUSHPULL);
    ACTIVAPAD(GPIOB, GPIOB_ONSERVO);
	pwmStart(&PWMD4, &pwmcfg);
}

void closeServo(void)
{
    palSetPadMode(GPIOB, GPIOB_PWMSERVO,PAL_MODE_INPUT_ANALOG );
    QUITAPAD(GPIOB, GPIOB_ONSERVO);
    pwmStop(&PWMD4);
}

void mueveServoAncho(uint16_t ancho, uint16_t ms)
{
  // minimo 2980, maximo 6068, medio 4529
  if (ancho<2700) ancho=2700;
  if (ancho>6500) ancho=6500;
  pwmEnableChannel(&PWMD4, 2, ancho);
  chThdSleepMilliseconds(ms);
}

void mueveServoPos(uint16_t porcPosicion)
{
  uint16_t ancho;
  // arranco servo
  initServo();
  ACTIVAPAD(GPIOB, GPIOB_ONSERVO);
  if (PWMD4.state==PWM_STOP)
      pwmStart(&PWMD4, &pwmcfg);
  if (porcPosicion>100) porcPosicion=100;
  // si esta en la posicion, recordar durante 1500ms
  if (porcPosicion==porcPosAnterior)
  {
      ancho = (uint16_t) (2700.0f+3800.0f*porcPosicion/100.0f);
      mueveServoAncho(ancho, 1500);
      closeServo();
      return;
  }
  if (porcPosicion>porcPosAnterior)
  {
      for (int16_t pos=porcPosAnterior;pos<=porcPosicion;pos++)
      {
          ancho = (uint16_t) (2700.0f+3800.0f*pos/100.0f);
          mueveServoAncho(ancho, 15);
      }
      mueveServoAncho(ancho, 500);
      porcPosAnterior = porcPosicion;
      closeServo();
      return;

  }
  if (porcPosicion<porcPosAnterior)
  {
      for (int16_t pos=porcPosAnterior;pos>=porcPosicion;pos--)
      {
          ancho = (uint16_t) (2700.0f+3800.0f*pos/100.0f);
          mueveServoAncho(ancho, 15);
      }
      mueveServoAncho(ancho, 500);
      porcPosAnterior = porcPosicion;
      closeServo();
      return;
  }
}

void mueveServoPosC(uint16_t porcPosicion)
{
    mueveServoPos(porcPosicion);
}

void cierraPuertaC(void)
{
    if (tensionCritica())
        return;
    uint16_t posIntermedia = posAbierto + ((posCerrado - posAbierto)>>2);
    mueveServoPos(posIntermedia);
    chThdSleepMilliseconds(1500);
    mueveServoPos(posCerrado);
}

void abrePuertaC(void)
{
    mueveServoPos(posAbierto);
}

void cierraPuerta(void)
{
    cierraPuertaC();
}


void abrePuertaHastaA1C(void)
{
    printSerialCPP("Apretado A1, abro puerta\n\r");
    mueveServoPos(posAbierto);
    // espera a dejar de pulsar A1 para salir, con 120s de timeout
    for (uint16_t ds=0;ds<1200;ds++)
    {
        if (palReadPad(GPIOA, GPIOA_SENS2))
            return;
        chThdSleepMilliseconds(100);
    }
    return;
}
