#include "stm32f10x_conf.h"
#include "stm32f10x.h"
#include "SDCard.h"
#include "FATFS.H"
#include "M25P16.h"

/*
Function: Underlying interface function
            Read memory sectors, default 512 bytes per sector
Parameters: u32 Addr absolute address of the first byte of the sector
            u32 *DataBuf data buffer, the data read
Return value: void
*/
void Disk_ReadSec(u32 Addr, u32 *DataBuf)
{
    SD_ReadBlock(Addr, DataBuf, BYTES_PERSEC);
}

/*
Function: Underlying interface function
            Write memory sector, default 512 bytes per sector
Parameters: u32 Addr absolute address of the first byte of the sector
            u32 *DataBuf data buffer, the data to be written to the memory
Return value: void
*/
void Disk_WriteSec(u32 Addr, u32 *DataBuf)
{
    SD_WriteBlock(Addr, DataBuf, BYTES_PERSEC);
}

/*
Function: Convert Unicode code to GB2312 code for reading long file names
            The implementation method is to query the Unicode to GB2312 conversion table
            The conversion table is stored on M25P16 flash memory chip.
Parameter: u16 Unicode 16-bit Unicode data
Return value : u16 GB2312 16-bit GB2312 data
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


