/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Target board general functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include "board.h"


/*!
 * Unique Devices IDs register set ( STM32L1xxx )
 */
#define         ID1                                 ( 0x1FF80050 )
#define         ID2                                 ( 0x1FF80054 )
#define         ID3                                 ( 0x1FF80064 )

/*!
 * IO Extander pins objects
 */

#define LED1_PORT                                   (GPIOB)
#define LED1_PIN                                    (GPIO_PIN_6)
#define LED1_EN_CLK()                               __GPIOB_CLK_ENABLE()



/*!
 * Initializes the unused GPIO to a know status
 */
static void BoardUnusedIoInit( void );

static void SystemClock_Config(void);

/*!
 * Flag to indicate if the MCU is Initialized
 */
static bool McuInitialized = false;

void BoardInitPeriph( void )
{
    GPIO_InitTypeDef tGpioInit;
    
    RtcInit( );
    
    /* Init the GPIO extender pins */
    tGpioInit.Mode = GPIO_MODE_OUTPUT_PP;
    tGpioInit.Pull = GPIO_PULLUP;
    tGpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    tGpioInit.Pin = LED1_PIN;
    
    HAL_GPIO_Init( LED1_PORT, &tGpioInit );
    // Switch LED 1 OFF
    HAL_GPIO_WritePin( LED1_PORT, LED1_PIN, GPIO_PIN_SET );

}

void BoardInitMcu( void )
{
    if( McuInitialized == false )
    {
        HAL_Init();
        
        SystemClock_Config();

        SX1276SpiInit();
        SX1276IoInit( );
        SX1276InitStruct( &SX1276 );

#if defined( LOW_POWER_MODE_ENABLE )
        TimerSetLowPowerEnable( true );
#else
        TimerSetLowPowerEnable( false );
#endif
        BoardUnusedIoInit( );
        
        McuInitialized = true;
    }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = MSI
  *            SYSCLK(Hz)                     = 2000000
  *            HCLK(Hz)                       = 2000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            Flash Latency(WS)              = 0
  *            Main regulator output voltage  = Scale3 mode
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Enable HSI Oscillator */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | /*RCC_OSCILLATORTYPE_MSI |*/ RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = 16;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    //RCC_OscInitStruct.MSIState = RCC_MSI_OFF;
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
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
    HAL_Delay(100);
    //while(1)
    {
        /* Add a 100ms Delay */
        HAL_Delay(100);
    }
}

void BoardDeInitMcu( void )
{

    //SX1276SpiDeInit( );
    //SX1276IoDeInit( );

    McuInitialized = false;
}

uint32_t BoardGetRandomSeed( void )
{
    return ( ( *( uint32_t* )ID1 ) ^ ( *( uint32_t* )ID2 ) ^ ( *( uint32_t* )ID3 ) );
}

void BoardGetUniqueId( uint8_t *id )
{
    id[7] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 24;
    id[6] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 16;
    id[5] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 8;
    id[4] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) );
    id[3] = ( ( *( uint32_t* )ID2 ) ) >> 24;
    id[2] = ( ( *( uint32_t* )ID2 ) ) >> 16;
    id[1] = ( ( *( uint32_t* )ID2 ) ) >> 8;
    id[0] = ( ( *( uint32_t* )ID2 ) );
}

/*!
 * Factory power supply
 */
#define FACTORY_POWER_SUPPLY                        3.0L

/*!
 * VREF calibration value
 */
#define VREFINT_CAL                                 ( *( uint16_t* )0x1FF80078 )

/*!
 * ADC maximum value
 */
#define ADC_MAX_VALUE                               4096

/*!                                                 
 * Battery thresholds                               
 */                                                 
#define BATTERY_MAX_LEVEL                           4150 // mV
#define BATTERY_MIN_LEVEL                           3200 // mV
#define BATTERY_SHUTDOWN_LEVEL                      3100 // mV

uint16_t BoardGetPowerSupply( void ) 
{
    #if 0
    float vref = 0;
    float vdiv = 0;
    float batteryVoltage = 0;
    
    AdcInit( &Adc, BAT_LEVEL );
    
    vref = AdcMcuRead( &Adc, ADC_Channel_17 );
    vdiv = AdcMcuRead( &Adc, ADC_Channel_8 );
    
    batteryVoltage = ( FACTORY_POWER_SUPPLY * VREFINT_CAL * vdiv ) / ( vref * ADC_MAX_VALUE );

    //                                vDiv
    // Divider bridge  VBAT <-> 1M -<--|-->- 1M <-> GND => vBat = 2 * vDiv
    batteryVoltage = 2 * batteryVoltage;
    
    return ( uint16_t )( batteryVoltage * 1000 );
    #endif
    return 0;
}

uint8_t BoardGetBatteryLevel( void )
{
    volatile uint8_t batteryLevel = 0;
#if 0
    uint16_t batteryVoltage = 0;
     
    if( GpioRead( &UsbDetect ) == 1 )
    {
        batteryLevel = 0;
    }
    else
    {
        batteryVoltage = BoardGetPowerSupply( );

        if( batteryVoltage >= BATTERY_MAX_LEVEL )
        {
            batteryLevel = 254;
        }
        else if( ( batteryVoltage > BATTERY_MIN_LEVEL ) && ( batteryVoltage < BATTERY_MAX_LEVEL ) )
        {
            batteryLevel = ( ( 253 * ( batteryVoltage - BATTERY_MIN_LEVEL ) ) / ( BATTERY_MAX_LEVEL - BATTERY_MIN_LEVEL ) ) + 1;
        }
        else if( batteryVoltage <= BATTERY_SHUTDOWN_LEVEL )
        {
            batteryLevel = 255;
            //GpioInit( &DcDcEnable, DC_DC_EN, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
            //GpioInit( &BoardPowerDown, BOARD_POWER_DOWN, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );
        }
        else // BATTERY_MIN_LEVEL
        {
            batteryLevel = 1;
        }
    }
#endif
    return batteryLevel;
}

static void BoardUnusedIoInit( void )
{
#if defined( USE_DEBUGGER )
    __HAL_RCC_DBGMCU_CLK_ENABLE();
    HAL_DBGMCU_EnableDBGSleepMode();
    HAL_DBGMCU_EnableDBGStopMode();
    HAL_DBGMCU_EnableDBGStandbyMode();
#else
    GPIO_InitTypeDef tGpioInit;
    HAL_DBGMCU_DisableDBGSleepMode();
    HAL_DBGMCU_DisableDBGStopMode();
    HAL_DBGMCU_DisableDBGStandbyMode();
    
    
    
    /* Init the GPIO SWDIO and SWDCLK */
    tGpioInit.Mode = GPIO_MODE_ANALOG;
    tGpioInit.Pull = GPIO_NOPULL;
    tGpioInit.Speed = GPIO_SPEED_FREQ_LOW;
    
    tGpioInit.Pin = GPIO_PIN_13;
    HAL_GPIO_Init( GPIOA, &tGpioInit );
    
    tGpioInit.Pin = GPIO_PIN_14;
    HAL_GPIO_Init( GPIOA, &tGpioInit );

#endif
}


void BoardCtrlLedSts( uint8_t a_ucId, bool a_bOn )
{
    switch ( a_ucId )
    {
        case LED1_ID:
            HAL_GPIO_WritePin( LED1_PORT, LED1_PIN, a_bOn == true ? GPIO_PIN_RESET : GPIO_PIN_SET );
        break;
        default:
        break;
    }
}
