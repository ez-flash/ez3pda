/*fat16.h	define some structure of fat16*/
#ifndef __FAT16_H
#define __FAT16_H

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef WORD
typedef unsigned short WORD;
#endif

#ifndef DWORD
typedef unsigned long DWORD;
#endif

struct _BPB
{
	BYTE NumFATs;	//The count of FAT data structures on the volume.

	BYTE SecPerClus;	//Number of sectors per allocation unit.

	WORD BytsPerSec;	//Count of bytes per sector.

	WORD RsvdSecCnt;	//Number of reserved sectors in the Reserved region of the volume starting at the first sector of the volume.

	WORD RootEntCnt;	//this field contains the count of 32-byte directory entries in the root directory.

	DWORD TotSec;	//This field is the total count of sectors on the volume.

	DWORD FATSz;	//This field is the FAT12/FAT16 16-bit count of sectors occupied by ONE FAT.
};

#define ERCHAR	0xe5

struct _DIR
{
	BYTE Name[11];
	BYTE Attr;
	BYTE NTRes;
	BYTE CrtTimeTenth;
	WORD CrtTime;
	WORD CrtDate;
	WORD LstAccDate;
	WORD FstClusHI;
	WORD WrtTime;
	WORD WrtDate;
	WORD FstClusLO;
	DWORD FileSize;
};
/* Up to 13 characters of the name */
struct dir_slot {
	BYTE    id;		/* sequence number for slot */
	BYTE    name0_4[10];	/* first 5 characters in name */
	BYTE    attr;		/* attribute byte */
	BYTE    reserved;	/* always 0 */
	BYTE    alias_checksum;	/* checksum for 8.3 alias */
	BYTE    name5_10[12];	/* 6 more characters in name */
	WORD   start;		/* starting cluster number, 0 in long slots */
	BYTE    name11_12[4];	/* last 2 characters in name */
};
#define maxNameLen 260  

#define ATTR_EXT		0x0F
#define ATTR_READ_ONLY	0x01
#define ATTR_HIDDEN		0x02
#define ATTR_SYSTEM		0x04
#define ATTR_VOLUME_ID	0x08
#define ATTR_DIRECTORY	0x10
#define ATTR_ARCHIVE	0x20

//The sector number of the first sector of that cluster.
//FirstSectorofCluster = ((N – 2) * BPB_SecPerClus) + FirstDataSector;
#define FirstSectorofCluster(N)	(((N – 2) * Bpb.SecPerClus) + FirstDataSector)

//int fat_init();
int fat_init(int useEram);
void fat_deinit();

//
//FAT16 Apis
//

struct _stat
{
	BYTE Attr;
	BYTE CrtTimeTenth;
	WORD CrtTime;
	WORD CrtDate;
	WORD LstAccDate;
	WORD WrtTime;
	WORD WrtDate;
	DWORD FileSize;
};

struct _file
{
	int valid; // 1 valid, 0 free.

	DWORD DirSectorNum;
	int DirIndex;

	DWORD StartSectorNum;
	DWORD CurrentSectorNum;
	DWORD SectorOffset;

	struct _DIR dir;

	DWORD offset;
	DWORD longfilename ;
	DWORD longfisrtSector ;
	DWORD longfisrtSectoroffset ;
};

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct FatDate
{
	WORD Day : 5;
	WORD Month : 4;
	WORD Year : 7;
};

struct FatTime
{
	WORD Second_2s : 5;
	WORD Minutes : 6;
	WORD Hours : 5;
};

struct FatDateTime
{
	union
	{
		struct FatDate fatdate;
		WORD Date;
	}Date;

	union
	{
		struct FatTime fattime;
		WORD Time;
	}Time;

	BYTE TimeTenth;
};

struct FatGet
{
	DWORD DirSectorNum;
	int DirIndex;

	int IsRootDir;

	char filename[maxNameLen];
	
	DWORD longfilename ;
	DWORD longfisrtSector ;
	DWORD longfisrtSectoroffset ;
};

extern struct _file handles[16];
extern struct FatGet gfat_get;

int fat_mkdir( const char *dirname );
int fat_rmdir( const char *dirname );
int fat_getfirst(const char *path, char* filename);
int fat_getnext(char* filename);

int fat_close_withouwrit(int handle);
int fat_close(int handle);
int fat_creat(const char* filename, BYTE attribute);
long fat_lseek(int handle, long offset, int origin);
int fat_open(const char* filename);
unsigned int fat_read(int handle, void* buffer, unsigned int bytes);
unsigned int fat_write(int handle, const char* buffer, unsigned int bytes);

int fat_remove( const char *filename);
int fat_get_stat( const char *filename, struct _stat* stat);
int fat_set_stat( const char *filename, struct _stat* stat);
int fat_rename( const char *oldname, const char *newname );
/*
//长文件名函数
wchar_t *vfat_unistrchr(const wchar_t *s, const wchar_t c);
int vfat_is_used_badchars(const wchar_t *s, int len);
int vfat_valid_longname(const unsigned char *name, unsigned int len);
*/
int strnicmp(const char *s1, const char *s2, int len) ;
int stricmp(const char *s1, const char *s2) ;

#endif //__FAT16_H