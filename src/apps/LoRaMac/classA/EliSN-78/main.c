/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: LoRaMac classA device implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include <string.h>
#include <math.h>
#include "board.h"

#include "LoRaMac.h"
#include "Comissioning.h"
#include "dataExchange.h"


/*!
 * Join requests trials duty cycle.
 */
#define OVER_THE_AIR_ACTIVATION_DUTYCYCLE           10000000  // 10 [s] value in us

/*!
 * Defines the application data transmission duty cycle. 5s, value in [us].20
 */
#define APP_TX_DUTYCYCLE                            20000000

/*!
 * Defines a random delay for application data transmission duty cycle. 1s,
 * value in [us].
 */
#define APP_TX_DUTYCYCLE_RND                        1000000

/*!
 * LoRaWAN confirmed messages
 */
#define LORAWAN_CONFIRMED_MSG_ON                    true

/*!
 * LoRaWAN Adaptive Data Rate
 *
 * \remark Please note that when ADR is enabled the end-device should be static
 */
#define LORAWAN_ADR_ON                              1

#if defined( USE_BAND_868 )

#include "LoRaMacTest.h"

/*!
 * LoRaWAN ETSI duty cycle control enable/disable
 *
 * \remark Please note that ETSI mandates duty cycled transmissions. Use only for test purposes
 */
#define LORAWAN_DUTYCYCLE_ON                        true

#endif

/*!
 * LoRaWAN application port
 */
#define LORAWAN_APP_PORT                            2

/*!
 * User application data buffer size
 */
#if defined( USE_BAND_433 ) || defined( USE_BAND_780 ) || defined( USE_BAND_868 )  || defined( USE_BAND_CN433 )

#define LORAWAN_APP_DATA_SIZE                       16

#elif defined( USE_BAND_915 ) || defined( USE_BAND_915_HYBRID )

#define LORAWAN_APP_DATA_SIZE                       11

#endif

#if( OVER_THE_AIR_ACTIVATION != 0 )

static uint8_t DevEui[] = LORAWAN_DEVICE_EUI;
static uint8_t AppEui[] = LORAWAN_APPLICATION_EUI;
static uint8_t AppKey[] = LORAWAN_APPLICATION_KEY;

#else

static uint8_t NwkSKey[] = LORAWAN_NWKSKEY;
static uint8_t AppSKey[] = LORAWAN_APPSKEY;

/*!
 * Device address
 */
static uint32_t DevAddr;

#endif

/*!
 * Application port
 */
static uint8_t AppPort = LORAWAN_APP_PORT;

/*!
 * User application data size
 */
static uint8_t AppDataSize = LORAWAN_APP_DATA_SIZE;

/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_MAX_SIZE                           64

/*!
 * User application data
 */
static uint8_t AppData[LORAWAN_APP_DATA_MAX_SIZE];

/*!
 * Indicates if the node is sending confirmed or unconfirmed messages
 */
static uint8_t IsTxConfirmed = LORAWAN_CONFIRMED_MSG_ON;

/*!
 * Defines the application data transmission duty cycle
 */
static uint32_t TxDutyCycleTime;

/*!
 * Timer to handle the application data transmission duty cycle
 */
static TimerEvent_t TxNextPacketTimer;

/*!
 * Specifies the state of the application LED
 */
static bool AppLedStateOn = false;

/*!
 * Timer to handle the state of LED1
 */
static TimerEvent_t Led1Timer;


/*!
 * Indicates if a new packet can be sent
 */
static bool NextTx = true;

/*!
 * Device states
 */
static enum eDevicState
{
    DEVICE_STATE_INIT,
    DEVICE_STATE_JOIN,
    DEVICE_STATE_SEND,
    DEVICE_STATE_CYCLE,
    DEVICE_STATE_SLEEP
} DeviceState;

/*!
 * LoRaWAN compliance tests support data
 */
struct ComplianceTest_s
{
    bool Running;
    uint8_t State;
    bool IsTxConfirmed;
    uint8_t AppPort;
    uint8_t AppDataSize;
    uint8_t *AppDataBuffer;
    uint16_t DownLinkCounter;
    bool LinkCheck;
    uint8_t DemodMargin;
    uint8_t NbGateways;
} ComplianceTest;


static void OnTxNextPacketTimerEvent( void );



static void loraWanPrepareComplianceTestFrame( void )
{
    AppPort = 224;
    if ( ComplianceTest.LinkCheck == true )
    {
        ComplianceTest.LinkCheck = false;
        AppDataSize = 3;
        AppData[0] = 5;
        AppData[1] = ComplianceTest.DemodMargin;
        AppData[2] = ComplianceTest.NbGateways;
        ComplianceTest.State = 1;
    }
    else
    {
        switch ( ComplianceTest.State )
        {
        case 4:
            ComplianceTest.State = 1;
            break;
        case 1:
            AppDataSize = 2;
            AppData[0] = ComplianceTest.DownLinkCounter >> 8;
            AppData[1] = ComplianceTest.DownLinkCounter;
            break;
        }
    }

}

bool loraWanPrepareDataFrame( uint8_t a_ucPort, uint8_t *a_pucDat, uint8_t a_ucLen )
{
    bool bRet = false;

    if ( ( a_ucPort == 224) || ( a_ucLen > LORAWAN_APP_DATA_MAX_SIZE ) )
    {
        bRet = false;
    }
    else
    {
        AppPort = a_ucPort;
        memcpy( AppData, a_pucDat, a_ucLen );
        bRet = true;
}

    return bRet;
}

bool loraWanIsNextTxEnable( void )
{
    return NextTx;
}

bool loraWanIsComplianceTesting( void )
{
    return ComplianceTest.Running;
}

bool loraWanIsNetworkJoined( void )
{
    bool bRet = false;
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_NETWORK_JOINED;

    if ( LORAMAC_STATUS_OK == LoRaMacMibGetRequestConfirm( &mibReq ) )
    {
        bRet = mibReq.Param.IsNetworkJoined;
    }

    return bRet;
}

void loraWanSendTrigger( void )
{
    OnTxNextPacketTimerEvent();
}

/*!
 * \brief   Prepares the payload of the frame
 *
 * \retval  [true: frame could be send, false: error]
 */
static bool SendFrame( void )
{
    McpsReq_t mcpsReq;
    LoRaMacTxInfo_t txInfo;

    if ( LoRaMacQueryTxPossible( AppDataSize, &txInfo ) != LORAMAC_STATUS_OK )
    {
        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = DR_0;
    }
    else
    {
        if ( IsTxConfirmed == false )
        {
            mcpsReq.Type = MCPS_UNCONFIRMED;
            mcpsReq.Req.Unconfirmed.fPort = AppPort;
            mcpsReq.Req.Unconfirmed.fBuffer = AppData;
            mcpsReq.Req.Unconfirmed.fBufferSize = AppDataSize;
            mcpsReq.Req.Unconfirmed.Datarate = DR_0;
        }
        else
        {
            mcpsReq.Type = MCPS_CONFIRMED;
            mcpsReq.Req.Confirmed.fPort = AppPort;
            mcpsReq.Req.Confirmed.fBuffer = AppData;
            mcpsReq.Req.Confirmed.fBufferSize = AppDataSize;
            mcpsReq.Req.Confirmed.nbRetries = 8;
            mcpsReq.Req.Confirmed.Datarate = DR_0;
        }
    }

    if ( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK )
    {
        NextTx = false;
    }
    return NextTx;
}

/*!
 * \brief Function executed on TxNextPacket Timeout event
 */
static void OnTxNextPacketTimerEvent( void )
{
    MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;

    TimerStop( &TxNextPacketTimer );

    mibReq.Type = MIB_NETWORK_JOINED;
    status = LoRaMacMibGetRequestConfirm( &mibReq );

    if ( status == LORAMAC_STATUS_OK )
    {
        if ( mibReq.Param.IsNetworkJoined == true )
        {
            DeviceState = DEVICE_STATE_SEND;
            NextTx = true;
        }
        else
        {
            DeviceState = DEVICE_STATE_JOIN;
        }
    }
}

/*!
 * \brief Function executed on Led 1 Timeout event
 */
static void OnLed1TimerEvent( void )
{
    TimerStop( &Led1Timer );
    // Switch LED 1 OFF

}

/*!
 * \brief   MCPS-Confirm event function
 *
 * \param   [IN] McpsConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void McpsConfirm( McpsConfirm_t *McpsConfirm )
{
    if ( McpsConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
    {
        switch ( McpsConfirm->McpsRequest )
        {
        case MCPS_UNCONFIRMED:
        {
            // Check Datarate
            // Check TxPower
            break;
        }
        case MCPS_CONFIRMED:
        {
            BoardCtrlLedSts( LED1_ID, false );
            // Check Datarate
            // Check TxPower
            // Check AckReceived
            // Check NbRetries
            break;
        }
        case MCPS_PROPRIETARY:
        {
            break;
        }
        default:
            break;
        }

    }
    NextTx = true;
}

/*!
 * \brief   MCPS-Indication event function
 *
 * \param   [IN] McpsIndication - Pointer to the indication structure,
 *               containing indication attributes.
 */
static void McpsIndication( McpsIndication_t *McpsIndication )
{
    if ( McpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK )
    {
        return;
    }

    switch ( McpsIndication->McpsIndication )
    {
    case MCPS_UNCONFIRMED:
    {
        break;
    }
    case MCPS_CONFIRMED:
    {
        break;
    }
    case MCPS_PROPRIETARY:
    {
        break;
    }
    case MCPS_MULTICAST:
    {
        break;
    }
    default:
        break;
    }

    // Check Multicast
    // Check Port
    // Check Datarate
    // Check FramePending
    // Check Buffer
    // Check BufferSize
    // Check Rssi
    // Check Snr
    // Check RxSlot

    if ( ComplianceTest.Running == true )
    {
        ComplianceTest.DownLinkCounter++;
    }

    if ( McpsIndication->RxData == true )
    {
        switch ( McpsIndication->Port )
        {
        case 1: // The application LED can be controlled on port 1 or 2
        case 2:
            if ( McpsIndication->BufferSize == 1 )
            {
                AppLedStateOn = McpsIndication->Buffer[0] & 0x01;
                BoardCtrlLedSts( LED1_ID, AppLedStateOn == 0x00 ? false : true );
            }
            break;
        case 224:
            if ( ComplianceTest.Running == false )
            {
                // Check compliance test enable command (i)
                if ( ( McpsIndication->BufferSize == 4 ) &&
                        ( McpsIndication->Buffer[0] == 0x01 ) &&
                        ( McpsIndication->Buffer[1] == 0x01 ) &&
                        ( McpsIndication->Buffer[2] == 0x01 ) &&
                        ( McpsIndication->Buffer[3] == 0x01 ) )
                {
                    IsTxConfirmed = false;
                    AppPort = 224;
                    AppDataSize = 2;
                    ComplianceTest.DownLinkCounter = 0;
                    ComplianceTest.LinkCheck = false;
                    ComplianceTest.DemodMargin = 0;
                    ComplianceTest.NbGateways = 0;
                    ComplianceTest.Running = true;
                    ComplianceTest.State = 1;

                    MibRequestConfirm_t mibReq;
                    mibReq.Type = MIB_ADR;
                    mibReq.Param.AdrEnable = true;
                    LoRaMacMibSetRequestConfirm( &mibReq );

#if defined( USE_BAND_868 )
                    LoRaMacTestSetDutyCycleOn( false );
#endif
                }
            }
            else
            {
                ComplianceTest.State = McpsIndication->Buffer[0];
                switch ( ComplianceTest.State )
                {
                case 0: // Check compliance test disable command (ii)
                    IsTxConfirmed = LORAWAN_CONFIRMED_MSG_ON;
                    AppPort = LORAWAN_APP_PORT;
                    AppDataSize = LORAWAN_APP_DATA_SIZE;
                    ComplianceTest.DownLinkCounter = 0;
                    ComplianceTest.Running = false;

                    MibRequestConfirm_t mibReq;
                    mibReq.Type = MIB_ADR;
                    mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
                    LoRaMacMibSetRequestConfirm( &mibReq );
#if defined( USE_BAND_868 )
                    LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON );
#endif
                    break;
                case 1: // (iii, iv)
                    AppDataSize = 2;
                    break;
                case 2: // Enable confirmed messages (v)
                    IsTxConfirmed = true;
                    ComplianceTest.State = 1;
                    break;
                case 3:  // Disable confirmed messages (vi)
                    IsTxConfirmed = false;
                    ComplianceTest.State = 1;
                    break;
                case 4: // (vii)
                    AppDataSize = McpsIndication->BufferSize;

                    AppData[0] = 4;
                    for ( uint8_t i = 1; i < AppDataSize; i++ )
                    {
                        AppData[i] = McpsIndication->Buffer[i] + 1;
                    }
                    break;
                case 5: // (viii)
                {
                    MlmeReq_t mlmeReq;
                    mlmeReq.Type = MLME_LINK_CHECK;
                    LoRaMacMlmeRequest( &mlmeReq );
                }
                break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }
}

/*!
 * \brief   MLME-Confirm event function
 *
 * \param   [IN] MlmeConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void MlmeConfirm( MlmeConfirm_t *MlmeConfirm )
{
    if ( MlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
    {
        switch ( MlmeConfirm->MlmeRequest )
        {
        case MLME_JOIN:
        {
            // Status is OK, node has joined the network
            break;
        }
        case MLME_LINK_CHECK:
        {
            // Check DemodMargin
            // Check NbGateways
            if ( ComplianceTest.Running == true )
            {
                ComplianceTest.LinkCheck = true;
                ComplianceTest.DemodMargin = MlmeConfirm->DemodMargin;
                ComplianceTest.NbGateways = MlmeConfirm->NbGateways;
            }
            break;
        }
        default:
            break;
        }
    }
    NextTx = true;
}


void mcu_init__( void )
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIOs clock */
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();
    __GPIOH_CLK_ENABLE();

    /* Configure all GPIO port pins in Analog Input mode (floating input trigger OFF) */
    GPIO_InitStructure.Pin = GPIO_PIN_All;
    GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    //HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
    HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);
    HAL_GPIO_Init(GPIOH, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = GPIO_PIN_All & (~GPIO_PIN_14) & (~GPIO_PIN_13);
    GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Disable GPIOs clock */
    __GPIOA_CLK_DISABLE();
    __GPIOB_CLK_DISABLE();
    __GPIOC_CLK_DISABLE();
    __GPIOD_CLK_DISABLE();
    __GPIOH_CLK_DISABLE();

}


/**
 * Main application entry point.
 */
int main( void )
{
    LoRaMacPrimitives_t LoRaMacPrimitives;
    LoRaMacCallback_t LoRaMacCallbacks;
    MibRequestConfirm_t mibReq;

    BlockLowPowerDuringTask(true);

    BoardInitMcu( );
    BoardInitPeriph( );

    DeviceState = DEVICE_STATE_INIT;
    loraWanUsrAppInit();
    /* delay 5 seconds for connect debuger */
    Delay(5);
    BlockLowPowerDuringTask(false);

    

    while ( 1 )
    {
        TimerIrqHandler( );
        switch ( DeviceState )
        {
            case DEVICE_STATE_INIT:
            {
                LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
                LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
                LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
                LoRaMacCallbacks.GetBatteryLevel = BoardGetBatteryLevel;
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks );

                TimerInit( &TxNextPacketTimer, OnTxNextPacketTimerEvent );

                TimerInit( &Led1Timer, OnLed1TimerEvent );
                TimerSetValue( &Led1Timer, 25000 );

                mibReq.Type = MIB_ADR;
                mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_PUBLIC_NETWORK;
                mibReq.Param.EnablePublicNetwork = LORAWAN_PUBLIC_NETWORK;
                LoRaMacMibSetRequestConfirm( &mibReq );

#if defined( USE_BAND_868 )
                LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON );
#endif
                DeviceState = DEVICE_STATE_JOIN;
                break;
            }
            case DEVICE_STATE_JOIN:
            {
#if( OVER_THE_AIR_ACTIVATION != 0 )
                MlmeReq_t mlmeReq;

                // Initialize LoRaMac device unique ID
                BoardGetUniqueId( DevEui );

                mlmeReq.Type = MLME_JOIN;

                mlmeReq.Req.Join.DevEui = DevEui;
                mlmeReq.Req.Join.AppEui = AppEui;
                mlmeReq.Req.Join.AppKey = AppKey;

                if ( NextTx == true )
                {
                    LoRaMacMlmeRequest( &mlmeReq );
                }

                // Schedule next packet transmission
                TxDutyCycleTime = OVER_THE_AIR_ACTIVATION_DUTYCYCLE;
                DeviceState = DEVICE_STATE_CYCLE;

#else
                // Random seed initialization
                srand1( BoardGetRandomSeed( ) );

                // Choose a random device address
                //DevAddr = randr( 0, 0x01FFFFFF );
                DevAddr = LORAWAN_DEVICE_ADDRESS;

                mibReq.Type = MIB_NET_ID;
                mibReq.Param.NetID = LORAWAN_NETWORK_ID;
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_DEV_ADDR;
                mibReq.Param.DevAddr = DevAddr;
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_NWK_SKEY;
                mibReq.Param.NwkSKey = NwkSKey;
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_APP_SKEY;
                mibReq.Param.AppSKey = AppSKey;
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_NETWORK_JOINED;
                mibReq.Param.IsNetworkJoined = true;
                LoRaMacMibSetRequestConfirm( &mibReq );

                DeviceState = DEVICE_STATE_CYCLE;
#endif
                break;
            }
            case DEVICE_STATE_SEND:
            {
                if ( true == loraWanIsNextTxEnable() )
                {
                    /* to compliance test, need prepare frame, for user data, prepare data by user app */
                    if (true == loraWanIsComplianceTesting() )
                    {
                        loraWanPrepareComplianceTestFrame();
                    }
                    if ( true == SendFrame() )
                    {
                        /* control led state when send was accepted  */
                        BoardCtrlLedSts( LED1_ID,  true );
                    }
                }

                if ( true == loraWanIsComplianceTesting() )
                {
                    // Schedule next packet transmission as soon as possible
                    TxDutyCycleTime = 1000; // 1 ms
                }

                DeviceState = DEVICE_STATE_CYCLE;
                break;
            }
            case DEVICE_STATE_CYCLE:
            {
                /* for user data send, no need timer wake up here */
                if ( ( true == loraWanIsComplianceTesting() ) || ( false == loraWanIsNetworkJoined() ) )
                {
                    // Schedule next packet transmission
                    TimerSetValue( &TxNextPacketTimer, TxDutyCycleTime );
                    TimerStart( &TxNextPacketTimer );
                }

                DeviceState = DEVICE_STATE_SLEEP;
                break;
            }
            case DEVICE_STATE_SLEEP:
            {
                // Wake up through events
                TimerLowPowerHandler( );
                //RtcRecoverMcuStatus();
                break;
            }
            default:
            {
                DeviceState = DEVICE_STATE_INIT;
                break;
            }
        }

    }
}
