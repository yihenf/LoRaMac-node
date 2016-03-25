/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    uart.h
* @data         2015/07/08
* @auther       chuanpengl
* @module       uart module
* @brief        uart module
***************************************************************************************************/
#ifndef __UART_H__
#define __UART_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "iuUart/iuUartType.h"
#include "iuFifo/cfifo.h"

//#if (APP_USE_DTU == APP_MODULE_ON)

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */

#define UART_EVT_NONE               (0x00)
#define UART_EVT_RX_TIMEOUT         (0x01)
#define UART_EVT_RX_FIFO_FULL       (0x02)
#define UART_EVT_RX_FIFO_HFULL      (0x04)
#define UART_EVT_RX_START           (0x08)
#define UART_EVT_TX_FIFO_EMPTY      (0x10)
#define UART_EVT_IDLE               (0x80)



#define UART_RX_STATE_NONE                (0x00)
#define UART_RX_STATE_START               (0x01)
#define UART_RX_STATE_ING                 (0x02)
#define UART_RX_STATE_TIMEOUT             (0x03)

#define UART_TX_STATE_IDLE                  (0x00)
#define UART_TX_STATE_ING                   (0x01)
#define UART_TX_STATE_EMPTY_EVT             (0x02)

#define __UART_DISABLE_INT()                __disable_irq();

#define __UART_ENABLE_INT()                 __enable_irq();



#define UART_INSTALL_INIT_HANDLE( __handle )        .hInit = (__handle)
#define UART_INSTALL_EXIT_HANDLE( __handle )        .hDeInit = (__handle)
#define UART_INSTALL_TXBYTE_WITH_IT_HANDLE( __handle )      .hSendByteWithIt = (__handle)
#define UART_INSTALL_EVENT_HANDLE( __handle )       .hEvt = (__handle)
#define UART_INSTALL_GET_TICK_HANDLE( __handle )    .hGetTick = (uart_GetTickHdl)(__handle)
#define UART_SET_RX_TIMEOUT_TIME( __timeout )       .u8RxInterval = (__timeout)
#define UART_SET_TX_FIFO( __fifo )                  .ptTxFifo = (fifoBase_t*)(__fifo)
#define UART_SET_RX_FIFO( __fifo )                  .ptRxFifo = (fifoBase_t*)(__fifo)
    
/***************************************************************************************************
 * TYPEDEFS
 */


typedef struct tag_UartHandle_t uartHandle_t;
typedef void (*uart_InitHdl)( uartHandle_t *a_ptUart, iuUartPara_t *a_ptUartPara );
typedef void (*uart_DeInitHdl)( void *a_ptUart );
typedef uartBool (*uart_SendByteWithItHdl)( uartU8 a_u8Byte );
typedef uartU32 (*uart_GetTickHdl)( void );
typedef uartBool (*uart_isTxingHdl)( void );
typedef uartU8 (*uart_EventHdl)( uartHandle_t *a_ptUart, uartU8 a_u8Evt);

struct tag_UartHandle_t{
    uartBool bInitial;          /* it is initial state or not */
    void *phUart;               /* this will be cast to type hal_uart use */
    iuUartPara_t *ptUartPara;
    fifoBase_t *ptTxFifo;
    fifoBase_t *ptRxFifo;
    uart_InitHdl hInit;
    uart_DeInitHdl hDeInit;
    uart_SendByteWithItHdl hSendByteWithIt;     /* transmit byte handle */
    uart_GetTickHdl hGetTick;       /* get tick */
    uart_EventHdl hEvt;             /* event export */
    uartU8 u8RxInterval;  /* when rx stop time is bigger than it, we think a fram end */
    
    
    /* parameter below no need to be inited */
    uartU8 u8RxTimeout;
    uartBool bRxMonitorTmo;  /* when true, should monitor rx timeout  */
    uartU8 u8TxState;       /* tx state */
    uartU8 u8RxState;       /* rx state, 0 - no rx, 1 - start rx, 2 - rx ing, 3 - rx timeout */
    uartU8 u8Evt;           /* event flag */
    uartBool bBusy;         /* uart busy or not */
};


//uartBool uart_ReceiveByte( uartHandle_t * a_ptUart )


/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
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
void uart_Init( uartHandle_t *a_ptUart, iuUartPara_t *a_ptUartPara );

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
void uart_DeInit( uartHandle_t *a_ptUart );

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
void uart_Poll( uartHandle_t *a_ptUart );
uartBool uart_IsBusy( uartHandle_t *a_ptUart );
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
uartBool uart_GetSendByte( uartHandle_t *a_ptUart, uartU8 *a_pu8Data );

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
uartBool uart_SetReceiveByte( uartHandle_t *a_ptUart, uartU8 a_pu8Data );
void uart_SetTxEmptyFlag( uartHandle_t *a_ptUart );
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
uartU16 uart_GetReceiveLength( uartHandle_t *a_ptUart );

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
uartU16 uart_ReceiveBytes( uartHandle_t *a_ptUart, uartU8 *a_pu8Data, uartU16 a_u16Length );


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
uartBool uart_SendBytes( uartHandle_t *a_ptUart, uartU8 *a_pu8Data, uartU8 a_u8Length );

#ifdef __cplusplus
}
#endif

//#endif  /* (APP_USE_DTU == APP_MODULE_ON) */

#endif /* __UART_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150708
*   context: here write modified history
*
***************************************************************************************************/
