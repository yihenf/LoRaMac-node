/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    file name.c
* @data         2014/09/01
* @auther       chuanpengl
* @module       module name
* @brief        file description
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "app_elevatoresaving.h"

#if APP_USE_ELEVATOR_E_SAVING

#include <stdio.h>
#include <string.h>
#include "os.h"
#include "os_signal.h"
#include "cfifo.h"
#include "stm32l0xx_hal.h"
#include "crc/crc.h"
#include "uart/uart_config.h"

#include "app_net.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */

#define OS_SIG_TASK_ES_RXTMO              	(((osUint32)1) << 1)
#define OS_SIG_TASK_ES_MREAD              	(((osUint32)1) << 2)
#define OS_SIG_TASK_ES_UPLOAD              	(((osUint32)1) << 3)
#define OS_SIG_TASK_ES_GETADDR              (((osUint32)1) << 4)

#define USART_SXTX_CONTROL( s, r )	\
		(s == esTrue)? HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET);\
		(r == esTrue)? HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_RESET):HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_SET);\


	

/***************************************************************************************************
 * TYPEDEFS
 */
typedef struct
{
	esU8	u8Head;
	esU8	u8Device;
	esU8	u8Code;
	esU8	*pu8Data;
	esU8	u8Crc16H;
	esU8	u8Crc16L;
}tFrameTypeDef;

typedef struct
{
	esU16	u16MVol;//母线电压
	esU8	u8MAmps;//母线电流
	esU16	u16SCVol;//超容电压
	esU8	u8SCAmps;//超容电流
	esU8	u8Esave[4];//节电量
	esU8	u8AbsEsave[4];//绝对节电量	
	float	fMinEsave;
	float	fHourEsave;
	float	fDayEsave;
	float	fAbsEsave;
	esU8	u8ElevAddr;	
}tElevESavInfoTypeDef;

typedef enum{
	ES_READY_EVENT = 0,
	ES_SEND_EVENT,
	ES_WAIT_EVENT,
	ES_RECV_EVENT,
	ES_TMO_EVENT,
	ES_USR_EVENT,
	ES_SLEEP_EVENT,
}eEsEventTypeDef;


typedef enum
{
	ES_WRITE_FCODE	=	0x03,
	ES_READ_FCODE	=	0x04,	
	ES_RADDR_FCODE = 0x05,
}eEsFunCodeTypeDef;

typedef struct
{	
	eEsEventTypeDef eEsMasterEvent;	
	eEsFunCodeTypeDef	eEsMasterFunCode;
	esU8	u8MasterTxPool[64];
	esU8	u8MasterTxLen;	
	esU8	u8MasterRxPool[64];
	esU8	u8MasterRxLen;
	esU8	u8TryTimes;
	esU8	u8ElevNums;
	esU8	u8ElevIndex;
	esU8	u8EParamIndex;
}tEsMasterSysTypeDef;



/*
8N1
半双工通信
baud  = 9600; 
addr = 1...247;

code = 0x03: 数据= 0x33(open),数据= 0x66(close)
 		返回 数据= 0x01(success),数据= 0x02(fail)
 
code = 0x04 :读数据 
	TX:	AA 01 04 CRCH CRCL
	RX: AA 01 04 电梯数 节能数据 CRCH CRCL ；有多少电梯就又多少组数
1、数据处理
上位机接收到的母线电压、母线电流、超容电压、超容电流直接转换为十进制。
节电量：发送给上位机的所有节电量都扩大了100倍，上位机接收到的数据需要除以100，以保证数据的正确性，并且需保留两位小数，保证一定的精确性。
2、上位机数据保存处理
母线电压、母线电流、超容电压、超容电流数据不需要保存，实时更新。
节电量保存一年的数量。



*/

/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */

static OS_TASK( es_Tsk__ );

static osBool es_Wakeup__(void);

static osBool es_Sleep__(void);

static void es_Init__( esBool a_bWake );

static void es_MasterSetEvent__(eEsEventTypeDef eEvent);

static eEsEventTypeDef es_MasterGetEvent__( void );

static void es_MasterPoll(void);

/***************************************************************************************************
 * GLOBAL VARIABLES
 */
osTask_t gs_tEsTsk;
osSigTimer_t gs_tEsRxTmoTmr;
osSigTimer_t gs_tEsMReadTmr;
osSigTimer_t gs_tEsUpLoadTmr;
osSigTimer_t gs_tEsRaddrTmr;

esU32	gs_u32Device = 0x10000001;
tEsMasterSysTypeDef gs_eEsMasterSys = { 0 };
tElevESavInfoTypeDef	gs_EsMasterInfo[4] = { 0 };
esU8 gs_u8Msg[6] = {ES_SENSOR_ID, 0x00, 0x00, 0x00, 0x00, 0x00};

/***************************************************************************************************
 * STATIC VARIABLES
 */

/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
 /*serial :  --- start --- byte --- EVEN --- stop ---*/
/***************************************************************************************************
 * @fn  
 *
 * @brief   Initial SPI Driver
 *
 * @author  
 *
 * @param   none
 *
 * @return  none
 */
void es_Init( void )
{

	GPIO_InitTypeDef GPIO_InitStructure;
	// send control  high available 
    GPIO_InitStructure.Pin = GPIO_PIN_7;	
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;	
    GPIO_InitStructure.Pull = GPIO_PULLUP;	
	__GPIOB_CLK_ENABLE();
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure); 
	// receive control  low available 
    GPIO_InitStructure.Pin = GPIO_PIN_1;	
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;	
    GPIO_InitStructure.Pull = GPIO_PULLUP;	
	__GPIOA_CLK_ENABLE();
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure); 
	
	uartCfg_Init(UART_CHN_2);
	uartCfg_EnableRx(UART_CHN_2,uartTrue);
	
    os_TaskCreate( &gs_tEsTsk, es_Tsk__, 5);
    
    os_TaskAddWakeupInit( &gs_tEsTsk, es_Wakeup__ );
    os_TaskAddSleepDeInit( &gs_tEsTsk, es_Sleep__ );
		
		os_SigSet(&gs_tEsTsk,OS_SIG_TASK_ES_GETADDR);//get address 
    os_TaskRun( &gs_tEsTsk );
	

} /* wm_Init() */



/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
/***************************************************************************************************
 * @fn      wm_Tsk__()
 *
 * @brief   water meter task
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
OS_TASK( es_Tsk__ )
{   	
    if( OS_SIG_STATE_SET(OS_SIG_SYS) )
    {      
				uartCfg_Poll(UART_CHN_2);
				es_MasterPoll();
        OS_SIG_CLEAR(OS_SIG_SYS);
    }

		//get address 
		if( OS_SIG_STATE_SET(OS_SIG_TASK_ES_GETADDR) )
		{	
			gs_eEsMasterSys.eEsMasterEvent = ES_SEND_EVENT;
			gs_eEsMasterSys.eEsMasterFunCode = ES_RADDR_FCODE;
			os_SigSetTimer(&gs_tEsRaddrTmr,&gs_tEsTsk,OS_SIG_TASK_ES_GETADDR,1000);  		
			
			OS_SIG_CLEAR(OS_SIG_TASK_ES_GETADDR);
		}
 #if 1   
    if(OS_SIG_STATE_SET(OS_SIG_TASK_ES_RXTMO))
    {
    	es_MasterSetEvent__(ES_TMO_EVENT);
    	//超时处理
			gs_eEsMasterSys.u8TryTimes++;
			if( gs_eEsMasterSys.u8TryTimes >= 5 )
			{
				es_MasterSetEvent__(ES_TMO_EVENT);
				os_SigStopTimer(&gs_tEsMReadTmr);
				os_SigStopTimer(&gs_tEsRaddrTmr);
			}
			else
			{
				es_MasterSetEvent__(ES_READY_EVENT);
			}				
			OS_SIG_CLEAR(OS_SIG_TASK_ES_RXTMO);
    }
		//get information
		if(OS_SIG_STATE_SET(OS_SIG_TASK_ES_MREAD))
		{
			gs_eEsMasterSys.eEsMasterEvent = ES_SEND_EVENT;
			gs_eEsMasterSys.eEsMasterFunCode = ES_READ_FCODE;
			os_SigSetTimer(&gs_tEsMReadTmr,&gs_tEsTsk,OS_SIG_TASK_ES_MREAD,5000);  
			OS_SIG_CLEAR(OS_SIG_TASK_ES_MREAD);
		}
#endif
    OS_SIG_REBACK()
}



/***************************************************************************************************
 * @fn      wm_Wakeup__()
 *
 * @brief   water meter wakeup
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
osBool es_Wakeup__(void)
{
    es_Init__(esTrue);
    
    return osTrue;
}

/***************************************************************************************************
 * @fn      ds_Sleep__()
 *
 * @brief   door sensor sleep
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
osBool es_Sleep__(void)
{
    es_Init__(esFalse);

    return osTrue;
}

/***************************************************************************************************
 * @fn      wm_InitSnr__()
 *
 * @brief    water meter init
 *
 * @author  chuanpengl
 *
 * @param   a_bWake  - dsTrue, init for wakeup
 *                   - dsFalse, init for sleep
 *
 * @return  none
 */
void es_Init__( esBool a_bWake )
{
    if(esTrue == a_bWake)
    {
		GPIO_InitTypeDef GPIO_InitStructure;
		/* send control  high available */	
		GPIO_InitStructure.Pin = GPIO_PIN_7;	
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;	
		GPIO_InitStructure.Pull = GPIO_PULLUP;	
		__GPIOB_CLK_ENABLE();
		HAL_GPIO_Init(GPIOB, &GPIO_InitStructure); 
		/* receive control  low available */
		GPIO_InitStructure.Pin = GPIO_PIN_1;	
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;	
		GPIO_InitStructure.Pull = GPIO_PULLUP;	
		__GPIOA_CLK_ENABLE();
		HAL_GPIO_Init(GPIOA, &GPIO_InitStructure); 
		gs_eEsMasterSys.u8TryTimes = 0;
		if(gs_eEsMasterSys.eEsMasterFunCode == ES_READ_FCODE)
			os_SigSet(&gs_tEsTsk,OS_SIG_TASK_ES_MREAD);
    }
    else
    {
				//
    }

}   /* es_InitSnr__() */


void es_MasterSetEvent__(eEsEventTypeDef eEvent)
{
	gs_eEsMasterSys.eEsMasterEvent = eEvent;
}

eEsEventTypeDef es_MasterGetEvent__( void )
{
	return gs_eEsMasterSys.eEsMasterEvent;
}


void es_MasterRequsetFrame__(void)
{
	eEsFunCodeTypeDef eCode;
	esU16	u16Crc;
	//esU8	u8Tab[20] = {0xAA, 0x01, 0x04, 0x01, 0x11, 0x11, 0x22, 0x33, 0x33, 0x44, 0x12, 0x34, 0x56, 0x78, 0x55, 0x66, 0x77, 0x88, 0xEF, 0x3B};
  //memcpy(gs_eEsMasterSys.u8MasterRxPool, u8Tab, 20); // get receive byte
  //gs_eEsMasterSys.u8MasterRxLen = 20;		
		
	eCode = gs_eEsMasterSys.eEsMasterFunCode;
	switch(eCode){
		case ES_WRITE_FCODE:
			gs_eEsMasterSys.u8MasterTxPool[0] = 0xAA;
			gs_eEsMasterSys.u8MasterTxPool[1] = (esU8)( (gs_u32Device & 0xff000000) >> 24 );
			gs_eEsMasterSys.u8MasterTxPool[2] = (esU8)( (gs_u32Device & 0x00ff0000) >> 16 );		
			gs_eEsMasterSys.u8MasterTxPool[3] = (esU8)( (gs_u32Device & 0x0000ff00) >> 8 );		
			gs_eEsMasterSys.u8MasterTxPool[4] = (esU8)( (gs_u32Device & 0x000000ff) >> 0 );
		
			gs_eEsMasterSys.u8MasterTxPool[5] = eCode;
			gs_eEsMasterSys.u8MasterTxPool[6] = 0x33;
					
			u16Crc = usMBCRC16(gs_eEsMasterSys.u8MasterTxPool, 7);
			gs_eEsMasterSys.u8MasterTxPool[7] = (esU8)((u16Crc & 0xff00) >> 8) ;				
			gs_eEsMasterSys.u8MasterTxPool[8] = (esU8)(u16Crc & 0x00ff);
			gs_eEsMasterSys.u8MasterTxLen = 9;
			break;
		case ES_READ_FCODE:
			gs_eEsMasterSys.u8MasterTxPool[0] = 0xAA;
			gs_eEsMasterSys.u8MasterTxPool[1] = (esU8)( (gs_u32Device & 0xff000000) >> 24 );
			gs_eEsMasterSys.u8MasterTxPool[2] = (esU8)( (gs_u32Device & 0x00ff0000) >> 16 );		
			gs_eEsMasterSys.u8MasterTxPool[3] = (esU8)( (gs_u32Device & 0x0000ff00) >> 8 );		
			gs_eEsMasterSys.u8MasterTxPool[4] = (esU8)( (gs_u32Device & 0x000000ff) >> 0 );
		
			gs_eEsMasterSys.u8MasterTxPool[5] = eCode;
					
			u16Crc = usMBCRC16(gs_eEsMasterSys.u8MasterTxPool, 6);
			gs_eEsMasterSys.u8MasterTxPool[6] = (esU8)((u16Crc & 0xff00) >> 8) ;				
			gs_eEsMasterSys.u8MasterTxPool[7] = (esU8)(u16Crc & 0x00ff);
			gs_eEsMasterSys.u8MasterTxLen = 8;		
			break;
		case ES_RADDR_FCODE:
			gs_eEsMasterSys.u8MasterTxPool[0] = 0xAA;
			gs_eEsMasterSys.u8MasterTxPool[1] = 0x10;
			gs_eEsMasterSys.u8MasterTxPool[2] = 0x00;		
			gs_eEsMasterSys.u8MasterTxPool[3] = 0x00;		
			gs_eEsMasterSys.u8MasterTxPool[4] = 0x00;
		
			gs_eEsMasterSys.u8MasterTxPool[5] = eCode;
					
			u16Crc = usMBCRC16(gs_eEsMasterSys.u8MasterTxPool, 6);
			gs_eEsMasterSys.u8MasterTxPool[6] = (esU8)((u16Crc & 0xff00) >> 8) ;				
			gs_eEsMasterSys.u8MasterTxPool[7] = (esU8)(u16Crc & 0x00ff);
			gs_eEsMasterSys.u8MasterTxLen = 8;			
			break;
		default:
			break;
	}
	
	USART_SXTX_CONTROL( esTrue, esFalse );// tx enable
	uartCfg_SendBytes( UART_CHN_2, gs_eEsMasterSys.u8MasterTxPool, gs_eEsMasterSys.u8MasterTxLen );
	USART_SXTX_CONTROL( esFalse,esTrue );// rx enable 		
}

//数据解析

// AA 10 00 00 00 05 XX XX
//01:寫   02:讀
//04  數據長度
// xx  xx  :crc16校驗
esU8 mb_MasterRecvFrame__(esU8 *pu8Data, esU8 u8Len)
{
	esU8 u8Status = 0;
	esU16 u16Crc = 0;
	esU8 u8Index = 0;
	esU32	u32AbsESave = 0;
	esU32 u32Device = 0;
	
	u16Crc = usMBCRC16(pu8Data, u8Len-2);
	
	if( u16Crc != (((esU16)pu8Data[u8Len-2]) << 8 | pu8Data[u8Len-1]) )
		return 1;
	
	u32Device = 	(( (esU32)pu8Data[1] ) << 24) | (( (esU32)pu8Data[2] ) << 16) | (( (esU32)pu8Data[3] ) << 8) | (( (esU32)pu8Data[4] ) << 0);
	u8Status = (pu8Data[0] == 0xAA)?1:0;
		
	if(u8Status == 0)
		return 2;
	
	switch(pu8Data[5]){
		case ES_WRITE_FCODE:
			if( (u32Device == gs_u32Device) && (pu8Data[6] == 0x01) ){
				//写入成功	
			}
			else{
				//写入失败
				return 3;
			}
			break;
		case ES_READ_FCODE:
			if(u32Device == gs_u32Device)
			{
				gs_eEsMasterSys.u8ElevNums = pu8Data[6];
				for(u8Index = 0; u8Index < pu8Data[6]; u8Index++)
				{
					gs_EsMasterInfo[u8Index].u8ElevAddr = pu8Data[u8Index*15 + 7];//电梯地址
					gs_EsMasterInfo[u8Index].u16MVol = ((esU16)pu8Data[8 + u8Index*15]) << 8 |  pu8Data[9 + u8Index*15];
					gs_EsMasterInfo[u8Index].u8MAmps = pu8Data[10 + u8Index*15];
					gs_EsMasterInfo[u8Index].u16SCVol = ((esU16)pu8Data[11 + u8Index*15]) << 8 |  pu8Data[12 + u8Index*15];
					gs_EsMasterInfo[u8Index].u8SCAmps = pu8Data[13 + u8Index*15];
					memcpy(gs_EsMasterInfo[u8Index].u8Esave,&pu8Data[14 + u8Index*15],4);
					memcpy(gs_EsMasterInfo[u8Index].u8AbsEsave,&pu8Data[18 + u8Index*15],4);
					gs_EsMasterInfo[u8Index].fMinEsave = ( (float)(gs_EsMasterInfo[u8Index].u8Esave[0]) )/100;
					gs_EsMasterInfo[u8Index].fHourEsave = ( (float)(gs_EsMasterInfo[u8Index].u8Esave[1]) )/100;
					gs_EsMasterInfo[u8Index].fDayEsave = ( ((esU16)( gs_EsMasterInfo[u8Index].u8AbsEsave[2] )) << 8  | gs_EsMasterInfo[u8Index].u8AbsEsave[3] )/100 ;
					
					u32AbsESave =  ( (esU32)gs_EsMasterInfo[u8Index].u8AbsEsave[0] ) << 24 | (esU32)( gs_EsMasterInfo[u8Index].u8AbsEsave[1] ) << 16 |
																								(esU32)( gs_EsMasterInfo[u8Index].u8AbsEsave[2] ) << 8  | gs_EsMasterInfo[u8Index].u8AbsEsave[3] ;
					gs_EsMasterInfo[u8Index].fAbsEsave =  ( (float)u32AbsESave )/100;
				}
			}
			else
			{
				return 5;
			}
			break;
		case ES_RADDR_FCODE:
			gs_u32Device = u32Device;
			break;
		default:
			break;
		}
		
	return 0;			
}

//数据 交由用户处理，上传等操作
eEsFunCodeTypeDef es_MasterProcessRecvedMsg__(void)
{
	eEsFunCodeTypeDef eFCode;
	eFCode = gs_eEsMasterSys.eEsMasterFunCode
	if(eFCode == ES_READ_FCODE)
	{
		switch(gs_eEsMasterSys.u8EParamIndex)
		{
			case 0://母线电压
				gs_u8Msg[1] = (esU8)( ( (esU16)(gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].u16MVol) & 0xFF00 ) >> 8 );
				gs_u8Msg[2] = (esU8)( ( (esU16)(gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].u16MVol) & 0x00FF ) >> 0 );
				gs_u8Msg[3] = 0;							
				break;
			case 1://母线电流
				gs_u8Msg[1] = gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].u8MAmps;
				gs_u8Msg[2] = 0;
				gs_u8Msg[3] = 0;								
				break;
			case 2://超容电压
				gs_u8Msg[1] = (esU8)( ( (esU16)(gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].u16SCVol) & 0xFF00 ) >> 8 );
				gs_u8Msg[2] = (esU8)( ( (esU16)(gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].u16SCVol) & 0x00FF ) >> 0 );
				gs_u8Msg[3] = 0;								
				break;
			case 3://超容电流
				gs_u8Msg[1] = gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].u8SCAmps;
				gs_u8Msg[2] = 0;
				gs_u8Msg[3] = 0;
				break;
			case 4://时间段节电量
				gs_u8Msg[1] = (esU8)(gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].fMinEsave*100);
				gs_u8Msg[2] = (esU8)(gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].fHourEsave*100);
				gs_u8Msg[3] = (esU8)(gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].fDayEsave*100);								
				break;
			case 5://绝对节电量
				gs_u8Msg[1] = (esU8)( ( (esU32)(gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].fAbsEsave) & 0xFF00 ) >> 8 );
				gs_u8Msg[2] = (esU8)( ( (esU32)(gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].fAbsEsave) & 0x00FF ) >> 0 );
				gs_u8Msg[3] = (esU8)((gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].fAbsEsave - (esU32)(gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].fAbsEsave) )*100);
				break;						
		}
		gs_u8Msg[4] = gs_eEsMasterSys.u8EParamIndex + 1;
		gs_u8Msg[5] = gs_EsMasterInfo[gs_eEsMasterSys.u8ElevIndex].u8ElevAddr;
		net_SendData(gs_u8Msg, 6, netFalse);
		uartCfg_SendBytes( UART_CHN_2, gs_u8Msg, 6 );
		gs_eEsMasterSys.u8EParamIndex++;
		
		if(gs_eEsMasterSys.u8EParamIndex >= 6)
		{
			gs_eEsMasterSys.u8EParamIndex = 0;
			gs_eEsMasterSys.u8ElevIndex ++;
			if(gs_eEsMasterSys.u8ElevIndex >= gs_eEsMasterSys.u8ElevNums)
			gs_eEsMasterSys.u8ElevIndex = 0;
		}
	}
	
	return eFCode;
	
}



void es_MasterPoll(void)
{
	eEsEventTypeDef eEvent;
	
	eEvent = es_MasterGetEvent__();
	switch(eEvent){
		case ES_READY_EVENT:
			break;
		case ES_SEND_EVENT:
			es_MasterRequsetFrame__();
			es_MasterSetEvent__(ES_WAIT_EVENT);
			os_SigSetTimer(&gs_tEsRxTmoTmr, &gs_tEsTsk, OS_SIG_TASK_ES_RXTMO,100);
			break;
		case ES_WAIT_EVENT:
			break;
		case ES_RECV_EVENT:
			if( 0 == mb_MasterRecvFrame__(gs_eEsMasterSys.u8MasterRxPool,gs_eEsMasterSys.u8MasterRxLen) )
				es_MasterSetEvent__(ES_USR_EVENT);
			else
				es_MasterSetEvent__(ES_SLEEP_EVENT);	
			break;
		case ES_TMO_EVENT:
			es_MasterSetEvent__(ES_SLEEP_EVENT);
			break;
		case ES_USR_EVENT:
			if(ES_READ_FCODE == es_MasterProcessRecvedMsg__())
				es_MasterSetEvent__(ES_SLEEP_EVENT) ;
			else
			{
				es_MasterSetEvent__(ES_READY_EVENT) ;
				os_SigSet(&gs_tEsTsk,OS_SIG_TASK_ES_MREAD);
			}
			break;
		case ES_SLEEP_EVENT:
			memset(gs_eEsMasterSys.u8MasterRxPool,0,gs_eEsMasterSys.u8MasterRxLen);
			memset(gs_eEsMasterSys.u8MasterTxPool,0,gs_eEsMasterSys.u8MasterTxLen);
			os_TaskSleep( &gs_tEsTsk, 1*60*1000 ); //1min 
			es_MasterSetEvent__(ES_READY_EVENT);
			break;
		default:
			break;
	}
}



/***************************************************************************************************
 * @fn      uartCfg_Usart2ReceiveBytes()
 *
 * @brief   receive data
 *
 * @author  chuanpengl
 *
 * @param   a_pu8Data  - data for sending
 *          a_u16Length  - data length
 *
 * @return  none
 */
void uartCfg_Usart2ReceiveBytes( uartU8  *a_pu8Data, uartU16 a_u16Length )
{		
		os_SigStopTimer(&gs_tEsMReadTmr);
		os_SigStopTimer(&gs_tEsRxTmoTmr );					// stop timer when received data;
		os_SigStopTimer(&gs_tEsRaddrTmr);
    memcpy(gs_eEsMasterSys.u8MasterRxPool, a_pu8Data, a_u16Length); // get receive byte
    gs_eEsMasterSys.u8MasterRxLen = a_u16Length;
    es_MasterSetEvent__(ES_RECV_EVENT);		
}





#endif 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
