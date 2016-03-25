/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    file name.c
* @data         2015/08/04
* @auther       chuanpengl
* @module       door sensor app
* @brief        door sensor app
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "cli_cmd.h"
#if APP_USE_CLI
#include "stm32l0xx_hal.h"
#include <stdio.h>
#include <string.h>

#include "app_doorsnr.h"
#include "app_distributecurrent.h"
#include "app_thermohumidity.h"
#include "app_waterpress.h"
#include "app_waterlevel.h"

#include "app_currentlimit.h"
#include "app_watermeter.h"
#include "app_electricmeter.h"
#include "app_elevator.h"
//#include "app_modbus.h"
#include "app_elevatoresaving.h"
#include "app_distributor.h"
#include "app_weatherstation.h"
#include "app_pmb.h"

#include "dataExchange.h"

#include "iuUart/iuUart.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define NET_MAC_ADDRESS_LOCATION        0x08080409
#define SN_CFG_START_ADDR   NET_MAC_ADDRESS_LOCATION  //SN配置在EEPROM中起始地址
#define SN_CFG_END_ADDR     0x080807FF  //SN配置在EEPROM中结束地址
#define LORA_CFG_START_ADDR PARAMETERS_ADDRESS_IN_FLASH  //LORA配置在EEPROM中起始地址
#define LORA_CFG_END_ADDR   0x080803FF  //LORA配置在EEPROM中结束地址
 
#define CLI_DEBUG_NODE      "-----------------------DEBUG------------------------"
#define CLI_CONFIG_NODE     "-----------------------CONFIG-----------------------" 
#define CLI_SN_NODE         "-------------------------SN-------------------------"
#define CLI_LORA_NODE       "------------------------LORA------------------------"

#define CLI_GET_MAC( _au8Mac )  \
    (((cliU32)(_au8Mac)[0] << 16) | ((cliU32)(_au8Mac)[1] << 8) | ((cliU32)(_au8Mac)[2] << 0))


#define CLI_SET_MAC( _au8Mac, _u32Mac )     \
        (_au8Mac)[0] = (cliU8)(((_u32Mac) >> 16) & 0xFF);\
        (_au8Mac)[1] = (cliU8)(((_u32Mac) >> 8) & 0xFF);\
        (_au8Mac)[2] = (cliU8)(((_u32Mac) >> 0) & 0xFF);

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
 * @fn      cli_Tsk__()
 *
 * @brief   door sensor task
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void cli_Tsk__( void );

/***************************************************************************************************
 * @fn      cli_Wakeup__()
 *
 * @brief   door sensor wakeup
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static osBool cli_Wakeup__(void);

/***************************************************************************************************
 * @fn      cli_Sleep__()
 *
 * @brief   door sensor sleep
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static osBool cli_Sleep__(void);

/***************************************************************************************************
 * GLOBAL VARIABLES
 */
static cliU16    gs_u16TimeCounter = 0;

cliBool gs_bCliActive = cliFalse;
sn_config_t gs_tSnConfig;

cliBool gs_bCliUseUart = cliTrue;


/***************************************************************************************************
 * STATIC VARIABLES
 */
TimerEvent_t s_tCliTmr;
static uint8_t s_ucState = TASK_SLEEP;


/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
#if 0
cliU32 cli_read_addr(cliU32 addr)
{
	return *(cliU32*)addr;	
}

void cli_write_addr(cliU32 addr,cliU32 val)
{
	*(cliU32*)addr=val; 	
}

//Config CLI Function
cliBool cli_save_config(void)
{
    cliU8 cfg_write[4];
    cliU32 Address = 0;
    cliU8 shift = 0;

    memcpy(&cfg_write[1], gs_tSnConfig.sn_mac_addr, 3);
    
    HAL_FLASHEx_DATAEEPROM_Unlock();
    
    //写入SN配置
    Address = SN_CFG_START_ADDR - 1;
    while (Address < (SN_CFG_START_ADDR -1 + 4))
    {
        //printf(".");
        if (HAL_FLASHEx_DATAEEPROM_Program(TYPEPROGRAM_WORD, Address, 
                        *((__IO cliU32*)(cfg_write+shift))) == HAL_OK)
        {
            Address = Address + 4;
            shift = shift + 4;            
            HAL_Delay(1);
        }
        else
        {
            //printf("save sn config ERROR!\r\n");
            return cliFalse;
        }
    }
    HAL_FLASHEx_DATAEEPROM_Lock(); 
    
    //printf("save sn config OK!\r\n");

    //写入LORA配置
    sx1278_cfg_SaveParameters();
    
    //printf("save lora config OK!\r\n");

    return cliTrue;
}

#if 0
void cli_read_config(void)
{
    dsU32 Address = 0;
    app_config_t app_config;    
    dsU8    *p_app_config = (dsU8 *)&app_config;
        
    Address = FLASH_USER_START_ADDR;
    memcpy(p_app_config, (dsU8 *)Address, sizeof(app_config));

    if(app_config.sn_mac_addr > 0)
    {
        gs_tAppConfig.sn_mac_addr = app_config.sn_mac_addr;
        net_SetSnMacAddr(app_config.sn_mac_addr);
    }

    if(app_config.lora_rf_frequency > 0)
    {
        gs_tAppConfig.lora_rf_frequency = app_config.lora_rf_frequency;
        SX1276LoRaCfgSetRFFrequency(app_config.lora_rf_frequency);
    }

    if(app_config.lora_rf_power > 0 && app_config.lora_rf_power <= 20)
    {
        gs_tAppConfig.lora_rf_power = app_config.lora_rf_power;
        SX1276LoRaCfgSetRFPower(app_config.lora_rf_power);
    }

    if(app_config.lora_signal_bandwidth >= 7 
                && app_config.lora_signal_bandwidth <= 9 )
    {
        gs_tAppConfig.lora_signal_bandwidth = app_config.lora_signal_bandwidth;
        SX1276LoRaCfgSetSignalBandwidth(app_config.lora_signal_bandwidth);
    }

    if(app_config.lora_spreading_factor >= 6 
                && app_config.lora_spreading_factor <= 12 )
    {
        gs_tAppConfig.lora_spreading_factor = app_config.lora_spreading_factor;
        SX1276LoRaCfgSetSpreadingFactor(app_config.lora_spreading_factor);
    }

    if(app_config.lora_error_coding >= 1 
                && app_config.lora_error_coding <= 4 )
    {
        gs_tAppConfig.lora_error_coding = app_config.lora_error_coding;
        SX1276LoRaCfgSetErrorCoding(app_config.lora_error_coding);
    }
}
#endif

//SN CLI Function
void cli_show_sn_mac_addr(void)
{    
    //printf("SN MAC Address: 0x%x\r\n", net_GetSnMacAddr());
}

void cli_set_sn_mac_addr(cliU32 sn_mac_addr)
{
    CLI_SET_MAC(gs_tSnConfig.sn_mac_addr, sn_mac_addr);
}

void cli_show_sn_manufactory(void)
{
    cliU32 i;

    //printf("SN Manufactory: ");
    for(i=0; i<16; i++)
    {
        //printf("%c", gs_tSnConfig.manufactory[i]);
    }
    //printf("\r\n");
}

void cli_show_sn_serial_number(void)
{
    cliU32 i;

    //printf("SN Serial Number: ");
    for(i=0; i<16; i++)
    {
        //printf("%c", gs_tSnConfig.serial_num[i]);
    }
    //printf("\r\n");
}

void cli_show_sn_hw_version(void)
{
    //printf("SN HW Version: V%x.%02x\r\n",
        gs_tSnConfig.hw_version[0], gs_tSnConfig.hw_version[1]);
}

void cli_show_sn_short_addr(void)
{    
    //printf("SN Short Address: %d\r\n", net_GetSnShortAddr());
}

void cli_show_sn_count(void)
{    
    //printf("SN Count: %d\r\n", net_GetSnCount());
}

void cli_show_sn_rssi(void)
{
    //printf("SN Rssi: -%d dBm\r\n", net_GetSnRssi());
}

void cli_show_sn_snr(void)
{
    //printf("SN Snr: %d dB\r\n", net_GetSnSnr());
}

void cli_show_sn_battery(void)
{
    if(os_PvdGetResult()== cliTrue)
        //printf("SN Battery: Low Power\r\n");
    else
        //printf("SN Battery: Normal\r\n");
}

void cli_show_sn_all(void)
{
    cli_show_sn_mac_addr();
    cli_show_sn_manufactory();
    cli_show_sn_serial_number();
    cli_show_sn_hw_version();
    cli_show_sn_short_addr();
    cli_show_sn_count();
    cli_show_sn_rssi();
    cli_show_sn_snr();
    cli_show_sn_battery();
}

//LoRa CLI Function
void cli_show_lora_freq(void)
{
    //printf("LoRa RF: %d.%02d MHz\r\n",
            SX1276LoRaGetRFFrequency()/1000000,
            SX1276LoRaGetRFFrequency()%1000000);    
}

cliBool cli_set_lora_freq(cliU32 freq)
{
    return (cliBool)sx1278_cfg_SetFreq(freq);
}

void cli_show_lora_power(void)
{
    //printf("LoRa Power: %d dBm\r\n", SX1276LoRaGetRFPower());
}

cliBool cli_set_lora_power(cliU8 power)
{
    return (cliBool)sx1278_cfg_SetPower(power);
}
    
void cli_show_lora_bw(void)
{
    cliU8 bw = 0;

    bw = SX1276LoRaGetSignalBandwidth();
    switch(bw)
    {
        case 0:
            //printf("LoRa BandWidth: 7.8 KHz\r\n");
            break;            
        case 1:
            //printf("LoRa BandWidth: 10.4 KHz\r\n");
            break;
        case 2:
            //printf("LoRa BandWidth: 15.6 KHz\r\n");
            break;            
        case 3:
            //printf("LoRa BandWidth: 20.8 KHz\r\n");
            break;
        case 4:
            //printf("LoRa BandWidth: 31.2 KHz\r\n");
            break;            
        case 5:
            //printf("LoRa BandWidth: 41.6 KHz\r\n");
            break;
        case 6:
            //printf("LoRa BandWidth: 62.5 KHz\r\n");
            break;            
        case 7:
            //printf("LoRa BandWidth: 125 KHz\r\n");
            break;
        case 8:
            //printf("LoRa BandWidth: 250 KHz\r\n");
            break;            
        case 9:
            //printf("LoRa BandWidth: 500 KHz\r\n");
            break;
        default:
            //printf("LoRa BandWidth: Reserved\r\n");
            break;            
    }

}

cliBool cli_set_lora_bw(cliU8 bw)
{
    return (cliBool)sx1278_cfg_SetBw(bw);
}

void cli_show_lora_sf(void)
{
    //printf("LoRa SF: %d\r\n", SX1276LoRaGetSpreadingFactor());
}

cliBool cli_set_lora_sf(cliU8 sf)
{
    return (cliBool)sx1278_cfg_SetSf(sf);
}
    
void cli_show_lora_cr(void)
{
    //printf("LoRa CR: %d\r\n", SX1276LoRaGetErrorCoding());
}

cliBool cli_set_lora_cr(cliU8 cr)
{
    return (cliBool)sx1278_cfg_SetCr(cr);
}

void cli_show_lora_preamble(void)
{
    //printf("LoRa Preamble Length: %d\r\n", SX1276LoRaGetPreambleLength());
}

cliBool cli_set_lora_preamble(cliU16 preamble)
{
    return (cliBool)sx1278_cfg_SetPreamble(preamble);
}

void cli_show_lora_all(void)
{
    cli_show_lora_freq();
    cli_show_lora_power();
    cli_show_lora_bw();
    cli_show_lora_sf();
    cli_show_lora_cr();
    cli_show_lora_preamble();
}

extern cliBool gs_bTestReTxHeartBeatflag;
extern cliBool gs_bTestReTxRdmEventflag;
void cli_set_test_ReTxHeartBeat(cliU8 para)
{
    if(para > 0)
        gs_bTestReTxHeartBeatflag = cliTrue;
    else        
        gs_bTestReTxHeartBeatflag = cliFalse;
}

void cli_set_test_ReTxRdmEvent(cliU8 para)
{
    if(para > 0)
        gs_bTestReTxRdmEventflag = cliTrue;
    else        
        gs_bTestReTxRdmEventflag = cliFalse;
}

cli_cmd_t gs_tCliCmd[CLI_MAX_CMD_NUM]=
{
    NULL,                                       CLI_DEBUG_NODE,
    (void *)cli_read_addr,                      "cliU32 read_addr(cliU32 addr)",
    (void *)cli_write_addr,                     "void write_addr(cliU32 addr,cliU32 val)",
    (void *)cli_set_test_ReTxHeartBeat,         "void set_test_ReTxHeartBeat(cliU8 para)",
    (void *)cli_set_test_ReTxRdmEvent,          "void set_test_ReTxRdmEvent(cliU8 para)",    

    NULL,                                       CLI_CONFIG_NODE,
    (void *)cli_save_config,                    "void save_config(void)",
    
    NULL,                                       CLI_SN_NODE,
    (void *)cli_set_sn_mac_addr,                "void set_sn_mac_addr(cliU32 sn_mac_addr)",
    (void *)cli_show_sn_mac_addr,               "void show_sn_mac_addr(void)",
    (void *)cli_show_sn_manufactory,            "void show_sn_manufactory(void)",
    (void *)cli_show_sn_serial_number,          "void show_sn_serial_number(void)",
    (void *)cli_show_sn_hw_version,             "void show_sn_hw_version(void)",
    (void *)cli_show_sn_short_addr,             "void show_sn_short_addr(void)",
    (void *)cli_show_sn_count,                  "void show_sn_count(void)",
    (void *)cli_show_sn_rssi,                   "void show_sn_rssi(void)",
    (void *)cli_show_sn_snr,                    "void show_sn_snr(void)",
    (void *)cli_show_sn_battery,                "void show_sn_battery(void)",
    (void *)cli_show_sn_all,                    "void show_sn_all(void)",

    NULL,                                       CLI_LORA_NODE,
    (void *)cli_show_lora_freq,                 "void show_lora_freq(void)",
    (cliBool *)cli_set_lora_freq,               "cliBool set_lora_freq(cliU32 freq)",
    (void *)cli_show_lora_power,                "void show_lora_power(void)",
    (cliBool *)cli_set_lora_power,              "cliBool set_lora_power(cliU8 power)",
    (void *)cli_show_lora_bw,                   "void show_lora_bw(void)",
    (cliBool *)cli_set_lora_bw,                 "cliBool set_lora_bw(cliU8 bw)",
    (void *)cli_show_lora_sf,                   "void show_lora_sf(void)",
    (cliBool *)cli_set_lora_sf,                 "cliBool set_lora_sf(cliU8 sf)",
    (void *)cli_show_lora_cr,                   "void show_lora_cr(void)",
    (cliBool *)cli_set_lora_cr,                 "cliBool set_lora_cr(cliU8 cr)",
    (void *)cli_show_lora_preamble,             "void show_lora_preamble(void)",
    (cliBool *)cli_set_lora_preamble,           "cliBool set_lora_preamble(cliU16 preamble)",    
    (void *)cli_show_lora_all,                  "void show_lora_all(void)",

};
#else
cli_cmd_t gs_tCliCmd[CLI_MAX_CMD_NUM] = {0};
#endif
 
/***************************************************************************************************
 * @fn      cli_Init()
 *
 * @brief   init door sensor app
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void cli_Init( void )
{
    memcpy(&gs_tSnConfig, (cliU8 *)SN_CFG_START_ADDR, sizeof(sn_config_t));
    
    TimerInit( &s_tCliTmr, cli_Tsk__ );
    TimerSetValue( &s_tCliTmr, 10000 );     /* 10ms */
    TimerStart( &s_tCliTmr );

    ds_Init();

} /* cli_Init() */

/***************************************************************************************************
 * @fn      cli_WakeUpEvent()
 *
 * @brief   when wakeup event happed, invoke this
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline void cli_WakeUpEvent( void )
{
    if( TASK_SLEEP == s_ucState ){
        cli_Tsk__();        /* wake task immediately by call task */
    }
}
#define CLI_TASK_RUN_PERIOD     (100000)
#define CLI_TASK_SLP_PERIOD     (100000)

void cli_Send( uint8_t *a_pucBuf, uint16_t a_usLen )
{
    cli_WakeUpEvent();
    uartCfg_SendBytes( IU_UART_2, a_pucBuf, a_usLen );
}

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */

/***************************************************************************************************
 * @fn      cli_Tsk__()
 *
 * @brief   door sensor task
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void cli_Tsk__( void )
{
    static uint8_t i = 0;
    if ( TASK_SLEEP == s_ucState )
    {
        /* init  */
        cli_Wakeup__();
        s_ucState = TASK_RUN;
        TimerSetValue( &s_tCliTmr, CLI_TASK_RUN_PERIOD );
    }

    if ( TASK_RUN == s_ucState )
    {
        /* do */
        gs_tCliDev.cmd_probe();
        gs_u16TimeCounter ++;
        
        if ( i < 255 ){
            i ++;
        }

        if(0)//(gs_u16TimeCounter >= 500 && gs_bCliActive == cliFalse)
        {
            gs_bCliUseUart = cliFalse;

            /* set state sleep request when need */
            s_ucState = TASK_SLEEP_REQUEST;

        }

        if ( uartFalse == uartCfg_IsBusy( IU_UART_2  ) ){
            if ( i > 3 ){
                s_ucState = TASK_SLEEP_REQUEST;
            }
        }

    }

    if ( TASK_SLEEP_REQUEST == s_ucState )
    {
        i = 0;
        /* deinit */
        cli_Sleep__();
        /* stop timer */
        //TimerSetValue( &s_tCliTmr, CLI_TASK_SLP_PERIOD );
        s_ucState = TASK_SLEEP;
        BlockLowPowerDuringTask(false);
        //TimerStart( &s_tCliTmr );
#if 0
#if APP_USE_MODBUS_RTU
    mb_MasterInit();
#endif 


#if APP_USE_THERMO_HUMIDITY
        //sdli 温湿度采集
        th_Init();
#endif

#if APP_USE_WATER_PRESS
        //sdli 水压采集
        wp_Init();
#endif

#if APP_USE_WATER_LEVEL
        //sdli 液位传感
        wl_Init();
#endif

#if APP_USE_DISTRIBUTE_CURRENT
        //sdli 电流采集
        dc_Init();
#endif

#if APP_USE_DOORSNR
        //sdli 门磁传感
        ds_Init();
#endif

#if APP_USE_WATER_METER
        //水表
        wm_Init();
#endif 

#if APP_USE_ELECTRIC_METER
        //电表
        em_Init();
#endif 

#if APP_USE_ELEVATOR_KONE
        //通力电梯
        elev_Init();
#endif 

#if APP_USE_ELEVATOR_E_SAVING
        //电梯节能
        es_Init();
#endif

#if APP_USE_DISTRIBUTOR
        //变送器
        db_Init();
#endif

#if APP_USE_CURRENTLIMIT
        //电流阈值监测模块
        cl_Init();
#endif

#if APP_USE_WEATHERSTATION_XPH
        xphws_Init();
#endif

#if APP_USE_PMB_MASTER
        pmb_Init();
#endif
#endif
    }
    else
    {
        BlockLowPowerDuringTask(true);
        /* this task will not wake auto, should wake by other task */
        TimerStartInIsr( &s_tCliTmr );
    }

}

/***************************************************************************************************
 * @fn      cli_Wakeup__()
 *
 * @brief   door sensor wakeup
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
osBool cli_Wakeup__(void)
{
    gs_tCliDev.cmd_init();    //初始化CLI

    return cliTrue;
}

/***************************************************************************************************
 * @fn      cli_Sleep__()
 *
 * @brief   door sensor sleep
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
osBool cli_Sleep__(void)
{
    return cliTrue;
}

#endif
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
