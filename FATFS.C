/*
***********************************************
         FATFS System Operating Library
              Only FAT32 Support
             Designed By SongJian
             All Rights Reserved
***********************************************
*/
#include "stm32f10x_conf.h"
#include "stm32f10x.h"
#include "FATFS.h"
#include "Interface.h"
#include "USART.H"
#include "FS_Structure.h"

static void CopyRam(const u8 *Source, u8 *Target, u16 Length);
static void Convert_LngNm(u8 *LngNm);
static void Convert_ShortNm(u8 *ShortNm);
static FS_Status CompareName(u8 *Name1, u8 *Name2);

static u32 Get_ClusAddr(u32 Clus);
static void ReadSec(u32 CurClus, u8 CurSec, u32 *Buffer);
static void WriteSec(u32 CurClus, u8 CurSec, u32 *Buffer);

static u32 Read_NextClusNum(u32 CurClus);
static u32  Write_NextClusNum(u32 LastClus);


u32 FS_Buffer[BYTES_PERSEC / sizeof(u32)];

FATFS FS;

FS_Object   RootDir;

/*
Function: File system initialization.
            Initializes the structure FS and gets the FAT file system parameters.
            Initialize the structure RootDir, get the parameters of root directory.
Parameter : void
Return Value : void
*/
void FATFS_Init(void)
{

    FAT32_BPB *BPB;
    BPB = (FAT32_BPB *)FS_Buffer;

    Disk_ReadSec(BPB_ADDR, FS_Buffer);

    FS.Sec_PerFAT = BPB->BPB_FATSz32;

    FS.Sec_PerClus = BPB->BPB_SecPerClus;
    //USART1_printf("Sec Per Clus:%d\r\n",FS.Sec_PerClus);

    FS.Bytes_PerClus = BYTES_PERSEC * FS.Sec_PerClus;

    //USART1_printf("Total Sec32:%d\r\n",BPB->BPB_TotalSec32);

    FS.FS_Size = BPB->BPB_TotalSec32 * BYTES_PERSEC;
    //USART1_printf("FS_Size:%dMB\r\n",FS.FS_Size>>20);

    FS.FAT_Addr = BPB->BPB_ReservedSec * BYTES_PERSEC;

    FS.Root_Clus = BPB->BPB_RootClus;

    FS.Data_Addr = FS.FAT_Addr +
                   BPB->BPB_FATSz32 * BPB->BPB_NumFATs * BYTES_PERSEC;

    RootDir.FstClus = FS.Root_Clus;
}

/*
Function: File system test
Parameter : void
Return Value : void
*/
void FS_Test(void)
{
    FS_Object TextFile;
    FS_Object TextDir;
    u8 str[] = "star dust!";

    CreateNewObject(&RootDir, &TextDir, "test", DIR);
    CreateNewObject(&TextDir, &TextFile, "text.txt", FILE);
    WriteFile(&TextFile, str, sizeof(str) - 1);

    if(Search_inDir(&RootDir, &TextDir, "picture", DIR) == FILE_EXIST)
    {
        LsDir(&TextDir);
    }
}

/*
Function: Read the next cluster number from the FAT
Parameters: CurClus Current Cluster Number
Return Value : NextClus Next Cluster Number
*/
u32 Read_NextClusNum(u32 CurClus)
{
    u32 NextClus;

    Disk_ReadSec(FS.FAT_Addr +
                 (CurClus / (BYTES_PERSEC / sizeof(u32))*
                  BYTES_PERSEC),
                 (u32 *)FS_Buffer);

    NextClus = *((u32 *)FS_Buffer +
                 (CurClus % (BYTES_PERSEC / sizeof(u32))));

    return NextClus;
}

/*
Function: Get the next blank cluster number when writing data to a file or folder
Parameters: LastClus Previous cluster number
Return value: New_ClusNum New cluster number to be written
*/
u32 Write_NextClusNum(u32 LastClus)
{
    u32     New_ClusNum;
    u32     FAT_SecAddr;
    u16     i;

    New_ClusNum = FS_EOF;

    for(FAT_SecAddr = FS.FAT_Addr;
            FAT_SecAddr < (FAT_SecAddr + FS.Sec_PerFAT * BYTES_PERSEC);
            FAT_SecAddr += BYTES_PERSEC)
    {
        Disk_ReadSec(FAT_SecAddr, FS_Buffer);

        for(i = 0; i < (BYTES_PERSEC / sizeof(u32)); i++)
        {
            if(*((u32 *)FS_Buffer + i) == 0)
            {
                New_ClusNum = (FAT_SecAddr - FS.FAT_Addr) / sizeof(u32) + i;
                *((u32 *)FS_Buffer + i) = FS_EOF;
                Disk_WriteSec(FAT_SecAddr, FS_Buffer);

                Disk_ReadSec(FAT_SecAddr + FS.Sec_PerFAT * BYTES_PERSEC, FS_Buffer);
                *((u32 *)FS_Buffer + i) = FS_EOF;
                Disk_WriteSec(FAT_SecAddr + FS.Sec_PerFAT * BYTES_PERSEC, FS_Buffer);

                if(LastClus != FST_CLUS)
                {
                    Disk_ReadSec(FS.FAT_Addr +
                                 (LastClus / (BYTES_PERSEC / sizeof(u32))*BYTES_PERSEC),
                                 (u32 *)FS_Buffer);
                    *((u32 *)FS_Buffer + (LastClus % (BYTES_PERSEC / sizeof(u32)))) = New_ClusNum;
                    Disk_WriteSec(FS.FAT_Addr +
                                  (LastClus / (BYTES_PERSEC / sizeof(u32))*BYTES_PERSEC),
                                  (u32 *)FS_Buffer);

                    Disk_ReadSec(FS.FAT_Addr + FS.Sec_PerFAT * BYTES_PERSEC +
                                 (LastClus / (BYTES_PERSEC / sizeof(u32))*BYTES_PERSEC),
                                 (u32 *)FS_Buffer);
                    *((u32 *)FS_Buffer + (LastClus % (BYTES_PERSEC / sizeof(u32)))) = New_ClusNum;
                    Disk_WriteSec(FS.FAT_Addr + FS.Sec_PerFAT * BYTES_PERSEC +
                                  (LastClus / (BYTES_PERSEC / sizeof(u32))*BYTES_PERSEC),
                                  (u32 *)FS_Buffer);
                }

                return New_ClusNum;
            }
        }
    }

    return New_ClusNum;
}

/*
Function: Read a sector
Parameters : CurClus Current cluster number
            CurSec Current sector number in the cluster
            Buffer Data Buffer
Return Value: void
*/
void ReadSec(u32 CurClus, u8 CurSec, u32 *Buffer)
{
    u32 SecAddr;
    SecAddr = Get_ClusAddr(CurClus) +
              CurSec * BYTES_PERSEC;
    Disk_ReadSec(SecAddr, Buffer);
}

/*
Function: Write a sector
Parameters : CurClus Current cluster number
            CurSec Current sector number in the cluster
            Buffer Data Buffer
Return Value: void
*/
void WriteSec(u32 CurClus, u8 CurSec, u32 *Buffer)
{
    u32 SecAddr;
    SecAddr = Get_ClusAddr(CurClus) +
              CurSec * BYTES_PERSEC;
    Disk_WriteSec(SecAddr, Buffer);
}

/*
Function: Get the memory address corresponding to the cluster number
Parameter : Clus Cluster Number
Return Value : Address
*/
u32 Get_ClusAddr(u32 Clus)
{
    return(FS.Data_Addr + (Clus - 2) * FS.Bytes_PerClus);
}

/*
Function: Memory Copy
Parameters : Source Data source address
            Target Data target address
            Length Data length
Return Value: void
*/
void CopyRam(const u8 *Source, u8 *Target, u16 Length)
{
    while(Length != 0)
    {
        Length--;
        *(Target + Length) = *(Source + Length);
    }
}

/*
Function: Memory area clear (set to 0)
Parameters : Target target address
            Length Target length
Return Value: void
*/
void ClearRam(u8 *Target, u16 Length)
{
    while(Length != 0)
    {
        Length--;
        *(Target + Length) = 0;
    }
}

/*
Function: Convert long file name, Unicode to Gb2312. Used to read the file name.
Parameter: LngNm long file name address
Return Value: void
*/
void Convert_LngNm(u8 *LngNm)
{
    u8  i, h;
    u8  LngNm_Buffer[106];
    u16 *LngNm_Unicode;
    LngNm_Unicode = (u16 *)LngNm;
    h = 0;

    for(i = 0; i < 106; i++)
    {
        if((i & 0x01) == 0)
        {
            LngNm_Unicode = (u16 *)(LngNm + i);

            if(*LngNm_Unicode > 0x00A0)
            {
                *LngNm_Unicode = UnicodeToGb2312(*LngNm_Unicode);
            }
            else if(*LngNm_Unicode == 0)
            {
                break;
            }
        }

        if(*(LngNm + i) != 0)
        {
            *(LngNm_Buffer + h) = *(LngNm + i);
            h++;
        }
    }

    *(LngNm_Buffer + h) = 0;
    CopyRam(LngNm_Buffer, LngNm, h + 1);
}

/*
Function: Convert short filenames, remove spaces from short filenames. Used to read the file name.
Arguments : ShortNm short filename address
Return Value: void
*/
void Convert_ShortNm(u8 *ShortNm)
{
    u8 i, h;
    u8 ShortNmBuffer[13];
    h = 0;

    for(i = 0; i < 8; i++)
    {
        if(*(ShortNm + i) == 0x20)
        {
            break;
        }

        *(ShortNmBuffer + h) = *(ShortNm + i);
        h++;
    }

    if(*(ShortNm + 8) != 0x20)
    {
        *(ShortNmBuffer + h) = '.';
        h++;

        for(i = 8; i < 11; i++)
        {
            if(*(ShortNm + i) == 0x20)
            {
                break;
            }

            *(ShortNmBuffer + h) = *(ShortNm + i);
            h++;
        }
    }

    *(ShortNmBuffer + h) = 0;
    CopyRam(ShortNmBuffer, ShortNm, h + 1);
}

/*
Function: Compare the file name, English case insensitive.
Parameters : Name1 Filename1 address
            Name2 filename2 address
Return value : SUCCESSED or FAILED
*/
FS_Status CompareName(u8 *Name1, u8 *Name2)
{
    u8 i;

    for(i = 0; i < 250; i++)
    {
        if(*(Name1 + i) > 128 || *(Name2 + i) > 128)
        {
            if(*(Name1 + i) != *(Name2 + i))
            {
                return FAILED;
            }
        }
        else
        {
            if(*(Name1 + i) > *(Name2 + i))
            {
                if((*(Name1 + i) - 32) != *(Name2 + i))
                {
                    return FAILED;
                }
            }

            if(*(Name2 + i) > *(Name1 + i))
            {
                if((*(Name2 + i) - 32) != *(Name1 + i))
                {
                    return FAILED;
                }
            }
        }

        if(*(Name1 + i) == 0 && *(Name2 + i) == 0)
        {
            return SUCCESSED;
        }
    }

    return FAILED;
}
/*
Function: Find the file(folder) in the folder according to the file(folder) name, if found, get the file(folder) parameter
Parameters : CurDir structure pointer, current folder
            Target structure pointer, target file(folder)
            Target_Name name of the target file(folder)
            Object FILE or DIR, used to distinguish whether the target is a file or a folder
Return value: FILE_EXIST or FILE_NOTEXIST

*/
FS_Status Search_inDir(FS_Object    *CurDir,
                       FS_Object    *Target,
                       u8           *Target_Name,
                       u8           Object)
{
    DIR_tag *DirTag;
    LongDir_Ent *LongDirEnt;
    u32     CurClus;
    u16     CurEntAddr;
    u8      CurSec, LngNmCnt, DirEntCnt;
    u16     Name_Buffer[53]; //Up to 4 directory entries totaling 52 Unicode characters
    u8      *Name;

    Name_Buffer[52] = 0;
    CurClus = CurDir->FstClus;
    LngNmCnt = 4;
    DirEntCnt = 4;

    do
    {
        /***** Read and Scan FS_Object Data Aera by Sector ******/
        for(CurSec = 0; CurSec < FS.Sec_PerClus; CurSec++)
        {
            ReadSec(CurClus, CurSec, FS_Buffer);

            /***** Scan Every DirEntry , 32 Bytes per DirEntry! *****/
            for(CurEntAddr = 0; CurEntAddr < BYTES_PERSEC; CurEntAddr += DIRENT_SIZE)
            {
                DirTag = (DIR_tag *)((u32)FS_Buffer + CurEntAddr);

                if(DirTag->FileName[0] == EMPTY)
                {
                    return  FILE_NOTEXIST;
                }

                if(DirTag->FileName[0] != DELETED)
                {
                    if(DirTag->Attribute == LONG_NAME)
                    {
                        if(LngNmCnt != 0)
                        {
                            LngNmCnt--;
                            LongDirEnt = (LongDir_Ent *)DirTag;
                            Name = (u8 *)Name_Buffer + LngNmCnt * 26;
                            CopyRam(LongDirEnt->Name1, Name, 10);
                            CopyRam(LongDirEnt->Name2, Name + 10, 12);
                            CopyRam(LongDirEnt->Name3, Name + 22, 4);
                        }
                    }
                    else if(DirTag->Attribute & Object)
                    {
                        if(LngNmCnt != 4)
                        {
                            LngNmCnt = 4;
                            Convert_LngNm(Name);
                        }
                        else
                        {
                            Name = (u8 *)Name_Buffer;
                            CopyRam(DirTag->FileName, Name, 11);
                            Convert_ShortNm(Name);
                        }

                        if(CompareName(Name, Target_Name) == SUCCESSED)
                        {
                            Target->Name    = Target_Name;
                            Target->Attrib  = DirTag->Attribute;
                            Target->Size    = DirTag->FileLength;
                            Target->FstClus = DirTag->FstClusHI;
                            Target->FstClus <<= 16;
                            Target->FstClus += DirTag->FstClusLO;
                            Target->DirEntryAddr = Get_ClusAddr(CurClus)
                                                   + CurSec * BYTES_PERSEC
                                                   + CurEntAddr;
                            Target->CurClus = Target->FstClus;
                            Target->CurSec = 0;
                            return FILE_EXIST;
                        }

                    }

                    if(LngNmCnt != 4)
                    {
                        DirEntCnt--;
                    }

                    if(LngNmCnt != DirEntCnt)
                    {
                        LngNmCnt = 4;
                        DirEntCnt = 4;
                    }
                }
            }
        }

        CurClus = Read_NextClusNum(CurClus);
    }
    while(CurClus != 0x0fffffff);

    return  FILE_NOTEXIST;
}

/*
Function: List all the items in the folder
Parameters : CurDir current folder
Return Value : void
*/
void LsDir(FS_Object *CurDir)
{
    DIR_tag     *DirTag;
    LongDir_Ent *LongDirEnt;
    u32     CurClus;
    u16     CurEntAddr;
    u8      CurSec, LngNmCnt, DirEntCnt;
    u16     Name_Buffer[53]; //Up to 4 directory entries totaling 52 Unicode characters
    u8      *Name;

    Name_Buffer[52] = 0;
    CurClus = CurDir->FstClus;
    LngNmCnt = 4;
    DirEntCnt = 4;

    do
    {
        /***** Read and Scan FS_Object Data Aera by Sector ******/
        for(CurSec = 0; CurSec < FS.Sec_PerClus; CurSec++)
        {
            ReadSec(CurClus, CurSec, FS_Buffer);

            /***** Scan Every DirEntry , 32 Bytes per DirEntry! *****/
            for(CurEntAddr = 0; CurEntAddr < BYTES_PERSEC; CurEntAddr += DIRENT_SIZE)
            {
                DirTag = (DIR_tag *)((u32)FS_Buffer + CurEntAddr);

                if(DirTag->FileName[0] == EMPTY)
                {
                    return;
                }

                if(DirTag->FileName[0] != DELETED)
                {
                    if(DirTag->Attribute == LONG_NAME)
                    {
                        if(LngNmCnt != 0)
                        {
                            LngNmCnt--;
                            LongDirEnt = (LongDir_Ent *)DirTag;
                            Name = (u8 *)Name_Buffer + LngNmCnt * 26;
                            CopyRam(LongDirEnt->Name1, Name, 10);
                            CopyRam(LongDirEnt->Name2, Name + 10, 12);
                            CopyRam(LongDirEnt->Name3, Name + 22, 4);
                        }
                    }
                    else if(DirTag->Attribute & (DIR | FILE))
                    {
                        if(DirTag->Attribute & DIR)
                        {
                            USART_SendStr(USART1, "Dir:");
                        }
                        else
                        {
                            USART_SendStr(USART1, "File:");
                        }

                        if(LngNmCnt != 4)
                        {
                            LngNmCnt = 4;
                            Convert_LngNm(Name);
                            USART_SendStr(USART1, Name);
                            USART1_printf("\r\n");
                        }
                        else
                        {
                            Name = (u8 *)Name_Buffer;
                            CopyRam(DirTag->FileName, Name, 11);
                            Convert_ShortNm(Name);
                            USART_SendStr(USART1, Name);
                            USART1_printf("\r\n");
                        }
                    }
                }

                if(LngNmCnt != 4)
                {
                    DirEntCnt--;
                }

                if(LngNmCnt != DirEntCnt)
                {
                    LngNmCnt = 4;
                    DirEntCnt = 4;
                }
            }
        }

        CurClus = Read_NextClusNum(CurClus);
    }
    while(CurClus != 0x0fffffff);

    return;
}

/*
Function: Open a file (folder). Find the file and get its parameters according to the full path given (e.g. "/English/Essays of Travel.txt").
Parameters: Target structure pointer, target file (folder).
            FullName Full path File (folder) name
Return value: FILE_EXIST or FILE_NOTEXIST
*/
FS_Status OpenFile(FS_Object *Target, u8 *FullName)
{
    u8  i, h;
    u8  NameBuf[106];
    h = 0;
    Target->FstClus = RootDir.FstClus;

    if(*FullName != '/')
    {
        return NAME_ERROR;
    }
    else
    {
        for(i = 1; * (FullName + i) != 0; i++)
        {
            if(*(FullName + i) != '/')
            {
                NameBuf[h] = *(FullName + i);
                h++;
            }
            else
            {
                NameBuf[h] = 0;

                if(Search_inDir(Target, Target, NameBuf, DIR) == FILE_NOTEXIST)
                {
                    return FILE_NOTEXIST;
                }

                h = 0;
            }
        }

        if(h == 0)
        {
            return FILE_EXIST;
        }
        else
        {
            NameBuf[h] = 0;

            if(Search_inDir(Target, Target, NameBuf, FILE) == FILE_NOTEXIST)
            {
                return FILE_NOTEXIST;
            }
            else
            {
                return FILE_EXIST;
            }
        }
    }
}

/*
Function: Read the entire file sequentially
Parameter: Target_File Target file
Return value : u8*
*/
u8 *ReadFile(FS_Object *Target_File)
{
    if(Target_File->CurClus == FS_EOF)
    {
        return (void *)0;
    }

    if(Target_File->CurSec == FS.Sec_PerClus)
    {
        Target_File->CurSec = 0;
        Target_File->CurClus = Read_NextClusNum(Target_File->CurClus);

        if(Target_File->CurClus == FS_EOF)
        {
            return (void *)0;
        }
    }

    ReadSec(Target_File->CurClus, Target_File->CurSec, FS_Buffer);
    Target_File->CurSec++;

    return (u8 *)FS_Buffer;
}

void SetFileClustoFst(FS_Object *Target_File)
{
    Target_File->CurClus = Target_File->FstClus;
    Target_File->CurSec = 0;
}

/*
Function: Write data to the file, append from the end of the file
Parameters : Target_File Target file
            dataBuf Data source
            dataLength Data length
Return Value : void
*/
void WriteFile(FS_Object *Target_File, u8 *dataBuf, u32 dataLength)
{
    DIR_tag *DirTag;
    u8  CurSec;
    u32 CurClus = Target_File->FstClus;
    u32 LstClus;
    u32 CurSec_Addr;
    u32 OfsInClus;
    u16 OfsInSec;
    u32 i = 0;

    do
    {
        LstClus = CurClus;
        CurClus = Read_NextClusNum(CurClus);
    }
    while(CurClus != FS_EOF);

    CurClus = LstClus;

    OfsInClus = Target_File->Size % FS.Bytes_PerClus;
    CurSec = OfsInClus / BYTES_PERSEC;
    OfsInSec = OfsInClus % BYTES_PERSEC;

    if(dataLength < (BYTES_PERSEC - OfsInSec))
    {
        CurSec_Addr = Get_ClusAddr(CurClus) + CurSec * BYTES_PERSEC;
        Disk_ReadSec(CurSec_Addr, FS_Buffer);

        for(; i < dataLength; i++, OfsInSec++)
        {
            *(u8 *)((u32)FS_Buffer + OfsInSec) = *(dataBuf + i);
        }

        Disk_WriteSec(CurSec_Addr, FS_Buffer);
    }
    else
    {
        do
        {
            if(CurSec == FS.Sec_PerClus)
            {
                CurClus = Write_NextClusNum(CurClus);

                if(CurClus == FS_EOF)
                {
                    break;
                }

                CurSec = 0;
            }

            CurSec_Addr = Get_ClusAddr(CurClus) + CurSec * BYTES_PERSEC;
            Disk_ReadSec(CurSec_Addr, FS_Buffer);

            for(; (OfsInSec < BYTES_PERSEC) && (i < dataLength); OfsInSec++, i++)
            {
                *(u8 *)((u32)FS_Buffer + OfsInSec) = *(dataBuf + i);
            }

            Disk_WriteSec(CurSec_Addr, FS_Buffer);

            OfsInSec = 0;
            CurSec++;
        }
        while(i < dataLength);
    }

    Target_File->Size += i;
    CurSec_Addr = Target_File->DirEntryAddr - (Target_File->DirEntryAddr % BYTES_PERSEC);
    Disk_ReadSec(CurSec_Addr, FS_Buffer);
    DirTag = (DIR_tag *)((u32)FS_Buffer + (Target_File->DirEntryAddr - CurSec_Addr));
    DirTag->FileLength = Target_File->Size;
    Disk_WriteSec(CurSec_Addr, FS_Buffer);
}

/*
Function: Write a short file (folder) name to the directory entry when creating a file (folder)
Parameters: Source file (folder) name address
            Target The address of the file name in the directory entry
Return Value : void
*/
void Write_ShortNm(u8 *Source, u8 *Target)
{
    u8 i = 0, h = 0;

    for(i = 0; i < 8; i++)
    {

        if((*(Source + h) != '.') && (*(Source + h) != 0))
        {
            *(Target + i) = *(Source + h);
            h++;
        }
        else
        {
            *(Target + i) = ' ';
        }

        if((*(Target + i) >= 'a') && (*(Target + i) <= 'z'))
        {
            *(Target + i) -= 0x20;
        }

    }

    if(*(Source + h) == '.')
    {
        h++;

        for(i = 8; i < 11; i++)
        {
            if(*(Source + h) != 0)
            {
                *(Target + i) = *(Source + h);
                h++;
            }
            else
            {
                *(Target + i) = ' ';
            }

            if((*(Target + i) >= 'a') && (*(Target + i) <= 'z'))
            {
                *(Target + i) -= 0x20;
            }
        }
    }
    else
    {
        for(i = 8; i < 11; i++)
        {
            *(Target + i) = ' ';
        }
    }
}

/*
Function: Create a new file(folder) in the folder and get the parameters of the newly created file(folder)
Parameters: CurDir current folder
            Target target file (folder), after the new file (folder) is created, the parameters are stored in this structure
            Target_Name The name of the target file (folder), the new file (folder) is named after it.
            Object FILE or DIR, used to distinguish whether it is a file or a folder that will be created
Return value: NAME_ERROR The file (folder) name is wrong, the file (folder) already exists.
            FILE_FAILED File (folder) creation failed, not enough space
            FILE_SUCCESSED File (folder) created successfully
*/
FS_Status CreateNewObject(FS_Object *CurDir, FS_Object *Target, u8 *Target_Name, u8 Object)
{
    DIR_tag *DirTag;
    u32     LastClus;
    u32     CurClus;
    u32     NewObject_FstClus;
    u8      CurSec;
    u16     CurEntAddr;
    u32     TotalClus;

    if(Search_inDir(CurDir, Target, Target_Name, Object) == FILE_NOTEXIST)
    {
        TotalClus = (FS.FS_Size - FS.Data_Addr) / FS.Bytes_PerClus;
        CurClus = CurDir->FstClus;

        do
        {
            /***** Read and Scan FS_Object Data Aera by Sector ******/
            for(CurSec = 0; CurSec < FS.Sec_PerClus; CurSec++)
            {
                ReadSec(CurClus, CurSec, FS_Buffer);

                /***** Scan Every DirEntry , 32 Bytes per DirEntry! *****/
                for(CurEntAddr = 0; CurEntAddr < BYTES_PERSEC; CurEntAddr += DIRENT_SIZE)
                {
                    DirTag = (DIR_tag *)((u32)FS_Buffer + CurEntAddr);

                    if((DirTag->FileName[0] == EMPTY) || (DirTag->FileName[0] == DELETED))
                    {
                        NewObject_FstClus = Write_NextClusNum(FST_CLUS);
                        ReadSec(CurClus, CurSec, FS_Buffer);
                        Write_ShortNm(Target_Name, DirTag->FileName);
                        DirTag->Attribute = Object;
                        DirTag->FstClusHI = (u16)(NewObject_FstClus >> 16);
                        DirTag->FstClusLO = (u16)NewObject_FstClus;
                        DirTag->FileLength = 0x00;
                        WriteSec(CurClus, CurSec, FS_Buffer);

                        Target->Name = Target_Name;
                        Target->Attrib = Object;
                        Target->Size = 0;
                        Target->FstClus = NewObject_FstClus;
                        Target->DirEntryAddr = Get_ClusAddr(CurClus) + CurSec * BYTES_PERSEC + CurEntAddr;
                        Target->CurClus = Target->FstClus;
                        Target->CurSec = 0;
                        return FILE_SUCCESSED;
                    }
                }
            }

            LastClus = CurClus;
            CurClus = Read_NextClusNum(CurClus);

            if(CurClus == FS_EOF)
            {
                CurClus = Write_NextClusNum(LastClus);
                USART1_printf("NextClusNum:0x%x", CurClus);

                if(CurClus == FS_EOF)
                {
                    return FILE_FAILED;
                }
            }
        }
        while(CurClus < TotalClus);

        return FILE_FAILED;
    }
    else
    {
        return NAME_ERROR;
    }
}

