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
	uint32_t 	FSI_Nxt_Free;
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
int create(uint, char*);
int read(string file_name, uint start_pos, uint num_bytes);
int write(string file_name, uint start_pos, string quoted_data);
int rm(uint, char*);
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
struct DIR setdir(uint, char*);
uint getdircluster(uint, char*);
uint unraveldirectory(uint, char*);
void setcluster(uint, uint);
long getunusedentry(uint);
long getentryoffset(uint, char*);
void emptycluster(uint);
void printcluster(uint);

// globals
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

		if(currentcluster == 0) {
			currentcluster = 2;
		}

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
			case CREATE: {
				int res = RESULT_WAIT;
				arg1 = strtok(NULL, " \n");
				
				if(arg1 == NULL) {
					printf(" No file name specified.\n");
				} else {	
					res = create(currentcluster, arg1);
				}

				if(res == RESULT_ERROR) {
					printf(" File already exists!\n");
				} else if(res == RESULT_OK) {
					printf(" File created.\n");
				}
				break;
				}
			case READ: break;
			case WRITE: break;
			case RM: {
				int res = RESULT_WAIT;
				arg1 = strtok(NULL, " \n");
				
				if(arg1 == NULL) {
					printf(" No file name specified.\n");
				} else {	
					res = rm(currentcluster, arg1);
				}

				if(res == RESULT_ERROR) {
					printf(" Can't delete file.\n");
				} else if(res == RESULT_OK) {
					printf(" File removed.\n");
				}
				break;
				}
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
			case LS: {
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
			case 32: printcluster(currentcluster); break;
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

int create(uint currentcluster, char* file_name) {
	long offset;
	uint newcluster;
	char fname[12];
	int temp;
	struct DIR EMPTY;

	if(currentcluster == 0) {
		currentcluster = 2;
	}

	// prepare file name before creating
	for(int i = 0; file_name[i] != '\0'; i++) {
		file_name[i] = toupper(file_name[i]); // convert lowercase to upper
	}

	for(int i = 0; i < 8; i++) { // read in name portion of file name into buffer
		if(file_name[i] != '\0' && file_name[i] != '.') {
			fname[i] = file_name[i];
		} else {
			temp = i;
			break;
		}
	}

	for(int i = temp; i < 8; i++) { // fill rest of name portion with spaces
		fname[i] = ' ';
	}

	if(file_name[temp++] == '.') { // deal with extensions
		for(int i = 8; i < 11; i++) {
			if(file_name[temp] != '\0') {
				fname[i] = file_name[temp++];
			} else {
				temp = i;
				break;
			}

			if(i == 10) {
				temp = ++i;
			}
		}

		for( ; temp < 11; temp++) { // found dot, not an extension
			fname[temp] = ' ';
		}
	} else { // no extension
		for( ; temp < 11; temp++) {
			fname[temp] = ' ';
		}
	}

	fname[11] = '\0';
	DIR = getdir(currentcluster,fname);

	//printf("NAME: [%s]\n", DIR.DIR_Name);

	if(DIR.DIR_Name[0] == 0) { // last entry
		offset = getunusedentry(currentcluster); // create file at next empty entry
		for(int i = 0; i < 11; i++) {
			EMPTY.DIR_Name[i] = fname[i];
		}

		EMPTY.DIR_Attr = ATTR_ARCHIVE; // defaults for 0 size file
		EMPTY.DIR_NTRes = 0;
		EMPTY.DIR_FileSize = 0;

		fseek(file, BPB32.BPB_FSInfo*BPB32.BPB_BytsPerSec, SEEK_SET);
		fread(&FSI, sizeof(struct FSI), 1, file);

		if(FSI.FSI_Nxt_Free == 0xFFFFFFFF) {
			newcluster = 2;
		} else {
			newcluster = FSI.FSI_Nxt_Free+1;
		}

		while(1) {
			if(getfat(newcluster) == 0) {
				EMPTY.DIR_FstClusHI = (newcluster >> 16);
				EMPTY.DIR_FstClusLO = (newcluster & 0xFFFF);
				
				setcluster(0x0FFFFFF8, newcluster);
				FSI.FSI_Nxt_Free = newcluster;

				fseek(file, BPB32.BPB_FSInfo*BPB32.BPB_BytsPerSec, SEEK_SET);
				fwrite(&FSI, sizeof(struct FSI), 1, file);
				fflush(file);
				break;
			}			

			if(newcluster == 0xFFFFFFFF) {
				newcluster = 1;
			}

			newcluster++;
		}

		fseek(file, offset, SEEK_SET);
		fwrite(&EMPTY, sizeof(struct DIR), 1, file);
		fflush(file);
		
		return RESULT_OK;
	} else {
		return RESULT_ERROR; // already exists?
	}
}

int rm(uint currentcluster, char* file_name) {
	long offset;
	uint newcluster;
	char fname[12];
	int temp;
	struct DIR DIR2;
	struct DIR EMPTY;
	char blank[32];

	if(currentcluster == 0) {
		currentcluster = 2;
	}

	for(int i = 0; file_name[i] != '\0'; i++) { // same file name code from create()
		file_name[i] = toupper(file_name[i]);
	}

	for(int i = 0; i < 8; i++) {
		if(file_name[i] != '\0' && file_name[i] != '.') {
			fname[i] = file_name[i];
		} else {
			temp = i;
			break;
		}
	}

	for(int i = temp; i < 8; i++) {
		fname[i] = ' ';
	}

	if(file_name[temp++] == '.') {
		for(int i = 8; i < 11; i++) {
			if(file_name[temp] != '\0') {
				fname[i] = file_name[temp++];
			} else {
				temp = i;
				break;
			}

			if(i == 10) {
				temp = ++i;
			}
		}

		for( ; temp < 11; temp++) {
			fname[temp] = ' ';
		}
	} else {
		for( ; temp < 11; temp++) {
			fname[temp] = ' ';
		}
	}

	fname[11] = '\0';

	for(int i = 0; i < 32; i++) {
		blank[i] = '\0';
	}

	DIR = getdir(currentcluster, fname);
	offset = getentryoffset(currentcluster, fname);

	if(DIR.DIR_Name[0] != 0) {
		if(DIR.DIR_Attr == ATTR_ARCHIVE) {
			//printf(" ### RM\n");
			//printf(" ### offset [%lu]\n", offset);

			newcluster = (DIR.DIR_FstClusHI << 16 | DIR.DIR_FstClusLO);
			emptycluster(newcluster);
			fseek(file, offset, SEEK_SET);
			fwrite(&blank, 32, 1, file);

			return RESULT_OK;
		} else {
			return RESULT_ERROR;		
		}
	} else {
		return RESULT_ERROR;
	}
	
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

	TEMPDIR = DIR;
	currentcluster = unraveldirectory(currentcluster, dir_name);
	parentcluster = getdircluster(currentcluster, parentstring);

	DIR = setdir(parentcluster,dir_name);

	if(DIR.DIR_Attr == ATTR_DIRECTORY) {
		parentcluster = getdircluster(currentcluster, parentstring);
		return RESULT_OK;
	} else {
		currentcluster = temp1; // something went wrong, revert to original values
		parentcluster = temp2;
		//DIR = TEMPDIR;
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
	} else if (cmd == "c") {
		return 32;
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

	if(cluster == 0) {
		cluster = 2;
	}

	while(1) {
		offset = getsectoroffset(getfirstsector(cluster));
		fseek(file, offset, SEEK_SET);

		while(offset < getsectoroffset(getfirstsector(cluster))+getsectorbytes()) {
			//printf(" OFFSET [%lu] < [%lu]\n",offset,getsectoroffset(getfirstsector(cluster))+getsectorbytes());
			fread(&TEMPDIR, sizeof(struct DIR), 1, file);

			if(TEMPDIR.DIR_Name[0] == 0x0) {
				offset += 32;
				continue;
			} else if(TEMPDIR.DIR_Name[0] == 0xE5) {
				break;
			} else if(TEMPDIR.DIR_Name[0] == 0x5) {
				TEMPDIR.DIR_Name[0] = 0xE5;
			}

			if(TEMPDIR.DIR_Attr == ATTR_DIRECTORY || TEMPDIR.DIR_Attr == ATTR_ARCHIVE) {

				for(int i = 0; i < 11; i++) {
					fname[i] = TEMPDIR.DIR_Name[i];
				}

				fname[11] = '\0';

				//printf("### GETDIR COMPARE [%s] [%s]\n",fname,dirname);
				if(strcmp(fname, dirname) == 0) {
					//printf( "got it\n");
					return TEMPDIR;
				}
			}

			offset += 32;
		}

		cluster = getfat(cluster);
		if (cluster >=  0x0FFFFFF8) { // EOF
			break;
		}
		
	}

	//return TEMPDIR;
}

struct DIR setdir(uint cluster, char* dirname) {
	long offset;
	char fname[12];
	struct DIR TEMPDIR;

	TEMPDIR = DIR;

	if(cluster == 0) {
		cluster = 2;
	}

	while(1) {
		offset = getsectoroffset(getfirstsector(cluster));
		//printf(" OFFSET = [%lu]\n",offset);
		fseek(file, offset, SEEK_SET);

		while(offset < getsectoroffset(getfirstsector(cluster))+getsectorbytes()) {
			//printf(" OFFSET [%lu] < [%lu]\n",offset,getsectoroffset(getfirstsector(cluster))+getsectorbytes());

			fread(&DIR, sizeof(struct DIR), 1, file);

			if(DIR.DIR_Name[0] == 0x0) {
				offset += 32;
				continue;
			} else if(DIR.DIR_Name[0] == 0xE5) {
				break;
			} else if(DIR.DIR_Name[0] == 0x5) {
				DIR.DIR_Name[0] = 0xE5;
			}

			if(DIR.DIR_Attr == ATTR_DIRECTORY || DIR.DIR_Attr == ATTR_ARCHIVE) {

				for(int i = 0; i < 11; i++) {
					fname[i] = DIR.DIR_Name[i];
				}

				fname[11] = '\0';

				//printf("### GETDIR COMPARE [%s] [%s]\n",fname,dirname);
				if(strcmp(fname, dirname) == 0) {
					printf( "got it?\n");
					return DIR;
				}
			}

			offset += 32;
		}

		cluster = getfat(cluster);
		if (cluster >=  0x0FFFFFF8) { // EOF
			break;
		}
		
	}

	DIR = TEMPDIR; // something went wrong, revert to original value
	return TEMPDIR;
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

void setcluster(uint v, uint cluster) {
	long offset = BPB32.BPB_RsvdSecCnt*BPB32.BPB_BytsPerSec + cluster*4;

	fseek(file, offset, SEEK_SET);
	fwrite(&v, sizeof(uint), 1, file);
	fflush(file);
}

long getunusedentry(uint cluster) {
	uint nextcluster;
	long offset;	
	long fsioffset = BPB32.BPB_FSInfo*BPB32.BPB_BytsPerSec;

	while(1) {
		offset = getsectoroffset(getfirstsector(cluster));
		fseek(file, offset, SEEK_SET);

		while(offset < getsectoroffset(getfirstsector(cluster))+getsectorbytes()) {
			fread(&DIR, sizeof(struct DIR), 1, file);
			if((DIR.DIR_Name[0] == 0) || (DIR.DIR_Name[0] == 0xE5)) {
				return offset;
			}

			offset += 32;
		}

		if (getfat(cluster) >= 0x0FFFFFF8) {
			fseek(file, fsioffset, SEEK_SET);
			fread(&FSI, sizeof(struct FSI), 1, file);

			if(FSI.FSI_Nxt_Free == 0xFFFFFFFF) { // no hint for next free cluster, start at 2
				nextcluster = 2;
			} else {
				nextcluster = FSI.FSI_Nxt_Free + 1;
			}

			while(1) {
				if(getfat(nextcluster) == 0) {
					setcluster(nextcluster, cluster);
					setcluster(0x0FFFFFF8, nextcluster);
					FSI.FSI_Nxt_Free = cluster;
	
					fseek(file, BPB32.BPB_FSInfo*BPB32.BPB_BytsPerSec, SEEK_SET);
					fwrite(&FSI, sizeof(struct FSI), 1, file);
					fflush(file);
					break;
				}

				if(nextcluster == 0xFFFFFFFF) {
					nextcluster = 1;
				}

				nextcluster++;
			}

			cluster = nextcluster;
		} else {
			cluster = getfat(cluster);
		}
	}
}

long getentryoffset(uint cluster, char* filename) {
	char fname[12];
	long offset;

	if(cluster == 0) {
		cluster = 2;
	}

	while(1) {
		offset = getsectoroffset(getfirstsector(cluster));
		fseek(file, offset, SEEK_SET);

		while(offset < getsectoroffset(getfirstsector(cluster))+getsectorbytes()) {
			//printf(" OFFSET [%lu] < [%lu]\n",offset,getsectoroffset(getfirstsector(cluster))+getsectorbytes());

			fread(&DIR, sizeof(struct DIR), 1, file);

			if(DIR.DIR_Name[0] == 0x0) {
				offset += 32;
				continue;
			} else if(DIR.DIR_Name[0] == 0xE5) {
				break;
			} else if(DIR.DIR_Name[0] == 0x5) {
				DIR.DIR_Name[0] = 0xE5;
			}

			if(DIR.DIR_Attr == ATTR_DIRECTORY || DIR.DIR_Attr == ATTR_ARCHIVE) {

				for(int i = 0; i < 11; i++) {
					fname[i] = DIR.DIR_Name[i];
				}

				fname[11] = '\0';

				if(strcmp(fname, filename) == 0) {
					return offset;
				}
			}

			offset += 32;
		}

		cluster = getfat(cluster);
		if (cluster >=  0x0FFFFFF8) { // EOF
			break;
		}
		
	}
}

void emptycluster(uint cluster) {
	int empty[getsectorbytes()];
	uint zero = 0;
	long offset;

	for(int i = 0; i < getsectorbytes(); i++) {
		empty[i] = 0;
	}

	BPB32.BPB_RsvdSecCnt*BPB32.BPB_BytsPerSec + cluster*4;

	if(getfat(cluster) >= 2 && getfat(cluster) <= 0x0FFFFFEF) {
		emptycluster(getfat(cluster));
	}

	fseek(file, getsectoroffset(getfirstsector(cluster)), SEEK_SET);
	fwrite(file, getsectorbytes(), 1, file);
	fseek(file, offset, SEEK_SET);
	fwrite(&zero, sizeof(uint), 1, file);
}

void printcluster(uint cluster) {
	printf( "CURRENT CLUSTER: [%d]\n",cluster);
}
