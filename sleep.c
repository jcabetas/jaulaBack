/*
 * sleep.c
 *
 *  Created on: 17 sept. 2022
 *      Author: joaquin
 */
#include "ch.h"
#include "hal.h"
#include "alimCalle.h"


/*
 * Ver http://forum.chibios.org/viewtopic.php?t=3381
 *
 */
uint8_t GL_Flag_External_WakeUp, GL_Sleep_Requested;

void cb_external_input_wake_up(void *arg)
{
  (void)arg;
  GL_Flag_External_WakeUp = 1;                                  // Set Flag fo  palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_OUTPUT_PUSHPULL);
}

void cb_external_input_wake_upA1(void *arg)
{
  (void)arg;
  GL_Flag_External_WakeUp = 2;
}

void ports_set_normal(void)
{
  palSetLineMode(LINE_A0_KEY,PAL_MODE_INPUT | PAL_STM32_PUPDR_PULLUP);
  palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOB, GPIOB_PWMSERVO,PAL_MODE_ALTERNATE(2) | PAL_STM32_OSPEED_HIGHEST);    /* PWM*/
  palSetLineMode(LINE_GPIOA_SWDIO, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLUP);
  palSetLineMode(LINE_GPIOA_SWCLK, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLDOWN);
}

void ports_set_lowpower(void)
{
  /* Set all I/O pins to Analog inputs */
  for(uint8_t i = 0; i < 16; i++ )
   {
       if (i!=GPIOA_W25Q16_CS)// && i!=LINE_GPIOA_SWDIO && i!=LINE_GPIOA_SWCLK )   // && i!=GPIOA_TX2 && i!=GPIOA_RX2)
         palSetPadMode( GPIOA, i,PAL_MODE_INPUT_ANALOG );
       palSetPadMode( GPIOB, i,PAL_MODE_INPUT_ANALOG );
       palSetPadMode( GPIOC, i,PAL_MODE_INPUT_ANALOG );
       palSetPadMode( GPIOD, i,PAL_MODE_INPUT_ANALOG );
       palSetPadMode( GPIOE, i,PAL_MODE_INPUT_ANALOG );
       palSetPadMode( GPIOH, ( i % 2 ),PAL_MODE_INPUT_ANALOG );
   }
  palSetLineMode(LINE_A0_KEY,PAL_MODE_INPUT | PAL_STM32_PUPDR_PULLUP);
  palSetLineMode(LINE_A1_SENS2,PAL_MODE_INPUT | PAL_STM32_PUPDR_PULLUP);

  palSetPad(GPIOA, GPIOA_W25Q16_CS);
  palSetPadMode(GPIOA, GPIOA_W25Q16_CS, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPad(GPIOB, GPIOB_ONSERVO);
  palSetPadMode(GPIOB, GPIOB_ONSERVO, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPad(GPIOB, GPIOB_ONAD);
  palSetPadMode(GPIOB, GPIOB_ONAD, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPad(GPIOB, GPIOB_ONGPS);
  palSetPadMode(GPIOB, GPIOB_ONGPS, PAL_MODE_OUTPUT_PUSHPULL);

  palSetLineMode(LINE_GPIOA_SWDIO, PAL_MODE_ALTERNATE(0) | PAL_MODE_INPUT_ANALOG);
  palSetLineMode(LINE_GPIOA_SWCLK, PAL_MODE_ALTERNATE(0) | PAL_MODE_INPUT_ANALOG);

//  palSetPadMode(GPIOB, GPIOB_PWMSERVO,PAL_MODE_ALTERNATE(2) | PAL_STM32_OSPEED_HIGHEST);    /* PWM*/
//  palSetLineMode(LINE_GPIOA_SWDIO, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLUP);
//  palSetLineMode(LINE_GPIOA_SWCLK, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLDOWN);
}


//http://forum.chibios.org/viewtopic.php?t=3381




// http://forum.chibios.org/viewtopic.php?f=16&t=5444
/** ****************************************************************************
 * \fn     void sleep_for_x_sec(uint16_t nb_sec)
 * \brief  Put the STM32 in the Stop2 mode
 *         Must be called only from main()  To be tested (seen some comments on forums)
 * \param[in]  (u16) nb_of sec to wait in sleep mode: 1-65535
 * \return     (u8)  wake source:  WAKE_SOURCE_TIMER, WAKE_SOURCE_EXTERNAL, WAKE_SOURCE_LPUART
 */
// funciona con consumo de 1.9mA (no cambia al quitar ADC ni puesrto serie)

uint8_t sleep_for_x_sec(uint32_t nb_sec)
{
    static RTCWakeup wakeupspec;
    static uint8_t wakeup_source;

    GL_Sleep_Requested = 0;                               // Reset Flag Sleep_Requested

    // prepara interrupcion por boton KEY
    GL_Flag_External_WakeUp = 0;                          // Reset flag that will be set by the CB
    palEnableLineEvent(LINE_A0_KEY, PAL_EVENT_MODE_FALLING_EDGE);     // Falling edge creates event
    palSetLineCallback(LINE_A0_KEY, cb_external_input_wake_up, NULL); // Active callback

    // desconozco si se comportara bien en el entorno de 0x10000
    if (nb_sec>0xFFFF && nb_sec<0x10005)
        nb_sec = 0xFFFF;
    // prepara despertar por timer
    if (nb_sec<=0x10000)
    {
        wakeupspec.wutr = ( (uint32_t)4 ) << 16; //antes 4    // bits 16-18 = WUCKSel : Select 1 Hz clk
        wakeupspec.wutr |= nb_sec - 1;                        // bits 0-15  = WUT : Period = x+1 sec
    }
    else
    {
        //WUT = Wakeup unit counter value. WUT = (0x0000 to 0xFFFF) + 0x10000 added when WUCKSEL[2:1 = 11].
        wakeupspec.wutr = ( (uint32_t)6 ) << 16;              // bits 16-18 = WUCKSel : Select 1 Hz clk
        wakeupspec.wutr |= (nb_sec - 0x10000 - 1);             // bits 0-15  = WUT : Period = x+1 sec
    }
    rtcSTM32SetPeriodicWakeup(&RTCD1, &wakeupspec);       // Set RTC wake-up config

    //  ports_set_lowpower();                                 // Set ports for low power
    //  palSetLineMode(LINE_GPIOA_SWDIO, PAL_MODE_INPUT_ANALOG);
    //  palSetLineMode(LINE_GPIOA_SWCLK, PAL_MODE_INPUT_ANALOG);

    chSysDisable();                               // No effect
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;   // No effect        // Disable TickInt Request
    chSysEnable();                                // No effect

    chSysDisable();
    __disable_irq();
    chSysEnable();

    // adormir
    __WFI();

    palSetLineMode(LINE_GPIOA_SWDIO, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLUP);
    palSetLineMode(LINE_GPIOA_SWCLK, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLDOWN);

    PWR->CSR |= PWR_CSR_SBF;                             // Clear standby flag

    __enable_irq();                               // No effect

    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    palDisableLineEvent(LINE_A0_KEY);

    return wakeup_source;
}


// http://forum.chibios.org/viewtopic.php?f=16&t=5444
/** ****************************************************************************
 * \fn     void sleep_for_x_sec(uint16_t nb_sec)
 * \brief  Put the STM32 in the Stop2 mode
 *         Must be called only from main()  To be tested (seen some comments on forums)
 * \param[in]  (u16) nb_of sec to wait in sleep mode: 1-65535
 * \return     (u8)  wake source:  WAKE_SOURCE_TIMER, WAKE_SOURCE_EXTERNAL, WAKE_SOURCE_LPUART
 */
// consumo 26uA, necesito hacer halInit() para que funcione PWM y sleep al volver
uint8_t stop(uint16_t nb_sec)
{
    static RTCWakeup wakeupspec;
    static uint8_t wakeup_source;

    if (nb_sec==0)
        nb_sec = 1;
    GL_Sleep_Requested = 0;                               // Reset Flag Sleep_Requested

    // prepara interrupcion por boton KEY
    GL_Flag_External_WakeUp = 0;                          // Reset flag that will be set by the CB
    palEnableLineEvent(LINE_A0_KEY, PAL_EVENT_MODE_FALLING_EDGE);     // Falling edge creates event
    palSetLineCallback(LINE_A0_KEY, cb_external_input_wake_up, NULL); // Active callback

    // hago igual con A1 LINE_A1_SENS1
    palEnableLineEvent(LINE_A1_SENS2, PAL_EVENT_MODE_FALLING_EDGE);     // Falling edge creates event
    palSetLineCallback(LINE_A1_SENS2, cb_external_input_wake_upA1, NULL); // Active callback

    // prepara despertar por timer
    wakeupspec.wutr = ( (uint32_t)4 ) << 16; //antes 4             // bits 16-18 = WUCKSel : Select 1 Hz clk
    wakeupspec.wutr |= nb_sec - 1;                        // bits 0-15  = WUT : Period = x+1 sec

    RTCD1.rtc->WPR = 0xCA;       // Disable write protection
    RTCD1.rtc->WPR = 0x53;

    rtcSTM32SetPeriodicWakeup(&RTCD1, &wakeupspec);       // Set RTC wake-up config

    RTCD1.rtc->WPR = 0xFF;

    chSysDisable();                               // No effect
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;   // No effect        // Disable TickInt Request
    chSysEnable();                                // No effect

    chSysDisable();
    __disable_irq();
    chSysEnable();

    DBGMCU->CR = 0;

    // lo que pone en http://forum.chibios.org/viewtopic.php?t=2315
    /* clear PDDS and LPDS bits */
    PWR->CR &= ~(PWR_CR_PDDS | PWR_CR_LPDS);
    /* set LPDS and clear  */
    PWR->CR |= (PWR_CR_LPDS | PWR_CR_CSBF | PWR_CR_CWUF);
    PWR->CR |= PWR_CR_LPLVDS;                             // Deep sleep mode in Stop2

    /* Setup the deepsleep mask */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    __WFI();

    __enable_irq();

    palSetLineMode(LINE_GPIOA_SWDIO, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLUP);
    palSetLineMode(LINE_GPIOA_SWCLK, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLDOWN);

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;                   // Clear deep sleep mask

    PWR->CSR |= PWR_CSR_SBF;                             // Clear standby flag

    stm32_clock_init();                           // Si lo pongo, en sleep se queda pillado        // Re-init RCC and power, Important
    halInit();

    __enable_irq();                               // No effect

    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;    // No effect          // Enable TickInt Request
    palDisableLineEvent(LINE_A0_KEY);
    palDisableLineEvent(LINE_A1_SENS2);

    //ports_set_normal();
    return wakeup_source;
}


uint8_t standby0(uint16_t nb_sec)
{
    static RTCWakeup wakeupspec;
    static uint8_t wakeup_source;

    GL_Sleep_Requested = 0;                               // Reset Flag Sleep_Requested

    // prepara interrupcion por boton KEY
    GL_Flag_External_WakeUp = 0;                          // Reset flag that will be set by the CB
    palEnableLineEvent(LINE_A0_KEY, PAL_EVENT_MODE_FALLING_EDGE);     // Falling edge creates event
    palSetLineCallback(LINE_A0_KEY, cb_external_input_wake_up, NULL); // Active callback

    // prepara despertar por timer
    wakeupspec.wutr = ( (uint32_t)4 ) << 16; //antes 4             // bits 16-18 = WUCKSel : Select 1 Hz clk
    wakeupspec.wutr |= nb_sec - 1;                        // bits 0-15  = WUT : Period = x+1 sec
    rtcSTM32SetPeriodicWakeup(&RTCD1, &wakeupspec);       // Set RTC wake-up config

    chSysDisable();                               // No effect
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;   // No effect        // Disable TickInt Request
    chSysEnable();                                // No effect

    chSysDisable();
    __disable_irq();
    chSysEnable();

    DBGMCU->CR = 0;
    // lo que dicen en https://www.youtube.com/watch?v=O82rj9qxkgs
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    /* Setup the deepsleep mask */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    // select standby mode
    PWR->CR |= PWR_CR_PDDS;
    // Enable wakeup pin
    PWR->CSR |= PWR_CSR_EWUP;
    // clear CWUF
    PWR->CR |= PWR_CR_CWUF;
    (void)PWR->CR; // para esperar a que el valor se ponga, hay que leer cualquier registro (al menos en STM32L4)

    __WFI();

    __enable_irq();

    palSetLineMode(LINE_GPIOA_SWDIO, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLUP);
    palSetLineMode(LINE_GPIOA_SWCLK, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLDOWN);

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;                   // Clear deep sleep mask

    PWR->CSR |= PWR_CSR_SBF;                             // Clear standby flag

    stm32_clock_init();                           // Si lo pongo, en sleep se queda pillado        // Re-init RCC and power, Important
    halInit();

    __enable_irq();                               // No effect

    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;    // No effect          // Enable TickInt Request
    palDisableLineEvent(LINE_A0_KEY);

    //ports_set_normal();
    return wakeup_source;
}



// http://forum.chibios.org/viewtopic.php?f=16&t=5444
/** ****************************************************************************
 * \fn     void sleep_for_x_sec(uint16_t nb_sec)
 * \brief  Put the STM32 in the Stop2 mode
 *         Must be called only from main()  To be tested (seen some comments on forums)
 * \param[in]  (u16) nb_of sec to wait in sleep mode: 1-65535
 * \return     (u8)  wake source:  WAKE_SOURCE_TIMER, WAKE_SOURCE_EXTERNAL, WAKE_SOURCE_LPUART
 *
 * // vienes de despertar de un standby?
 * //  if ((PWR->CSR) && PWR_CSR_SBF)
 *
 */
void standby(uint16_t nb_sec)
{
    static RTCWakeup wakeupspec;

    GL_Sleep_Requested = 0;                               // Reset Flag Sleep_Requested

    // prepara interrupcion por boton KEY
    GL_Flag_External_WakeUp = 0;                          // Reset flag that will be set by the CB
    palEnableLineEvent(LINE_A0_KEY, PAL_EVENT_MODE_FALLING_EDGE);     // Falling edge creates event
    palSetLineCallback(LINE_A0_KEY, cb_external_input_wake_up, NULL); // Active callback

    //
    wakeupspec.wutr = ( (uint32_t)4 ) << 16; //antes 4             // bits 16-18 = WUCKSel : Select 1 Hz clk
    wakeupspec.wutr |= nb_sec - 1;                        // bits 0-15  = WUT : Period = x+1 sec
    rtcSTM32SetPeriodicWakeup(&RTCD1, &wakeupspec);       // Set RTC wake-up config

    //ports_set_lowpower();                                 // Set ports for low power
    //  palSetLineMode(LINE_GPIOA_SWDIO, PAL_MODE_INPUT_ANALOG);
    //  palSetLineMode(LINE_GPIOA_SWCLK, PAL_MODE_INPUT_ANALOG);

    //  DBGMCU->CR = DBGMCU_CR_DBG_STOP;                      // Allow debug in Stop mode: +130uA !!!

    //  chSysLock();                                // Wakeup in strange mode: 200uA, freezed

    //  chSysDisable();                               // No effect
    //  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;   // No effect        // Disable TickInt Request
    //  chSysEnable();                                // No effect
    //
    //  chSysDisable();
    //  __disable_irq();
    //  chSysEnable();

    /* Setup the deepsleep mask */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    // lo que pone en http://forum.chibios.org/viewtopic.php?t=2315
    /* clear PDDS and LPDS bits */
    // set PDDS
    PWR->CR |= PWR_CR_PDDS;
    // clear CWUF
    PWR->CR &= ~PWR_CR_CWUF;
    //           PWR->CR &= ~(PWR_CR_PDDS | PWR_CR_LPDS);

    /* set LPDS and clear  */
    //           PWR->CR |= (PWR_CR_LPDS | PWR_CR_CSBF | PWR_CR_CWUF);



    chSysDisable();                               // No effect
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;   // No effect        // Disable TickInt Request
    chSysEnable();                                // No effect
    chSysDisable();
    __disable_irq();
    chSysEnable();

    __SEV();
    __WFI();
    __enable_irq();

    /* clear the deepsleep mask */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;




    //  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;                    // al activar, luego no funciona PWM... Low power mode in deep sleep
    //  PWR->CR |= PWR_CR_LPLVDS;                             // Deep sleep mode in Stop2

    // -----------------------------------------------------
    //
    //  __WFI();                                              // Sleep now !!!
    //
    // -----------------------------------------------------

    //LPUART1->CR1 &= ~USART_CR1_UESM;                      // LPUART not able to wakeup from Stop

    //  if (GL_Flag_External_WakeUp)                          // Search for the wake-up sources
    //  {
    //    wakeup_source = WAKE_SOURCE_EXTERNAL;               // External signal indicated by callback
    //  }
    //  else if ( !(LPUART1->ISR & USART_ISR_IDLE) )
    //  {
    //    wakeup_source = WAKE_SOURCE_LPUART;                 // LPUART
    //  }
    //  else                                                  // Else
    //  {
    //    wakeup_source = WAKE_SOURCE_TIMER;                  // Its the internal timer
    //  }

    palSetLineMode(LINE_GPIOA_SWDIO, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLUP);
    palSetLineMode(LINE_GPIOA_SWCLK, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLDOWN);

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;                   // Clear deep sleep mask

    PWR->CSR |= PWR_CSR_SBF;                             // Clear standby flag
    //LPUART1->ICR |= USART_ICR_PECF;                       // Clear Parity error

    //  chSysUnlock();
    //  chSysDisable();                             // No effect

    stm32_clock_init();                           // Si lo pongo, en sleep se queda pillado        // Re-init RCC and power, Important
    __enable_irq();                               // No effect

    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;    // No effect          // Enable TickInt Request
    palDisableLineEvent(LINE_A0_KEY);

    //ports_set_normal();
    return ;
}

