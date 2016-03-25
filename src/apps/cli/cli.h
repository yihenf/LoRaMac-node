#ifndef __CLI_H__
#define __CLI_H__

#include "app_cfg.h"

#if APP_USE_CLI
#include "cli_str.h"
#include "stm32l0xx_hal.h"

#define CLI_MAX_RCV_LEN         256 //最大接收字节数
#define CLI_MAX_CMD_LEN         30  //函数名最大长度
#define CLI_MAX_CMD_PARM        10  //最大为10个参数
#define CLI_MAX_PARAM_LEN       256 //所有参数之和的长度

#define CLI_OK              0  //无错误
#define CLI_CMDERR          1  //命令错误
#define CLI_PARAMERR        2  //参数错误
#define CLI_PARAMOVER       3  //参数过多
#define CLI_NOMATCH         4  //未找到匹配命令

typedef struct
{
    UART_HandleTypeDef *ptUart;
    //wPTxFifo_t *ptTxFifo;
    //wPRxFifo_t *ptRxFifo;
    cliBool  bSending;
    cliU8    u8ReceByte;
    cliU32   u32RxInterval;  /* when rx stop time is bigger than it, we think a fram end */
    cliU32   u32RxTimeout;
    cliBool  bRxTmoMonitor;  /* when true, should monitor rx timeout  */
}cli_uartInfo_t;


//CLI命令列表
typedef struct cli_cmd
{
    void* func; //函数指针
    const cliU8* name; //CLI命令名(查找串)
}cli_cmd_t;

//CLI控制管理器
typedef struct cli_dev
{
    cli_cmd_t *cmd; //CLI命令

    void (*cmd_init)(void);             //初始化
    cliU8 (*cmd_check)(cliU8*str);          //识别函数名及参数
    void (*cmd_execute)(void);              //执行
    void (*cmd_probe)(void);             //扫描

    cliU8 fnum;                         //函数数量
    cliU8 pnum;                        //参数数量
    cliU8 id;                           //函数id
    cliU16 parmtype;                    //参数的类型
    cliU8  plentbl[CLI_MAX_CMD_PARM];       //每个参数的长度暂存表
    cliU8  parm[CLI_MAX_PARAM_LEN];             //函数的参数
}cli_dev_t;

void cli_cmd_init(void); //初始化
cliU8 cli_cmd_check(cliU8*str); //检查
void cli_cmd_execute(void); //执行
void cli_cmd_probe(void); //探测

extern cli_dev_t gs_tCliDev;

#endif

#endif

