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
#include <time.h>
#include "board.h"
#include "rtc-board.h"

/*!
 * RTC Time base in us
 */
#define RTC_ALARM_TIME_BASE                             122.07

/*!
 * MCU Wake Up Time
 */
/* @test default = 3400 */
#define MCU_WAKE_UP_TIME                                5400
#define MCU_WAKE_UP_TICK                                (MCU_WAKE_UP_TIME / RTC_ALARM_TIME_BASE)
/* MCU最小睡眠时间，如果距离下次任务执行时间长度大于该值，将允许睡眠  */
/* @test default = 5500 */
#define MCU_MIN_SLEEP_TIME                              (7500)
#define MCU_MIN_SLEEP_TICK                              (MCU_MIN_SLEEP_TIME / RTC_ALARM_TIME_BASE)

static void RTC_AlarmConfig( void );
/*!
 * \brief Start the Rtc Alarm (time base 1s)
 */
static void RtcStartWakeUpAlarm( uint32_t timeoutValue );

/*!
 * \brief Read the MCU internal Calendar value
 *
 * \retval Calendar value
 */
static TimerTime_t RtcGetCalendarValue( void );

/*!
 * \brief Clear the RTC flags and Stop all IRQs
 */
static void RtcClearStatus( void );

/*!
 * \brief Indicates if the RTC is already Initalized or not
 */
static bool RtcInitalized = false;

/*!
 * \brief Flag to indicate if the timestamps until the next event is long enough 
 * to set the MCU into low power mode
 */
static bool RtcTimerEventAllowsLowPower = false;

/*!
 * \brief Flag to disable the LowPower Mode even if the timestamps until the
 * next event is long enough to allow Low Power mode 
 */
static bool LowPowerDisableDuringTask = false;

/*!
 * Keep the value of the RTC timer when the RTC alarm is set
 */
static TimerTime_t RtcTimerContext = 0;

/*!
 * Number of seconds in a minute
 */
static const uint8_t SecondsInMinute = 60;

/*!
 * Number of seconds in an hour
 */
static const uint16_t SecondsInHour = 3600;

/*!
 * Number of seconds in a day
 */
static const uint32_t SecondsInDay = 86400;

/*!
 * Number of hours in a day
 */
static const uint8_t HoursInDay = 24;

/*!
 * Number of days in a standard year
 */
static const uint16_t DaysInYear = 365;

/*!
 * Number of days in a leap year
 */
static const uint16_t DaysInLeapYear = 366;

/*!
 * Number of days in a century = 36524.219，但是stm32的RTC实际时间为2000~2099年
 * 之间循环 故一个世纪天数为366*25+365*75 = 36525
 */
static const double DaysInCentury = 36525;

/*!
 * Number of days in each month on a normal year
 */
static const uint8_t DaysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*!
 * Number of days in each month on a leap year
 */
static const uint8_t DaysInMonthLeapYear[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*!
 * Hold the previous year value to detect the turn of a century
 */
static uint8_t PreviousYear = 0;

RTC_HandleTypeDef RtcHandle;

/*!
 * Century counter：由于最小时间单位为122.07us，所以计时的真正最大时间为8个世纪
 */
static uint16_t Century = 0;

void RtcInit( void )
{
    if( RtcInitalized == false )
    {
        RTC_AlarmConfig( );
        RtcInitalized = true;
    }
}

#define RTC_ASYNCH_PREDIV       (0x01)
#define RTC_SYNCH_PREDIV        (0x01)

static void RTC_AlarmConfig( void )
{
    RTC_DateTypeDef  sdatestructure;
    RTC_TimeTypeDef  stimestructure;

    /*##-1- Configure the RTC peripheral #######################################*/
    /* Configure RTC prescaler and RTC data registers */
    /* RTC configured as follow:
       - Hour Format    = Format 24
       - Asynch Prediv  = Value according to source clock
       - Synch Prediv   = Value according to source clock
       - OutPut         = Output Disable
       - OutPutPolarity = High Polarity
       - OutPutType     = Open Drain */
    RtcHandle.Instance = RTC;
    RtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
    RtcHandle.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
    RtcHandle.Init.SynchPrediv = RTC_SYNCH_PREDIV;
    RtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
    RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    RtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if(HAL_RTC_Init(&RtcHandle) != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler(); 
    }
#if 1
    /*##-2- Configure the RTC Alarm peripheral #################################*/
    /* Disable alarm a and b */

    if (HAL_RTC_DeactivateAlarm( &RtcHandle, RTC_ALARM_A ) != HAL_OK )
    {
        Error_Handler();
    }
    if (HAL_RTC_DeactivateAlarm( &RtcHandle, RTC_ALARM_B ) != HAL_OK )
    {
        Error_Handler();
    }
#endif
    /*##-3- Configure the Date #################################################*/
    /* Set Date: Saturday January 1th 2000 */
    sdatestructure.Year = 0;
    sdatestructure.Month = 1;
    sdatestructure.Date = 1;
    sdatestructure.WeekDay = RTC_WEEKDAY_SATURDAY;

    if( HAL_RTC_SetDate( &RtcHandle,&sdatestructure,RTC_FORMAT_BIN ) \
        != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }

    /*##-4- Configure the Time #################################################*/
    /* Set Time: 00:00:00 */
    stimestructure.Hours = 0x00;
    stimestructure.Minutes = 0x00;
    stimestructure.Seconds = 0x00;
    stimestructure.TimeFormat = RTC_HOURFORMAT12_AM;
    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE ;
    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

    if( HAL_RTC_SetTime( &RtcHandle,&stimestructure,RTC_FORMAT_BIN ) != HAL_OK )
    {
        /* Initialization Error */
        Error_Handler();
    }
    
    /* Wait for RTC APB registers synchronisation */
    HAL_RTC_WaitForSynchro( &RtcHandle );

}

void RtcStopTimer( void )
{
    RtcClearStatus( );
}
uint32_t u32SetTimeDelay = 0;
/* 最小超时时间太小会导致RTC Alarm不工作  */
uint32_t RtcGetMinimumTimeout( void )
{
    return( ceil( 8 * RTC_ALARM_TIME_BASE ) );
}

uint64_t RtcTimeToTick( uint64_t time )
{
    return ((time << 13) / 1000000);
}

uint32_t RtcGetMinimumTimeoutTick( void )
{
    return 8;
}
uint32_t rtcAlarmSet = 0;
void RtcSetTimeout( TimerTime_t timeout )
{
    uint32_t timeoutValue = 0;
    TimerTime_t curTime = RtcGetTimerValue();

    if ( timeout < ( curTime + RtcGetMinimumTimeoutTick() ) ){
        timeoutValue = RtcGetMinimumTimeoutTick();
    }else{
        timeoutValue = timeout - curTime;
    }

    if( timeoutValue < MCU_MIN_SLEEP_TICK )
    {
        // we don't go in Low Power mode for delay below 50ms (needed for LEDs)
        RtcTimerEventAllowsLowPower = false;
    }
    else
    {
        RtcTimerEventAllowsLowPower = true;
    }

    if( ( LowPowerDisableDuringTask == false ) && ( RtcTimerEventAllowsLowPower == true ) )
    {
        timeoutValue = timeoutValue - MCU_WAKE_UP_TICK;
    }

    //__disable_irq();
    RtcStartWakeUpAlarm( timeoutValue );
    //__enable_irq();

    rtcAlarmSet++;
}


uint32_t RtcGetTimerElapsedTime( void )
{
    TimerTime_t CalendarValue = 0;

    CalendarValue = RtcGetCalendarValue( );

    return( ( uint32_t )( ( CalendarValue - RtcTimerContext ) + 2 ) );
}

TimerTime_t RtcGetTimerValue( void )
{
    TimerTime_t CalendarValue = 0;

    CalendarValue = RtcGetCalendarValue( );

    return(  CalendarValue + 2 );
}

static void RtcClearStatus( void )
{
    /* Clear RTC Alarm Flag */
    __HAL_RTC_ALARM_CLEAR_FLAG(&RtcHandle, RTC_FLAG_ALRAF);

    HAL_RTC_DeactivateAlarm( &RtcHandle, RTC_ALARM_A);
}

static void RtcStartWakeUpAlarm( uint32_t timeoutValue )
{
    uint16_t rtcSeconds = 0;
    uint16_t rtcMinutes = 0;
    uint16_t rtcHours = 0;
    uint16_t rtcDays = 0;

    uint8_t rtcAlarmSeconds = 0;
    uint8_t rtcAlarmMinutes = 0;
    uint8_t rtcAlarmHours = 0;
    uint16_t rtcAlarmDays = 0;
    
    RTC_AlarmTypeDef RTC_AlarmStructure;
    RTC_TimeTypeDef RTC_TimeStruct;
    RTC_DateTypeDef RTC_DateStruct;

    RtcClearStatus( );
    RtcTimerContext = RtcGetCalendarValue( );
    HAL_RTC_WaitForSynchro( &RtcHandle );
    HAL_RTC_GetTime( &RtcHandle, &RTC_TimeStruct, RTC_FORMAT_BIN );
    HAL_RTC_GetDate( &RtcHandle, &RTC_DateStruct, RTC_FORMAT_BIN );

    if( timeoutValue > 2160000 ) // 25 "days" in tick 
    {                            // drastically reduce the computation time
        rtcAlarmSeconds = RTC_TimeStruct.Seconds;
        rtcAlarmMinutes = RTC_TimeStruct.Minutes;
        rtcAlarmHours = RTC_TimeStruct.Hours;
        rtcAlarmDays = 25 + RTC_DateStruct.Date;  // simply add 25 days to current date and time

        if( ( RTC_DateStruct.Year == 0 ) || ( RTC_DateStruct.Year % 4 == 0 ) )
        {
            if( rtcAlarmDays > DaysInMonthLeapYear[ RTC_DateStruct.Month - 1 ] )
            {
                rtcAlarmDays = rtcAlarmDays % DaysInMonthLeapYear[ RTC_DateStruct.Month - 1];
            }
        }
        else
        {
            if( rtcAlarmDays > DaysInMonth[ RTC_DateStruct.Month - 1 ] )
            {
                rtcAlarmDays = rtcAlarmDays % DaysInMonth[ RTC_DateStruct.Month - 1];
            }
        }
    }
    else
    {
        rtcSeconds = ( timeoutValue % SecondsInMinute ) + RTC_TimeStruct.Seconds;
        rtcMinutes = ( ( timeoutValue / SecondsInMinute ) % SecondsInMinute ) + RTC_TimeStruct.Minutes;
        rtcHours = ( ( timeoutValue / SecondsInHour ) % HoursInDay ) + RTC_TimeStruct.Hours;
        rtcDays = ( timeoutValue / SecondsInDay ) + RTC_DateStruct.Date;

        rtcAlarmSeconds = ( rtcSeconds ) % 60;
        rtcAlarmMinutes = ( ( rtcSeconds / 60 ) + rtcMinutes ) % 60;
        rtcAlarmHours   = ( ( ( ( rtcSeconds / 60 ) + rtcMinutes ) / 60 ) + rtcHours ) % 24;
        rtcAlarmDays    = ( ( ( ( ( rtcSeconds / 60 ) + rtcMinutes ) / 60 ) + rtcHours ) / 24 ) + rtcDays;

        if( ( RTC_DateStruct.Year == 0 ) || ( RTC_DateStruct.Year % 4 == 0 ) )
        {
            if( rtcAlarmDays > DaysInMonthLeapYear[ RTC_DateStruct.Month - 1 ] )
            {   
                rtcAlarmDays = rtcAlarmDays % DaysInMonthLeapYear[ RTC_DateStruct.Month - 1 ];
            }
        }
        else
        {
            if( rtcAlarmDays > DaysInMonth[ RTC_DateStruct.Month - 1 ] )            
            {   
                rtcAlarmDays = rtcAlarmDays % DaysInMonth[ RTC_DateStruct.Month - 1 ];
            }
        }
    }


    RTC_AlarmStructure.AlarmTime.Seconds = rtcAlarmSeconds;
    RTC_AlarmStructure.AlarmTime.Minutes = rtcAlarmMinutes;
    RTC_AlarmStructure.AlarmTime.Hours   = rtcAlarmHours;
    RTC_AlarmStructure.AlarmDateWeekDay      = ( uint8_t )rtcAlarmDays;
    RTC_AlarmStructure.AlarmTime.TimeFormat     = RTC_TimeStruct.TimeFormat;
    RTC_AlarmStructure.AlarmDateWeekDaySel   = RTC_ALARMDATEWEEKDAYSEL_DATE;  
    RTC_AlarmStructure.AlarmMask             = RTC_ALARMMASK_NONE;
    RTC_AlarmStructure.AlarmTime.SubSeconds = 0;
    RTC_AlarmStructure.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    RTC_AlarmStructure.Alarm = RTC_ALARM_A;

    HAL_RTC_SetAlarm_IT( &RtcHandle, &RTC_AlarmStructure, RTC_FORMAT_BIN);

    /* Wait for RTC APB registers synchronisation */
    HAL_RTC_WaitForSynchro( &RtcHandle );
    

}
bool bRecoverMcuFlg = true;
void RtcEnterLowPowerStopMode( void )
{   
    if( ( LowPowerDisableDuringTask == false ) && ( RtcTimerEventAllowsLowPower == true ) )
    {   
        // Disable IRQ while the MCU is being deinitialized to prevent race issues
        __disable_irq( );
    
        BoardDeInitMcu( );
        
            /* Disable GPIOs clock */
        __GPIOA_CLK_DISABLE();
        __GPIOB_CLK_DISABLE();
        __GPIOC_CLK_DISABLE();
        __GPIOD_CLK_DISABLE();
        __GPIOH_CLK_DISABLE();
    
        
        
        bRecoverMcuFlg = false;
        //SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

        /* Disable the Power Voltage Detector */
        //HAL_PWR_DisablePVD();

        /* Set MCU in ULP (Ultra Low Power) */
        HAL_PWREx_EnableUltraLowPower();

        /*Disable fast wakeUp*/
        HAL_PWREx_DisableFastWakeUp();
        __dsb(15);
        /* Select HSI as system clock source after Wake Up from Stop mode */
        //__HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_StopWakeUpClock_HSI);
        //SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
        /* Enter Stop Mode */
        HAL_PWR_EnterSTOPMode( PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI );
        __isb(15);
        __NOP();
        HAL_Init();
        __enable_irq( );
        //RtcRecoverMcuStatus();//add by liucp @ 20160323
        //wait for recovery
        #if 0
        uint32_t StartTick = HAL_GetTick();
        while( bRecoverMcuFlg == false)
        {
            if( StartTick + 10 <=  HAL_GetTick() )
            {
                RtcRecoverMcuStatus();
            }
        }
        #endif
        RtcRecoverMcuStatus();
    }
}

void RtcRecoverMcuStatus( void )
{
    //RCC_ClkInitTypeDef RCC_ClkInitStruct;
    //RCC_OscInitTypeDef RCC_OscInitStruct;
    if ( bRecoverMcuFlg == true ){
        //return ;
    }
    if( TimerGetLowPowerEnable( ) == true )
    {
        if( ( LowPowerDisableDuringTask == false ) && ( RtcTimerEventAllowsLowPower == true ) )
        {
            #if 0
            /* Enable Power Control clock */
            __HAL_RCC_PWR_CLK_ENABLE();

            /* The voltage scaling allows optimizing the power consumption when the device is 
             clocked below the maximum system frequency, to update the voltage scaling value 
             regarding system frequency refer to product datasheet.  */
            __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

            /* Enable MSI Oscillator */
            RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
            RCC_OscInitStruct.HSICalibrationValue = 16;
            RCC_OscInitStruct.HSIState = RCC_HSI_ON;
            RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
            RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_8;
            RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_4;
            RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
            if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
            {
                /* Initialization Error */
                Error_Handler();
            }


            /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
             clocks dividers */
            RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
            RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
            RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
            RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
            if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
            {
            /* Initialization Error */
            Error_Handler();
            }

            /* update tick */
            HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
            HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
            #endif
            /* Set MCU in ULP (Ultra Low Power) */
            HAL_PWREx_DisableUltraLowPower(); // add up to 3ms wakeup time
            
            /* Enable the Power Voltage Detector */
            //HAL_PWR_EnablePVD();
                
            BoardInitMcu( );
            bRecoverMcuFlg = true;
        }
    }
}
uint32_t rtcAlarmIrq = 0;
/*!
 * \brief RTC IRQ Handler on the RTC Alarm
 */
void RTC_IRQHandler( void )
{
    //HAL_RTC_AlarmIRQHandler( &RtcHandle );
  /* Clear the EXTI's line Flag for RTC Alarm */
  __HAL_RTC_ALARM_EXTI_CLEAR_FLAG();
      /* Get the AlarmA interrupt source enable status */
  if(__HAL_RTC_ALARM_GET_IT_SOURCE(&RtcHandle, RTC_IT_ALRA) != RESET)
  {
    /* Get the pending status of the AlarmA Interrupt */
    if(__HAL_RTC_ALARM_GET_FLAG(&RtcHandle, RTC_FLAG_ALRAF) != RESET)
    {


      /* Clear the AlarmA interrupt pending bit */
      __HAL_RTC_ALARM_CLEAR_FLAG(&RtcHandle, RTC_FLAG_ALRAF);

      /* AlarmA callback */
      HAL_RTC_AlarmAEventCallback(&RtcHandle);
    }
  }

  /* Get the AlarmB interrupt source enable status */
  if(__HAL_RTC_ALARM_GET_IT_SOURCE(&RtcHandle, RTC_IT_ALRB) != RESET)
  {
    /* Get the pending status of the AlarmB Interrupt */
    if(__HAL_RTC_ALARM_GET_FLAG(&RtcHandle, RTC_FLAG_ALRBF) != RESET)
    {
      /* AlarmB callback */
      HAL_RTCEx_AlarmBEventCallback(&RtcHandle);

      /* Clear the AlarmB interrupt pending bit */
      __HAL_RTC_ALARM_CLEAR_FLAG(&RtcHandle, RTC_FLAG_ALRBF);
    }
  }

  

  /* Change RTC state */
  RtcHandle.State = HAL_RTC_STATE_READY;
  rtcAlarmIrq++;
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    //RtcRecoverMcuStatus( );
    
    //TimerIrqHandler( );
}


void BlockLowPowerDuringTask( bool status )
{
    //if( status == true )
    //{
    //    RtcRecoverMcuStatus( );
    //}
    LowPowerDisableDuringTask = status;
}

void RtcDelayMs( uint32_t delay )
{
    TimerTime_t delayValue = 0;
    TimerTime_t timeout = 0;

    delayValue = RtcTimeToTick( delay * 1000 );

    // Wait delay ms
    timeout = RtcGetTimerValue( );
    while( ( ( RtcGetTimerValue( ) - timeout ) ) < delayValue )
    {
        __NOP( );
    }
}

/* 获取RTC时间，并转成“秒”单位，实际上，此处“秒”为最小计时单位 */
/* 由于年份始终在2000~2099年之间循环，故不会出现需要被400整出判断闰年的情况 */
TimerTime_t RtcGetCalendarValue( void )
{
    TimerTime_t calendarValue = 0;
    uint16_t i = 0;

    RTC_TimeTypeDef RTC_TimeStruct;
    RTC_DateTypeDef RTC_DateStruct;

    HAL_RTC_WaitForSynchro( &RtcHandle );
    HAL_RTC_GetTime( &RtcHandle, &RTC_TimeStruct, RTC_FORMAT_BIN );
    HAL_RTC_GetDate( &RtcHandle, &RTC_DateStruct, RTC_FORMAT_BIN );

    HAL_RTC_WaitForSynchro( &RtcHandle );

    if( ( PreviousYear == 99 ) && ( RTC_DateStruct.Year == 0 ) )
    {
        Century++;
    }
    PreviousYear = RTC_DateStruct.Year;

    // century
    for( i = 0; i < Century; i++ )
    {
        calendarValue += ( TimerTime_t )( DaysInCentury * SecondsInDay );
    }

    // years
    for( i = 0; i < RTC_DateStruct.Year; i++ )
    {
        if( ( i == 0 ) || ( i % 4 == 0 ) )
        {
            calendarValue += DaysInLeapYear * SecondsInDay;
        }
        else
        {
            calendarValue += DaysInYear * SecondsInDay;
        }
    }

    // months
    if( ( RTC_DateStruct.Year == 0 ) || ( RTC_DateStruct.Year % 4 == 0 ) )
    {
        for( i = 0; i < ( RTC_DateStruct.Month - 1 ); i++ )
        {
            calendarValue += DaysInMonthLeapYear[i] * SecondsInDay;
        }
    }
    else
    {
        for( i = 0;  i < ( RTC_DateStruct.Month - 1 ); i++ )
        {
            calendarValue += DaysInMonth[i] * SecondsInDay;
        }
    }       

    // days
    calendarValue += ( ( uint32_t )RTC_TimeStruct.Seconds + 
                      ( ( uint32_t )RTC_TimeStruct.Minutes * SecondsInMinute ) +
                      ( ( uint32_t )RTC_TimeStruct.Hours * SecondsInHour ) + 
                      ( ( uint32_t )( RTC_DateStruct.Date * SecondsInDay ) ) );

    return( calendarValue );
}
