/*
 * qei.c
 *
 *  Created on: 27 jul. 2019
 *      Author: joaquin
 */

#include "hal.h"
#include "ch.h"


extern event_source_t arrancaADC_source;

/**
 * @brief TIM1 interrupt vector
 */

OSAL_IRQ_HANDLER(STM32_TIM1_UP_TIM10_HANDLER) {
    OSAL_IRQ_PROLOGUE();
    uint32_t numPulsos;
    // cuenta = us*3,571428 == (us*3657)>>10
    numPulsos = (pulsos1[pulsosActivos1][estadoPulso1]*3657)>>10;
    if (numPulsos==0)
      numPulsos = 4500;
    TIM1->ARR = numPulsos;
    TIM1->CCR1 = (300*3657)>>10;
    if (++estadoPulso1>=NUMPULSOS) // en el sincronismo, leo A/D para el siguiente
    {
        chSysLockFromISR();
        if (chEvtIsListeningI(&arrancaADC_source))
            chEvtBroadcastI(&arrancaADC_source);
        estadoPulso1 = 0;
        chSysUnlockFromISR();
    }
    STM32_TIM1->SR = 0;
    OSAL_IRQ_EPILOGUE();
}


void confTIM1(void)
{
    uint16_t Prescaler, i;

    estadoPin1 = 1;
    estadoPulso1 = 0;
    pulsosActivos1 = 0;
    chEvtBroadcast(&arrancaADC_source);
    osalThreadSleepMilliseconds(10);

    for (i=0;i<NUMPWM;i++)
    {
        pulsos1[0][i] = 1500;
        pulsos1[1][i] = 1500;
    }

//    palSetLineMode(LINE_A8_TIM1CH1, PAL_MODE_ALTERNATE(1));          /* PA8 TIM1_CH1 */

    rccEnableTIM1(TRUE);
    rccResetTIM1();
    // con una trama a 50 Hz (20 ms) y tres canales de salida, el pulso de sincronismo
    // puede llegar a 17 ms. Con reloj a 25 MHz y Prescaler a 10, se necesita contar
    // 0,017 = (1/25000000)*7*Conteo => Conteo = 60714 (us*3,571428)
    // p.e. me dicen ancho=1500us Conteo =1500*3571/1000 = 5356
    // o lo que es lo mismo, 3571/1000 = 3657 << 10
    Prescaler = 7 -1; //(uint16_t) (SystemCoreClock / 4000000) - 1;

  /* Output compare mode
   * Procedure:
    1. Select the counter clock (internal, external, prescaler).
    2. Write the desired data in the TIMx_ARR and TIMx_CCRx registers.
    3. Set the CCxIE and/or CCxDE bits if an interrupt and/or a DMA request is to be
       generated.
    4. Select the output mode. For example, the user must write OCxM=011, OCxPE=0,
       CCxP=0 and CCxE=1 to toggle OCx output pin when CNT matches CCRx, CCRx
       preload is not used, OCx is enabled and active high.
    5. Enable the counter by setting the CEN bit in the TIMx_CR1 register.*/

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    TIM1->CR1 = 0;
    TIM1->CR1 |= TIM_CR1_URS;
    nvicEnableVector(STM32_TIM1_UP_TIM10_NUMBER, 10);
    // Pulso de sincronismo, por rellenar algo
    TIM1->ARR = 28000-1;
    TIM1->CCR1 = 1200-1;    // Sincronismo: 300us alto y 6200 alto
    TIM1->PSC = Prescaler; //1
    TIM1->EGR = TIM_EGR_UG; /* Generate an update event to reload the Prescaler*/
    TIM1->DIER |= TIM_DIER_UIE;

    /* Set the Output Compare Polarity to High */
    TIM1->CCMR1 |= STM32_TIM_CCMR1_OC1M(6);//((0b110)<< TIM_CCMR1_OC1M_Pos);//TIM_OCPOLARITY_HIGH;
    TIM1->CCER = TIM_CCER_CC1E; /* Enable the Compare output channel 1 */
    TIM1->CCER |= TIM_CCER_CC1P; /* polaridad invertida */
    TIM1->BDTR = STM32_TIM_BDTR_MOE; // solo para TIM1
    TIM1->CR1 |= TIM_CR1_CEN;
}




