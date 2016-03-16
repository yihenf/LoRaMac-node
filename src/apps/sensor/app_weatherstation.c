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
#include "app_weatherstation.h"



#if APP_USE_WEATHERSTATION_XPH

#include <stdio.h>
#include <string.h>
#include "os.h"
#include "os_signal.h"
#include "cfifo.h"
#include "crc/crc.h"
#include "stm32l0xx_hal.h"
#include "uart/uart_config.h"

#include "app_net.h"
#include "app_modbus.h"
/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define MB_SLAVE_ADDRESS	( 0x01 )

#define OS_SIG_TASK_XPHWS_RXDATATMO              		(((osUint32)1) << 1)	//get data info tmo flag
#define OS_SIG_TASK_XPHWS_RXDATATMR              		(((osUint32)1) << 2)	//get data info flag 
#define OS_SIG_TASK_XPHWS_UPLOADTMR              		(((osUint32)1) << 3)	//
#define OS_SIG_TASK_XPHWS_REQUIRE              			(((osUint32)1) << 4)	//
	
#define WIND_SPEED_CHN			( 0x00 )//风速 OK
#define TEMPERATURE_CHN			( 0x02 )//温度 OK
#define SUNSHINE_TM_CHN			( 0x05 )//日照时数
#define WIND_DIRECTION_CHN		( 0x06 )//风向 OK
#define TOTAL_RADIATION_CHN		( 0x07 )//总辐射 OK
#define HUMIDITY_CHN			( 0x08 )//湿度 OK
#define RADIATION_SUM_CHN		( 0x09 )//总辐射累加辐射量
#define DIRECT_RADIATION_CHN	( 0x0C )//直接辐射 OK
#define DIRECT_RADIASUM_CHN		( 0x0D )//直接辐射累加辐射量
#define SCATTER_RADIATION_CHN	( 0x0E )//散射辐射 OK
#define SCATTER_RADIASUM_CHN	( 0x0F )//散射辐射累加辐射量

#define MASK_POS_NEG	( 0x8000 )

#define MAKE_HALFWORD(ByteH, ByteL)	\
	( (xphU16)ByteH ) << 8 |  (xphU16)ByteL 

/***************************************************************************************************
 * TYPEDEFS
 */

typedef struct
{
	xphU16 u16OrigData[16];//原始数据
	float fResolution[16];//分辨率
}tXphwsInfoTypeDef;


typedef struct
{	
	float fWindSpeed;
	float fTemperature;
	float fSunshineTime;
	xphU8 u8WindDirection;
	xphU16 u16TotalRadiation;
	float fThumidity;
	float fTRadiationSum;
	xphU16 u16DirRadiation;
	float fDRadiationSum; 
	xphU16 u16ScatteRadiation;
	float fSRadiationSum;
}tXphwsValTypeDef;



typedef struct
{
	xphU8 u8Type;
	float fValue;
}tXphwsUpLoadInfoTypeDef;

/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
/*****************************************************************************************************
*****************************************************************************************************/
// main task process function
static OS_TASK( xphws_Tsk__ );
// main task wakeup function
static osBool xphws_Wakeup__(void);
// main task sleep function
static osBool xphws_Sleep__(void);
// wake-sleep init function
static void xphws_Init__( xphBool a_bWake );

/*****************************************************************************************************
*****************************************************************************************************/

static void xphws_UpLoadEmInfo__(void);

static void xphws_GetReceivedData__(xphU8 *u8Data, xphU8 u8Len);


/***************************************************************************************************
 * GLOBAL VARIABLES
 */
osTask_t gs_tXphwsTsk;
osSigTimer_t gs_tXphwsRxDataIfoTmoTmr;
osSigTimer_t gs_tXphwsRxDataIfoTmr;
osSigTimer_t gs_tXphwsUpLoadTmr;
osSigTimer_t gs_tXphwsRequireTmr;


xphU8 gs_u8RxBuf[64] = { 0 } ;
xphU8 gs_u8Msg[6] = {WS_XPH_ID, 0x00, 0x00, 0x00, 0x0A, 0x00};
xphU8	gs_u8TmoTimes = 0;
xphU8	gs_u8Index = 0;

tXphwsValTypeDef	gs_tXphwsVal = { 0 };
tXphwsInfoTypeDef	gs_tXphwsInfo = {	
	{ 0 },
	{ 0.1, 0.0, 0.1, 0.0, 0.0, 0.1, 1.0, 1.0, 0.1, 0.01, 0.0, 0.0, 1.0, 0.01, 1.0, 0.01 }
/*	通道1，风速，分辨率0.1 				OK 
	通道2，无
	通道3，温度，分辨率0.1 				OK
	通道4，无
	通道5，无
	通道6，日照时数，分辨率0.1
	通道7，风向，分辨率1.0 				OK
	通道8，总辐射，分辨率1.0 				OK
	通道9，湿度，分辨率0.1 				OK
	通道10，总辐射累加辐射量，分辨率0.01
	通道11，无
	通道12，无
	通道13，直接辐射，分辨率1.0 			OK
	通道14，直接累加辐射量，分辨率0.01
	通道15，散射辐射，分辨率1.0 			OK
	通道16，散射累加辐射量，分辨率0.01
*/
};

tXphwsUpLoadInfoTypeDef		gs_tXphwsUpLoadInfo[7] = {
	{0x0A,0},//風速
	{0x0B,0},//風向
	{0x0C,0},//溫度
	{0x0D,0},//濕度
	{0x0E,0},//全輻射
	{0x0F,0},//直接輻射
	{0x10,0},//散射輻射
};


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
void xphws_Init( void )
{ 

	net_SendData(gs_u8Msg, 6, netFalse);	
	os_TaskCreate( &gs_tXphwsTsk, xphws_Tsk__, 10);

	os_TaskAddWakeupInit( &gs_tXphwsTsk, xphws_Wakeup__ );
	os_TaskAddSleepDeInit( &gs_tXphwsTsk, xphws_Sleep__ );

	os_TaskRun( &gs_tXphwsTsk );

} /* wm_Init() */


void mb_MasterSetRcvTmrFlg(xphU8	*pu8Buf, xphU16 u16Len)
{	
	// get data
	memcpy(gs_u8RxBuf, pu8Buf, u16Len);  
	os_SigSetTimer(&gs_tXphwsRxDataIfoTmr, &gs_tXphwsTsk, OS_SIG_TASK_XPHWS_RXDATATMR, 10);		

}


void mb_MasterSetRcvTmoFlg(void)
{
	// get data tmo	
	os_SigSetTimer(&gs_tXphwsRxDataIfoTmoTmr, &gs_tXphwsTsk, OS_SIG_TASK_XPHWS_RXDATATMO, 10);	
	
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
OS_TASK( xphws_Tsk__ )
{    		
	if( OS_SIG_STATE_SET(OS_SIG_SYS) )
	{      
		//mb_MasterReadHldRegIf(MB_SLAVE_ADDRESS, 0x00, 2);// require data	
		OS_SIG_CLEAR(OS_SIG_SYS);
		
	}
#if 1	
	if( OS_SIG_STATE_SET(OS_SIG_TASK_XPHWS_REQUIRE) )
	{
		mb_MasterReadHldRegIf(MB_SLAVE_ADDRESS, 0x00, 16);// require data
		OS_SIG_CLEAR(OS_SIG_TASK_XPHWS_REQUIRE);		
	}
	
	if( OS_SIG_STATE_SET(OS_SIG_TASK_XPHWS_UPLOADTMR) )
	{
		xphws_UpLoadEmInfo__();
		
		gs_u8Index = ( gs_u8Index + 1 )%7;
		if(gs_u8Index == 0)
		{
			os_SigStopTimer(&gs_tXphwsUpLoadTmr);
			os_TaskSleep( &gs_tXphwsTsk, 60*1000 );
		}
		os_SigSetTimer(&gs_tXphwsUpLoadTmr,&gs_tXphwsTsk,OS_SIG_TASK_XPHWS_UPLOADTMR,5000);
		OS_SIG_CLEAR(OS_SIG_TASK_XPHWS_UPLOADTMR);
	}
	
	// get data ok flag  
	if( OS_SIG_STATE_SET(OS_SIG_TASK_XPHWS_RXDATATMR) )
	{	
		os_SigStopTimer(&gs_tXphwsRequireTmr);
		xphws_GetReceivedData__(&gs_u8RxBuf[3], gs_u8RxBuf[2]);
		os_SigSet(&gs_tXphwsTsk,OS_SIG_TASK_XPHWS_UPLOADTMR);
		OS_SIG_CLEAR(OS_SIG_TASK_XPHWS_RXDATATMR);
	}	
	// get dadta tmo flag  
	if( OS_SIG_STATE_SET(OS_SIG_TASK_XPHWS_RXDATATMO) )
	{
		if(gs_u8TmoTimes ++ >= 5)
		{
			gs_u8TmoTimes = 0;
			os_SigStopTimer(&gs_tXphwsRequireTmr);
			os_TaskSleep( &gs_tXphwsTsk, 60*1000 );
		}
		else
		{
			os_SigSetTimer(&gs_tXphwsRequireTmr,&gs_tXphwsTsk,OS_SIG_TASK_XPHWS_REQUIRE,5000);
		}			
		OS_SIG_CLEAR(OS_SIG_TASK_XPHWS_RXDATATMO);
	}
#endif	

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
osBool xphws_Wakeup__(void)
{
    xphws_Init__(xphTrue);
    
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
osBool xphws_Sleep__(void)
{
    xphws_Init__(xphFalse);

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
void xphws_Init__( xphBool a_bWake )
{
    if(xphTrue == a_bWake)
    {
		GPIO_InitTypeDef GPIO_InitStructure;
		/* send control  high available */	 
		/* receive control  low available */
		GPIO_InitStructure.Pin = GPIO_PIN_0;	
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;	
		GPIO_InitStructure.Pull = GPIO_PULLUP;	
		__GPIOA_CLK_ENABLE();
		HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

		uartCfg_Init(UART_CHN_2);
		uartCfg_EnableRx(UART_CHN_2,uartTrue);
		
		os_SigSetTimer(&gs_tXphwsRequireTmr,&gs_tXphwsTsk,OS_SIG_TASK_XPHWS_REQUIRE,5000);

    }
    else
    {
		//
    }

}   /* ds_InitSnr__() */


void xphws_UpLoadEmInfo__(void)
{	

	if(gs_tXphwsUpLoadInfo[gs_u8Index].fValue < 0)
	{
		gs_u8Msg[1] = 0;
		gs_u8Msg[2] = (xphI8)gs_tXphwsUpLoadInfo[gs_u8Index].fValue; 
		gs_u8Msg[3] = (xphU8)( 0 - (gs_tXphwsUpLoadInfo[gs_u8Index].fValue - (xphU16)gs_tXphwsUpLoadInfo[gs_u8Index].fValue)*100 );	
	}
	else
	{
		gs_u8Msg[1] = (xphU8)( (((xphU16)gs_tXphwsUpLoadInfo[gs_u8Index].fValue) & 0xFF00 ) >> 8 ); 
		gs_u8Msg[2] = (xphU8)( (((xphU16)gs_tXphwsUpLoadInfo[gs_u8Index].fValue) & 0x00FF ) >> 0 ); 
		gs_u8Msg[3] = (xphU8)( (gs_tXphwsUpLoadInfo[gs_u8Index].fValue - (xphU16)gs_tXphwsUpLoadInfo[gs_u8Index].fValue)*100 );
	}
	

	gs_u8Msg[4] = gs_tXphwsUpLoadInfo[gs_u8Index].u8Type;;	
	net_SendData(gs_u8Msg, sizeof(gs_u8Msg), netTrue);	
}



void xphws_GetReceivedData__(xphU8 *u8Data, xphU8 u8Len)
{
	xphU16 u16DataArry[16] = { 0 };
	xphU8 u8Index = 0;
	xphI16 i16Temp = 0;
	
	for(u8Index = 0; u8Index < u8Len/2; u8Index++)
	{ 
		u16DataArry[u8Index] = MAKE_HALFWORD( u8Data[u8Index*2],u8Data[u8Index*2 + 1] );
		if(u16DataArry[u8Index] == 0x7FFF)
			u16DataArry[u8Index] = 0;	
	}
	memcpy(gs_tXphwsInfo.u16OrigData, u16DataArry, u8Len/2);
	//计算自动气象站数据
	gs_tXphwsVal.fWindSpeed = gs_tXphwsInfo.u16OrigData[WIND_SPEED_CHN] * gs_tXphwsInfo.fResolution[WIND_SPEED_CHN];
	if(!(MASK_POS_NEG & gs_tXphwsInfo.u16OrigData[TEMPERATURE_CHN])){
		gs_tXphwsVal.fTemperature = gs_tXphwsInfo.u16OrigData[TEMPERATURE_CHN] * gs_tXphwsInfo.fResolution[TEMPERATURE_CHN];
	}
	else {
		gs_tXphwsInfo.u16OrigData[TEMPERATURE_CHN] = (0xFFFF - gs_tXphwsInfo.u16OrigData[TEMPERATURE_CHN]) + 1;
		gs_tXphwsVal.fTemperature = 0 - gs_tXphwsInfo.u16OrigData[TEMPERATURE_CHN] * gs_tXphwsInfo.fResolution[TEMPERATURE_CHN];
	}	
	gs_tXphwsVal.fSunshineTime = gs_tXphwsInfo.u16OrigData[SUNSHINE_TM_CHN] * gs_tXphwsInfo.fResolution[SUNSHINE_TM_CHN];
	gs_tXphwsVal.u8WindDirection = gs_tXphwsInfo.u16OrigData[WIND_DIRECTION_CHN] * gs_tXphwsInfo.fResolution[WIND_DIRECTION_CHN];
	gs_tXphwsVal.u16TotalRadiation = gs_tXphwsInfo.u16OrigData[TOTAL_RADIATION_CHN] * gs_tXphwsInfo.fResolution[TOTAL_RADIATION_CHN];
	gs_tXphwsVal.fThumidity = gs_tXphwsInfo.u16OrigData[HUMIDITY_CHN] * gs_tXphwsInfo.fResolution[HUMIDITY_CHN];
	gs_tXphwsVal.fTRadiationSum = gs_tXphwsInfo.u16OrigData[RADIATION_SUM_CHN] * gs_tXphwsInfo.fResolution[RADIATION_SUM_CHN];
	gs_tXphwsVal.u16DirRadiation = gs_tXphwsInfo.u16OrigData[DIRECT_RADIATION_CHN] * gs_tXphwsInfo.fResolution[DIRECT_RADIATION_CHN];
	gs_tXphwsVal.fDRadiationSum = gs_tXphwsInfo.u16OrigData[DIRECT_RADIASUM_CHN] * gs_tXphwsInfo.fResolution[DIRECT_RADIASUM_CHN];
	gs_tXphwsVal.u16ScatteRadiation = gs_tXphwsInfo.u16OrigData[SCATTER_RADIATION_CHN] * gs_tXphwsInfo.fResolution[SCATTER_RADIATION_CHN];
	gs_tXphwsVal.fSRadiationSum = gs_tXphwsInfo.u16OrigData[SCATTER_RADIASUM_CHN] * gs_tXphwsInfo.fResolution[SCATTER_RADIASUM_CHN];

	gs_tXphwsUpLoadInfo[0].fValue = gs_tXphwsVal.fWindSpeed;//风速
	gs_tXphwsUpLoadInfo[1].fValue = gs_tXphwsVal.u8WindDirection;//风向
	gs_tXphwsUpLoadInfo[2].fValue = gs_tXphwsVal.fTemperature;//温度
	gs_tXphwsUpLoadInfo[3].fValue = gs_tXphwsVal.fThumidity;//湿度
	gs_tXphwsUpLoadInfo[4].fValue = gs_tXphwsVal.fTRadiationSum;//全辐射
	gs_tXphwsUpLoadInfo[5].fValue = gs_tXphwsVal.fDRadiationSum;//直接辐射
	gs_tXphwsUpLoadInfo[6].fValue = gs_tXphwsVal.fSRadiationSum;//散射辐射
}



#endif 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
