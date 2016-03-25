/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: SX1276 driver specific target board functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/

#include "sx1276-board.h"
#include "radio.h"




/*!
 * spi sck I/O definitions
 */
#define SCK_PORT                        GPIOA
#define SCK_PIN                         GPIO_PIN_5
#define SCK_CLK_ENABLE()                __GPIOA_CLK_ENABLE()
/*!
 * spi miso I/O definitions
 */
#define MISO_PORT                       GPIOA
#define MISO_PIN                        GPIO_PIN_6
#define MISO_CLK_ENABLE()                __GPIOA_CLK_ENABLE()

/*!
 * spi mosi I/O definitions
 */
#define MOSI_PORT                       GPIOA
#define MOSI_PIN                        GPIO_PIN_7
#define MOSI_CLK_ENABLE()                __GPIOA_CLK_ENABLE()

/*!
 * spi nss I/O definitions
 */
#define NSS_IOPORT                        GPIOA
#define NSS_PIN                         GPIO_PIN_4
#define NSS_CLK_ENABLE()                __GPIOA_CLK_ENABLE()

/*!
 * SX1276 RESET I/O definitions
 */
#define RESET_IOPORT                                GPIOB
#define RESET_PIN                                   GPIO_PIN_3
#define RESET_CLK_ENABLE()                          __GPIOB_CLK_ENABLE()

/*!
 * SX1276 DIO pins  I/O definitions
 */
#define DIO0_IOPORT                                 GPIOA
#define DIO0_PIN                                    GPIO_PIN_12
#define DIO0_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

#define DIO1_IOPORT                                 GPIOA
#define DIO1_PIN                                    GPIO_PIN_11
#define DIO1_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

#define DIO2_IOPORT                                 GPIOA
#define DIO2_PIN                                    GPIO_PIN_10
#define DIO2_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

#define DIO3_IOPORT                                 GPIOA
#define DIO3_PIN                                    GPIO_PIN_9
#define DIO3_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

#define DIO4_IOPORT                                 GPIOA
#define DIO4_PIN                                    GPIO_PIN_8
#define DIO4_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

#define DIO5_IOPORT                                 GPIOB
#define DIO5_PIN                                    GPIO_PIN_2
#define DIO5_CLK_ENABLE()                          __GPIOB_CLK_ENABLE()


#define CTRL1_IOPORT                                 GPIOB
#define CTRL1_PIN                                    GPIO_PIN_4
#define CTRL1_CLK_ENABLE()                          __GPIOB_CLK_ENABLE()

#define CTRL2_IOPORT                                 GPIOB
#define CTRL2_PIN                                    GPIO_PIN_5
#define CTRL2_CLK_ENABLE()                          __GPIOB_CLK_ENABLE()

#define ENABLE_DIO_INTERRUPT()                      {\
        HAL_NVIC_SetPriority(EXTI4_15_IRQn, 3, 0);\
        HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);\
        HAL_NVIC_SetPriority(EXTI2_3_IRQn, 3, 0);\
        HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);\
}


/*!
 * Flag used to set the RF switch control pins in low power mode when the radio is not active.
 */
static bool RadioIsActive = false;

static SPI_HandleTypeDef tSpiHdl;
static void SX1276Reset( bool a_bReset );
/*!
 * Radio driver structure initialization
 */
const struct Radio_s Radio =
{
    SX1276Init,
    SX1276GetStatus,
    SX1276SetModem,
    SX1276SetChannel,
    SX1276IsChannelFree,
    SX1276Random,
    SX1276SetRxConfig,
    SX1276SetTxConfig,
    SX1276CheckRfFrequency,
    SX1276GetTimeOnAir,
    SX1276Send,
    SX1276SetSleep,
    SX1276SetStby, 
    SX1276SetRx,
    SX1276StartCad,
    SX1276ReadRssi,
    SX1276Write,
    SX1276Read,
    SX1276WriteBuffer,
    SX1276ReadBuffer,
    SX1276SetMaxPayloadLength
};

DioIrqHandler* g_hDioIrq[6];

void SX1276InitStruct( SX1276_t *a_ptSx1276 )
{
    a_ptSx1276->hReset = SX1276Reset;
    a_ptSx1276->hSpiInOut = SX1276SpiInOut;
    a_ptSx1276->hSpiNssLow = SX1276SpiSetNssLow;
    
}
TimerHighEvent_t DioEvt[5];
extern DioIrqHandler *DioIrq[];
void DioIsr( uint8_t a_ucId )
{
    DioEvt[a_ucId].Callback = DioIrq[a_ucId];
    TimerAddHighEvent( &DioEvt[a_ucId ] );
}


/*!
 * Antenna switch GPIO pins objects
 */

void SX1276IoInit( void )
{
    
    GPIO_InitTypeDef tGpioInit;

    g_hDioIrq[0] = DioIrq[0];
    g_hDioIrq[1] = DioIrq[1];
    g_hDioIrq[2] = DioIrq[2];
    g_hDioIrq[3] = DioIrq[3];
    g_hDioIrq[4] = DioIrq[0];
    g_hDioIrq[5] = NULL;
    
    tGpioInit.Mode      = GPIO_MODE_IT_RISING;
    tGpioInit.Pull      = GPIO_PULLUP;
    tGpioInit.Speed     = GPIO_SPEED_FAST;

    /* dio 0 init as input */
    DIO0_CLK_ENABLE();
    tGpioInit.Pin       = DIO0_PIN;
    HAL_GPIO_Init(DIO0_IOPORT, &tGpioInit);
    
    /* dio 1 init as input */
    DIO1_CLK_ENABLE();
    tGpioInit.Pin       = DIO1_PIN;
    HAL_GPIO_Init(DIO1_IOPORT, &tGpioInit);
    
    /* dio 2 init as input */
    DIO2_CLK_ENABLE();
    tGpioInit.Pin       = DIO2_PIN;
    HAL_GPIO_Init(DIO2_IOPORT, &tGpioInit);
    
    /* dio 3 init as input */
    DIO3_CLK_ENABLE();
    tGpioInit.Pin       = DIO3_PIN;
    HAL_GPIO_Init(DIO3_IOPORT, &tGpioInit);
    
    /* dio 4 init as input */
    DIO4_CLK_ENABLE();
    tGpioInit.Pin       = DIO4_PIN;
    HAL_GPIO_Init(DIO4_IOPORT, &tGpioInit);
    
    /* dio 5 init as input */
    tGpioInit.Mode      = GPIO_MODE_INPUT;
    DIO5_CLK_ENABLE();
    tGpioInit.Pin       = DIO5_PIN;
    HAL_GPIO_Init(DIO5_IOPORT, &tGpioInit);
    
    ENABLE_DIO_INTERRUPT();
    
    tGpioInit.Mode      = GPIO_MODE_OUTPUT_PP;
    tGpioInit.Pull      = GPIO_PULLUP;
    tGpioInit.Speed     = GPIO_SPEED_FAST;

    /* reset init as output */
    RESET_CLK_ENABLE();
    tGpioInit.Pin       = RESET_PIN;
    HAL_GPIO_Init(RESET_IOPORT, &tGpioInit);

}

void SX1276IoIrqInit( DioIrqHandler **irqHandlers )
{
    uint8_t i = 0;
    
    for ( i = 0; i < 6; i++ )
    {
        g_hDioIrq[i] = irqHandlers[i];
    }
}

void SX1276IoDeInit( void )
{
    /*  radio sleep */
    if ( true == RadioIsActive )
    {
        GPIO_InitTypeDef tGpioInit;
        tGpioInit.Mode      = GPIO_MODE_ANALOG;
        tGpioInit.Pull      = GPIO_NOPULL;
        tGpioInit.Speed     = GPIO_SPEED_LOW;

        tGpioInit.Pin       = DIO0_PIN;
        HAL_GPIO_DeInit( DIO0_IOPORT, DIO0_PIN);
        HAL_GPIO_Init(DIO0_IOPORT, &tGpioInit);
        tGpioInit.Pin       = DIO1_PIN;
        HAL_GPIO_DeInit( DIO1_IOPORT, DIO1_PIN);
        HAL_GPIO_Init(DIO1_IOPORT, &tGpioInit);
        tGpioInit.Pin       = DIO2_PIN;
        HAL_GPIO_DeInit( DIO2_IOPORT, DIO2_PIN);
        HAL_GPIO_Init(DIO2_IOPORT, &tGpioInit);
        tGpioInit.Pin       = DIO3_PIN;
        HAL_GPIO_DeInit( DIO3_IOPORT, DIO3_PIN);
        HAL_GPIO_Init(DIO3_IOPORT, &tGpioInit);
        tGpioInit.Pin       = DIO4_PIN;
        HAL_GPIO_DeInit( DIO4_IOPORT, DIO4_PIN);
        HAL_GPIO_Init(DIO4_IOPORT, &tGpioInit);
        tGpioInit.Pin       = DIO5_PIN;
        HAL_GPIO_DeInit( DIO5_IOPORT, DIO5_PIN);
        HAL_GPIO_Init(DIO5_IOPORT, &tGpioInit);
    }
    #if 0        /* wait to do */
    GpioInit( &SX1276.DIO0, RADIO_DIO_0, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX1276.DIO1, RADIO_DIO_1, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX1276.DIO2, RADIO_DIO_2, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX1276.DIO3, RADIO_DIO_3, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX1276.DIO4, RADIO_DIO_4, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX1276.DIO5, RADIO_DIO_5, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    #endif
}

static void SX1276Reset( bool a_bReset )
{
    HAL_GPIO_WritePin( RESET_IOPORT, RESET_PIN, a_bReset == true ? GPIO_PIN_RESET : GPIO_PIN_SET );
}

void SX1276SpiInit( void )
{
    GPIO_InitTypeDef tGpioInit;
    
    tSpiHdl.Instance = SPI1;
    tSpiHdl.Instance               = SPI1;
    tSpiHdl.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    tSpiHdl.Init.Direction         = SPI_DIRECTION_2LINES;
    tSpiHdl.Init.CLKPhase          = SPI_PHASE_1EDGE;
    tSpiHdl.Init.CLKPolarity       = SPI_POLARITY_LOW;
    tSpiHdl.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
    tSpiHdl.Init.CRCPolynomial     = 7;
    tSpiHdl.Init.DataSize          = SPI_DATASIZE_8BIT;
    tSpiHdl.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    tSpiHdl.Init.NSS               = SPI_NSS_SOFT;
    tSpiHdl.Init.TIMode            = SPI_TIMODE_DISABLED;
    tSpiHdl.Init.Mode = SPI_MODE_MASTER;

    /* deinit spi port */
    HAL_SPI_DeInit(&tSpiHdl);
    /* Enable GPIO TX/RX clock */

    /* init spi port - sck */
    SCK_CLK_ENABLE();
    tGpioInit.Pin       = SCK_PIN;
    tGpioInit.Mode      = GPIO_MODE_AF_PP;
    tGpioInit.Pull      = GPIO_PULLUP;
    tGpioInit.Speed     = GPIO_SPEED_FAST;
    tGpioInit.Alternate = GPIO_AF0_SPI1;
    HAL_GPIO_Init(SCK_PORT, &tGpioInit);
    
    /* init spi port - mosi */
    MOSI_CLK_ENABLE();
    tGpioInit.Pin       = MOSI_PIN;
    tGpioInit.Pull      = GPIO_PULLUP;
    HAL_GPIO_Init(MOSI_PORT, &tGpioInit);
    
    /* init spi port - miso */
    MISO_CLK_ENABLE();
    tGpioInit.Pin       = MISO_PIN;
    tGpioInit.Pull      = GPIO_PULLUP;
    HAL_GPIO_Init(MISO_PORT, &tGpioInit);
    
    /* init spi port - nss */
    NSS_CLK_ENABLE();
    tGpioInit.Pin       = NSS_PIN;
    tGpioInit.Mode      = GPIO_MODE_OUTPUT_PP;
    tGpioInit.Pull      = GPIO_PULLUP;
    tGpioInit.Speed     = GPIO_SPEED_FAST;
    //tGpioInit.Alternate = GPIO_AF0_SPI1;
    HAL_GPIO_Init(NSS_IOPORT, &tGpioInit);
    
    SX1276SpiSetNssLow(false);
    __SPI1_CLK_ENABLE();
    /* init spi port */
    if(HAL_SPI_Init(&tSpiHdl) != HAL_OK)
    {
        /* Initialization Error */
        //Error_Handler();
    }
    __HAL_SPI_ENABLE(&tSpiHdl);
}

/***************************************************************************************************
 * @fn      SX1276SpiSetNssLow()
 *
 * @brief   sx1278 spi ctrl nss
 *
 * @author  chuanpengl
 *
 * @param   a_bState  - true, set low
 *                    - false, set high
 *
 * @return  none
 */
void SX1276SpiSetNssLow( bool a_bState )
{
    HAL_GPIO_WritePin(NSS_IOPORT, NSS_PIN, a_bState == true ? GPIO_PIN_RESET : GPIO_PIN_SET );
}

uint8_t SX1276SpiInOut( uint8_t a_ucData )
{
    uint8_t ucRx = 0;
    
    HAL_SPI_TransmitReceive( &tSpiHdl, &a_ucData, &ucRx, 1, 0x1000 );
    
    return ucRx;
}

void SX1276SpiDeInit( void )
{
    GPIO_InitTypeDef tGpioInit;
    
    HAL_SPI_DeInit(&tSpiHdl);
    __SPI1_CLK_DISABLE();
    
    SCK_CLK_ENABLE();
    tGpioInit.Pin       = SCK_PIN;
    tGpioInit.Mode      = GPIO_MODE_ANALOG;
    tGpioInit.Pull      = GPIO_NOPULL;
    HAL_GPIO_Init(SCK_PORT, &tGpioInit);
    
    /* init spi port - mosi */
    MOSI_CLK_ENABLE();
    tGpioInit.Pin       = MOSI_PIN;
    HAL_GPIO_Init(MOSI_PORT, &tGpioInit);
    
    /* init spi port - miso */
    MISO_CLK_ENABLE();
    tGpioInit.Pin       = MISO_PIN;
    HAL_GPIO_Init(MISO_PORT, &tGpioInit);
    
    /* set spi nss high for inactive slave spi */
    HAL_GPIO_WritePin( NSS_IOPORT, NSS_PIN, GPIO_PIN_SET );
}

uint8_t SX1276GetPaSelect( uint32_t channel )
{
    if( channel < RF_MID_BAND_THRESH )
    {
        return RF_PACONFIG_PASELECT_PABOOST;
    }
    else
    {
        return RF_PACONFIG_PASELECT_RFO;
    }
}

void SX1276SetAntSwLowPower( bool status )
{
    if( RadioIsActive != status )
    {
        RadioIsActive = status;
    
        if( status == false )
        {
            SX1276AntSwInit( );
        }
        else
        {
            SX1276AntSwDeInit( );
        }
    }
}

void SX1276AntSwInit( void )
{
    GPIO_InitTypeDef tGpioInit;
    CTRL1_CLK_ENABLE();
    tGpioInit.Mode = GPIO_MODE_OUTPUT_PP;
    tGpioInit.Pull = GPIO_PULLDOWN;
    tGpioInit.Pin       = CTRL1_PIN;
    HAL_GPIO_Init(CTRL1_IOPORT, &tGpioInit);
    
    /* ctrl 2 init as output */
    CTRL2_CLK_ENABLE();
    tGpioInit.Pin       = CTRL2_PIN;
    HAL_GPIO_Init(CTRL2_IOPORT, &tGpioInit);
    
    /* set state for receive */
    HAL_GPIO_WritePin( CTRL1_IOPORT, CTRL1_PIN, GPIO_PIN_SET );
    HAL_GPIO_WritePin( CTRL2_IOPORT, CTRL2_PIN, GPIO_PIN_RESET );
}

void SX1276AntSwDeInit( void )
{
    /* set state for low power */
    HAL_GPIO_WritePin( CTRL1_IOPORT, CTRL1_PIN, GPIO_PIN_RESET );
    HAL_GPIO_WritePin( CTRL2_IOPORT, CTRL2_PIN, GPIO_PIN_RESET );
}

void SX1276SetAntSw( uint8_t rxTx )
{
    if( SX1276.RxTx == rxTx )
    {
        return;
    }

    SX1276.RxTx = rxTx;

    if( rxTx != 0 ) // 1: TX, 0: RX
    {
        HAL_GPIO_WritePin( CTRL1_IOPORT, CTRL1_PIN, GPIO_PIN_RESET );
        HAL_GPIO_WritePin( CTRL2_IOPORT, CTRL2_PIN, GPIO_PIN_SET );
    }
    else
    {
        HAL_GPIO_WritePin( CTRL1_IOPORT, CTRL1_PIN, GPIO_PIN_SET );
        HAL_GPIO_WritePin( CTRL2_IOPORT, CTRL2_PIN, GPIO_PIN_RESET );
    }
}

bool SX1276CheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}
