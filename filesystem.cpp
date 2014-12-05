#include <iostream>
#include <string>
#include <string.h>
#include <cstdint>
using namespace std;

// commands
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

// modes
#define M_ERROR	0
#define M_READ	1
#define	M_WRITE	2
#define	M_RW	3

// DIR file attributes (taken from writeup)
#define ATTR_READ_ONLY	0x01
#define ATTR_HIDDEN	0x02
#define ATTR_SYSTEM	0x04
#define ATTR_VOLUME_ID	0x08
#define ATTR_DIRECTORY	0x10
#define ATTR_ARCHIVE	0x20
#define ATTR_LONG_NAME	ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID

// operation results
#define RESULT_WAIT	0x0000
#define RESULT_OK	0xFFF0
#define RESULT_ERROR	0xFFFE

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

struct DIR {
	uint8_t 	DIR_Name[11];
	uint8_t 	DIR_Attr;
	uint8_t 	DIR_NTRes;
	uint8_t 	DIR_CrtTimeTenth;
	uint16_t 	DIR_CrtTime;
	uint16_t 	DIR_CrtDate;
	uint16_t 	DIR_LstAccDate;
	uint16_t 	DIR_FstClusHI;
	uint16_t 	DIR_WrtTime;
	uint16_t 	DIR_WrtDate;
	uint16_t 	DIR_FstClusLO;
	uint32_t 	DIR_FileSize;
} __attribute__((packed));

int fsinfo(void);
int open(string file_name, string mode);
int close(string file_name);
int create(string file_name);
int read(string file_name, uint start_pos, uint num_bytes);
int write(string file_name, uint start_pos, string quoted_data);
int rm(string file_name);
int cd(uint&, uint&, char*);
int ls(uint currentcluster);
int mkdir(string dir_name);
int rmdir(string dir_name);
int size(string entry_name);
int undelete();

// helper functions
int parsecommand(string);
int getmode(string);
long getsectoroffset(long);
long getfirstsector(uint);
uint getfat(uint);
long getsectorbytes(void);
struct DIR getdir(uint, char*);
uint getdircluster(uint, char*);
uint unraveldirectory(uint, char*);

FILE *file;
struct BPB32 BPB32;
struct FSI FSI;
struct DIR DIR;

int main(int argc, char* argv[])
{
	int halt = 0;
	string cmd, imgname, working, parent;
	uint currentcluster, parentcluster;
	char *token, *temp, *arg1, *arg2;

	if(argc != 2) { printf("./a.out [image]\n"); exit(0); }
	
	imgname = argv[1];

	file = fopen(imgname.c_str(), "rb+");
	fread(&BPB32, sizeof(struct BPB32), 1, file);

	working = '/' + '\0'; // TODO actually use this to print out working directory
	parent = '\0';

	currentcluster = BPB32.BPB_RootClus;
	parentcluster = -1;
	
	while(halt == 0) {
		cout << "prompt$ ";

		// read & tokenize input
		getline(cin, cmd);
		temp = strdup(cmd.c_str());
		token = strtok(temp, " \n");

		// do command (q to quit)
		switch(parsecommand(token)) {
			case FSINFO: fsinfo(); break;
			case OPEN: { // TODO wrong one to start with lmao
				arg1 = strtok(NULL, " \n");
				arg2 = strtok(NULL, " \n");
				//printf("TEST - [%s] [%s]\n",arg1,arg2);
				//open(arg1,arg2);
				break;
			}
			case CLOSE: break;
			case CREATE: break;
			case READ: break;
			case WRITE: break;
			case RM: break;
			case CD: {
				int res = RESULT_WAIT;
				arg1 = strtok(NULL, " \n");
				
				if(arg1 == NULL) {
					printf(" No directory specified.\n");
				} else {	
					//printf(" arg1 is [%s]\n", arg1);
					res = cd(currentcluster, parentcluster, arg1);
				}

				if(res == RESULT_ERROR) {
					printf(" Directory not found!\n");
				}
				break;
				}
			case LS: { // TODO test ls on parent directories later
				int res = RESULT_WAIT;
				arg1 = strtok(NULL, " \n");

				if(arg1 == NULL) { 
					res = ls(currentcluster);
				} else if(strcmp(arg1,".") == 0) {
					res = ls(currentcluster);
				} else if(strcmp(arg1,"..") == 0) {
					if(parentcluster == -1) {
						res = RESULT_ERROR;
					} else {
						res = ls(parentcluster);
					}
				} else {
					res = ls(unraveldirectory(currentcluster,arg1));
				}

				if(res == RESULT_ERROR) { 
					printf(" Directory not found!\n");
				}
				break;
			}				
			case MKDIR: break;
			case RMDIR: break;
			case SIZE: break;
			case UNDELETE: break;
			case QUIT: halt = 1; break;	// break out of loop
		}

		//printf("### %s %d\n",token, parsecommand(token));
	}

	return 0;
}

// main functions
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

int open(string file_name, string mode) {

}

int cd(uint &currentcluster, uint &parentcluster, char* dir_name) {
	struct DIR TEMPDIR;
	uint temp1, temp2;
	char parentstring[3] = {'.','.','\0'}; // idk, g++ yells at me if i don't do this

	if(currentcluster == 0x0) { // ends up as 0 for some reason, set as RootClus
		currentcluster = BPB32.BPB_RootClus;
	}

	temp1 = currentcluster;
	temp2 = parentcluster;
	
	if(strcmp(dir_name,"/") == 0) { // root
		currentcluster = BPB32.BPB_RootClus;
		parentcluster = -1;
		return RESULT_OK;
	} else if(strcmp(dir_name,".") == 0) { // do nothing?
		return RESULT_OK;
	} else if(strcmp(dir_name, "..") == 0) {
		if(currentcluster != BPB32.BPB_RootClus) { // do nothing if root
			parentcluster = getdircluster(currentcluster, parentstring);
			currentcluster = parentcluster;			
		} else {
			currentcluster = BPB32.BPB_RootClus;
			parentcluster = -1;
		}
		return RESULT_OK;
	}

	TEMPDIR = DIR;
	currentcluster = unraveldirectory(currentcluster, dir_name);
	parentcluster = getdircluster(currentcluster, parentstring);

	DIR = getdir(currentcluster,dir_name);

	if(DIR.DIR_Attr == ATTR_DIRECTORY) {
		parentcluster = getdircluster(currentcluster, parentstring);
		return RESULT_OK;
	} else {
		currentcluster = temp1; // something went wrong, revert to original values
		parentcluster = temp2;
		DIR = TEMPDIR;
		return RESULT_ERROR;
	}
}

int ls(uint currentcluster) {
	long offset;
	char fname[9];
	char ext[4];

	//printf(" cluster = [%d]\n",currentcluster);

	if(currentcluster == RESULT_ERROR) {
		return RESULT_ERROR; // directory not found?
	}

	if(currentcluster == 0x0) {
		currentcluster = 2;
	}

	while(1) {
		offset = getsectoroffset(getfirstsector(currentcluster));
		fseek(file, offset, SEEK_SET);

		while(offset < getsectoroffset(getfirstsector(currentcluster))+getsectorbytes()) {
			fread(&DIR, sizeof(struct DIR), 1, file);
			offset+=32;

			if(DIR.DIR_Name[0] == 0) { // Empty entry, go to next iteration
				continue;
			} else if(DIR.DIR_Name[0] == 0xE5) { // Last entry, break from loop
				break;
			} else if(DIR.DIR_Name[0] == 0x5) { // Some kanji fix, I dunno, it's in the writeup
				DIR.DIR_Name[0] == 0xE5;
			}

			if(DIR.DIR_Attr == ATTR_DIRECTORY || DIR.DIR_Attr == ATTR_ARCHIVE) {
				//printf("*** [%d]",DIR.DIR_Attr);

				for(int i = 0; i < 8; i++) { // Copy filename into char array, break on last character
					if(DIR.DIR_Name[i] != ' ') {
						fname[i] = DIR.DIR_Name[i];
					} else {
						fname[i] = '\0';
						break;
					}
				}

				fname[8] = '\0'; // Make sure last character is null terminator

				for(int i = 0; i < 3; i++) { // Copy extension
					if(DIR.DIR_Name[i+8] != ' ') {
						ext[i] = DIR.DIR_Name[i+8];
					} else {
						ext[i] = '\0';
						break;
					}
				}

				ext[3] = '\0';

				if (strcmp(fname, ".") != 0 && strcmp(fname, "..") != 0) {
					if(ext[0] != '\0') {
						printf(" %s.%s\n",fname,ext); // extension
					} else {
						if(DIR.DIR_Attr == ATTR_DIRECTORY) { // no extension
							printf("\x1b[36m %s\x1b[0m\n",fname); // color output for directories
						} else {
							printf(" %s\n",fname);
						}
					}
				}
			}
		}
	
		currentcluster = getfat(currentcluster);
		if (currentcluster >=  0x0FFFFFF8) { // EOF
			break;
		}
	}

	return RESULT_OK;
}



// HELPERS
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

int getmode(string mode) {
	if(mode == "r") {
		return M_READ;	
	} else if (mode == "w") {
		return M_WRITE;
	} else if (mode == "rw") {
		return M_RW;	
	}

	return M_ERROR;
}

long getsectoroffset(long sector) {
	return sector*BPB32.BPB_BytsPerSec;
}

long getfirstsector(uint cluster) {
	uint adjusted = cluster - 2;
	long sectors = adjusted * BPB32.BPB_SecPerClus;

	return sectors + BPB32.BPB_RsvdSecCnt + (BPB32.BPB_NumFATs*BPB32.BPB_FATSz32);
}

uint getfat(uint cluster) {
	uint nextcluster;
	long offset = BPB32.BPB_RsvdSecCnt*BPB32.BPB_BytsPerSec + cluster*4;

	fseek(file, offset, SEEK_SET);
	fread(&nextcluster, sizeof(uint), 1, file);

	return nextcluster;
}

long getsectorbytes() {
	return BPB32.BPB_BytsPerSec*BPB32.BPB_SecPerClus;
}

struct DIR getdir(uint cluster, char* dirname) {
	long offset;
	char fname[12];
	struct DIR TEMPDIR;

	TEMPDIR = DIR;

	while(1) {
		offset = getsectoroffset(getfirstsector(cluster));
		fseek(file, offset, SEEK_SET);

		while(offset < getsectoroffset(getfirstsector(cluster))+getsectorbytes()) {

			fread(&DIR, sizeof(struct DIR), 1, file);
			offset += 32;

			if(DIR.DIR_Name[0] == 0x0) {
				continue;
			} else if(DIR.DIR_Name[0] == 0xE5) {
				break;
			} else if(DIR.DIR_Name[0] == 0x5) {
				DIR.DIR_Name[0] = 0xE5;
			}

			if(DIR.DIR_Attr == ATTR_DIRECTORY || DIR.DIR_Attr == ATTR_ARCHIVE) {
				for(int i = 0; i < 11; i++) {
					fname[i] = '\0';
				}
				
				for(int i = 0; i < 11; i++) {
					if(DIR.DIR_Name[i] != ' ') {
						fname[i] = DIR.DIR_Name[i];
					} else {
						break;
					}
				}

				//printf("### GETDIR COMPARE [%s] [%s]\n",fname,dirname);
				if(strcmp(fname, dirname) == 0) {
					return DIR;
				}
			}

			//offset += 32;
		}

		cluster = getfat(cluster);
		if (cluster >=  0x0FFFFFF8) { // EOF
			break;
		}
		
	}

	DIR = TEMPDIR; // something went wrong, revert to original value
	return DIR;
}

uint getdircluster(uint cluster, char* dirname) {
	long offset;
	char fname[12];

	while(1) {
		offset = getsectoroffset(getfirstsector(cluster));
		fseek(file, offset, SEEK_SET);

		while(offset < getsectoroffset(getfirstsector(cluster))+getsectorbytes()) {

			fread(&DIR, sizeof(struct DIR), 1, file);
			offset += 32;

			if(DIR.DIR_Name[0] == 0) {
				//offset += 32;
				continue;
			} else if(DIR.DIR_Name[0] == 0xE5) {
				break;
			} else if(DIR.DIR_Name[0] == 0x5) {
				DIR.DIR_Name[0] = 0xE5;
			}

			if(DIR.DIR_Attr == ATTR_DIRECTORY) {
				for(int i = 0; i < 11; i++) {
					fname[i] = '\0';
				}
				
				for(int i = 0; i < 11; i++) {
					if(DIR.DIR_Name[i] != ' ') {
						fname[i] = DIR.DIR_Name[i];
					} else {
						break;
					}
				}
				

				if(strcmp(fname, dirname) == 0) {
					return (DIR.DIR_FstClusHI << 16 | DIR.DIR_FstClusLO);
				}
			}

			//offset += 32;
		}

		cluster = getfat(cluster);
		if (cluster >=  0x0FFFFFF8) { // EOF
			break;
		}
		
	}

	return RESULT_ERROR;
}

uint unraveldirectory(uint cluster, char* dirname) {
	char* token;
	char* rest;

	if(cluster == RESULT_ERROR) {
		return RESULT_ERROR;
	}

	token = strtok(dirname, " /\n");
	rest = strtok(NULL, " \n");

	//printf(" ### TOKEN [%s]\n",token);

	if(rest == NULL) {
		return getdircluster(cluster, token);
	} else {
		unraveldirectory(getdircluster(cluster,token), rest);
	}
}
