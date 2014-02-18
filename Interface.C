#include "stm32f10x_conf.h"
#include "stm32f10x.h"
#include "SDCard.h"
#include "FATFS.H"
#include "M25P16.h"

/*
函数功能：  底层接口函数
            读存储器扇区，默认每扇区512字节
参数    ：  u32 Addr        扇区第一字节的绝对地址
            u32 *DataBuf    数据缓存区，读取到的数据
返回值  ：  void
*/
void Disk_ReadSec(u32 Addr, u32 *DataBuf)
{
    SD_ReadBlock(Addr, DataBuf, BYTES_PERSEC);
}

/*
函数功能：  底层接口函数
            写存储器扇区，默认每扇区512字节
参数    ：  u32 Addr        扇区第一字节的绝对地址
            u32 *DataBuf    数据缓存区，将要写入存储器的的数据
返回值  ：  void
*/
void Disk_WriteSec(u32 Addr, u32 *DataBuf)
{
    SD_WriteBlock(Addr, DataBuf, BYTES_PERSEC);
}

/*
函数功能：  Unicode码转换为GB2312码，用于长文件名的读取
            实现方法是查询Unicode到GB2312的转换表
            此转换表保存在 M25P16 flash存储芯片上
参数    ：  u16 Unicode 16位Unicode数据
返回值  ：  u16 GB2312  16位GB2312数据
*/
u16 UnicodeToGb2312(u16 Unicode)
{
    u16 GB2312;
    u8 tem;
    u8 *temp = (u8 *)(&GB2312);
    Flash_Read(0x0A0000 + Unicode * 2, (u8 *)&GB2312, 2);

    /* Big endian to Little endian */
    tem = *(temp + 1);
    *(temp + 1) = *temp;
    *temp = tem;

    return GB2312;
}


