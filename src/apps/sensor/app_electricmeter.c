/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    file name.c
* @data         2015/11/20
* @auther       
* @module       module name
* @brief        file description
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "app_electricmeter.h"



#if APP_USE_ELECTRIC_METER

#include <stdio.h>
#include <string.h>
#include "os.h"
#include "os_signal.h"
#include "cfifo.h"
#include "crc/crc.h"
#include "stm32l0xx_hal.h"
#include "uart/uart_config.h"
#include "stm32l0xx_hal_flash_ex.h"

#include "app_net.h"
#include "app_modbus.h"

#include "cli_cmd.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */



#define 	EM_485__		1
#define 	EM_PULSE__		2

//electricmeter config 
//电表类型 485 ，脉冲
#define EM_TYPE		EM_485__	

//485 电表
#if EM_TYPE == EM_485__
//#define EM_SONYE__
	#define EM_HXDZ__	
//脉冲电表
#elif EM_TYPE == EM_PULSE__

	#define EM_DDS791_PULSE__ 
#else
//其他电表
#endif


#define EEPROM_PULSESTART_POS		( DATA_EEPROM_BASE + 0x0400 )
#define EEPROM_PULSEPARAM_POS		( DATA_EEPROM_BASE + 0x0404 )


#define EM_PULS_PARAM		1600



#define MB_SLAVE_ADDRESS	( 0x05 )

#define OS_SIG_TASK_EM_RXDEVICETMO              	(((osUint32)1) << 1)	//get device info tmo flag
#define OS_SIG_TASK_EM_RXDATATMO              		(((osUint32)1) << 2)	//get data info tmo flag
#define OS_SIG_TASK_EM_RXDEVICETMR              	(((osUint32)1) << 3)	//get device info flag 
#define OS_SIG_TASK_EM_RXDATATMR             		(((osUint32)1) << 4)	//get data info flag 

#define OS_SIG_TASK_EM_EEPROMWRTMR              		(((osUint32)1) << 5)
#define OS_SIG_TASK_EM_EEPROMRDTMR              		(((osUint32)1) << 6)
#define OS_SIG_TASK_EM_UARTRXDTMR              			(((osUint32)1) << 7)
#define OS_SIG_TASK_EM_CFGTMR              					(((osUint32)1) << 8)

#define OS_SIG_TASK_EM_PULSEPARATMR								(((osUint32)1) << 9)

#define EM_WBADDRESS_WORD			0
#define EM_WBADDRESS_BYTE			1


#define GET_FLOAT_DOT_PART_DATA( x )				( (float)x - (dsU32)x )


#define CMD_PULSE_UART_WR		( 0x01 )
#define CMD_PULSE_UART_RD		( 0x02 )
#define CMD_PULSE_PARA_WR		( 0x03 )

/***************************************************************************************************
 * TYPEDEFS
 */

typedef union
{
	float	fRealVal;
	dsU8	u8OrgVal[4];
}uDataCVTypeDef;


typedef struct
{
	uDataCVTypeDef	uUv[3];		//a,b,c
	uDataCVTypeDef	uIa[3];		//a,b,c
	uDataCVTypeDef	uWpp;		//wpp	
}tEmDataTypeDef;


typedef struct
{
	dsU8 u8StartAddr;
	dsU8 u8Len;
}tRegAttrTypeDef;

typedef struct
{
	dsU8 u8Index;
	tRegAttrTypeDef	tRegInfo[7];
	dsU8	u8AddrType;	// EM_WBADDRESS_WORD, BYTE
	dsU8	u8InfoType;	// device info or data info 
}tRegInfoTypeDef;

typedef struct
{
	dsU32	u32Puls;
	dsU32	u32Energy;
	float fEnergy;
	union{
		float fSumEnergy;
		dsU32	u32SumEnergy;
	}uSumEnergy;
	float	(* PtoECallBack)(dsU32 *au32Puls, dsU32 *u32Energy);
}tPulsEmTypeDef;

typedef struct
{
	dsU8	u8RxBuf[16];
	dsU8	u8Len;
	dsU8	u8Data[4];//串口数据
	dsU32	u32OrgE;
	union{
		float fOrgE;
		dsU32	u32OrgE;
	}uOrgE;
}tPulsCfgEmTypeDef;

/***************************************************************************************************
 * CONSTANTS
 */
const tRegAttrTypeDef	gs_tRegInfoWord[7]	=	{
//Ua  			Ub			Uc	    Ia         Ib       Ic      Wpp
	13,2,		15,2,		17,2,		25,2,			27,2,		29,2,		73,2,
};
const tRegAttrTypeDef	gs_tRegInfoByte[7]	={
	//Ua  			Ub			Uc	    Ia         Ib       Ic      Wpp
	26,4,			30,4,		34,4,		50,4,			54,4,		58,4,		146,4,};

const 	tRegAttrTypeDef  gs_tRegInfoHxDz[7] = {
//Ua  		Ub		Uc	    Ia         	Ib       	Ic      	Wpp
	20,1,		21,1,		22,1,		26,1,			27,1,		28,1,		47,3,};
	
const dsU8 u8SignalType[7] = { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
	
/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */

static OS_TASK( em_Tsk__ );

static osBool em_Wakeup__(void);

static osBool em_Sleep__(void);

static void em_Init__( dsBool a_bWake );

static void em_MakeUploadFrame__(float fOrgData, dsU8 *u8Msg);

static void em_UpLoadEmInfo__(void);

static void em_GetReceivedData__(dsU8 *u8Data, dsU8 u8Len);

static float	em_PulsToEnergy__(dsU32 *au32Puls ,dsU32 *u32Energy);

static void em_PulsCfgResp__(dsU8 *pu8Buf, dsU8 u8Len, dsU8 u8Type);
/***************************************************************************************************
 * GLOBAL VARIABLES
 */
osTask_t gs_tEmTsk;

osSigTimer_t gs_tEmRxDevIfoTmoTmr;
osSigTimer_t gs_tEmRxDataIfoTmoTmr;

osSigTimer_t gs_tEmRxDevIfoTmr;
osSigTimer_t gs_tEmRxDataIfoTmr;

osSigTimer_t gs_tEmPulsCfgTmr;

dsU8 gs_u8RxBuf[32] = { 0 } ;

tEmDataTypeDef gs_tEmInfo = { 0 };
dsU8 gs_u8Msg[6] = {EM_SENSOR_ID, 0x00, 0x00, 0x00, 0x0A, 0x00};
dsU8	gs_u8TmoTimes = 0;
dsU16	gs_u16Pt = 0;//电压变比PT
dsU16	gs_u16Ct = 0;//电流变比CT
tRegInfoTypeDef		gs_RegInfo = {0};

tPulsEmTypeDef	gs_PulsEmInfo;
tPulsCfgEmTypeDef	gs_PulsCfg;

dsU16 gs_u16PulseParam = EM_PULS_PARAM;
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
 #if EM_TYPE == EM_485__
void em_Init( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;
#if 0	
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
#else
	GPIO_InitStructure.Pin = GPIO_PIN_0;	
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;	
	GPIO_InitStructure.Pull = GPIO_PULLUP;	
	__GPIOA_CLK_ENABLE();
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure); 	
#endif 

	

	gs_RegInfo.u8Index = 0;
	gs_RegInfo.u8AddrType = EM_WBADDRESS_WORD;
	gs_RegInfo.u8InfoType = 0;// 0 = device info ,1 = data info 
	memcpy(gs_RegInfo.tRegInfo, gs_tRegInfoWord, sizeof(gs_tRegInfoWord));

	
	net_SendData(gs_u8Msg, 6, netFalse);		
	os_TaskCreate( &gs_tEmTsk, em_Tsk__, 5*1000);

	os_TaskAddWakeupInit( &gs_tEmTsk, em_Wakeup__ );
	os_TaskAddSleepDeInit( &gs_tEmTsk, em_Sleep__ );

	os_TaskRun( &gs_tEmTsk );

} /* wm_Init() */
#elif EM_TYPE == EM_PULSE__ 
void em_Init( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	// receive interrupt  
	GPIO_InitStructure.Pin = GPIO_PIN_1;	
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;	
	GPIO_InitStructure.Pull = GPIO_NOPULL;	
	__GPIOA_CLK_ENABLE();
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure); 
	HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 1);
	HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);	

	GPIO_InitStructure.Pin = GPIO_PIN_7;	
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;	
	GPIO_InitStructure.Pull = GPIO_NOPULL;	
	__GPIOA_CLK_ENABLE();
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure); 
	HAL_NVIC_SetPriority(EXTI4_15_IRQn, 3, 1);
	HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);		
	
	
	uartCfg_Init(UART_CHN_2);
	uartCfg_EnableRx(UART_CHN_2,uartTrue);	

	HAL_FLASHEx_DATAEEPROM_Unlock();
	gs_u16PulseParam = ( 0 == *( (dsU32 *)(EEPROM_PULSEPARAM_POS) ))?(EM_PULS_PARAM):(*( (dsU32 *)(EEPROM_PULSEPARAM_POS) ));
	HAL_FLASHEx_DATAEEPROM_Lock();
		
	gs_PulsEmInfo.u32Energy = 0;
	gs_PulsEmInfo.u32Puls = 0;
	gs_PulsEmInfo.fEnergy = 0;
	gs_PulsEmInfo.PtoECallBack = em_PulsToEnergy__;
	
	gs_u8Msg[4] = 0x10;	//	能耗
	net_SendData(gs_u8Msg, 6, netFalse);		
	os_TaskCreate( &gs_tEmTsk, em_Tsk__, 5);

	os_TaskAddWakeupInit( &gs_tEmTsk, em_Wakeup__ );
	os_TaskAddSleepDeInit( &gs_tEmTsk, em_Sleep__ );
	os_SigSet(&gs_tEmTsk, OS_SIG_TASK_EM_EEPROMRDTMR);
	os_TaskAddItWake( IT_WAKE_ID_3, &gs_tEmTsk );
	os_SigSetTimer(&gs_tEmPulsCfgTmr, &gs_tEmTsk, OS_SIG_TASK_EM_CFGTMR,24*60*60*1000);//1 day 
	
	os_TaskRun( &gs_tEmTsk );

	//os_TaskSleep( &gs_tEmTsk, 0 );	
}


float	em_PulsToEnergy__(dsU32 *au32Puls ,dsU32 *u32Energy)
{	
	float fEnergy = 0;
	
	if(*au32Puls == gs_u16PulseParam)
	{
		*u32Energy = *u32Energy + 1;  
		*au32Puls = 0;
	}
	
	fEnergy = (float)*u32Energy + (float)*au32Puls/gs_u16PulseParam;
	*u32Energy = (*u32Energy >= 0xFFFFFFFF)?0:*u32Energy;
	return fEnergy;
}
//pulse interrupt 
void subEXIT7_IRQHandler( void )
{
		gs_PulsEmInfo.u32Puls ++;
    if( osFalse == os_TaskIsRun( &gs_tEmTsk ) ){
        os_TaskWakeupByIt(IT_WAKE_ID_3);
    }else{

    }
}

// low voltage interrupt 
void subEXIT1_IRQHandler( void )
{
    if( osFalse == os_TaskIsRun( &gs_tEmTsk ) ){
        os_TaskWakeupByIt(IT_WAKE_ID_3);
    }else{

    }
		
		gs_PulsCfg.uOrgE.fOrgE = gs_PulsEmInfo.uSumEnergy.fSumEnergy;
		os_SigSet(&gs_tEmTsk, OS_SIG_TASK_EM_EEPROMWRTMR);
}

#endif 

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
 #if EM_TYPE == EM_485__

OS_TASK( em_Tsk__ )
{    
		
	if( OS_SIG_STATE_SET(OS_SIG_SYS) )
	{      
		if(gs_RegInfo.u8InfoType == 0) // get device 
		{
			if ( gs_u8TmoTimes++ >= 5 )
			{
				gs_u8TmoTimes = 0;
				os_TaskSleep( &gs_tEmTsk, 5*60*1000 );
			}
			else
			{
#if defined (EM_SONYE__)				
				mb_MasterReadHldRegIf(MB_SLAVE_ADDRESS, 0, 1);// require device 
#elif defined (EM_HXDZ__)				
				mb_MasterReadInputRegIf(MB_SLAVE_ADDRESS, 2, 2);//get pt ct value 
#endif				
			}
		}
		else // get data  
		{
			if ( gs_u8TmoTimes++ >= 5 )
			{
				gs_u8TmoTimes = 0;
				os_TaskSleep( &gs_tEmTsk, 5*60*1000 );
			}
			else
			{	
#if defined (EM_SONYE__)				
				mb_MasterReadHldRegIf(MB_SLAVE_ADDRESS, gs_RegInfo.tRegInfo[gs_RegInfo.u8Index].u8StartAddr, gs_RegInfo.tRegInfo[gs_RegInfo.u8Index].u8Len);// require data 
#elif defined (EM_HXDZ__)					
				mb_MasterReadInputRegIf(MB_SLAVE_ADDRESS, gs_RegInfo.tRegInfo[gs_RegInfo.u8Index].u8StartAddr, gs_RegInfo.tRegInfo[gs_RegInfo.u8Index].u8Len);// require data		
#endif				
			}				
		}
		
		OS_SIG_CLEAR(OS_SIG_SYS);
	}
	// get device ok flag  
	if( OS_SIG_STATE_SET(OS_SIG_TASK_EM_RXDEVICETMR) )
	{	
		gs_RegInfo.u8InfoType = 1;
		gs_u8TmoTimes = 0;
#if defined (EM_SONYE__)		
		if(gs_u8RxBuf[2] == 1) // byte address
		{
			gs_RegInfo.u8AddrType = EM_WBADDRESS_BYTE;
			memcpy(gs_RegInfo.tRegInfo, gs_tRegInfoByte, sizeof(gs_tRegInfoByte));								
		}
		else if(gs_u8RxBuf[2] == 2) // word address;
		{
			gs_RegInfo.u8AddrType = EM_WBADDRESS_WORD;	
			memcpy(gs_RegInfo.tRegInfo, gs_tRegInfoWord, sizeof(gs_tRegInfoWord));	
		}
#elif defined (EM_HXDZ__ )		
			memcpy(gs_RegInfo.tRegInfo, gs_tRegInfoHxDz, sizeof(gs_tRegInfoHxDz));
			gs_u16Pt = 	( (dsU16)gs_u8RxBuf[3] ) << 8 | gs_u8RxBuf[4];
			gs_u16Ct = 	( (dsU16)gs_u8RxBuf[5] ) << 8 | gs_u8RxBuf[6];
#endif			
		OS_SIG_CLEAR(OS_SIG_TASK_EM_RXDEVICETMR);
	}
	// get device tmo flag  
	if( OS_SIG_STATE_SET(OS_SIG_TASK_EM_RXDEVICETMO) )
	{	
		gs_RegInfo.u8InfoType = 0;	
		OS_SIG_CLEAR(OS_SIG_TASK_EM_RXDEVICETMO);
	}
	// get data ok flag  
	if( OS_SIG_STATE_SET(OS_SIG_TASK_EM_RXDATATMR) )
	{	
		gs_u8TmoTimes = 0;		
		em_GetReceivedData__(&gs_u8RxBuf[3], gs_u8RxBuf[2]);

		em_UpLoadEmInfo__();
		gs_RegInfo.u8Index++;
		if(gs_RegInfo.u8Index >=  0x07) gs_RegInfo.u8Index = 0x00;
		memset(gs_u8RxBuf,0,sizeof(gs_u8RxBuf));
		os_TaskSleep( &gs_tEmTsk, 5*60*1000 );
		OS_SIG_CLEAR(OS_SIG_TASK_EM_RXDATATMR);
	}	
	// get data tmo flag  
	if( OS_SIG_STATE_SET(OS_SIG_TASK_EM_RXDATATMO) )
	{	
		os_TaskSleep( &gs_tEmTsk, 5*60*1000 );	
		OS_SIG_CLEAR(OS_SIG_TASK_EM_RXDATATMO);
	}	
	
	OS_SIG_REBACK();
}


void mb_MasterSetRcvTmrFlg(dsU8	*pu8Buf, dsU16 u16Len)
{	
	memcpy(gs_u8RxBuf, pu8Buf, u16Len);
	if(gs_RegInfo.u8InfoType == 0) // get device 
	{
		os_SigSetTimer(&gs_tEmRxDevIfoTmr, &gs_tEmTsk, OS_SIG_TASK_EM_RXDEVICETMR,10);		
	}
	else // get data  
	{
		os_SigSetTimer(&gs_tEmRxDataIfoTmr,&gs_tEmTsk, OS_SIG_TASK_EM_RXDATATMR,10);		
	}
}


void mb_MasterSetRcvTmoFlg(void)
{
	if(gs_RegInfo.u8InfoType == 0) // get device tmo
	{
		os_SigSetTimer(&gs_tEmRxDevIfoTmoTmr, &gs_tEmTsk, OS_SIG_TASK_EM_RXDEVICETMO,10);	
	}
	else // get data  tmo
	{
		os_SigSetTimer(&gs_tEmRxDataIfoTmoTmr, &gs_tEmTsk, OS_SIG_TASK_EM_RXDATATMO,10);	
		//os_SigSet(&gs_tEmTsk, OS_SIG_TASK_EM_RXDATATMO);		
	}	
	
}

#elif EM_TYPE == EM_PULSE__
OS_TASK( em_Tsk__ )
{
	dsU8 au8Buf[4] = { 0 };
	
	if( OS_SIG_STATE_SET(OS_SIG_TASK_EM_CFGTMR) )
	{
		HAL_FLASHEx_DATAEEPROM_Unlock();
		HAL_FLASHEx_DATAEEPROM_Program(TYPEPROGRAM_WORD, DATA_EEPROM_BASE, gs_PulsEmInfo.uSumEnergy.u32SumEnergy);
		HAL_FLASHEx_DATAEEPROM_Lock();
		OS_SIG_CLEAR(OS_SIG_TASK_EM_CFGTMR);
	}
	
	if( OS_SIG_STATE_SET(OS_SIG_TASK_EM_UARTRXDTMR) )
	{
		em_GetReceivedData__(gs_PulsCfg.u8RxBuf, gs_PulsCfg.u8Len);
		memset(gs_PulsCfg.u8RxBuf, 0, sizeof(gs_PulsCfg.u8RxBuf));	
		gs_PulsCfg.u8Len = 0;
		OS_SIG_CLEAR(OS_SIG_TASK_EM_UARTRXDTMR);
	}
	//配置脉冲系数
	if( OS_SIG_STATE_SET(OS_SIG_TASK_EM_PULSEPARATMR) )
	{
		HAL_FLASHEx_DATAEEPROM_Unlock();
		HAL_FLASHEx_DATAEEPROM_Program(TYPEPROGRAM_WORD, EEPROM_PULSEPARAM_POS, gs_u16PulseParam);
		HAL_FLASHEx_DATAEEPROM_Lock();
	
		au8Buf[0] = (dsU8)( ( gs_u16PulseParam & 0xFF00 ) >> 8 );	
		au8Buf[1] = (dsU8)( ( gs_u16PulseParam & 0x00FF ) >> 0 );	
		em_PulsCfgResp__(au8Buf,2,CMD_PULSE_PARA_WR);
		
		OS_SIG_CLEAR(OS_SIG_TASK_EM_PULSEPARATMR);
	}
	
	if( OS_SIG_STATE_SET(OS_SIG_TASK_EM_EEPROMWRTMR) )
	{
		HAL_FLASHEx_DATAEEPROM_Unlock();
		HAL_FLASHEx_DATAEEPROM_Program(TYPEPROGRAM_WORD, EEPROM_PULSESTART_POS, gs_PulsCfg.uOrgE.u32OrgE);
		HAL_FLASHEx_DATAEEPROM_Lock();
		au8Buf[0] = (dsU8)( ( (dsU32)(gs_PulsCfg.uOrgE.fOrgE*100) & 0xFF000000 ) >> 24 );
		au8Buf[1] = (dsU8)( ( (dsU32)(gs_PulsCfg.uOrgE.fOrgE*100) & 0x00FF0000 ) >> 16 );		
		au8Buf[2] = (dsU8)( ( (dsU32)(gs_PulsCfg.uOrgE.fOrgE*100) & 0x0000FF00 ) >> 8 );	
		au8Buf[3] = (dsU8)( ( (dsU32)(gs_PulsCfg.uOrgE.fOrgE*100) & 0x000000FF ) >> 0 );	
		em_PulsCfgResp__(au8Buf,4,CMD_PULSE_UART_WR);		
		//em_PulsCfgResp__(gs_PulsCfg.u8Data,4,CMD_PULSE_UART_WR);
		//printf("gs_PulsEmInfo.u32Puls = %d \n",gs_PulsEmInfo.u32Puls);
		//printf("gs_PulsEmInfo.uSumEnergy.fSumEnergy = %f \n",gs_PulsEmInfo.uSumEnergy.fSumEnergy);
		//printf("gs_PulsEmInfo.fEnergy = %f \n",gs_PulsEmInfo.fEnergy);
		//printf("gs_PulsCfg.uOrgE.fOrgE = %f \n",gs_PulsCfg.uOrgE.fOrgE);
		
		OS_SIG_CLEAR(OS_SIG_TASK_EM_EEPROMWRTMR);
	}
	if( OS_SIG_STATE_SET(OS_SIG_TASK_EM_EEPROMRDTMR) )
	{
		HAL_FLASHEx_DATAEEPROM_Unlock();
		gs_PulsCfg.uOrgE.u32OrgE = *( (dsU32 *)EEPROM_PULSESTART_POS );
		HAL_FLASHEx_DATAEEPROM_Lock();
		au8Buf[0] = (dsU8)( ( (dsU32)(gs_PulsCfg.uOrgE.fOrgE*100) & 0xFF000000 ) >> 24 );
		au8Buf[1] = (dsU8)( ( (dsU32)(gs_PulsCfg.uOrgE.fOrgE*100) & 0x00FF0000 ) >> 16 );		
		au8Buf[2] = (dsU8)( ( (dsU32)(gs_PulsCfg.uOrgE.fOrgE*100) & 0x0000FF00 ) >> 8 );	
		au8Buf[3] = (dsU8)( ( (dsU32)(gs_PulsCfg.uOrgE.fOrgE*100) & 0x000000FF ) >> 0 );	
		em_PulsCfgResp__(au8Buf,4,CMD_PULSE_UART_RD);
		OS_SIG_CLEAR(OS_SIG_TASK_EM_EEPROMRDTMR);
	}	
	
	
	if( OS_SIG_STATE_SET(OS_SIG_SYS) )
	{    
		gs_PulsEmInfo.fEnergy = gs_PulsEmInfo.PtoECallBack(&(gs_PulsEmInfo.u32Puls), &(gs_PulsEmInfo.u32Energy));
		gs_PulsEmInfo.uSumEnergy.fSumEnergy =gs_PulsCfg.uOrgE.fOrgE + gs_PulsEmInfo.fEnergy;
		em_MakeUploadFrame__(gs_PulsEmInfo.uSumEnergy.fSumEnergy,gs_u8Msg);
		net_SendData(gs_u8Msg, 6, netFalse);
		
		uartCfg_Poll(UART_CHN_2);
		
		OS_SIG_CLEAR(OS_SIG_SYS);
	}	
	

	
	OS_SIG_REBACK();	
}

void em_PulsCfgResp__(dsU8 *pu8Buf, dsU8 u8Len, dsU8 u8Type)
{
	dsU8	u8TxBuf[16] = { 0 };
	dsU8	u8Index = 0;
	dsU16 u16Crc = 0;
	
	u8TxBuf[0] = 0xFF;
	u8TxBuf[1] = 0x80 | u8Type;
	u8TxBuf[2] = u8Len;
	for(u8Index = 0; u8Index < u8Len; u8Index++)
	{
		u8TxBuf[3 + u8Index] = pu8Buf[u8Index];
	}
	u16Crc = usMBCRC16(u8TxBuf,3 + u8Len);
	u8TxBuf[3 + u8Len] =	(dsU8)( (u16Crc & 0xFF00) >> 8 );
	u8TxBuf[4 + u8Len] = 	(dsU8)( (u16Crc & 0x00FF) >> 0 );
	u8TxBuf[5 + u8Len] = 0xF5;
	
	uartCfg_SendBytes( UART_CHN_2, u8TxBuf, 6 + u8Len );
}


void uartCfg_Usart2ReceiveBytes( uartU8  *a_pu8Data, uartU16 a_u16Length )
{
    if( osFalse == os_TaskIsRun( &gs_tEmTsk ) )
        os_TaskWakeupByIt(IT_WAKE_ID_3);
    memcpy(gs_PulsCfg.u8RxBuf, a_pu8Data, a_u16Length); // get receive byte
		gs_PulsCfg.u8Len = a_u16Length;
		os_SigSet(&gs_tEmTsk,OS_SIG_TASK_EM_UARTRXDTMR);			
}
// HEAD CMD LEN DATA CRC16 END
//     CMD = 01:写，02:读  LEN = DATA 的字节数 CRCH CRCL 
// FF 01 04 00 00 00 00 XX XX F5
void em_GetReceivedData__(dsU8 *u8Data, dsU8 u8Len)
{
	dsU16	u16Crc = 0;
	dsU8	u8State = 0;
	
	u16Crc = ( (dsU16)u8Data[u8Len - 3] << 8 ) | u8Data[u8Len - 2];
	
	if( u16Crc == usMBCRC16(u8Data,u8Len - 3) )
	{
		u8State = (u8Data[0] == 0xFF)?1:0;
		u8State &= (u8Data[u8Len - 1] == 0xF5)?1:0;
		if(u8State)
		switch(u8Data[1])
		{
			case CMD_PULSE_UART_WR:
				memcpy(gs_PulsCfg.u8Data, &u8Data[3], 4);
				gs_PulsCfg.u32OrgE = ( (dsU32)gs_PulsCfg.u8Data[0] ) << 24  | ( (dsU32)gs_PulsCfg.u8Data[1] ) << 16 | 
															( (dsU32)gs_PulsCfg.u8Data[2] ) << 8 | ( (dsU32)gs_PulsCfg.u8Data[3] ) << 0;
				gs_PulsCfg.uOrgE.fOrgE = ( (float)gs_PulsCfg.u32OrgE )/100;
				os_SigSet(&gs_tEmTsk, OS_SIG_TASK_EM_EEPROMWRTMR);
				break;
			case CMD_PULSE_UART_RD:
				os_SigSet(&gs_tEmTsk, OS_SIG_TASK_EM_EEPROMRDTMR);
				break;
			case CMD_PULSE_PARA_WR:
				gs_u16PulseParam = ((dsU16)u8Data[3]) << 8 | u8Data[4];
				os_SigSet(&gs_tEmTsk, OS_SIG_TASK_EM_PULSEPARATMR);
				break;
			default:
				break;
		}
	}
	
}


#endif 


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
osBool em_Wakeup__(void)
{
    em_Init__(dsTrue);
    
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
osBool em_Sleep__(void)
{
    em_Init__(dsFalse);

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
#if (EM_TYPE == EM_485__)
void em_Init__( dsBool a_bWake )
{
    if(dsTrue == a_bWake)
    {
			GPIO_InitTypeDef GPIO_InitStructure;
#if 0	
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
#else
		GPIO_InitStructure.Pin = GPIO_PIN_0;	
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;	
		GPIO_InitStructure.Pull = GPIO_PULLUP;	
		__GPIOA_CLK_ENABLE();
		HAL_GPIO_Init(GPIOA, &GPIO_InitStructure); 	
#endif 
		if(gs_bCliUseUart == cliFalse)
		{
			uartCfg_Init(UART_CHN_2);
			uartCfg_EnableRx(UART_CHN_2,uartTrue);
		}		
		gs_RegInfo.u8InfoType = 0;
    }
    else
    {
		//
    }

}   /* em_InitSnr__() */
#elif (EM_TYPE == EM_PULSE__)
void em_Init__( dsBool a_bWake )
{
	if(gs_bCliUseUart == cliFalse)
	{
		uartCfg_Init(UART_CHN_2);
		uartCfg_EnableRx(UART_CHN_2,uartTrue);
	}
	
	GPIO_InitTypeDef GPIO_InitStructure;
	// receive interrupt  
	GPIO_InitStructure.Pin =  GPIO_PIN_1;	
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;	
	GPIO_InitStructure.Pull = GPIO_NOPULL;	
	__GPIOA_CLK_ENABLE();
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure); 	
	HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 1);
	HAL_NVIC_EnableIRQ(EXTI0_1_IRQn); 

	GPIO_InitStructure.Pin = GPIO_PIN_7;	
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;	
	GPIO_InitStructure.Pull = GPIO_NOPULL;	
	__GPIOA_CLK_ENABLE();
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure); 
	HAL_NVIC_SetPriority(EXTI4_15_IRQn, 3, 1);
	HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);	
	

}


#endif 

#if defined (EM_SONYE__ )
void em_GetReceivedData__(dsU8 *u8Data, dsU8 u8Len)
{

	if(gs_RegInfo.u8Index < 3)
	{
		gs_tEmInfo.uUv[gs_RegInfo.u8Index].u8OrgVal[0] = u8Data[3];
		gs_tEmInfo.uUv[gs_RegInfo.u8Index].u8OrgVal[1] = u8Data[2];
		gs_tEmInfo.uUv[gs_RegInfo.u8Index].u8OrgVal[2] = u8Data[1];
		gs_tEmInfo.uUv[gs_RegInfo.u8Index].u8OrgVal[3] = u8Data[0];
		
	}
	//memcpy( gs_tEmInfo.uUv[gs_RegInfo.u8Index].u8OrgVal, u8Data,4);
	else if(gs_RegInfo.u8Index > 5)
	{
		gs_tEmInfo.uWpp.u8OrgVal[0] = u8Data[3];
		gs_tEmInfo.uWpp.u8OrgVal[1] = u8Data[2];
		gs_tEmInfo.uWpp.u8OrgVal[2] = u8Data[1];
		gs_tEmInfo.uWpp.u8OrgVal[3] = u8Data[0];		
	//	memcpy( &( gs_tEmInfo.uWpp.u8OrgVal ), u8Data,4);
	}
	else
	{
		gs_tEmInfo.uIa[gs_RegInfo.u8Index - 3].u8OrgVal[0] = u8Data[3];
		gs_tEmInfo.uIa[gs_RegInfo.u8Index - 3].u8OrgVal[1] = u8Data[2];
		gs_tEmInfo.uIa[gs_RegInfo.u8Index - 3].u8OrgVal[2] = u8Data[1];
		gs_tEmInfo.uIa[gs_RegInfo.u8Index - 3].u8OrgVal[3] = u8Data[0];
	//memcpy( gs_tEmInfo.uIa[gs_RegInfo.u8Index - 3].u8OrgVal, u8Data,4);
	
	}
}

#elif defined   (EM_HXDZ__)
void em_GetReceivedData__(dsU8 *u8Data, dsU8 u8Len)
{
	dsU16 u16Uabc = 0;
	dsU16 u16Iabc = 0;
	dsU32 u32P = 0;
	dsU16 u16Pdot = 0;
	
	if(gs_RegInfo.u8Index < 3)
	{
		//gs_tEmInfo.uUv[gs_RegInfo.u8Index].u8OrgVal[0] = u8Data[0];
		//gs_tEmInfo.uUv[gs_RegInfo.u8Index].u8OrgVal[1] = u8Data[1];
		u16Uabc = ( (dsU16)u8Data[0])<< 8 | u8Data[1];
		gs_tEmInfo.uUv[gs_RegInfo.u8Index].fRealVal = ((float)u16Uabc)*gs_u16Pt/10;
	}
	else if(gs_RegInfo.u8Index > 5)
	{
		//gs_tEmInfo.uWpp.u8OrgVal[0] = u8Data[3];

		u32P = ( (dsU16)u8Data[0])<< 24 | ( (dsU16)u8Data[1])<< 16 | ( (dsU16)u8Data[2])<< 8 | u8Data[3];
		u16Pdot =  ( (dsU16)u8Data[4])<< 8 | u8Data[5];
		gs_tEmInfo.uWpp.fRealVal = u32P + ( (float)u16Pdot )/1000;
	}
	else
	{
		//gs_tEmInfo.uIa[gs_RegInfo.u8Index - 3].u8OrgVal[0] = u8Data[0];
		//gs_tEmInfo.uIa[gs_RegInfo.u8Index - 3].u8OrgVal[1] = u8Data[1];
		u16Iabc = ( (dsU16)u8Data[0])<< 8 | u8Data[1];
		gs_tEmInfo.uIa[gs_RegInfo.u8Index - 3].fRealVal = ((float)u16Iabc)*gs_u16Ct/1000;		
	}
}
#endif 



void em_UpLoadEmInfo__(void)
{
	dsU32	u32Power = 0;
	
	if(gs_RegInfo.u8Index < 3)
		em_MakeUploadFrame__(gs_tEmInfo.uUv[gs_RegInfo.u8Index].fRealVal,gs_u8Msg);
	else if(gs_RegInfo.u8Index > 5)	
	{
#if defined (EM_SONYE__ )		
		u32Power = (dsU32)(gs_tEmInfo.uWpp.u8OrgVal[3] ) << 24 |  (dsU32)(gs_tEmInfo.uWpp.u8OrgVal[2] ) << 16 |   (dsU32)(gs_tEmInfo.uWpp.u8OrgVal[1] ) << 8 |  (dsU32)(gs_tEmInfo.uWpp.u8OrgVal[0] );	
		em_MakeUploadFrame__( ((float)u32Power)/1000,gs_u8Msg );
#elif defined (EM_HXDZ__)
		em_MakeUploadFrame__( gs_tEmInfo.uWpp.fRealVal,gs_u8Msg );
#endif		
	}
	else
		em_MakeUploadFrame__(gs_tEmInfo.uIa[gs_RegInfo.u8Index - 3].fRealVal,gs_u8Msg);
	
	net_SendData(gs_u8Msg, 6, netFalse);		
}



void em_MakeUploadFrame__(float fOrgData, dsU8 *u8Msg)
{
	dsU32	 u32Int ;
	dsU8	u8Dot;

	u32Int	=	(dsU32)fOrgData;
	u8Dot = 100*GET_FLOAT_DOT_PART_DATA( fOrgData );
	u8Msg[1] = (dsU8)( ( u32Int  & 0x0000FF00 ) >> 8 );
	u8Msg[2] = (dsU8)( ( u32Int  & 0x000000FF )  );
	u8Msg[3] = u8Dot;
#if (EM_TYPE == EM_485__)	
	u8Msg[4] = u8SignalType[gs_RegInfo.u8Index];
#elif (EM_TYPE == EM_PULSE__)
	u8Msg[4] = 0x10;
#endif	
	
}




#endif 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
