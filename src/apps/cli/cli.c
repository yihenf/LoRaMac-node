#include "cli.h"

#if APP_USE_CLI
#include "cli_cmd.h"
#include "debug_usart.h"
#include <stdio.h>
#include "iuUart/iuUart.h"

static cliU8 gs_u8CliRcvBuf[CLI_MAX_RCV_LEN]; //接收缓冲,最大USART_REC_LEN个字节.
static cliU16 gs_u16CliRcvState=0; /*
                                        接收状态标记
                                        bit15，  接收完成标志
                                        bit14，  接收到0x0d
                                        bit13~0，    接收到的有效字节数目
                                    */
static cliU8 *gs_u8CliSysCmd[]= {"easylinkin_sn_cli_active", "?", "help", "list",}; //系统命令
static cliU8 gs_ucUartRunning = 0x00;

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
    uartU8 sendByte = (uartU8)ch;
    uartCfg_SendBytes( IU_UART_2, &sendByte, 1);
    return ch;
}


void cliUartInd( uartU8 a_ucIndType, uartI32 a_uiVal )
{
    uartU16 usRxBytes = 0;
    uartU16 usRcvdBytes = gs_u16CliRcvState & 0x3FFF;
    uartU16 i= 0;

    if ( (CLI_MAX_RCV_LEN - 1) <= usRcvdBytes ){
        usRcvdBytes = 0;
    }

    switch( a_ucIndType )
    {
    case UART_IND_TX_EMPTY:
        break;
    case UART_IND_RX_START:
        break;
    case UART_IND_RX_RECEIVED:
        usRxBytes = uartCfg_GetReceiveBytes( IU_UART_2,
                &gs_u8CliRcvBuf[ usRcvdBytes ], \
                CLI_MAX_RCV_LEN - 1 - usRcvdBytes );

        /* 在最新接收到的usRxBytes字节中查找0x0d 0x0a */
        for ( i = 0; i < usRxBytes; i ++ ){
            if ( 0x0d == gs_u8CliRcvBuf[ usRcvdBytes + i ] ){
                if ( 0 == (gs_u16CliRcvState & 0x4000) ){
                    gs_u16CliRcvState |= 0x4000;
                }else { /* 收到无效的0x0d, reset */
                    gs_u16CliRcvState = 0;
                    break;
                }
            }else if ( 0x0a == gs_u8CliRcvBuf[ usRcvdBytes + i ] ){

                /* 收到无效的0x0d, reset */
                if ( 0 == (gs_u16CliRcvState & 0x4000) ){
                    gs_u16CliRcvState = 0;
                }else {     /* 收到有效的0x0a */
                    gs_u16CliRcvState |= 0x8000;
                    break;
                }
            }else if(gs_u8CliRcvBuf[ usRcvdBytes + i ] >= 0x80 || gs_u8CliRcvBuf[ usRcvdBytes + i ] == 0x00){
                gs_u16CliRcvState = 0;
                break;
            }else {
                gs_u16CliRcvState += 1;
            }
        }
        break;
    default:
        break;
    }

}

cli_dev_t gs_tCliDev=
{
    gs_tCliCmd,
    cli_cmd_init,
    cli_cmd_check,
    cli_cmd_execute,
    cli_cmd_probe,
    sizeof(gs_tCliCmd)/sizeof(cli_cmd_t),//函数数量
    0,      //参数数量
    0,      //函数ID
    1,      //参数显示类型,0,10进制;1,16进制
    0,      //参数类型.bitx:,0,数字;1,字符串
    0,      //每个参数的长度暂存表,需要MAX_PARM个0初始化
    0,      //函数的参数,需要PARM_LEN个0初始化
};

cliU8 cli_sys_cmd_exe(cliU8 *str)
{
    cliU8 i;
    cliU8 sfname[CLI_MAX_CMD_LEN];//存放本地函数名
    cliU32 res;
    res=cli_get_cmdname(str,sfname,&i,CLI_MAX_CMD_LEN);//得到指令及指令长度
    if(res)
        return CLI_CMDERR;//错误的指令
    str+=i;
    for(i=0;i<sizeof(gs_u8CliSysCmd)/4;i++)//支持的系统指令
    {
        if(cli_strcmp(sfname,gs_u8CliSysCmd[i])==0)break;
    }
    switch(i)
    {
        case 0://"easylinkin_sn_cli_active"
            gs_bCliActive = cliTrue;
            //printf("easylinkin sn cli active.\r\n");
        uartCfg_SendBytes( IU_UART_2, "easylinkin sn cli active.\r\n", strlen("easylinkin sn cli active.\r\n"));
            break;
        case 1://"?"
        case 2://"help"
        case 3://"list"
            for(i=0;i<gs_tCliDev.fnum;i++){
                //printf("%s\r\n", gs_tCliDev.cmd[i].name);
                //printf("\r\n");
            }

            break;
        default://非法指令
            return CLI_CMDERR;
    }
    return 0;
}

void cli_cmd_init(void)
{
    iuUartPara_t tPar = {
            38400,
            IU_UART_PARITY_NONE,
            IU_UART_BIT_LEN_8,
            IU_UART_STOPBITS_1,
            IU_UART_HWCONTROL_NONE,
            IU_UART_RUN_RTX,
            cliUartInd
    };

    uartCfg_Init( IU_UART_2, &tPar );
    uartCfg_SendBytes( IU_UART_2, "uart init\r\n", strlen("uart init\r\n"));
}

cliU8 cli_cmd_check(cliU8*str)
{
    cliU8 sta,i,rval;//状态
    cliU8 rpnum,spnum;
    cliU8 rfname[CLI_MAX_CMD_LEN];//暂存空间,用于存放接收到的函数名
    cliU8 sfname[CLI_MAX_CMD_LEN];//存放本地函数名

    sta=cli_get_fname(str,rfname,&rpnum,&rval);//得到接收到的数据的函数名及参数个数
    if(sta)
        return sta;//错误

    for(i=0;i<gs_tCliDev.fnum;i++)
    {
        if(gs_tCliDev.cmd[i].name[0] == '-'
                && gs_tCliDev.cmd[i].name[1] == '-')
            continue;

        sta=cli_get_fname((cliU8*)gs_tCliDev.cmd[i].name,sfname,&spnum,&rval);
        if(sta)
            return sta;//本地解析有误

        if(cli_strcmp(sfname,rfname)==0)//相等
        {
            if(spnum>rpnum)
                return CLI_PARAMERR;//参数错误(输入参数比源函数参数少)

            gs_tCliDev.id=i;//记录函数ID.
            break;
        }
    }

    if(i==gs_tCliDev.fnum)
        return CLI_NOMATCH; //未找到匹配的函数

    sta=cli_get_fparam(str,&i);//得到函数参数个数
    if(sta)
        return sta;

    gs_tCliDev.pnum=i;//参数个数记录

    return CLI_OK;
}

//usamrt执行函数
//该函数用于最终执行从串口收到的有效函数.
//最多支持10个参数的函数,更多的参数支持也很容易实现.不过用的很少.一般5个左右的参数的函数已经很少见了.
//该函数会在串口打印执行情况.以:"函数名(参数1，参数2...参数N)=返回值".的形式打印.
//当所执行的函数没有返回值的时候,所打印的返回值是一个无意义的数据.
void cli_cmd_execute(void)
{
    cliU8 id,i;
    cliU32 res;
    cliU32 temp[CLI_MAX_CMD_PARM];//参数转换,使之支持了字符串
    cliU8 sfname[CLI_MAX_CMD_LEN];//存放本地函数名
    cliU8 pnum,rval;

    id=gs_tCliDev.id;
    if(id>=gs_tCliDev.fnum)
        return;//不执行.

    cli_get_fname((cliU8*)gs_tCliDev.cmd[id].name,sfname,&pnum,&rval);//得到本地函数名,及参数个数

    ////printf("\r\n%s(",sfname);//输出正要执行的函数名

    for(i=0;i<pnum;i++)//输出参数
    {
        if(gs_tCliDev.parmtype&(1<<i))//参数是字符串
        {
            //printf("%c",'"');
            //printf("%s",gs_tCliDev.parm+cli_get_parmpos(i));
            //printf("%c",'"');
            temp[i]=(cliU32)&(gs_tCliDev.parm[cli_get_parmpos(i)]);
        }
        else                          //参数是数字
        {
            temp[i]=*(cliU32*)(gs_tCliDev.parm+cli_get_parmpos(i));
            //printf("0X%X",temp[i]);//16进制参数显示
        }
        if(i!=pnum-1)
        {
            //printf(",");
        }
    }

    switch(gs_tCliDev.pnum)
    {
        case 0://无参数(void类型)
            res=(*(cliU32(*)())gs_tCliDev.cmd[id].func)();
            break;
        case 1://有1个参数
            res=(*(cliU32(*)())gs_tCliDev.cmd[id].func)(temp[0]);
            break;
        case 2://有2个参数
            res=(*(cliU32(*)())gs_tCliDev.cmd[id].func)(temp[0],temp[1]);
            break;
        case 3://有3个参数
            res=(*(cliU32(*)())gs_tCliDev.cmd[id].func)(temp[0],temp[1],temp[2]);
            break;
        case 4://有4个参数
            res=(*(cliU32(*)())gs_tCliDev.cmd[id].func)(temp[0],temp[1],temp[2],temp[3]);
            break;
        case 5://有5个参数
            res=(*(cliU32(*)())gs_tCliDev.cmd[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4]);
            break;
        case 6://有6个参数
            res=(*(cliU32(*)())gs_tCliDev.cmd[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
            temp[5]);
            break;
        case 7://有7个参数
            res=(*(cliU32(*)())gs_tCliDev.cmd[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
            temp[5],temp[6]);
            break;
        case 8://有8个参数
            res=(*(cliU32(*)())gs_tCliDev.cmd[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
            temp[5],temp[6],temp[7]);
            break;
        case 9://有9个参数
            res=(*(cliU32(*)())gs_tCliDev.cmd[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
            temp[5],temp[6],temp[7],temp[8]);
            break;
        case 10://有10个参数
            res=(*(cliU32(*)())gs_tCliDev.cmd[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
            temp[5],temp[6],temp[7],temp[8],temp[9]);
            break;
    }

    if(rval==1)//需要返回值.
    {
        //printf("=0X%X;\r\n",res);//输出执行结果(16进制参数显示)
    }

}

void cli_cmd_probe(void)
{
    cliU8 sta,len;

    uartCfg_Poll( IU_UART_2 );

    if(gs_u16CliRcvState&0x8000)//串口接收完成
    {
        len=gs_u16CliRcvState&0x3fff;   //得到此次接收到的数据长度
        gs_u8CliRcvBuf[len]='\0';   //在末尾加入结束符.
    #if 1
        sta=gs_tCliDev.cmd_check(gs_u8CliRcvBuf);//得到函数各个信息
        if(sta==0)
            gs_tCliDev.cmd_execute();   //执行函数
        else
        {
            len=cli_sys_cmd_exe(gs_u8CliRcvBuf);
            if(len!=CLI_CMDERR)
                sta=len;
            if(sta)
            {
                switch(sta)
                {
                    case CLI_CMDERR:
                        //printf("command error!\r\n");
                        break;
                    case CLI_PARAMERR:
                        //printf("parameters error!\r\n");
                        break;
                    case CLI_PARAMOVER:
                        //printf("too much parameters!\r\n");
                        break;
                    case CLI_NOMATCH:
                        //printf("no match command!\r\n");
                        break;
                }
            }
        }
        //printf("\r\n");
          #endif
        gs_u16CliRcvState=0;//状态寄存器清空
    }
}

#endif

