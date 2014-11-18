#include <iostream>
#include <string>
#include <string.h>
#include <cstdint>
using namespace std;

#define FSINFO	 0
#define OPEN	 1
#define CLOSE	 2
#define CREATE	 3
#define READ	 4
#define WRITE	 5
#define RM	 6
#define CD	 7
#define LS	 8
#define MKDIR	 9
#define RMDIR	 10
#define SIZE	 11
#define UNDELETE 12
#define QUIT	 13
#define OTHER	 999

// structs (taken from FAT32 writeup)
struct BPB32 {
	uint8_t		BS_jmpBoot[3];
	uint8_t		BS_OEMName[8];
	uint16_t	BPB_BytsPerSec;
	uint8_t		BPB_SecPerClus;
	uint16_t	BPB_RsvdSecCnt;
	uint8_t		BPB_NumFATs;
	uint16_t	BPB_RootEntCnt;
	uint16_t	BPB_TotSec16;
	uint8_t		BPB_Media;
	uint16_t	BPB_FATSz16;
	uint16_t	BPB_SecPerTrk;
	uint16_t	BPB_NumHeads;
	uint32_t	BPB_HiddSec;
	uint32_t	BPB_TotSec32;
	uint32_t	BPB_FATSz32;
	uint16_t	BPB_ExtFlags;
	uint16_t	BPB_FSVer;
	uint32_t	BPB_RootClus;
	uint16_t	BPB_FSInfo;
	uint16_t	BPB_BkBootSec;
	uint8_t		BPB_Reserved[12];
	uint8_t		BS_DrvNum;
	uint8_t		BS_Reserved1;
	uint8_t		BS_BootSig;
	uint32_t	BS_VolID;
	uint8_t		BS_VolLab[11];
	uint8_t		BS_FilSysType[8];
} __attribute__((packed));

struct FSI {
	uint32_t 	FSI_LeadSig;
	uint8_t 	FSI_Reserved1[480];
	uint32_t 	FSI_StrucSig;
	uint32_t 	FSI_Free_Count;
	uint32_t 	FSI_Nxt_free;
	uint8_t		FSI_Reserved2[12];
	uint32_t 	FSI_TrailSig;
} __attribute__((packed));

int fsinfo(void);
int open(string file_name, string mode);
int close(string file_name);
int create(string file_name);
int read(string file_name, uint start_pos, uint num_bytes);
int write(string file_name, uint start_pos, string quoted_data);
int rm(string file_name);
int cd(string dir_name);
int ls(string dir_name);
int mkdir(string dir_name);
int rmdir(string dir_name);
int size(string entry_name);
int undelete();

int parsecommand(string);

FILE *file;
struct BPB32 BPB32;
struct FSI FSI;

int main(void)
{
	int halt = 0;
	string cmd;
	char *token, *temp;
	
	file = fopen("fat32.img", "rb+");
	fread(&BPB32, sizeof(struct BPB32), 1, file);
	
	while(halt == 0) {
		cout << "prompt$ ";

		// read & tokenize input
		getline(cin, cmd);
		temp = strdup(cmd.c_str());
		token = strtok(temp, " \n");

		// do command (q to quit)
		switch(parsecommand(token)) {
			case FSINFO: fsinfo(); break;
			case OPEN: break;
			case CLOSE: break;
			case CREATE: break;
			case READ: break;
			case WRITE: break;
			case RM: break;
			case CD: break;
			case LS: break;
			case MKDIR: break;
			case RMDIR: break;
			case SIZE: break;
			case UNDELETE: break;
			case QUIT: halt = 1; break;	// break out of loop
		}

		printf("### %s %d\n",token, parsecommand(token));
	}

	return 0;
}

int fsinfo(void) {
	long offset = BPB32.BPB_FSInfo*BPB32.BPB_BytsPerSec;

	fseek(file, offset, SEEK_SET);
	fread(&FSI, sizeof(struct FSI), 1, file);

	printf(" Bytes per sector: %d\n", BPB32.BPB_BytsPerSec);
	printf(" Sectors per cluster: %d\n", BPB32.BPB_SecPerClus);
	printf(" Total sectors: %d\n", BPB32.BPB_TotSec32);
	printf(" Number of FATs: %d\n", BPB32.BPB_NumFATs);
	printf(" Sectors per FAT: %d\n", BPB32.BPB_FATSz32);
	printf(" Number of free sectors: %d\n", FSI.FSI_Free_Count);

	return 0;
}

int parsecommand(string cmd) {
	if(cmd == "fsinfo") {
		return FSINFO;
	} else if (cmd == "open") {
		return OPEN;	
	} else if (cmd == "close") {
		return CLOSE;	
	} else if (cmd == "create") {
		return CREATE;	
	} else if (cmd == "read") {
		return READ;	
	} else if (cmd == "write") {
		return WRITE;	
	} else if (cmd == "rm") {
		return RM;	
	} else if (cmd == "cd") {
		return CD;	
	} else if (cmd == "ls") {
		return LS;	
	} else if (cmd == "mkdir") {
		return MKDIR;	
	} else if (cmd == "rmdir") {
		return RMDIR;	
	} else if (cmd == "size") {
		return SIZE;	
	} else if (cmd == "undelete") {
		return UNDELETE;	
	} else if (cmd == "quit" || cmd == "q") {
		return QUIT;	
	} else {
		return OTHER;
	}

}
