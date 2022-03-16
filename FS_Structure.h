/**** Specify Align value as 1byte to prevent compile alignment optimization *******/
#pragma pack(1)
typedef struct
{
    vu8     bJmpBoot[3]; 	//ofs:0. Typical ones are: 0xEB,0x3E,0x90.
    vu8     bOEMName[8]; 	//ofs:3. Typical e.g.: "MSWIN4.1".
    vu16    BPB_BytesPerSec;	//ofs:11. Number of bytes per sector.
    vu8     BPB_SecPerClus; 	//ofs:13. Number of sectors per cluster.
    vu16    BPB_ReservedSec; 	//ofs:14. Number of reserved sectors, from DBR to FAT.
    vu8     BPB_NumFATs; 	//ofs:16.
    vu16    BPB_RootEntCnt; 	//ofs:17. Number of FAT16 root directory entries.
    vu16    BPB_TotalSec; 	//ofs:19. Number of total sectors in the partition (used when <32M).
    vu8     BPB_Media; 		//ofs:21. Partition media identifier, generally 0xF8 for flash drives.
    vu16    BPB_FATSz16; 	//ofs:22. Number of sectors per FAT in FAT16. 0 in FAT32
    vu16    BPB_SecPerTrk; 	//ofs:24. Number of sectors per FAT16. vu16 BPB_SecPerTrk; 	//ofs:24.
    vu16    BPB_NumHeads; 	//ofs:26. Number of heads.
    vu32    BPB_HiddSec; 	//ofs:28. Number of hidden sectors, from MBR to DBR.
    vu32    BPB_TotalSec32; 	//ofs:32. Total number of sectors in the partition (used when >=32M).
    vu32    BPB_FATSz32; 	//ofs:36. Number of sectors per FAT in FAT32. 0 in FAT16
    vu16    BPB_ExtFlags32; //ofs:40.
    vu16    BPB_FSVer32;	//ofs:42. FAT32 version number
    vu32    BPB_RootClus;	//ofs:44. FAT32 cluster number of the first cluster where the root directory is located
    vu16    BPB_FSInfo;		//ofs:48. The number of sectors occupied by FSInfo in the FAT32 reserved area".
    vu32    BPB_BK32; //ofs:50
    vu8     FileSysType[8]; 	//ofs:54. "FAT32".
    vu8     ExecutableCode[448];//ofs:62.
    vu16    EndingFlag; 	//ofs:510. end flag:0xAA55.
} FAT32_BPB;

typedef struct
{
    vu32    FSI_LeadSig; 	//ofs:0. FSI flag 0x41615252
    vu8     FSI_Reserved1[480]; //ofs:4. This field is reserved All zeros
    vu32    FSI_Strucsig; 	//ofs:484 FSI flag 0x61417272
    vu32    FSI_Free_Count; 	//ofs:488 Holds the latest number of remaining clusters
    // 		if 0xffffffff, then recalculate
    vu32    FSI_NxtFree; 	//ofs:492 Save the next remaining cluster number
    // 		if 0xffffffff, recalculate
    vu8     FSI_Reserved2[12]; 	//ofs:496 Reserved
    vu32    FSI_Trailsig; 	//ofs:508 FSI end flag 0xAA550000
} FAT32_FSinfo;

typedef struct
{
    u8  FileName[8]; 		//ofs:0. filename OEM character
    u8  ExtName[3]; 		//ofs:8. extension name
    u8  Attribute; 		//ofs:11. file attribute. Typical values: archive (0x20), volume label (0x08).
    u8  NT_Res; 		//ofs:12. Reserved
    u8  CtrTimeTeenth; 		//ofs:13. creation time (milliseconds)
    u16 CtrTime; 		//ofs:14. creation time
    u16 CtrDate; 		//ofs:16. creation date
    u16 LastAccDate; 		//ofs:18. last access time
    u16 FstClusHI; 		//ofs:20. file start cluster number high
    u16 WrtTime; 		//ofs:22. last write time
    u16 WrtDate; 		//ofs:24. last write date
    u16 FstClusLO; 		//ofs:26. fileStartClusterNumberLow
    u32 FileLength; 		//ofs:28. file length
} DIR_tag;

typedef struct
{
    u8  Ord;        //ofs:0. Serial number of this long directory item in this group
    u8  Name1[10];  //ofs:1. 1st~5th character of the long file name sub-item Unicode character
    u8  Attr;       //ofs:11. Attribute must be Long_Name 0x0F
    u8  Type;       //ofs:12. Generally zero
    u8  ChkSum;     //ofs:13. Short filename checksum
    u8  Name2[12];  //ofs:14. 6~11th character of long filename subparagraph Unicode character
    u16 FstClusLO;  //ofs:26. meaningless here must be zero
    u8  Name3[4];   //ofs:28. 12th~13th character of long file name sub-item Unicode character
} LongDir_Ent;
#pragma pack()
