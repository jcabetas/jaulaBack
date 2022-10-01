/*
 * sleep.c
 *
 *  Created on: 17 sept. 2022
 *      Author: joaquin
 */
#include "ch.h"
#include "hal.h"


uint8_t GL_Flag_External_WakeUp, GL_Sleep_Requested;

void cb_external_input_wake_up(void *arg)
{
  (void)arg;
  GL_Flag_External_WakeUp = 1;                                  // Set Flag fo  palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_OUTPUT_PUSHPULL);
}

void ports_set_normal(void)
{
  palSetLineMode(LINE_A0_KEY,PAL_MODE_INPUT | PAL_STM32_PUPDR_PULLUP);
  palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOB, GPIOB_MOSFET, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOB, GPIOB_PWMSERVO,PAL_MODE_ALTERNATE(2) | PAL_STM32_OSPEED_HIGHEST);    /* PWM*/
  palSetLineMode(LINE_GPIOA_SWDIO, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLUP);
  palSetLineMode(LINE_GPIOA_SWCLK, PAL_MODE_ALTERNATE(0) | PAL_STM32_PUPDR_PULLDOWN);
}

void ports_set_lowpower(void)
{
  /* Set all I/O pins to Analog inputs */
  for(uint8_t i = 0; i < 16; i++ )
   {
       palSetPadMode( GPIOA, i,PAL_MODE_INPUT_ANALOG );
       palSetPadMode( GPIOB, i,PAL_MODE_INPUT_ANALOG );
       palSetPadMode( GPIOC, i,PAL_MODE_INPUT_ANALOG );
       palSetPadMode( GPIOD, i,PAL_MODE_INPUT_ANALOG );
       palSetPadMode( GPIOE, i,PAL_MODE_INPUT_ANALOG );
       palSetPadMode( GPIOH, ( i % 2 ),PAL_MODE_INPUT_ANALOG );
   }
  palSetLineMode(LINE_A0_KEY,PAL_MODE_INPUT | PAL_STM32_PUPDR_PULLUP);
}


// http://forum.chibios.org/viewtopic.php?f=16&t=5444
/** ****************************************************************************
 * \fn     void sleep_for_x_sec(uint16_t nb_sec)
 * \brief  Put the STM32 in the Stop2 mode
 *         Must be called only from main()  To be tested (seen some comments on forums)
 * \param[in]  (u16) nb_of sec to wait in sleep mode: 1-65535
 * \return     (u8)  wake source:  WAKE_SOURCE_TIMER, WAKE_SOURCE_EXTERNAL, WAKE_SOURCE_LPUART
*/
uint8_t sleep_for_x_sec(uint16_t nb_sec)
{
  static RTCWakeup wakeupspec;
  static uint8_t wakeup_source;

  GL_Sleep_Requested = 0;                               // Reset Flag Sleep_Requested

                                                        // Config CB for ext button
  GL_Flag_External_WakeUp = 0;                          // Reset flag that will be set by the CB
  palEnableLineEvent(LINE_A0_KEY, PAL_EVENT_MODE_FALLING_EDGE);     // Falling edge creates event
  palSetLineCallback(LINE_A0_KEY, cb_external_input_wake_up, NULL); // Active callback

                                                        // Config LPUART for wakeup
//  LPUART1->CR1 &= ~USART_CR1_UE;                        // Disable LPUART to   palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_OUTPUT_PUSHPULL);
//  palSetPadMode(GPIOB, GPIOB_MOSFET, PAL_MODE_OUTPUT_PUSHPULL);
//  palSetPadMode(GPIOB, GPIOB_PWMSERVO,PAL_MODE_ALTERNATE(2) | PAL_STM32_OSPEED_HIGHEST);    /* PWM*/
//  LPUART1->CR1 |= USART_CR1_UESM;                       // LPUART able to wakeup from Stop
//  LPUART1->CR3 |= USART_CR3_WUS_1;                      // Wakeup on Start bit detected
//  LPUART1->CR3 |= USART_CR3_WUFIE;                      // Wakeup from Stop IRQ enabled
//  LPUART1->CR3 |= (0x1U << 23);                         // LPUART clk enabled during STOP (UCESM)
//  LPUART1->ICR = USART_ICR_WUCF;                        // Clear WUF flag
//  LPUART1->CR1 |= USART_CR1_UE;                         // Enable LPUART after modif

                                                        // Config RTC for wakeup after delay
  wakeupspec.wutr = ( (uint32_t)4 ) << 16; //antes 4             // bits 16-18 = WUCKSel : Select 1 Hz clk
  wakeupspec.wutr |= nb_sec - 1;                        // bits 0-15  = WUT : Period = x+1 sec
  rtcSTM32SetPeriodicWakeup(&RTCD1, &wakeupspec);       // Set RTC wake-up config

  //ports_set_lowpower();                                 // Set ports for low power
//  palSetLineMode(LINE_GPIOA_SWDIO, PAL_MODE_INPUT_ANALOG);
//  palSetLineMode(LINE_GPIOA_SWCLK, PAL_MODE_INPUT_ANALOG);

//  DBGMCU->CR = DBGMCU_CR_DBG_STOP;                      // Allow debug in Stop mode: +130uA !!!

//  chSysLock();                                // Wakeup in strange mode: 200uA, freezed

  chSysDisable();                               // No effect
  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;   // No effect        // Disable TickInt Request
  chSysEnable();                                // No effect

  chSysDisable();
  __disable_irq();
  chSysEnable();

 // SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;                    // Low power mode in deep sleep
//  PWR->CR |= PWR_CR_LPLVDS;                             // Deep sleep mode in Stop2

// -----------------------------------------------------
                                                        //
  __WFI();                                              // Sleep now !!!
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

 // SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;                   // Clear deep sleep mask

  //PWR->CSR |= PWR_CSR_SBF;                             // Clear standby flag
  //LPUART1->ICR |= USART_ICR_PECF;                       // Clear Parity error

//  chSysUnlock();
//  chSysDisable();                             // No effect

 // stm32_clock_init();                           // Si lo pongo, en sleep se queda pillado        // Re-init RCC and power, Important
  __enable_irq();                               // No effect

  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;    // No effect          // Enable TickInt Request
  palDisableLineEvent(LINE_A0_KEY);

  //ports_set_normal();
  return wakeup_source;
}


