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
#include "app_watermeter.h"

#if APP_USE_WATER_METER

#include <stdio.h>
#include <string.h>
#include "os.h"
#include "os_signal.h"
#include "cfifo.h"
#include "stm32l0xx_hal.h"

#include "uart/uart_config.h"

#include "app_net.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */

#define OS_SIG_TASK_WM_SAMPLE               (((osUint32)1) << 1)
#define OS_SIG_TASK_WM_RXTMO              	(((osUint32)1) << 2)
#define OS_SIG_TASK_WM_EXE              	(((osUint32)1) << 3) 
#define OS_SIG_TASK_WM_UPLOAD				(((osUint32)1) << 4)

 
#define WM_TASK_INTERVAL			5 
#define WM_TASK_SLEEP               30  //minute


#define  	CC_RESERVED							( 0 )
#define 	CC_READ_DATA						( 1 ) 
#define 	CC_WRITE_DATA						( 4 ) 	
#define 	CC_READ_KEYVERESION					( 9 ) 	
#define 	CC_READ_ADDRESS						( 3 ) 
#define 	CC_WRITE_ADDRESS					( 37 ) 
#define 	CC_WRITE_SYCCOUNT					( 38 ) 
#define 	CC_USERDEFINE						( 64 ) 	

#define RMD_DATA_ID0		(	0x90 )
#define RMD_DATA_ID1		(	0x1F )

#define RMA_ADDR_DI0 		( 0x81 )
#define RMA_ADDR_DI1 		( 0x0A )

#define WR_ACCOUNT_DAY_DI0		( 0xA0)
#define WR_ACCOUNT_DAY_DI1		( 0x11)

#define RD_ACCOUNT_DAY_DI0		( 0x81 )
#define RD_ACCOUNT_DAY_DI1		( 0x03 )

#define RMD_HISTORY1_DI0		( 0xD1 )
#define RMD_HISTORY1_DI1		( 0x20 )

#define MASTER_FRAME_HEAD		( 0x68 )
#define MASTER_FRAME_TAIL		( 0x16 )

#define MASTER_CONTROL_FRAME	( 0 )
#define SLAVE_ACK_FRAME			( 1 )

#define COMMUNICATE_NORMAL		( 0 )
#define COMMUNICATE_FAULT		( 1 )

/*
s:send
r:recv
*/
#define USART_SXTX_CONTROL( s, r )	\
		(s == dsTrue)? HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET);\
		(r == dsTrue)? HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_RESET):HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_SET);\


#define  BCD_TO_DEC( BCD ) \
	((BCD >> 4) & 0x0F) * 10 + (BCD & 0x0F)
		

#define POWER_ENABLE	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_SET)
#define POWER_DISABLE	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_RESET)

/***************************************************************************************************
 * TYPEDEFS
 */
/*  Meter Type */
typedef enum
{
	/* water meter  */
	WATER_METER_COLD = 0x10,
	WATER_METER_LIFEHOT = 0x11,
	WATER_METER_DRINK	=	0x12,
	WATER_METER_MIDWATER = 0x13,
	/* Heat meter  */
	HEAT_METER_COLD = 0x20,
	WATER_METER_HOT = 0x21,	
	/* Gas Meter */
	GAS_METER	=	0x30,
	/* Other Meter, Electric */	
	OTHER_METER = 0x40,	
}eMeterType;

/* Control Code  */
typedef union
{
	dsU8  ControlCode;
	struct{
		dsU8 u6Control:6;
		dsU8 u1ComState:1;
		dsU8 u1TransmitDir:1;		
	}sControlCodeBitAera;
}uControlCode;


/* Frame Struct  */
#define FRAME_TYPE( len )			\
	struct {										\
		dsU8 u8FrmHead;	/* 0x68 */		\
		eMeterType eMtype;									\
		dsU8	u8AddrAera[7];	/*  A0 ... A6  Total 7 bytes */		\
		uControlCode uCtrlCode;					\
		dsU8	u8DataLenth;		/*  <=0x64 when read ,<=0x32H when write */			\
		dsU8   u8Data[len]; 	/*  data lenth change with  Control Code */		\
		dsU8   u8CsCode;			/*  From Frame Head to the byte before CsCode , binary algorithm add */		\
		dsU8   u8FrameTail;	/* 0x16 */		\
	}


typedef 		FRAME_TYPE(3)			FrameReadDataTypeDef;
typedef 		FRAME_TYPE(3)			FrameReadAddrsesTypeDef;
typedef 		FRAME_TYPE(3)			FrameReadAccountDayTypeDef;
typedef 		FRAME_TYPE(4)			FrameWriteAccountDayTypeDef;

typedef struct 
{
		dsU8 u8Year[2];
		dsU8 u8Month;
		dsU8 u8Day;
		dsU8 u8Hour;
		dsU8 u8Min;
		dsU8 u8Sec;
}tTimeTypeDef;


/* State   */
typedef union
{
	dsU16	u16Status;
	struct{
		dsU8 u8ValvesState:2;	// 00:open, 01:close, 11:fault
		dsU8 u8BatStatus:1;	// 0:normal ,	1:low
		dsU8 u8Reserved:5;
	}sStByteOneBitAera;
}uSTStateTypeDef;

typedef struct
{
	dsU8 u8CurrentAccmulateFlow[4];	// current accmulate flow 4 byte  
	dsU8 u8DeadLineAccmulateFlow[4];		// deadline accmulate flow 4 byte  
	tTimeTypeDef tRealTime;	// real time 7 byte 
	uSTStateTypeDef	uStatus;	// status  2 byte 
}tWmDataTypeDef;


typedef enum
{
	INIT_STATE = 0,
	TX_STATE,
	RX_STATE,
	RX_TMO_STATE,
	EXE_STATE,
	UPLOAD_STATE,
	SLEEP_STATE,
}eWmStateTypeDef;


enum 
{
	CURRENT_ACCMULATE_FLOW = 1,
	DEADLINE_ACCMULATE_FLOW = 2,
	WM_STATUS = 3,
	REAL_TIME = 4,
};

/***************************************************************************************************
 * CONSTANTS
 */
const dsU8	cu8AddressAera[7] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
dsU8 cu8PrecursorCode[3] = { 0xFE, 0xFE, 0xFE };
static dsU8 au8Msg[6] = {WM_SENSOR_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
static dsU8 au8MsgCount = 0;
/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */

static OS_TASK( wm_Tsk__ );

static osBool wm_Wakeup__(void);

static osBool wm_Sleep__(void);

static void wm_Init__( dsBool a_bWake );

static dsU8 wm_CsCheck__( dsU8 *pu8Data, dsU8 u8Len );

static void wm_MasterRequestReadData__(void);

static void wm_MasterRequestReadAddress__(void);

static void wm_WmSampleDataParse__(uartU8  *a_pu8Data, uartU16 a_u16Length);

static void wm_StateMachineSwitch__(void);

static void wm_MakeUpLoadFramePackage__(dsU8 *pu8Msg,dsU8 u8MsgType) ;	/* j */

static void wm_MasterWriteAccountDay__(void);	/*  read account day */

static void wm_MasterReadAccountDay__(void); /*  write account day */


/***************************************************************************************************
 * GLOBAL VARIABLES
 */
osTask_t gs_tWmTsk;
osSigTimer_t gs_tWmRxTmoTmr;
osSigTimer_t gs_tWmUpLoadTmr;
dsU8 gs_u8SerialNum = 0;
dsU8 gs_u8RxBuf[64] = { 0 } ;
tWmDataTypeDef	gs_tWmInfo = { 0 };
eWmStateTypeDef gs_eWmState = INIT_STATE;
dsU8	u8TxTimes = 0;
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
 * @fn      Hal_SpiInit()
 *
 * @brief   Initial SPI Driver
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void wm_Init( void )
{

	GPIO_InitTypeDef GPIO_InitStructure;
	
	// send control  high available 
	__GPIOB_CLK_ENABLE();
	GPIO_InitStructure.Pin = GPIO_PIN_7;	
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;	
	GPIO_InitStructure.Pull = GPIO_PULLUP;	
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure); 
	
	// receive control  low available
	__GPIOA_CLK_ENABLE();	
	GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_0;	
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;	
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed	= GPIO_SPEED_FAST;
	
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure); 
	

	
	
	uartCfg_Init(UART_CHN_2);
	uartCfg_EnableRx(UART_CHN_2,uartTrue);
	net_SendData(au8Msg, 6, netFalse);
    os_TaskCreate( &gs_tWmTsk, wm_Tsk__, WM_TASK_INTERVAL);
    
    os_TaskAddWakeupInit( &gs_tWmTsk, wm_Wakeup__ );
    os_TaskAddSleepDeInit( &gs_tWmTsk, wm_Sleep__ );
		
    os_TaskRun( &gs_tWmTsk );


} /* wm_Init() */

void wm_StateMachineSwitch__(void)
{
	switch(gs_eWmState)
	{
		case INIT_STATE:
			gs_eWmState = TX_STATE;
			break;
		case TX_STATE:
			USART_SXTX_CONTROL(dsTrue, dsFalse);
			POWER_ENABLE;			
			wm_MasterRequestReadData__();	
			//wm_MasterRequestReadAddress__();
			//wm_MasterWriteAccountDay__();
			//wm_MasterReadAccountDay__();
			gs_eWmState = RX_STATE;
			os_SigSetTimer( &gs_tWmRxTmoTmr, &gs_tWmTsk, OS_SIG_TASK_WM_RXTMO, 600);					//600ms ÏìÓ¦Ê±¼ä
			USART_SXTX_CONTROL(dsFalse, dsTrue);	
			break;
		case RX_STATE:

			break;
		case RX_TMO_STATE:
			 if(u8TxTimes >= 5)
			 {
				gs_eWmState = SLEEP_STATE;
				u8TxTimes = 0;
			 }
			 else
			 {
				gs_eWmState = TX_STATE;
				u8TxTimes ++;
			 }					
			break;
		case EXE_STATE:
			wm_WmSampleDataParse__(gs_u8RxBuf, 64); // parse data 
			os_SigSet(  &gs_tWmTsk, OS_SIG_TASK_WM_EXE);						
			break;
		case UPLOAD_STATE:
			break;
		case SLEEP_STATE:
			POWER_DISABLE;
			memset(gs_u8RxBuf ,0x00 ,sizeof(gs_u8RxBuf));	// clear receive buffer
			os_TaskSleep( &gs_tWmTsk, 30*60*1000 );
			break;
		default:
			break;
	}				
}

/*
		CURRENT_ACCMULATE_FLOW = 1,
		DEADLINE_ACCMULATE_FLOW = 2,
		WM_STATUS = 3,
	  REAL_TIME = 4,
*/
void wm_MakeUpLoadFramePackage__(dsU8 *pu8Msg,dsU8 u8MsgType) 
{
	dsU32 u32FlowSum = 0;
	
	switch (u8MsgType)
	{
		case CURRENT_ACCMULATE_FLOW:	// current Flow sum
			u32FlowSum = ( (dsU32) gs_tWmInfo.u8CurrentAccmulateFlow[ 3 ] )*10000 + ( (dsU32)gs_tWmInfo.u8CurrentAccmulateFlow[ 2 ] )*100 + gs_tWmInfo.u8CurrentAccmulateFlow[ 1 ];
			pu8Msg[1] =  (dsU8)( (u32FlowSum & 0xFF00) >> 8 );
			pu8Msg[2] =  (dsU8)( (u32FlowSum & 0x00FF) >> 0 );
			pu8Msg[3] =  gs_tWmInfo.u8CurrentAccmulateFlow[ 0 ];
			//pu8Msg[4] = gs_tWmInfo.u8CurrentAccmulateFlow[ 0 ] ;
			//pu8Msg[1]  |= (CURRENT_ACCMULATE_FLOW << 4 );	//msgtype 
		break;
		case DEADLINE_ACCMULATE_FLOW: // deadline flow sum
			pu8Msg[1] = gs_tWmInfo.u8DeadLineAccmulateFlow[ 3 ] ;
			pu8Msg[2] = gs_tWmInfo.u8DeadLineAccmulateFlow[ 2 ] ;
			pu8Msg[3] = gs_tWmInfo.u8DeadLineAccmulateFlow[ 1 ] ;
			//pu8Msg[4] = gs_tWmInfo.u8DeadLineAccmulateFlow[ 0 ] ;
			//pu8Msg[1]  |= (DEADLINE_ACCMULATE_FLOW << 4 );	//msgtype 			
			break;
		case WM_STATUS: // status 
			pu8Msg[1] = gs_tWmInfo.uStatus.sStByteOneBitAera.u8ValvesState;	// valves state
			pu8Msg[2] = gs_tWmInfo.uStatus.sStByteOneBitAera.u8BatStatus;	// battery  status 		
			//pu8Msg[1]  |= (WM_STATUS << 4 );	// msgtype 
			break;
		case REAL_TIME: // real time 
			break;
		default:
			break;
	}
}

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
OS_TASK( wm_Tsk__ )
{    	
    if( OS_SIG_STATE_SET(OS_SIG_SYS) )
    {      
		uartCfg_Poll(UART_CHN_2);
		wm_StateMachineSwitch__();
        OS_SIG_CLEAR(OS_SIG_SYS);
    }

    if( OS_SIG_STATE_SET(OS_SIG_TASK_WM_RXTMO) )
	{
		gs_eWmState = RX_TMO_STATE;
        OS_SIG_CLEAR(OS_SIG_TASK_WM_RXTMO);
	}			
		
    if( OS_SIG_STATE_SET(OS_SIG_TASK_WM_EXE) )
	{
		gs_eWmState = UPLOAD_STATE;
		os_SigSetTimer( &gs_tWmUpLoadTmr, &gs_tWmTsk, OS_SIG_TASK_WM_UPLOAD, 500);	
		OS_SIG_CLEAR(OS_SIG_TASK_WM_EXE);
    }		

	if( OS_SIG_STATE_SET(OS_SIG_TASK_WM_UPLOAD) )
	{
		au8MsgCount	 = 1;      
		wm_MakeUpLoadFramePackage__(au8Msg, au8MsgCount);			
		net_SendData(au8Msg, 6, netFalse);
		os_SigSetTimer( &gs_tWmUpLoadTmr, &gs_tWmTsk, OS_SIG_TASK_WM_UPLOAD, 500);	
		if(au8MsgCount >= 1 )
		{
			gs_eWmState = SLEEP_STATE;	
			au8MsgCount = 0;
			os_SigStopTimer( &gs_tWmUpLoadTmr);								
		}
		
		OS_SIG_CLEAR(OS_SIG_TASK_WM_UPLOAD);
	}	
    
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
osBool wm_Wakeup__(void)
{
    wm_Init__(dsTrue);
    
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
osBool wm_Sleep__(void)
{
    wm_Init__(dsFalse);

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
void wm_Init__( dsBool a_bWake )
{
	GPIO_InitTypeDef GPIO_InitStructure;
    if(dsTrue == a_bWake)
    {
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
		
		gs_eWmState = TX_STATE;
    }
    else
    {
				//
    }
		__GPIOA_CLK_ENABLE();
		GPIO_InitStructure.Pin =  GPIO_PIN_0;	
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;	
		GPIO_InitStructure.Pull = GPIO_PULLUP;
		GPIO_InitStructure.Speed	= GPIO_SPEED_FAST;		
		
		HAL_GPIO_Init(GPIOA, &GPIO_InitStructure); 		
	
}   /* ds_InitSnr__() */


/***************************************************************************************************
 * @fn      wm_CsCheck__()
 *
 * @brief   CS Check
 *
 * @author  chuanpengl
 *
 * @param   a_bWake  - dsTrue, init for wakeup
 *                   - dsFalse, init for sleep
 *
 * @return  none
 */
dsU8 wm_CsCheck__( dsU8 *pu8Data, dsU8 u8Len )
{
	dsU8 u8Index;
	dsU32 u32Sum = 0;
	for(u8Index = 0; u8Index < u8Len; u8Index ++)
	{
			u32Sum += pu8Data[u8Index];
	}
	
	return ( (dsU8)u32Sum);

}   /* ds_InitSnr__() */

 
/***************************************************************************************************
 * @fn      wm_MasterRequestReadData__()
 *
 * @brief   request read data 
 *
 * @author  chuanpengl
 *
 * @param   
 *                   
 *
 * @return  none
 */
void wm_MasterRequestReadData__(void)
{
	FrameReadDataTypeDef	 tRequestFrame;

	tRequestFrame.u8FrmHead		=		MASTER_FRAME_HEAD;
	tRequestFrame.eMtype = WATER_METER_COLD;
	memcpy(tRequestFrame.u8AddrAera, cu8AddressAera, sizeof(cu8AddressAera));
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u1TransmitDir = MASTER_CONTROL_FRAME;
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u1ComState = COMMUNICATE_NORMAL;
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u6Control =  CC_READ_DATA;
	tRequestFrame.u8DataLenth = 0x03;
	tRequestFrame.u8Data[0]  = RMD_DATA_ID1;
	tRequestFrame.u8Data[1]  = RMD_DATA_ID0;	  
	//tRequestFrame.u8Data[0]  = RMD_HISTORY1_DI1;
	//tRequestFrame.u8Data[1]  = RMD_HISTORY1_DI0;
	
	tRequestFrame.u8Data[2]  = (gs_u8SerialNum++)  % 255; //00
	tRequestFrame.u8CsCode = wm_CsCheck__(&(tRequestFrame.u8FrmHead) , sizeof(tRequestFrame) - 2);
	tRequestFrame.u8FrameTail = MASTER_FRAME_TAIL;
	
	uartCfg_SendBytes(UART_CHN_2, cu8PrecursorCode, 3);
	uartCfg_SendBytes( UART_CHN_2, (&tRequestFrame.u8FrmHead), sizeof( tRequestFrame ) );
}
	

void wm_MasterRequestReadAddress__(void)
{
	FrameReadAddrsesTypeDef		tRequestFrame;

//		USART_SXTX_CONTROL(dsTrue, dsFalse);
	tRequestFrame.u8FrmHead		=		MASTER_FRAME_HEAD;
	tRequestFrame.eMtype = WATER_METER_COLD;
	memcpy(tRequestFrame.u8AddrAera, cu8AddressAera, sizeof(cu8AddressAera));
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u1TransmitDir = MASTER_CONTROL_FRAME;
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u1ComState = COMMUNICATE_NORMAL;
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u6Control =  CC_READ_ADDRESS;
	tRequestFrame.u8DataLenth = 0x03;
	tRequestFrame.u8Data[0]  = RMA_ADDR_DI1;
	tRequestFrame.u8Data[1]  = RMA_ADDR_DI0;
	tRequestFrame.u8Data[2]  = (gs_u8SerialNum++)  % 255; //0x00;//
	tRequestFrame.u8CsCode = wm_CsCheck__(&(tRequestFrame.u8FrmHead) , sizeof(tRequestFrame) - 2);
	tRequestFrame.u8FrameTail = MASTER_FRAME_TAIL;
	
	uartCfg_SendBytes(UART_CHN_2, cu8PrecursorCode, 3);
	uartCfg_SendBytes( UART_CHN_2, (&tRequestFrame.u8FrmHead), sizeof( tRequestFrame ) );	
//	 USART_SXTX_CONTROL(dsFalse, dsTrue);	
	
}


void wm_MasterWriteAccountDay__(void)
{
	FrameWriteAccountDayTypeDef	tRequestFrame;
	
	tRequestFrame.u8FrmHead		=		MASTER_FRAME_HEAD;
	tRequestFrame.eMtype = WATER_METER_COLD;
	memcpy(tRequestFrame.u8AddrAera, cu8AddressAera, sizeof(cu8AddressAera));
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u1TransmitDir = MASTER_CONTROL_FRAME;
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u1ComState = COMMUNICATE_NORMAL;
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u6Control =  CC_WRITE_DATA;
	tRequestFrame.u8DataLenth = 0x04;
	tRequestFrame.u8Data[0]  = WR_ACCOUNT_DAY_DI1;
	tRequestFrame.u8Data[1]  = WR_ACCOUNT_DAY_DI0;
	tRequestFrame.u8Data[2]  = (gs_u8SerialNum++)  % 255; //0x00;//
	tRequestFrame.u8Data[3] = 0x1D; // 29
	tRequestFrame.u8CsCode = wm_CsCheck__(&(tRequestFrame.u8FrmHead) , sizeof(tRequestFrame) - 2);
	tRequestFrame.u8FrameTail = MASTER_FRAME_TAIL;
	
	uartCfg_SendBytes(UART_CHN_2, cu8PrecursorCode, 3);
	uartCfg_SendBytes( UART_CHN_2, (&tRequestFrame.u8FrmHead), sizeof( tRequestFrame ) );	
	
}


void wm_MasterReadAccountDay__(void)
{
	FrameReadAccountDayTypeDef	tRequestFrame;
	
	tRequestFrame.u8FrmHead		=		MASTER_FRAME_HEAD;
	tRequestFrame.eMtype = WATER_METER_COLD;
	memcpy(tRequestFrame.u8AddrAera, cu8AddressAera, sizeof(cu8AddressAera));
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u1TransmitDir = MASTER_CONTROL_FRAME;
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u1ComState = COMMUNICATE_NORMAL;
	tRequestFrame.uCtrlCode.sControlCodeBitAera.u6Control =  CC_READ_DATA;
	tRequestFrame.u8DataLenth = 0x04;
	tRequestFrame.u8Data[0]  = RD_ACCOUNT_DAY_DI1;
	tRequestFrame.u8Data[1]  = RD_ACCOUNT_DAY_DI0;
	tRequestFrame.u8Data[2]  = (gs_u8SerialNum++)  % 255; //0x00;//
	tRequestFrame.u8CsCode = wm_CsCheck__(&(tRequestFrame.u8FrmHead) , sizeof(tRequestFrame) - 2);
	tRequestFrame.u8FrameTail = MASTER_FRAME_TAIL;
	
	uartCfg_SendBytes(UART_CHN_2, cu8PrecursorCode, 3);
	uartCfg_SendBytes( UART_CHN_2, (&tRequestFrame.u8FrmHead), sizeof( tRequestFrame ) );	
	
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
	os_SigStopTimer( &gs_tWmRxTmoTmr);					// stop timer when received data;
	memcpy(gs_u8RxBuf, a_pu8Data, a_u16Length); // get receive byte
	gs_eWmState = EXE_STATE;
}

/*
FE FE FE 
68 // head 
10 // type
59 28 41 50 01 00 00 // address 
81 // read recv 
16  //data len 
1F 90 // DI0, DI1 
00 // serial number 
00 00 00 00 2C // current accmulate flow  2C----- m*m*m  unit 
00 00 00 00 2C // deadline accmulate flow 
00 00 00 00 00 00 00 // real time 
00 00 // status 
29 // cs 
16 // tail 

*/
void wm_WmSampleDataParse__(uartU8  *a_pu8Data, uartU16 a_u16Length)
{
	
	//dsU8 	u8Status = 0;
	dsU8	u8Index = 0;
	dsU8	u8Len = 0;
	dsU32 u32Tmp[4] = { 0 };
	
	for(u8Index = 0;  u8Index <  a_u16Length ; u8Index++)
	{
		if( (a_pu8Data[u8Index] == MASTER_FRAME_HEAD) && (a_pu8Data[u8Index + 1] == WATER_METER_COLD) && (a_pu8Data[u8Index + 9] == 0x81) )
			break ;
		else if(u8Index == a_u16Length - 1)
			return ;
	}
	
	u8Len	=	a_pu8Data[u8Index + 10]; 
	if( (a_pu8Data[u8Index + 10 + u8Len +2 ] == MASTER_FRAME_TAIL) &&  wm_CsCheck__( &a_pu8Data[u8Index] ,u8Len + 11 ) == a_pu8Data[u8Index + 10 + u8Len +1 ] )
	{
		//  a_pu8Data[u8Index + 13 + 4],   a_pu8Data[u8Index + 13 + 3]  ,a_pu8Data[u8Index + 13 + 2] , a_pu8Data[u8Index + 13 + 1],
		
		gs_tWmInfo.u8CurrentAccmulateFlow[ 0 ] = BCD_TO_DEC( a_pu8Data[u8Index + 13 + 4] ) ;	//l
		gs_tWmInfo.u8CurrentAccmulateFlow[ 1 ] = BCD_TO_DEC( a_pu8Data[u8Index + 13 + 3] ) ;
		gs_tWmInfo.u8CurrentAccmulateFlow[ 2 ] = BCD_TO_DEC( a_pu8Data[u8Index + 13 + 2] ) ;
		gs_tWmInfo.u8CurrentAccmulateFlow[ 3 ] = BCD_TO_DEC( a_pu8Data[u8Index + 13 + 1] );	//h
	
		gs_tWmInfo.u8DeadLineAccmulateFlow[ 0 ] = BCD_TO_DEC( a_pu8Data[u8Index + 18 + 4] ) ; //l
		gs_tWmInfo.u8DeadLineAccmulateFlow[ 1 ] = BCD_TO_DEC( a_pu8Data[u8Index + 18 + 3] ) ;
		gs_tWmInfo.u8DeadLineAccmulateFlow[ 2 ] = BCD_TO_DEC( a_pu8Data[u8Index + 18 + 2] ) ;
		gs_tWmInfo.u8DeadLineAccmulateFlow[ 3 ] = BCD_TO_DEC( a_pu8Data[u8Index + 18 + 1] ); //h
		

		gs_tWmInfo.tRealTime.u8Year[ 1 ] = BCD_TO_DEC( a_pu8Data[u8Index + 23 + 2] ) ;
		gs_tWmInfo.tRealTime.u8Year[ 0 ] = BCD_TO_DEC( a_pu8Data[u8Index + 23 + 1] ) ;	
		//gs_tWmInfo.tRealTime.u8Year = u32Tmp[ 1 ] *100 + u32Tmp[ 0 ];
		
		gs_tWmInfo.tRealTime.u8Month = BCD_TO_DEC( a_pu8Data[u8Index + 23 + 3] ) ;	
	
		gs_tWmInfo.tRealTime.u8Day = BCD_TO_DEC( a_pu8Data[u8Index + 23 + 4] ) ;	
		
		gs_tWmInfo.tRealTime.u8Hour = BCD_TO_DEC( a_pu8Data[u8Index + 23 + 5] ) ;	
	
		gs_tWmInfo.tRealTime.u8Min = BCD_TO_DEC( a_pu8Data[u8Index + 23 + 6] ) ;		
		
		gs_tWmInfo.tRealTime.u8Sec = BCD_TO_DEC( a_pu8Data[u8Index + 23 + 7] ) ;	
		
		u32Tmp[0] = a_pu8Data[u8Index + 23 + 9] ;
		u32Tmp[0]  = ( u32Tmp[0]  << 8 )  |  a_pu8Data[u8Index + 23 + 8];
		gs_tWmInfo.uStatus.u16Status = ( dsU16) u32Tmp[0] ;
		
		uartCfg_SendBytes(UART_CHN_2, cu8PrecursorCode, 3);
		uartCfg_SendBytes(UART_CHN_2, &a_pu8Data[u8Index + 13], 1);
		
	}
	
}



#endif 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
