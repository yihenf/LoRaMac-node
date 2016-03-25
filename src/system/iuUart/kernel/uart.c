/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    uart.c
* @data         2015/07/08
* @auther       chuanpengl
* @module       dtu module
* @brief        dtu module
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "uart.h"

//#if (APP_USE_DTU == APP_MODULE_ON)


/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */


/***************************************************************************************************
 * TYPEDEFS
 */


/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */

/***************************************************************************************************
 * @fn      uart_TimeToTimeout__()
 *
 * @brief   uart assert
 *
 * @author  chuanpengl
 *
 * @param   a_u8Timeout  - time out value
 *          a_u8Now  - time now
 *
 * @return  none
 */
static uartI8 uart_TimeToTimeout__( uartU8 a_u8Timeout, uartU8 a_u8Now );

/***************************************************************************************************
 * @fn      uart_AssertParam__()
 *
 * @brief   uart assert
 *
 * @author  chuanpengl
 *
 * @param   a_bCondition
 *
 * @return  none
 */
void uart_AssertParam__( uartBool a_bCondition );

/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */


/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
/***************************************************************************************************
 * @fn      uart_Init()
 *
 * @brief   uart interface init 
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void uart_Init( uartHandle_t *a_ptUart, iuUartPara_t *a_ptUartPara )
{
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart) );
    //uart_AssertParam__( (uartBool)(uartNull != a_ptUart->phUart) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hInit) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hGetTick) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hDeInit) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hSendByteWithIt) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->ptRxFifo) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->ptTxFifo) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hEvt) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUartPara) );

    a_ptUart->bRxMonitorTmo = uartFalse;
    a_ptUart->u8TxState = UART_TX_STATE_IDLE;
    a_ptUart->u8RxState = UART_RX_STATE_NONE;
    a_ptUart->u8Evt = UART_EVT_NONE;
    
    a_ptUart->hInit( a_ptUart, a_ptUartPara );
    a_ptUart->bInitial = uartTrue;

}


/***************************************************************************************************
 * @fn      uart_DeInit()
 *
 * @brief   uart interface deinit 
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void uart_DeInit( uartHandle_t *a_ptUart )
{
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->phUart) );

    
    a_ptUart->hDeInit( uartNull );
}

/***************************************************************************************************
 * @fn      uart_Poll()
 *
 * @brief   uart interface poll, this will trigger event handle when event exist
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *
 * @return  none
 */
void uart_Poll( uartHandle_t *a_ptUart )
{
    uartU8 u8Evt = UART_EVT_NONE;

    uart_AssertParam__( (uartBool)(uartNull != a_ptUart) );
    //uart_AssertParam__( (uartBool)(uartNull != a_ptUart->phUart) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hInit) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hGetTick) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hDeInit) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hSendByteWithIt) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->ptRxFifo) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->ptTxFifo) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hEvt) );
    
    if( a_ptUart->bInitial != uartTrue ){
        return;
    }

    if( fifoTrue == fifo_isFull(a_ptUart->ptRxFifo) ){
        u8Evt |= UART_EVT_RX_FIFO_FULL;
    }else{
        if( fifo_Size( a_ptUart->ptRxFifo ) > (fifo_Capacity( a_ptUart->ptRxFifo ) >> 1 )){
            u8Evt |= UART_EVT_RX_FIFO_HFULL;
        }
    }
    
    if( UART_RX_STATE_START == a_ptUart->u8RxState ){
        a_ptUart->u8RxState = UART_RX_STATE_ING;
        u8Evt |= UART_EVT_RX_START;
    }
    
    if( (uartTrue == a_ptUart->bRxMonitorTmo) && (a_ptUart->u8RxState != UART_RX_STATE_TIMEOUT) ){
        if( 0 >= uart_TimeToTimeout__( a_ptUart->u8RxTimeout, (uartU8)(a_ptUart->hGetTick() & 0xFF) )){
            a_ptUart->bRxMonitorTmo = uartFalse;
            a_ptUart->u8RxState = UART_RX_STATE_TIMEOUT;
            u8Evt |= UART_EVT_RX_TIMEOUT;
        }
    }
    
    if( UART_TX_STATE_EMPTY_EVT == a_ptUart->u8TxState ){
        u8Evt |= UART_EVT_TX_FIFO_EMPTY;
        a_ptUart->u8TxState =UART_TX_STATE_IDLE;
    }
    
    if ( (a_ptUart->u8TxState == UART_TX_STATE_IDLE) && \
            (a_ptUart->u8RxState == UART_RX_STATE_NONE ) ){
        if ( uartTrue == a_ptUart->bBusy ){
            u8Evt |= UART_EVT_IDLE;
            a_ptUart->bBusy = uartFalse;
        }
    }else{
        a_ptUart->bBusy = uartTrue;
    }
    
    if( u8Evt > 0 ){
        a_ptUart->hEvt( a_ptUart, u8Evt );
    }

}   /* uart_Poll() */

uartBool uart_IsBusy( uartHandle_t *a_ptUart )
{
    return a_ptUart->bBusy;
}

/***************************************************************************************************
 * @fn      uart_GetSendByte()
 *
 * @brief   uart get send byte
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *          a_pu8Data  - get fifo data
 *
 * @return  uartFalse  - failed, uartTrue  - success
 */
uartBool uart_GetSendByte( uartHandle_t *a_ptUart, uartU8 *a_pu8Data )
{
    if( a_ptUart->bInitial != uartTrue ){
        return uartFalse;
    }

    return (uartBool)fifo_Pop( a_ptUart->ptTxFifo, a_pu8Data );
}   /* uart_GetSendByte() */

/***************************************************************************************************
 * @fn      uart_SetReceiveByte()
 *
 * @brief   uart set receive byte
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *          a_pu8Data  - receive fifo data
 *
 * @return  uartFalse  - failed, uartTrue  - success
 */
uartBool uart_SetReceiveByte( uartHandle_t *a_ptUart, uartU8 a_u8Data )
{
    if( a_ptUart->bInitial != uartTrue ){
        return uartFalse;
    }
    
    if( (a_ptUart->u8RxState == UART_RX_STATE_NONE) || (a_ptUart->u8RxState == UART_RX_STATE_TIMEOUT) ){
        a_ptUart->u8RxState = UART_RX_STATE_START;
    }
    
    a_ptUart->bRxMonitorTmo = uartTrue;
    a_ptUart->u8RxTimeout = (a_ptUart->hGetTick() & 0xFF) + a_ptUart->u8RxInterval;
    return (uartBool)fifo_Push( a_ptUart->ptRxFifo, a_u8Data );
}   /* uart_SetReceiveByte() */

void uart_SetTxEmptyFlag( uartHandle_t *a_ptUart )
{
    a_ptUart->u8TxState = UART_TX_STATE_EMPTY_EVT;
}


/***************************************************************************************************
 * @fn      uart_GetReceiveLength()
 *
 * @brief   uart get receive length
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *
 * @return  receive length
 */
uartU16 uart_GetReceiveLength( uartHandle_t *a_ptUart )
{
    if( a_ptUart->bInitial != uartTrue ){
        return 0;
    }
    return (uartU16)fifo_Size( a_ptUart->ptRxFifo );
}

/***************************************************************************************************
 * @fn      uart_SendBytes()
 *
 * @brief   uart interface poll, this will trigger event handle when event exist
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *
 * @return  real receive bytes
 */
uartU16 uart_ReceiveBytes( uartHandle_t *a_ptUart, uartU8 *a_pu8Data, uartU16 a_u16Length )
{
    uartU16 u16RealReceBytes = a_u16Length;    /* real receive bytes equal request receive bytes */

    if( a_ptUart->bInitial != uartTrue ){
        return 0;
    }
    
    for( uartU16 i = 0; i < a_u16Length; i++ ){
        /* if fifo length is smaller than request length */
        if( fifoFalse == fifo_Pop( a_ptUart->ptRxFifo, &a_pu8Data[i] )){
            a_ptUart->u8RxState = UART_RX_STATE_NONE;
            u16RealReceBytes = i + 1;        /* real receive bytes is fifo used size */
            break;
        }
    }
    return u16RealReceBytes;
}


/***************************************************************************************************
 * @fn      uart_SendBytes()
 *
 * @brief   uart interface poll, this will trigger event handle when event exist
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *
 * @return  none
 */
uartBool uart_SendBytes( uartHandle_t *a_ptUart, uartU8 *a_pu8Data, uartU8 a_u8Length )
{
    uartBool bSuccess = uartTrue;
    uartU8 u8Data;
    
    if( a_ptUart->bInitial != uartTrue ){
        return uartFalse;
    }
    
    for( uartU8 i = 0; i < a_u8Length; i++ ){
        if ( fifoTrue != fifo_Push( a_ptUart->ptTxFifo, a_pu8Data[i] ) ){
            bSuccess = uartFalse;
            break;
        }
    }

    __UART_DISABLE_INT();
    if ( a_ptUart->u8TxState != UART_TX_STATE_ING ){
        a_ptUart->u8TxState = UART_TX_STATE_ING;
        if( uartTrue == uart_GetSendByte( a_ptUart, &u8Data ) ){
            a_ptUart->hSendByteWithIt( u8Data );
        }
    }
    __UART_ENABLE_INT();

    return bSuccess;
}

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */





/***************************************************************************************************
 * @fn      dtu_UartIfPoll__()
 *
 * @brief   dtu uart interface poll, dtu will communicate with host through this interface
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void dtu_UartIfPoll__( void )
{
    #if 0
    os_EnterCritical();
    if( dtuTrue == gs_tUart.bRxTmoMonitor ){
        if(0 >= os_TaskTimeToTmo(gs_tUart.u32RxTimeout, os_GetTick()))
        {
            gs_tUart.bRxTmoMonitor = dtuFalse;
            
            /* timeout event */
            gs_u16DtuLoRaSendReqSize = fifo_Size(gs_tUart.ptRxFifo);
            gs_eDtuRfStReq = DTU_RF_ST_TX;
        }
    }
    os_ExitCritical();
    #endif
}

/***************************************************************************************************
 * @fn      uart_TimeToTimeout__()
 *
 * @brief   uart assert
 *
 * @author  chuanpengl
 *
 * @param   a_u8Timeout  - time out value
 *          a_u8Now  - time now
 *
 * @return  none
 */
uartI8 uart_TimeToTimeout__( uartU8 a_u8Timeout, uartU8 a_u8Now )
{
    uartI8 i8Delta = 0;
    uartU8 u8Delta = 0;
    
    
    
    u8Delta = a_u8Timeout >= a_u8Now ? a_u8Timeout - a_u8Now : a_u8Now - a_u8Timeout;
    
    if( u8Delta < 0x80 ){
        i8Delta = a_u8Timeout >= a_u8Now ? u8Delta : -u8Delta;
    }else{
        u8Delta = 0x100 - u8Delta;
        i8Delta = a_u8Timeout > a_u8Now ? -u8Delta : u8Delta;
    }
    
    return i8Delta;
}   /* uart_TimeToTimeout__() */


/***************************************************************************************************
 * @fn      uart_AssertParam__()
 *
 * @brief   uart assert
 *
 * @author  chuanpengl
 *
 * @param   a_bCondition
 *
 * @return  none
 */
void uart_AssertParam__( uartBool a_bCondition )
{
    while( uartFalse == a_bCondition ){
    }
}   /* uart_AssertParam__() */



/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150708
*   context: here write modified history
*
***************************************************************************************************/
