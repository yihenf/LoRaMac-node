#include "cli_str.h"
#include "cli.h"

#if APP_USE_CLI

//对比字符串str1和str2
cliU8 cli_strcmp(cliU8 *str1,cliU8 *str2)
{
    while(1)
    {
        if(*str1!=*str2)return 1;//不相等
        if(*str1=='\0')break;//对比完成了.
        str1++;
        str2++;
    }
    return 0;//两个字符串相等
}

//把str1的内容copy到str2
void cli_strcopy(cliU8*str1,cliU8 *str2)
{
    while(1)
    {
        *str2=*str1;    //拷贝
        if(*str1=='\0')break;//拷贝完成了.
        str1++;
        str2++;
    }
}

//得到字符串的长度(字节)
cliU8 cli_strlen(cliU8*str)
{
    cliU8 len=0;
    while(1)
    {
        if(*str=='\0')break;//拷贝完成了.
        len++;
        str++;
    }
    return len;
}

//m^n函数
cliU32 cli_pow(cliU8 m,cliU8 n)
{
    cliU32 result=1;
    while(n--)result*=m;
    return result;
}

//把字符串转为数字
cliU8 cli_str2num(cliU8*str,cliU32 *res)
{
    cliU32 t;
    cliU8 bnum=0;   //数字的位数
    cliU8 *p;
    cliU8 hexdec=10;//默认为十进制数据
    p=str;
    *res=0;//清零.
    while(1)
    {
        if((*p<='9'&&*p>='0')||(*p<='F'&&*p>='A')||(*p=='X'&&bnum==1))//参数合法
        {
            if(*p>='A')hexdec=16;   //字符串中存在字母,为16进制格式.
            bnum++;                 //位数增加.
        }else if(*p=='\0')break;    //碰到结束符,退出.
        else return 1;              //不全是十进制或者16进制数据.
        p++;
    }
    p=str;              //重新定位到字符串开始的地址.
    if(hexdec==16)      //16进制数据
    {
        if(bnum<3)return 2;         //位数小于3，直接退出.因为0X就占了2个,如果0X后面不跟数据,则该数据非法.
        if(*p=='0' && (*(p+1)=='X'))//必须以'0X'开头.
        {
            p+=2;   //偏移到数据起始地址.
            bnum-=2;//减去偏移量
        }else return 3;//起始头的格式不对
    }else if(bnum==0)return 4;//位数为0，直接退出.
    while(1)
    {
        if(bnum)bnum--;
        if(*p<='9'&&*p>='0')t=*p-'0';   //得到数字的值
        else t=*p-'A'+10;               //得到A~F对应的值
        *res+=t*cli_pow(hexdec,bnum);
        p++;
        if(*p=='\0')break;//数据都查完了.
    }
    return 0;//成功转换
}

//得到指令名
cliU8 cli_get_cmdname(cliU8*str,cliU8*cmdname,cliU8 *nlen,cliU8 maxlen)
{
    *nlen=0;
    while(*str!=' '&&*str!='\0') //找到空格或者结束符则认为结束了
    {
        *cmdname=*str;
        str++;
        cmdname++;
        (*nlen)++;//统计命令长度
        if(*nlen>=maxlen)return 1;//错误的指令
    }
    *cmdname='\0';//加入结束符
    return 0;//正常返回
}

//获取下一个字符（当中间有很多空格的时候，此函数直接忽略空格，找到空格之后的第一个字符）
cliU8 cli_search_nextc(cliU8* str)
{
    str++;
    while(*str==' '&&str!='\0')str++;
    return *str;
} 

//从str中得到函数名
cliU8 cli_get_fname(cliU8*str,cliU8*fname,cliU8 *pnum,cliU8 *rval)
{
    cliU8 res;
    cliU8 fover=0;    //括号深度
    cliU8 *strtemp;
    cliU8 offset=0;
    cliU8 parmnum=0;
    cliU8 temp=1;
    cliU8 fpname[6];//void+X+'/0'
    cliU8 fplcnt=0; //第一个参数的长度计数器
    cliU8 pcnt=0;    //参数计数器
    cliU8 nchar;
    //判断函数是否有返回值
    strtemp=str;
    while(*strtemp!='\0')//没有结束
    {
        if(*strtemp!=' '&&(pcnt&0X7F)<5)//最多记录5个字符
        {
            if(pcnt==0)pcnt|=0X80;//置位最高位,标记开始接收返回值类型
            if(((pcnt&0x7f)==4)&&(*strtemp!='*'))break;//最后一个字符,必须是*
            fpname[pcnt&0x7f]=*strtemp;//记录函数的返回值类型
            pcnt++;
        }else if(pcnt==0X85)break;
        strtemp++;
    }
    if(pcnt)//接收完了
    {
        fpname[pcnt&0x7f]='\0';//加入结束符
        if(cli_strcmp(fpname,"void")==0)*rval=0;//不需要返回值
        else *rval=1;                              //需要返回值
        pcnt=0;
    }
    res=0;
    strtemp=str;
    while(*strtemp!='('&&*strtemp!='\0') //此代码找到函数名的真正起始位置
    {
        strtemp++;
        res++;
        if(*strtemp==' '||*strtemp=='*')
        {
            nchar=cli_search_nextc(strtemp);        //获取下一个字符
            if(nchar!='('&&nchar!='*')offset=res;   //跳过空格和*号
        }
    }
    strtemp=str;
    if(offset)strtemp+=offset+1;//跳到函数名开始的地方
    res=0;
    nchar=0;//是否正在字符串里面的标志,0，不在字符串;1，在字符串;
    while(1)
    {
        if(*strtemp==0)
        {
            res=CLI_CMDERR;//函数错误
            break;
        }else if(*strtemp=='('&&nchar==0)fover++;//括号深度增加一级
        else if(*strtemp==')'&&nchar==0)
        {
            if(fover)fover--;
            else res=CLI_CMDERR;//错误结束,没收到'('
            if(fover==0)break;//到末尾了,退出
        }else if(*strtemp=='"')nchar=!nchar;

        if(fover==0)//函数名还没接收完
        {
            if(*strtemp!=' ')//空格不属于函数名
            {
                *fname=*strtemp;//得到函数名
                fname++;
            }
        }else //已经接受完了函数名了.
        {
            if(*strtemp==',')
            {
                temp=1;     //使能增加一个参数
                pcnt++;
            }else if(*strtemp!=' '&&*strtemp!='(')
            {
                if(pcnt==0&&fplcnt<5)       //当第一个参数来时,为了避免统计void类型的参数,必须做判断.
                {
                    fpname[fplcnt]=*strtemp;//记录参数特征.
                    fplcnt++;
                }
                temp++; //得到有效参数(非空格)
            }
            if(fover==1&&temp==2)
            {
                temp++;     //防止重复增加
                parmnum++;  //参数增加一个
            }
        }
        strtemp++;
    }
    if(parmnum==1)//只有1个参数.
    {
        fpname[fplcnt]='\0';//加入结束符
        if(cli_strcmp(fpname,"void")==0)parmnum=0;//参数为void,表示没有参数.
    }
    *pnum=parmnum;  //记录参数个数
    *fname='\0';    //加入结束符
    return res;     //返回执行结果
}

//从str中得到一个函数的参数
cliU8 cli_get_aparm(cliU8 *str,cliU8 *fparm,cliU8 *ptype)
{
    cliU8 i=0;
    cliU8 enout=0;
    cliU8 type=0;//默认是数字
    cliU8 string=0; //标记str是否正在读
    while(1)
    {
        if(*str==','&& string==0)enout=1;           //暂缓立即退出,目的是寻找下一个参数的起始地址
        if((*str==')'||*str=='\0')&&string==0)break;//立即退出标识符
        if(type==0)//默认是数字的
        {
            if((*str>='0' && *str<='9')||(*str>='a' && *str<='f')||(*str>='A' && *str<='F')||*str=='X'||*str=='x')//数字串检测
            {
                if(enout)break;                 //找到了下一个参数,直接退出.
                if(*str>='a')*fparm=*str-0X20;  //小写转换为大写
                else *fparm=*str;               //小写或者数字保持不变
                fparm++;
            }else if(*str=='"')//找到字符串的开始标志
            {
                if(enout)break;//找到,后才找到",认为结束了.
                type=1;
                string=1;//登记STRING 正在读了
            }else if(*str!=' '&&*str!=',')//发现非法字符,参数错误
            {
                type=0XFF;
                break;
            }
        }else//string类
        {
            if(*str=='"')string=0;
            if(enout)break;         //找到了下一个参数,直接退出.
            if(string)              //字符串正在读
            {
                if(*str=='\\')      //遇到转义符(不复制转义符)
                {
                    str++;          //偏移到转义符后面的字符,不管什么字符,直接COPY
                    i++;
                }
                *fparm=*str;        //小写或者数字保持不变
                fparm++;
            }
        }
        i++;//偏移量增加
        str++;
    }
    *fparm='\0';    //加入结束符
    *ptype=type;    //返回参数类型
    return i;       //返回参数长度
}

//得到指定参数的起始地址
cliU8 cli_get_parmpos(cliU8 num)
{
    cliU8 temp=0;
    cliU8 i;
    for(i=0;i<num;i++)temp+=gs_tCliDev.plentbl[i];
    return temp;
}

//从str中得到函数参数
cliU8 cli_get_fparam(cliU8*str,cliU8 *parn)
{
    cliU8 i,type;
    cliU32 res;
    cliU8 n=0;
    cliU8 len;
    cliU8 tstr[CLI_MAX_PARAM_LEN+1];//字节长度的缓存,最多可以存放PARM_LEN个字符的字符串
    for(i=0;i<CLI_MAX_CMD_PARM;i++)gs_tCliDev.plentbl[i]=0;//清空参数长度表
    while(*str!='(')//偏移到参数开始的地方
    {
        str++;
        if(*str=='\0')return CLI_CMDERR;//遇到结束符了
    }
    str++;//偏移到"("之后的第一个字节
    while(1)
    {
        i=cli_get_aparm(str,tstr,&type);    //得到第一个参数
        str+=i;                             //偏移
        switch(type)
        {
            case 0: //数字
                if(tstr[0]!='\0')               //接收到的参数有效
                {
                    i=cli_str2num(tstr,&res);   //记录该参数
                    if(i)return CLI_PARAMERR;       //参数错误.
                    *(cliU32*)(gs_tCliDev.parm+cli_get_parmpos(n))=res;//记录转换成功的结果.
                    gs_tCliDev.parmtype&=~(1<<n);   //标记数字
                    gs_tCliDev.plentbl[n]=4;        //该参数的长度为4
                    n++;                            //参数增加
                    if(n>CLI_MAX_CMD_PARM)return CLI_PARAMOVER;//参数太多
                }
                break;
            case 1://字符串
                len=cli_strlen(tstr)+1; //包含了结束符'\0'
                cli_strcopy(tstr,&gs_tCliDev.parm[cli_get_parmpos(n)]);//拷贝tstr数据到usmart_dev.parm[n]
                gs_tCliDev.parmtype|=1<<n;  //标记字符串
                gs_tCliDev.plentbl[n]=len;  //该参数的长度为len
                n++;
                if(n>CLI_MAX_CMD_PARM)return CLI_PARAMOVER;//参数太多
                break;
            case 0XFF://错误
                return CLI_PARAMERR;//参数错误
        }
        if(*str==')'||*str=='\0')break;//查到结束标志了.
    }
    *parn=n;    //记录参数的个数
    return CLI_OK;//正确得到了参数
}

#endif

