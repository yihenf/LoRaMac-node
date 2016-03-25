/***************************************************************************************************
 * Copyright (c) 2015-2020 XXXXXXXXXXXXXXXX. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    eli_uart_type.h
* @data         13/11/2015
* @author       chuanpengl
* @module       eli uart
* @brief        file description
***************************************************************************************************/
#ifndef __ELI_UART_TYPE_H__
#define __ELI_UART_TYPE_H__
/***************************************************************************************************
 * INCLUDES
 */


#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define UART_INIT_DEF( __name )          __name##Init
#define UART_EXIT_DEF( __name )          __name##DeInit
#define UART_SEND_BYTE_WITH_IT_DEF( __name ) __name##SendByteWithIt
#define UART_EVENT_DEF( __name )        __name##UartEvent
#define UART_TX_CPLT_CALLBACK_DEF( __name )     __name##TxCpltCallback
#define UART_RX_CPLT_CALLBACK_DEF( __name )     __name##RxCpltCallback
#define UART_ERROR_CPLT_CALLBACK_DEF( __name )     __name##ErrorCpltCallback

/***************************************************************************************************
 * TYPEDEFS
 */
typedef char uartChar;
typedef unsigned char uartU8;
typedef signed char uartI8;
typedef unsigned short uartU16;
typedef signed short uartI16;
typedef unsigned int uartU32;
typedef signed int uartI32;
typedef enum{uartFalse = 0, uartTrue = !uartFalse}uartBool;
#define uartNull     ((void*)0)

typedef enum{
    IU_UART_PARITY_NONE = 0,
    IU_UART_PARITY_EVEN = 1,
    IU_UART_PARITY_ODD = 2
}iuUartParity_e;

typedef enum{
    IU_UART_BIT_LEN_8 = 0,
    IU_UART_BIT_LEN_9 = 1
}iuUartBitLength_e;

typedef enum{
    IU_UART_STOPBITS_1 = 0,
    IU_UART_STOPBITS_2 = 1
}iuUartStopBits_e;

typedef enum{
    IU_UART_HWCONTROL_NONE = 0,
    IU_UART_HWCONTROL_RTS = 1,
    IU_UART_HWCONTROL_CTS = 2,
    IU_UART_HWCONTROL_RCTS = 3
}iuUartHwFlowCtl_e;

typedef enum{
    IU_UART_RUN_RX = 0,
    IU_UART_RUN_TX = 1,
    IU_UART_RUN_RTX = 2
}iuUartRun_e;

enum{
    UART_IND_TX_EMPTY = 0,
    UART_IND_RX_START = 1,
    UART_IND_RX_RECEIVED = 2,
    UART_IND_IDLE = 3
};

typedef void (*iuUartInd_h)( uartU8 a_ucIndType, uartI32 a_uiVal );

typedef struct
{
    uartU32 ulBaudrate;
    iuUartParity_e eParity;
    iuUartBitLength_e eBitLen;
    iuUartStopBits_e eStopBits;
    iuUartHwFlowCtl_e eHwFlowCtl;
    iuUartRun_e eRunSt;
    iuUartInd_h hInd;
}iuUartPara_t;


/* uart exit type */
typedef void (*eli_UartExit_h)( void );
/* uart poll type */
typedef void (*eli_UartPoll_h)( void );
/* uart send bytes type */
typedef void (*eli_UartSendBytes_h)( uartU8 *a_pu8Data, uartU16 a_u16Len );
/* uart read receive bytes */
typedef uartU16 (*eli_UartReadReceiveBytes_h)( uartU8 *a_pu8Data, uartU16 a_u16Len );
/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
 */


#ifdef __cplusplus
}
#endif

#endif /* __ELI_UART_TYPE_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 28/08/2015
*   context: here write modified history
*
***************************************************************************************************/
