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
#include "LoRaMacLog.h"

#if ( LORAMAC_LOG_ON == true )
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
static LoRaMacLog_t s_tLoRaMacLog;

/*******************************************************************************
 * EXTERNAL VARIABLES
 */


/*******************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
/** ****************************************************************************
 * @fn void loraMacLogInit( void )
 * @brief loraMac log init
 * @warning none
 * @exception none
 * @note <b>History:</b>
 * 1. create function
 *
 */
void loraMacLogInit( void )
{
    s_tLoRaMacLog.txLog.txReqCnt = 0;
    s_tLoRaMacLog.txLog.txRetryCnt = 0;
    s_tLoRaMacLog.txLog.txSucCnt = 0;
    s_tLoRaMacLog.txLog.txFailedCnt = 0;
    s_tLoRaMacLog.rxLog.rxCnt = 0;
    s_tLoRaMacLog.rxLog.rxLostCnt = 0;
    s_tLoRaMacLog.rxLog.rxRetryCnt = 0;
}


/** ****************************************************************************
 * @fn void loraMacLogTxSuccess( void )
 * @brief loraMac tx success log, when this is called, txSucCnt += 1
 * @warning none
 * @exception none
 * @note <b>History:</b>
 * 1. create function
 *
 */
void loraMacLogTxSuccess( void )
{
    s_tLoRaMacLog.txLog.txSucCnt ++;
}

/** ****************************************************************************
 * @fn void loraMacLogTxFailed( void )
 * @brief loraMac tx failed log, when this is called, txFailedCnt += 1
 * @warning none
 * @exception none
 * @note <b>History:</b>
 * 1. create function
 *
 */
void loraMacLogTxFailed( void )
{
    s_tLoRaMacLog.txLog.txFailedCnt ++;
}

/** ****************************************************************************
 * @fn void loraMacLogTxRequest( void )
 * @brief loraMac tx request log, when this is called, txReqCnt += 1
 * @warning none
 * @exception none
 * @note <b>History:</b>
 * 1. create function
 *
 */
void loraMacLogTxRequest( void )
{
    s_tLoRaMacLog.txLog.txReqCnt ++;
}

/** ****************************************************************************
 * @fn void loraMacLogTxRequest( void )
 * @brief loraMac tx retry log, when this is called, txRetryCnt += 1
 * @warning none
 * @exception none
 * @note <b>History:</b>
 * 1. create function
 *
 */
void loraMacLogTxRetry( void )
{
    s_tLoRaMacLog.txLog.txRetryCnt ++;
}



/*******************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */

#endif
