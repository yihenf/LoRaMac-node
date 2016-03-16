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
#include "app_elevator.h"



#if APP_USE_ELEVATOR_KONE

#include <stdio.h>
#include <string.h>
#include "os.h"
#include "os_signal.h"
#include "cfifo.h"
#include "crc/crc.h"
#include "stm32l0xx_hal.h"

#include "app_net.h"
#include "app_modbus.h"
/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */

 
#define ELEV_TASK_SLEEP           30  //minute

#define MB_SLAVE_ADDRESS	( 0x01 )




#define OS_SIG_TASK_ELEV_RXDATATMO              		(((osUint32)1) << 1)	//get data info tmo flag
#define OS_SIG_TASK_ELEV_RXDATATMR              		(((osUint32)1) << 2)	//get data info flag 





		
#define	FLOOR_INVALID_STATE		( 0x00 )
#define	FLOOR_UPPING_STATE		( 0x01 )
#define	FLOOR_DOWNING_STATE		( 0x02 ) 


#define	FLOOR_LOCKING_STATE		( 0x01 )	
#define	FLOOR_INSPECTING_STATE	( 0x03 )
#define	FLOOR_FIRESAFE_STATE	( 0x02 )
#define	FLOOR_DRIVER_STATE		( 0x04 )
#define	FLOOR_OVERLOAD_STATE	( 0x05 )
#define	FLOOR_VIP_STATE			( 0x06 )
#define	FLOOR_FAULT_1			( 0x07 )
#define	FLOOR_FAULT_2			( 0x08 )


/***************************************************************************************************
 * TYPEDEFS
 */

typedef struct
{
	dsU8	u8Floor;	// current floor
	dsU8	u8RunState; // elevator running state
	dsU8	u8LockState; // elevator in  locking state
	dsU8	u8InspectState;	// elevator in inspecting state
	dsU8	u8FireSafetyState; // elevator in firesafety state
	dsU8	u8DriverState; // elevator in driver  state
	dsU8	u8OverLoadState; // elevator in overloading state
	dsU8	u8VipState;	// elevator in vip state
	
}tElevInfoTypeDef;



/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
/*****************************************************************************************************
*****************************************************************************************************/
//elevator main task process function
static OS_TASK( elev_Tsk__ );
//elevator main task wakeup function
static osBool elev_Wakeup__(void);
//elevator main task sleep function
static osBool elev_Sleep__(void);
//elevator wake-sleep init function
static void elev_Init__( dsBool a_bWake );

/*****************************************************************************************************
*****************************************************************************************************/

static void elev_UpLoadEmInfo__(void);

static void elev_GetReceivedData__(dsU8 *u8Data, dsU8 u8Len);


/***************************************************************************************************
 * GLOBAL VARIABLES
 */
osTask_t gs_tElevTsk;

osSigTimer_t gs_tElevRxDataIfoTmoTmr;
osSigTimer_t gs_tElevRxDataIfoTmr;

dsU8 gs_u8RxBuf[32] = { 0 } ;
dsU8 gs_u8Msg[6] = {ELEV_SENSOR_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
dsU8 gs_u8Lmsg[6] = {ELEV_SENSOR_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
dsU8	gs_u8TmoTimes = 0;
tElevInfoTypeDef	gs_tElevInfo = { 0 };


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
void elev_Init( void )
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

	net_SendData(gs_u8Msg, 6, netFalse);	
	os_TaskCreate( &gs_tElevTsk, elev_Tsk__, 2000);

	os_TaskAddWakeupInit( &gs_tElevTsk, elev_Wakeup__ );
	os_TaskAddSleepDeInit( &gs_tElevTsk, elev_Sleep__ );

	os_TaskRun( &gs_tElevTsk );


} /* wm_Init() */


void mb_MasterSetRcvTmrFlg(dsU8	*pu8Buf, dsU16 u16Len)
{	
	// get data
	memcpy(gs_u8RxBuf, pu8Buf, u16Len);  
	os_SigSetTimer(&gs_tElevRxDataIfoTmr, &gs_tElevTsk, OS_SIG_TASK_ELEV_RXDATATMR, 10);		

}


void mb_MasterSetRcvTmoFlg(void)
{
	// get data tmo	
	os_SigSetTimer(&gs_tElevRxDataIfoTmoTmr, &gs_tElevTsk, OS_SIG_TASK_ELEV_RXDATATMO, 10);	
	
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
OS_TASK( elev_Tsk__ )
{    	
		
	if( OS_SIG_STATE_SET(OS_SIG_SYS) )
	{      
		mb_MasterReadHldRegIf(MB_SLAVE_ADDRESS, 0x00, 3);// require data 
		OS_SIG_CLEAR(OS_SIG_SYS);
	}

	// get data ok flag  
	if( OS_SIG_STATE_SET(OS_SIG_TASK_ELEV_RXDATATMR) )
	{	
		elev_GetReceivedData__(&gs_u8RxBuf[3], gs_u8RxBuf[2]);
		elev_UpLoadEmInfo__();
		os_TaskSleep( &gs_tElevTsk, 5000 );
		OS_SIG_CLEAR(OS_SIG_TASK_ELEV_RXDATATMR);
	}	
	// get dadta tmo flag  
	if( OS_SIG_STATE_SET(OS_SIG_TASK_ELEV_RXDATATMO) )
	{
		if(gs_u8TmoTimes ++ >= 5)
		{
			gs_u8TmoTimes = 0;
			os_TaskSleep( &gs_tElevTsk, 20*1000 );
		}			
		OS_SIG_CLEAR(OS_SIG_TASK_ELEV_RXDATATMO);
	}	

    OS_SIG_REBACK();
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
osBool elev_Wakeup__(void)
{
    elev_Init__(dsTrue);
    
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
osBool elev_Sleep__(void)
{
    elev_Init__(dsFalse);

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
void elev_Init__( dsBool a_bWake )
{
    if(dsTrue == a_bWake)
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

    }
    else
    {
		//
    }

}   /* ds_InitSnr__() */


void elev_UpLoadEmInfo__(void)
{	

	gs_u8Msg[1] = gs_tElevInfo.u8Floor;// floor information 
	gs_u8Msg[2] = gs_tElevInfo.u8RunState; // elevator running state
	gs_u8Msg[3] =	( gs_tElevInfo.u8LockState != FLOOR_INVALID_STATE ) ? gs_tElevInfo.u8LockState: \
					( gs_tElevInfo.u8InspectState != FLOOR_INVALID_STATE ) ? gs_tElevInfo.u8InspectState:\
					( gs_tElevInfo.u8FireSafetyState != FLOOR_INVALID_STATE ) ? gs_tElevInfo.u8FireSafetyState:\
					( gs_tElevInfo.u8DriverState != FLOOR_INVALID_STATE ) ? gs_tElevInfo.u8DriverState:\
					( gs_tElevInfo.u8OverLoadState != FLOOR_INVALID_STATE ) ? gs_tElevInfo.u8OverLoadState:\
					( gs_tElevInfo.u8VipState != FLOOR_INVALID_STATE ) ? gs_tElevInfo.u8VipState:FLOOR_INVALID_STATE;
	
	if( strcmp((const dsChar *)gs_u8Msg,(const dsChar *)gs_u8Lmsg) ) // elevator upload information immediately  when elevator state changes 
	{
		net_SendData(gs_u8Msg, sizeof(gs_u8Msg), netTrue);		
		memcpy(gs_u8Lmsg,gs_u8Msg,sizeof(gs_u8Msg));
	}
	else	// elevator upload information with heartbeat when elevator state keep
	{
		net_SendData(gs_u8Msg, sizeof(gs_u8Msg), netFalse);	
	}

}



void elev_GetReceivedData__(dsU8 *u8Data, dsU8 u8Len)
{
	if (  ( ( (u8Data[1]  >= 0x30) && (u8Data[1]  <= 0x39) ) && ( (u8Data[3]  >= 0x30) && (u8Data[3]  <= 0x39) ) )  )
	{
			gs_tElevInfo.u8Floor = (u8Data[1] - 0x30) *10 + (u8Data[3] - 0x30) ;// REG 1,2
	}
	else if( ( u8Data[1] >= 0x41 ) && ( (u8Data[3]  >= 0x30) && (u8Data[3]  <= 0x39) ) )
	{
		gs_tElevInfo.u8Floor = (u8Data[3] - 0x30) + 1 ;// REG 1,2
	}
	else  if(  (u8Data[1] == 0x00)  && ( (u8Data[3]  >= 0x30) && (u8Data[3]  <= 0x39) )  )
	{
		gs_tElevInfo.u8Floor = (u8Data[3] - 0x30)  ;// REG 1,2
	}
	
	gs_tElevInfo.u8RunState = ( (u8Data[5] & 0x01) != 0 ) ? FLOOR_UPPING_STATE :(( (u8Data[5] & 0x02) != 0 ) ? FLOOR_DOWNING_STATE : FLOOR_INVALID_STATE);
	
	gs_tElevInfo.u8LockState = ( (u8Data[5] & 0x04) != 0 ) ? FLOOR_LOCKING_STATE : FLOOR_INVALID_STATE;
	
	gs_tElevInfo.u8InspectState = ( (u8Data[5] & 0x08) != 0 ) ? FLOOR_INSPECTING_STATE : FLOOR_INVALID_STATE;
	
	gs_tElevInfo.u8FireSafetyState = ( (u8Data[5] & 0x10) != 0 ) ? FLOOR_FIRESAFE_STATE : FLOOR_INVALID_STATE;
	
	gs_tElevInfo.u8DriverState = ( (u8Data[5] & 0x20) != 0 ) ? FLOOR_DRIVER_STATE : FLOOR_INVALID_STATE;
	
	gs_tElevInfo.u8OverLoadState = ( (u8Data[5] & 0x40) != 0 ) ? FLOOR_OVERLOAD_STATE : FLOOR_INVALID_STATE;
	
	gs_tElevInfo.u8VipState = ( (u8Data[5] & 0x80) != 0 ) ? FLOOR_VIP_STATE : FLOOR_INVALID_STATE;
	
}




#endif 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
