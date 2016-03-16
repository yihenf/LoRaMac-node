/** ****************************************************************************
 * Copyright (c) 2015 ~ 2018 EasyLinkIn\n
 * All rights reserved.
 *
 * @file    DATAEXCHANGE.c
 * @brief   data exchange for user and lora wan
 * @version 1.0
 * @author  liucp
 * @date    2016-3-14
 * @warning No Warnings
 * @note <b>history:</b>
 * 1. first version
 * 
 */

/*******************************************************************************
 * INCLUDES
 */
#include "dataExchange.h"
#include "LoRaMac.h"
#include "Comissioning.h"
#include "app_sample.h"

/*******************************************************************************
 * DEBUG SWITCH MACROS
 */


/*******************************************************************************
 * MACROS
 */


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONSTANTS
 */


/*******************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */


/*******************************************************************************
 * GLOBAL VARIABLES
 */


/*******************************************************************************
 * STATIC VARIABLES
 */


/*******************************************************************************
 * EXTERNAL VARIABLES
 */


/*******************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */


void loraWanUsrAppInit( void )
{
    /* init user app here */
#if ( defined( APP_USE_SAMPLE ) && ( APP_MODULE_ON == APP_USE_SAMPLE ) )
    sampleInit();
#endif

#if ( defined( APP_USE_DOORSNR ) && ( APP_MODULE_ON == APP_USE_DOORSNR ) )
    ds_Init();
#endif

}


/** ****************************************************************************
 * @fn bool loraWanSendRequest( uint8_t a_ucPort, \
 *                          uint8_t *a_pucData, \
 *                          uint8_t a_ucLen )
 * @brief mcps user request, request to send
 * @param [in] a_ucPort port for loraWan Data
 * @param [in] a_pcData data for send
 * @param [in] a_ucLen data length
 * @warning none
 * @exception none
 * @note <b>History:</b>
 * 1. create function
 *
 */
bool loraWanSendRequest( uint8_t a_ucPort, uint8_t *a_pucData, uint8_t a_ucLen )
{
    bool bRet = false;
    
    if ( false == loraWanIsComplianceTesting() )
    {
        loraWanSendTrigger();	/* wake */

        if ( true == loraWanIsNextTxEnable() )
        {
            /* prepare lora wan app data frame */
            bRet = loraWanPrepareDataFrame( a_ucPort, a_pucData, a_ucLen );
        }
    }
    else
    {
        bRet = false;
    }
    
    return bRet;
}   /* loraWanSendRequest() */



/** ****************************************************************************
 * @fn void loraWanSendConfirm( McpsConfirm_t *McpsConfirm )
 * @brief mcps user confirm, user can get request result here
 * @param [in] McpsConfirm mcps confirm struct
 * @warning none
 * @exception none
 * @note <b>History:</b>
 * 1. create function
 *
 */
void loraWanSendConfirm( McpsConfirm_t *McpsConfirm )
{
    if( McpsConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
    {
        switch( McpsConfirm->McpsRequest )
        {
            case MCPS_UNCONFIRMED:
            {
                // Check Datarate
                // Check TxPower
                break;
            }
            case MCPS_CONFIRMED:
            {
                // Check Datarate
                // Check TxPower
                // Check AckReceived
                // Check NbRetries
                break;
            }
            default:
                break;
        }
    }
}   /* loraWanSendConfirm()  */


/** ****************************************************************************
 * @fn void loraWanRecvIndication( McpsIndication_t *McpsIndication )
 * @brief mcps user indication, user can get indication here
 * @param [in] McpsConfirm mcps confirm struct
 * @warning none
 * @exception none
 * @note <b>History:</b>
 * 1. create function
 *
 */
void loraWanRecvIndication( McpsIndication_t *McpsIndication )
{
    switch( McpsIndication->McpsIndication )
    {
        case MCPS_UNCONFIRMED:
        {
            break;
        }
        case MCPS_CONFIRMED:
        {
            break;
        }
        default:
            break;
    }
    
    if( McpsIndication->RxData == true )
    {
        switch( McpsIndication->Port )
        {
        case 1: // The application LED can be controlled on port 1 or 2
        case 2:
            if( McpsIndication->BufferSize == 1 )
            {
                // sample for indication data use
            }
            break;
        default:
            break;
        }
    }
    
}	/* loraWanRecvIndication() */




/*******************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */


 
