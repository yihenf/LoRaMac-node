/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: MCU RTC timer and low power modes management

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include <math.h>
#include "board.h"
#include "timer-board.h"

/*!
 * Hardware Time base in us
 */
#define HW_TIMER_TIME_BASE                              100 //us 

/*!
 * Hardware Timer tick counter
 */
volatile TimerTime_t TimerTickCounter = 1;

/*!
 * Saved value of the Tick counter at the start of the next event
 */
static TimerTime_t TimerTickCounterContext = 0;

/*!
 * Value trigging the IRQ
 */
volatile TimerTime_t TimeoutCntValue = 0;

/*!
 * Increment the Hardware Timer tick counter
 */
void TimerIncrementTickCounter( void );

/*!
 * Counter used for the Delay operations
 */
volatile uint32_t TimerDelayCounter = 0;

/*!
 * Return the value of the counter used for a Delay
 */
uint32_t TimerHwGetDelayValue( void );

/*!
 * Increment the value of TimerDelayCounter
 */
void TimerIncrementDelayCounter( void );

TIM_HandleTypeDef    TimHandle;


void TimerHwInit( void )
{
    /*##-1- Configure the TIM peripheral #######################################*/ 
  /* Set TIMx instance */
  TimHandle.Instance = TIM2;
    
  /* Initialize TIMx peripheral as follow:
       + Period = 10000 - 1
       + Prescaler = SystemCoreClock/10000 Note that APB clock = TIMx clock if
                     APB prescaler = 1.
       + ClockDivision = 0
       + Counter direction = Up
  */
  TimHandle.Init.Period = 3199;
  TimHandle.Init.Prescaler = 0;
  TimHandle.Init.ClockDivision = 0;
  TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;

  if(HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
  
  /*##-2- Start the TIM Base generation in interrupt mode ####################*/
  /* Start Channel1 */
  if(HAL_TIM_Base_Start_IT(&TimHandle) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler();
  }

}

void TimerHwDeInit( void )
{
    /* Deinitialize the timer */
    HAL_TIM_Base_DeInit( &TimHandle );
}

uint32_t TimerHwGetMinimumTimeout( void )
{
    return( ceil( 2 * HW_TIMER_TIME_BASE ) );
}

void TimerHwStart( uint32_t val )
{
    TimerTickCounterContext = TimerHwGetTimerValue( );

    if( val <= HW_TIMER_TIME_BASE + 1 )
    {
        TimeoutCntValue = TimerTickCounterContext + 1;
    }
    else
    {
        TimeoutCntValue = TimerTickCounterContext + ( ( val - 1 ) / HW_TIMER_TIME_BASE );
    }
}

void TimerHwStop( void )
{
    HAL_TIM_Base_DeInit( &TimHandle );
}

void TimerHwDelayMs( uint32_t delay )
{
    uint32_t delayValue = 0;
    uint32_t delayStart = 0;
    uint32_t curValue = 0;

    delayStart = HAL_GetTick();
    delayValue = delay + delayStart;

    while( 1 )
    {
        curValue = HAL_GetTick();
        
        if ( delayValue >= delayStart )
        {
            if ( curValue >= delayValue )
            {
                break;
            }
        }
        else
        {
            if ( ( curValue >= delayValue ) && ( curValue < delayStart ) )
            {
                break;
            }
        }
    }

}

TimerTime_t TimerHwGetElapsedTime( void )
{
     return( ( ( TimerHwGetTimerValue( ) - TimerTickCounterContext ) + 1 )  * HW_TIMER_TIME_BASE );
}

TimerTime_t TimerHwGetTimerValue( void )
{
    TimerTime_t val = 0;

    __disable_irq( );

    val = TimerTickCounter;

    __enable_irq( );

    return( val );
}

TimerTime_t TimerHwGetTime( void )
{
    return TimerHwGetTimerValue( ) * HW_TIMER_TIME_BASE;
}



void TimerIncrementTickCounter( void )
{
    __disable_irq( );

    TimerTickCounter++;

    __enable_irq( );
}



/*!
 * Timer IRQ handler
 */
void TIM2_IRQHandler( void )
{
    if( __HAL_TIM_GET_IT_SOURCE(&TimHandle, TIM_IT_UPDATE) !=RESET )
    {
        TimerIncrementTickCounter( );
        __HAL_TIM_CLEAR_IT(&TimHandle, TIM_IT_UPDATE);
    
        if( TimerTickCounter == TimeoutCntValue )
        {
            TimerIrqHandler( );
        }
    }
}



void TimerHwEnterLowPowerStopMode( void )
{
#ifndef USE_DEBUGGER
    __WFI( );
#endif
}
