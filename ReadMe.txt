轻量级FAT32文件系统。功能有：读取文件（夹）、创建新文件（夹）、向文件中写入数据。

向上层提供的API函数声明在FATFS.H中，如下：
FS_Status   Search_inDir(FS_Object *CurDir, FS_Object *Target, u8 *Target_Name, u8 Object);
u8         *ReadFile(FS_Object *Target_File);
void        SetFileClustoFst(FS_Object *Target_File);
FS_Status   OpenFile(FS_Object *Target, u8 *FullName);
void        LsDir(FS_Object *CurDir);

FS_Status   CreateNewObject(FS_Object *CurDir, FS_Object *Target, u8 *Target_Name, u8 Object);
void        WriteFile(FS_Object *Target_File, u8 *dataBuf, u32 dataLength);

移植时需实现的底层接口函数在Interface.C中。声明如下：
void Disk_ReadSec(u32 Addr, u32 *DataBuf);

void Disk_WriteSec(u32 Addr, u32 *DataBuf);


u16 UnicodeToGb2312(u16 Unicode);