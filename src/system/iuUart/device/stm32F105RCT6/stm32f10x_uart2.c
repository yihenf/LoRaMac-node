/***************************************************************************************************
 * Copyright (c) 2015-2020 XXXXXXXXXXXXXXXX. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    stm32f10x_uart2.c
* @data         17/11/2015
* @author       chuanpengl
* @module       module name
* @brief        file description
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "stm32f10x_uart2.h"
#include "stm32f1xx_hal.h"
#include "iuUart/kernel/uart.h"

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
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */

static UART_HandleTypeDef s_tUART = {USART2};
static uartU8 gs_UartTxByte = 0;
static uartU8 gs_UartRxByte = 0;
static iuUartInd_h s_hInd = uartNull;
static uartHandle_t *s_ptUartHandle = uartNull;

/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */

/***************************************************************************************************
 * @fn      UART_INIT_DEF(uart2)()
 *
 * @brief   uart2 interface init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void UART_INIT_DEF(uart2)( uartHandle_t *a_ptUartHdl, iuUartPara_t *a_ptUartPara )
{
    GPIO_InitTypeDef tGpio_Init;

    s_ptUartHandle = a_ptUartHdl;
    s_hInd = a_ptUartPara->hInd;
    s_tUART.Instance = USART2;
    
    s_tUART.Init.BaudRate = a_ptUartPara->ulBaudrate;
    
    switch ( a_ptUartPara->eParity )
    {
        case IU_UART_PARITY_EVEN:
        s_tUART.Init.Parity = UART_PARITY_EVEN;
        break;
        case IU_UART_PARITY_ODD:
        s_tUART.Init.Parity = UART_PARITY_ODD;
        break;
        case IU_UART_PARITY_NONE:
        s_tUART.Init.Parity = UART_PARITY_NONE;
        break;
        default:
        s_tUART.Init.Parity = UART_PARITY_NONE;
        break;
    }

    switch ( a_ptUartPara->eBitLen )
    {
        case IU_UART_BIT_LEN_8:
        s_tUART.Init.WordLength = UART_WORDLENGTH_8B;
        break;
        case IU_UART_BIT_LEN_9:
        s_tUART.Init.WordLength = UART_WORDLENGTH_9B;
        break;
        default:
        s_tUART.Init.WordLength = UART_WORDLENGTH_8B;
        break;
    }
    
    switch ( a_ptUartPara->eStopBits )
    {
        case IU_UART_STOPBITS_1:
        s_tUART.Init.StopBits = UART_STOPBITS_1;
        break;
        case IU_UART_STOPBITS_2:
        s_tUART.Init.StopBits = UART_STOPBITS_2;
        break;
        default:
        s_tUART.Init.StopBits = UART_STOPBITS_1;
        break;
    }
    
    switch ( a_ptUartPara->eRunSt )
    {
        case IU_UART_RUN_RX:
        s_tUART.Init.Mode = UART_MODE_RX;
        break;
        case IU_UART_RUN_TX:
        s_tUART.Init.Mode = UART_MODE_TX;
        break;
        case IU_UART_RUN_RTX:
        s_tUART.Init.Mode = UART_MODE_TX_RX;
        break;
        default:
        s_tUART.Init.Mode = UART_MODE_TX_RX;
        break;
    }

    switch ( a_ptUartPara->eHwFlowCtl )
    {
        case IU_UART_HWCONTROL_RTS:
        s_tUART.Init.HwFlowCtl = UART_HWCONTROL_RTS;
        break;
        case IU_UART_HWCONTROL_CTS:
        s_tUART.Init.HwFlowCtl = UART_HWCONTROL_CTS;
        break;
        case IU_UART_HWCONTROL_RCTS:
        s_tUART.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
        break;
        case IU_UART_HWCONTROL_NONE:
        s_tUART.Init.HwFlowCtl = UART_HWCONTROL_NONE;
        break;
        default:
        s_tUART.Init.HwFlowCtl = UART_HWCONTROL_NONE;
        break;
    }

    s_tUART.Init.OverSampling = UART_OVERSAMPLING_16;

    __USART2_CLK_ENABLE();

    HAL_UART_Init( &s_tUART );

    /* configure NVIC preemption priority bits */
    //HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_0);

    HAL_NVIC_SetPriority( USART2_IRQn, 0, 1 );    /* configLIBRARY_LOWEST_INTERRUPT_PRIORITY */

    HAL_NVIC_EnableIRQ( USART2_IRQn );

    __GPIOA_CLK_ENABLE();
    /* Configure USART Tx as alternate function push-pull, PA2 */
    tGpio_Init.Mode = GPIO_MODE_AF_PP;
    tGpio_Init.Pin = GPIO_PIN_2;
    tGpio_Init.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOA, &tGpio_Init);

    /* Configure USART Rx as input floating, PA3 */
    tGpio_Init.Pin = GPIO_PIN_3;
    tGpio_Init.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOA, &tGpio_Init);

    if ( a_ptUartPara->eRunSt != IU_UART_RUN_TX )
    {
        HAL_UART_Receive_IT( &s_tUART, &gs_UartRxByte, 1);
    }

}

/***************************************************************************************************
 * @fn      UART_EXIT_DEF(uart2)()
 *
 * @brief   uart interface deinit
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void UART_EXIT_DEF(uart2)( void *a_ptUart )
{
    GPIO_InitTypeDef tGpio_Init;

    __USART2_CLK_ENABLE(); 
    HAL_UART_DeInit( &s_tUART );
    __USART2_CLK_DISABLE(); 
    
    /* Enable GPIO TX/RX clock */
    __GPIOA_CLK_ENABLE();
    
    /* UART TX GPIO pin configuration  */
    tGpio_Init.Pin       = GPIO_PIN_2;
    tGpio_Init.Mode      = GPIO_MODE_ANALOG;
    tGpio_Init.Pull      = GPIO_NOPULL;

    HAL_GPIO_Init(GPIOA, &tGpio_Init);

    /* UART RX GPIO pin configuration  */
    tGpio_Init.Pin = GPIO_PIN_3;

    HAL_GPIO_Init(GPIOA, &tGpio_Init);
}

/***************************************************************************************************
 * @fn      UART_SEND_BYTE_WITH_IT_DEF(uart2)()
 *
 * @brief   uart send byte with interrupt
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
uartBool UART_SEND_BYTE_WITH_IT_DEF(uart2)( uartU8 a_u8Byte )
{
    gs_UartTxByte = a_u8Byte;
    HAL_UART_Transmit_IT( &s_tUART, (uint8_t*)&gs_UartTxByte, 1 );
    return uartTrue;
}


/***************************************************************************************************
 * @fn      UART_EVENT_DEF(uart2)()
 *
 * @brief   uart event
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
uartU8 UART_EVENT_DEF(uart2)(  uartHandle_t *a_ptUart, uartU8 a_u8Evt )
{
    uartU8 u8ReceiveLength = 0;

    if( UART_EVT_RX_START == (a_u8Evt & UART_EVT_RX_START) ){
        if(uartNull != s_hInd){
            s_hInd( UART_IND_RX_START, 0 );
        }
    }
    
    if( UART_EVT_RX_TIMEOUT == (a_u8Evt & UART_EVT_RX_TIMEOUT) ){
        u8ReceiveLength = uart_GetReceiveLength( a_ptUart );
        s_hInd( UART_IND_RX_RECEIVED, u8ReceiveLength );
    }else{
        if( UART_EVT_RX_FIFO_FULL == (a_u8Evt & UART_EVT_RX_FIFO_FULL) ){
            u8ReceiveLength = uart_GetReceiveLength( a_ptUart );
            s_hInd( UART_IND_RX_RECEIVED, u8ReceiveLength );
        }

        if( UART_EVT_RX_FIFO_HFULL == (a_u8Evt & UART_EVT_RX_FIFO_HFULL) ){
            u8ReceiveLength = uart_GetReceiveLength( a_ptUart );
            s_hInd( UART_IND_RX_RECEIVED, u8ReceiveLength );
        }
    }

    if( UART_EVT_TX_FIFO_EMPTY == (a_u8Evt & UART_EVT_TX_FIFO_EMPTY) ){
        s_hInd( UART_IND_TX_EMPTY, 0 );
    }

    return 0;
}   /* UART_EVENT_DEF(uart2)() */

/***************************************************************************************************
 * @fn      UART_TX_CPLT_CALLBACK_DEF(uart2)()
 *
 * @brief   dtu uart tx cplt callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void UART_TX_CPLT_CALLBACK_DEF(uart2)(UART_HandleTypeDef *huart)
{
    if ( uartTrue == uart_GetSendByte( s_ptUartHandle, &gs_UartTxByte ) )
    {
        HAL_UART_Transmit_IT( &s_tUART, (uint8_t*)&gs_UartTxByte, 1 );
    }
}   /* UART_TX_CPLT_CALLBACK_DEF(uart2) */

/***************************************************************************************************
 * @fn      UART_RX_CPLT_CALLBACK_DEF(uart2)()
 *
 * @brief   dtu uart tx cplt callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void UART_RX_CPLT_CALLBACK_DEF(uart2)(UART_HandleTypeDef *huart)
{
    uart_SetReceiveByte( s_ptUartHandle, gs_UartRxByte );
    HAL_UART_Receive_IT( &s_tUART, &gs_UartRxByte, 1 );
}   /* UART_RX_CPLT_CALLBACK_DEF(uart2) */

/***************************************************************************************************
 * @fn      UART_ERROR_CPLT_CALLBACK_DEF(uart2)()
 *
 * @brief   dtu uart error callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void UART_ERROR_CPLT_CALLBACK_DEF(uart2)(UART_HandleTypeDef *huart)
{
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    
    /* restart uart */
    
    HAL_UART_Receive_IT( &s_tUART, &gs_UartRxByte, 1 );
    
    if ( uartTrue == uart_GetSendByte( s_ptUartHandle, &gs_UartTxByte ) )
    {
        HAL_UART_Transmit_IT( &s_tUART, (uint8_t*)&gs_UartTxByte, 1 );
    }
    
    
}

/***************************************************************************************************
 * @fn      USART2_IRQHandler()
 *
 * @brief   dtu uart handle
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&s_tUART);
}

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */

/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 27/08/2015
*   context: here write modified history
*
***************************************************************************************************/
