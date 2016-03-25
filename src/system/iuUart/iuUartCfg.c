/***************************************************************************************************
 * Copyright (c) 2015-2020 XXXXXXXXXXXXXXXX. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    uart_config
* @data         28/08/2015
* @author       chuanpengl
* @module       module name
* @brief        file description
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <stdbool.h>
#include "iuUartCfg.h"
#include "device/iuUartDevice.h"
#include "kernel/uart.h"
#include "iuFifo/cfifo.h"
#include "timer.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */


#define UART1_TX_FIFO_SIZE      (64)
#define UART1_RX_FIFO_SIZE      (256)

#define UART2_TX_FIFO_SIZE      (64)
#define UART2_RX_FIFO_SIZE      (128)

#define UART3_TX_FIFO_SIZE      (64)
#define UART3_RX_FIFO_SIZE      (64)

#define UART4_TX_FIFO_SIZE      (64)
#define UART4_RX_FIFO_SIZE      (64)

#define UART5_TX_FIFO_SIZE      (64)
#define UART5_RX_FIFO_SIZE      (64)

/***************************************************************************************************
 * TYPEDEFS
 */
#if UART_USE_UART1
FIFO_DEF(uart1TxFifo_t, UART1_TX_FIFO_SIZE)
FIFO_DEF(uart1RxFifo_t, UART1_RX_FIFO_SIZE)
#endif

#if UART_USE_UART2
FIFO_DEF(uart2TxFifo_t, UART2_TX_FIFO_SIZE)
FIFO_DEF(uart2RxFifo_t, UART2_RX_FIFO_SIZE)
#endif

#if UART_USE_UART3
FIFO_DEF(uart3TxFifo_t, UART3_TX_FIFO_SIZE)
FIFO_DEF(uart3RxFifo_t, UART3_RX_FIFO_SIZE)
#endif

#if UART_USE_UART4
FIFO_DEF(uart4TxFifo_t, UART4_TX_FIFO_SIZE)
FIFO_DEF(uart4RxFifo_t, UART4_RX_FIFO_SIZE)
#endif

#if UART_USE_UART5
FIFO_DEF(uart5TxFifo_t, UART5_TX_FIFO_SIZE)
FIFO_DEF(uart5RxFifo_t, UART5_RX_FIFO_SIZE)
#endif
/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
uartU32 uartGetTickMs__( void );

/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */
#if UART_USE_UART1
uart1TxFifo_t s_tUart1TxFifo = FIFO_INIT_VALUE(UART1_TX_FIFO_SIZE);
uart1RxFifo_t s_tUart1RxFifo = FIFO_INIT_VALUE(UART1_RX_FIFO_SIZE);
#endif

#if UART_USE_UART2
uart2TxFifo_t s_tUart2TxFifo = FIFO_INIT_VALUE(UART2_TX_FIFO_SIZE);
uart2RxFifo_t s_tUart2RxFifo = FIFO_INIT_VALUE(UART2_RX_FIFO_SIZE);
#endif

#if UART_USE_UART3
uart3TxFifo_t s_tUart3TxFifo = FIFO_INIT_VALUE(UART3_TX_FIFO_SIZE);
uart3RxFifo_t s_tUart3RxFifo = FIFO_INIT_VALUE(UART3_RX_FIFO_SIZE);
#endif

#if UART_USE_UART4
uart4TxFifo_t s_tUart4TxFifo = FIFO_INIT_VALUE(UART4_TX_FIFO_SIZE);
uart4RxFifo_t s_tUart4RxFifo = FIFO_INIT_VALUE(UART4_RX_FIFO_SIZE);
#endif

#if UART_USE_UART5
uart5TxFifo_t s_tUart5TxFifo = FIFO_INIT_VALUE(UART5_TX_FIFO_SIZE);
uart5RxFifo_t s_tUart5RxFifo = FIFO_INIT_VALUE(UART5_RX_FIFO_SIZE);
#endif


uartHandle_t s_atUartHdl[] = 
{
#if (UART_USE_UART1 == 1)
    {
        UART_SET_TX_FIFO( &s_tUart1TxFifo ),
        UART_SET_RX_FIFO( &s_tUart1RxFifo ),
        UART_INSTALL_INIT_HANDLE( UART_INIT_DEF(uart1) ),
        UART_INSTALL_EXIT_HANDLE( UART_EXIT_DEF(uart1) ),
        UART_INSTALL_TXBYTE_WITH_IT_HANDLE( UART_SEND_BYTE_WITH_IT_DEF(uart1) ),
        UART_INSTALL_EVENT_HANDLE( UART_EVENT_DEF(uart1) ),
        UART_INSTALL_GET_TICK_HANDLE( uartGetTickMs__ ),
        UART_SET_RX_TIMEOUT_TIME( 10 )
    },
#endif
#if (UART_USE_UART2 == 1)
    {
        UART_SET_TX_FIFO( &s_tUart2TxFifo ),
        UART_SET_RX_FIFO( &s_tUart2RxFifo ),
        UART_INSTALL_INIT_HANDLE( UART_INIT_DEF(uart2) ),
        UART_INSTALL_EXIT_HANDLE( UART_EXIT_DEF(uart2) ),
        UART_INSTALL_TXBYTE_WITH_IT_HANDLE( UART_SEND_BYTE_WITH_IT_DEF(uart2) ),
        UART_INSTALL_EVENT_HANDLE( UART_EVENT_DEF(uart2) ),
        UART_INSTALL_GET_TICK_HANDLE( uartGetTickMs__ ),
        UART_SET_RX_TIMEOUT_TIME( 10 )
    },
#endif
#if (UART_USE_UART3 == 1)
    {
        UART_SET_TX_FIFO( &s_tUart3TxFifo ),
        UART_SET_RX_FIFO( &s_tUart3RxFifo ),
        UART_INSTALL_INIT_HANDLE( UART_INIT_DEF(uart3) ),
        UART_INSTALL_EXIT_HANDLE( UART_EXIT_DEF(uart3) ),
        UART_INSTALL_TXBYTE_WITH_IT_HANDLE( UART_SEND_BYTE_WITH_IT_DEF(uart3) ),
        UART_INSTALL_EVENT_HANDLE( UART_EVENT_DEF(uart3) ),
        UART_INSTALL_GET_TICK_HANDLE( uartGetTickMs__ ),
        UART_SET_RX_TIMEOUT_TIME( 10 )
    },
#endif
#if (UART_USE_UART4 == 1)
    {
        UART_SET_TX_FIFO( &s_tUart4TxFifo ),
        UART_SET_RX_FIFO( &s_tUart4RxFifo ),
        UART_INSTALL_INIT_HANDLE( UART_INIT_DEF(uart4) ),
        UART_INSTALL_EXIT_HANDLE( UART_EXIT_DEF(uart4) ),
        UART_INSTALL_TXBYTE_WITH_IT_HANDLE( UART_SEND_BYTE_WITH_IT_DEF(uart4) ),
        UART_INSTALL_EVENT_HANDLE( UART_EVENT_DEF(uart4) ),
        UART_INSTALL_GET_TICK_HANDLE( uartGetTickMs__ ),
        UART_SET_RX_TIMEOUT_TIME( 10 )
    },
#endif
#if (UART_USE_UART5 == 1)
    {
        UART_SET_TX_FIFO( &s_tUart5TxFifo ),
        UART_SET_RX_FIFO( &s_tUart5RxFifo ),
        UART_INSTALL_INIT_HANDLE( UART_INIT_DEF(uart5) ),
        UART_INSTALL_EXIT_HANDLE( UART_EXIT_DEF(uart5) ),
        UART_INSTALL_TXBYTE_WITH_IT_HANDLE( UART_SEND_BYTE_WITH_IT_DEF(uart5) ),
        UART_INSTALL_EVENT_HANDLE( UART_EVENT_DEF(uart5) ),
        UART_INSTALL_GET_TICK_HANDLE( uartGetTickMs__ ),
        UART_SET_RX_TIMEOUT_TIME( 10 )
    },
#endif
};


/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      uartCfg_Init()
 *
 * @brief   Init usart
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void uartCfg_Init( uartU8 a_ucChn, iuUartPara_t *a_ptUartPara )
{
    if ( a_ucChn < IU_UART_CNT )
    {
        fifo_Init( s_atUartHdl[a_ucChn].ptTxFifo, s_atUartHdl[a_ucChn].ptTxFifo->tFifo.capacity);//sizeof( s_atUartHdl[a_ucChn].ptTxFifo->au8Array ));
        fifo_Init( s_atUartHdl[a_ucChn].ptRxFifo, s_atUartHdl[a_ucChn].ptRxFifo->tFifo.capacity);
        uart_Init(&s_atUartHdl[a_ucChn], a_ptUartPara);
    }
} /* uartCfg_Init() */

/***************************************************************************************************
 * @fn      uartCfg_DeInit()
 *
 * @brief   DeInit usart
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void uartCfg_DeInit( uartU8 a_u8Chn )
{
    if ( a_u8Chn < 5 )
    {
        uart_DeInit( &s_atUartHdl[a_u8Chn] );
    }

} /* uartCfg_DeInit() */


/***************************************************************************************************
 * @fn      uartCfg_SendBytes()
 *
 * @brief   send data
 *
 * @author  chuanpengl
 *
 * @param   a_pu8Data  - data for sending
 *          a_u16Length  - data length
 *
 * @return  none
 */
void uartCfg_SendBytes( uartU8 a_u8Chn, uartU8  *a_pu8Data, uartU16 a_u16Length )
{
    if ( a_u8Chn < IU_UART_CNT )
    {
        uart_SendBytes( &s_atUartHdl[a_u8Chn], a_pu8Data, a_u16Length );
    }
    
}

/***************************************************************************************************
 * @fn      uartCfg_GetReceiveBytes()
 *
 * @brief   get receive data
 *
 * @author  chuanpengl
 *
 * @param   a_pu8Data  - data pointer
 *          a_u16Length  - data length
 *
 * @return  get bytes
 */
uartU16 uartCfg_GetReceiveBytes( uartU8 a_u8Chn, uartU8  *a_pu8Data, uartU16 a_u16Length )
{
    uartU16 u16GetByte = 0;
    
    if ( a_u8Chn < IU_UART_CNT )
    {
        u16GetByte = uart_ReceiveBytes( &s_atUartHdl[a_u8Chn], a_pu8Data, a_u16Length );
    }
    
    return u16GetByte;
}

/***************************************************************************************************
 * @fn      uartCfg_Poll()
 *
 * @brief   usart poll
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void uartCfg_Poll( uartU8 a_u8Chn )
{
    if ( a_u8Chn < IU_UART_CNT )
    {
        uart_Poll( &s_atUartHdl[a_u8Chn] );
    }
} /* uartCfg_Poll() */

uartBool uartCfg_IsBusy( uartU8 a_u8Chn )
{
    return uart_IsBusy( &s_atUartHdl[ a_u8Chn ] );
}

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */

/***************************************************************************************************
 * @fn      HAL_UART_TxCpltCallback()
 *
 * @brief   dtu uart tx cplt callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
#if (UART_USE_UART1 == 1 )
    if ( USART1 == huart->Instance )
    {
        UART_TX_CPLT_CALLBACK_DEF(uart1)( huart );
    }
#endif
#if (UART_USE_UART2 == 1 )
    if ( USART2 == huart->Instance )
    {
        UART_TX_CPLT_CALLBACK_DEF(uart2)( huart );
    }
#endif
#if (UART_USE_UART3 == 1 )
    if ( USART3 == huart->Instance )
    {
        UART_TX_CPLT_CALLBACK_DEF(uart3)( huart );
    }
#endif
#if (UART_USE_UART4 == 1 )
    if ( UART4 == huart->Instance )
    {
        UART_TX_CPLT_CALLBACK_DEF(uart4)( huart );
    }
#endif


}

/***************************************************************************************************
 * @fn      HAL_UART_RxCpltCallback()
 *
 * @brief   dtu uart tx cplt callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
#if UART_USE_UART1
    if ( USART1 == huart->Instance )
    {
        UART_RX_CPLT_CALLBACK_DEF(uart1)( huart );
    }
#endif
#if UART_USE_UART2
    if ( USART2 == huart->Instance )
    {
        UART_RX_CPLT_CALLBACK_DEF(uart2)( huart );
    }
#endif
#if UART_USE_UART3
    if ( USART3 == huart->Instance )
    {
        UART_RX_CPLT_CALLBACK_DEF(uart3)( huart );
    }
#endif
#if UART_USE_UART4
    if ( UART4 == huart->Instance )
    {
        UART_RX_CPLT_CALLBACK_DEF(uart4)( huart );
    }
#endif
#if UART_USE_UART5
    if ( UART5 == huart->Instance )
    {
        UART_RX_CPLT_CALLBACK_DEF(uart5)( huart );
    }
#endif
}

/***************************************************************************************************
 * @fn      HAL_UART_ErrorCallback()
 *
 * @brief   dtu uart error callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
#if UART_USE_UART1
    if ( USART1 == huart->Instance )
    {
        UART_ERROR_CPLT_CALLBACK_DEF(uart1)( huart );
    }
#endif
#if UART_USE_UART2
    if ( USART2 == huart->Instance )
    {
        UART_ERROR_CPLT_CALLBACK_DEF(uart2)( huart );
    }
#endif
#if UART_USE_UART3
    if ( USART3 == huart->Instance )
    {
        UART_ERROR_CPLT_CALLBACK_DEF(uart3)( huart );
    }
#endif
#if UART_USE_UART4
    if ( UART4 == huart->Instance )
    {
        UART_ERROR_CPLT_CALLBACK_DEF(uart4)( huart );
    }
#endif
#if UART_USE_UART5
    if ( UART5 == huart->Instance )
    {
        UART_ERROR_CPLT_CALLBACK_DEF(uart5)( huart );
    }
#endif
}

uartU32 uartGetTickMs__( void )
{
    return HAL_GetTick();
}

/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 28/08/2015
*   context: here write modified history
*
***************************************************************************************************/
