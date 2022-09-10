#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

//data structure to store the information about the file
struct tarFileDesc {
	char name[20];
	char directory[300];
	long size;
};

//data structure to store the information about tarFile (total file count, etc)
struct tarStats {
	int fileCount;
};

//functions for the main tar operations
void createTar (char tarFile[], char directory[]);
void extract (char tarFile[]);
void extractSingle (char tarFile[], char file[]);
void list (char tarFile[]);

//utility function to create filePath from directory and name
void makeFilePath (char* path, char* directory, char* name) {
	strcpy(path, directory);
	strcat(path, "/");
	strcat(path, name);
}

//utility function to read totSize bytes of data from f_read in chunks of 512 bytes, and write them to f_write if toWrite is 1
//returns 0 on success, otherwise returns -1
int read512 (int f_read, int f_write, int toWrite, int totSize) {
	char *buffer = (char*)malloc(513 * sizeof(char));
	int readLength;
	int totRead = 0;
	//read 512Bytes of data at a time
	while (totRead + 512 < totSize) {
		readLength = read(f_read, buffer, 512 * sizeof(char));
		totRead += readLength;
		buffer[readLength] = '\0';
		if (toWrite) {
			if (write(f_write, buffer, readLength) != readLength) {
				return -1;
			}
		}
	}
	//read the last chunk
	readLength = read(f_read, buffer, totSize-totRead);
	buffer[readLength] = '\0';
	if (toWrite) {
		if (write(f_write, buffer, readLength) != readLength) {
			return -1;
		}
	}
		
	free(buffer);
	return 0;
}

int main(int argc, char *argv[])
{
	//no arguments mentioned
	if (argc == 1) {
		printf("No arguments mentioned\n");
		exit(-1);
	}
	//create tar file
	if (!strcmp(argv[1], "-c")) {
		if (argc != 4) {
			printf("Failed to complete creation operation\n");
			exit(-1);
		}
		char tarFile[300];
		makeFilePath(tarFile, argv[2], argv[3]); //get tarfile path
		createTar(tarFile, argv[2]);
	}
	//extract all files
	else if (!strcmp(argv[1], "-d")) {
		if (argc != 3) {
			printf("Failed to complete extraction operation\n");
			exit(-1);
		}
		extract(argv[2]);
	}
	//extract one single file
	else if (!strcmp(argv[1], "-e")) {
		if (argc != 4) {
			printf("Failed to complete extraction operation\n");
			exit(-1);
		}
		extractSingle(argv[2], argv[3]);
	}
	//list the contents
	else if (!strcmp(argv[1], "-l")) {
		if (argc != 3) {
			printf("Failed to complete list operation\n");
			exit(-1);
		}
		list(argv[2]);
	}
	else {
		printf("Invalid argument\n");
		exit(-1);
	}
	return 0;
}

void createTar (char tarFile[], char directory[]) {
	//create the tar file in write mode and 0644 permission, assuming that it doesn't exist currently
	int tar = open(tarFile, O_CREAT|O_WRONLY, 0644);
	if (tar < 0) {
		printf("Failed to complete creation operation\n");
		exit(-1);
	}
	struct tarStats tarStat;
	tarStat.fileCount = 0;
	//open the directory to read it
	DIR* tarDir = opendir(directory);
	if (tarDir == NULL) {
		printf("Failed to complete creation operation\n");
		exit(-1);		
	}
	struct dirent *file;

	//count the no. of regular files in the current directory
	while ((file = readdir(tarDir)) != NULL) {
		if (file-> d_type == DT_REG) {
			char filename[300];
			makeFilePath(filename, directory, file->d_name);

			//ignore the tarfile in the count
			if (!strcmp(filename, tarFile)) {
				continue;
			}
			tarStat.fileCount++;
		}
	}
	//enter the tar file info first
	write(tar, &tarStat, sizeof(struct tarStats));
	closedir(tarDir);
	tarDir = opendir(directory); //to read the directory from the start again
	while ((file = readdir(tarDir)) != NULL) {
		if (file->d_type == DT_REG) {
			char filename[300];
			makeFilePath(filename, directory, file->d_name);

			//ignore the tarfile
			if (!strcmp(filename, tarFile)) {
				continue;
			}

			//get the file information using stat
			struct stat fileDesc;
			stat(filename, &fileDesc);

			//fill the tarFileDesc objectss
			struct tarFileDesc object;
			strcpy(object.name, file->d_name);
			strcpy(object.directory, directory);
			object.size = fileDesc.st_size;
			//enter file information in the tar file
			if(write(tar, &object, sizeof(struct tarFileDesc)) != sizeof(struct tarFileDesc)) {
				printf("Failed to complete creation operation\n");
				exit(-1);
			};

			int fd = open(filename, O_RDONLY);
			if (fd < 0) {
				printf("Failed to complete creation operation\n");
				exit(-1);
			}
			//read data from fd, and write into tar
			if (read512(fd, tar, 1, object.size) < 0) {
				printf("Failed to complete creation operation\n");
				exit(-1);				
			};
			close(fd);
		}
	}
	closedir(tarDir);
	close(tar);
}

void extract (char tarFile[]) {
	int tar = open(tarFile, O_RDONLY);
	if (tar < 0) {
		printf("Failed to complete extraction operation\n");
		exit(-1);
	}
	int index = -1;

	//from the tarFile path, extract the directory by removing the string after last "/"
	for (int i = 0; i < strlen(tarFile); i++) {
		if (tarFile[i] == '/') {
			index = i; //directory path is before this index
		}
	}
	
	//change the name from <tarFile>.tar to <tarFile>Dump
	char name[100];
	strcpy(name, &tarFile[index+1]);
	strcpy(name, strtok(name, "."));
	strcat(name, "Dump");

	//get the path to the Dump directory
	char dumpDir[300];
	strcpy(dumpDir, tarFile);
	dumpDir[index] = '\0';
	strcat(dumpDir, "/");
	strcat(dumpDir, name);

	//make the dumpDir, assuming that it doesn't already exist
	if(mkdir(dumpDir, 0777) <  0) {
		printf("Failed to complete extraction operation\n");
		exit(-1);
	};

	struct tarFileDesc object;
	struct tarStats tarStat;
	//read the tarFile information (file count)
	if(read(tar, &tarStat, sizeof(struct tarStats)) != sizeof(struct tarStats)) {
		printf("Failed to complete extraction operation\n");
		exit(-1);
	}
	//keep reading an tarFileDesc objects till possible
	while ((read(tar, &object, sizeof(struct tarFileDesc))) > 0) {
		char filename[300];
		makeFilePath(filename, dumpDir, object.name);
		int fd = open(filename, O_CREAT|O_WRONLY, 0644);
		if (fd < 0) {
			printf("Failed to complete extraction operation\n");
			exit(-1);
		}
		//read from tar and write into fd
		if (read512(tar, fd, 1, object.size) == -1) {
			printf("Failed to complete extraction operation\n");
			exit(-1);		
		}
		close(fd);
	};
	close(tar);
}

void extractSingle (char tarFile[], char file[]) {
	int tar = open(tarFile, O_RDONLY);
	if (tar < 0) {
		printf("Failed to complete extraction operation\n");
		exit(-1);
	}
	int index = -1;

	//from the tarFile path, extract the directory by removing the string after last "/"
	for (int i = 0; i < strlen(tarFile); i++) {
		if (tarFile[i] == '/') {
			index = i;
		}
	}

	//get the path to IndividualDump directory
	char dumpDir[300];
	strcpy(dumpDir, tarFile);
	dumpDir[index] = '\0';
	strcat(dumpDir, "/IndividualDump");

	int exists = 0; //to check if directory exists or not

	struct stat dir;
	if (stat(dumpDir, &dir) == 0 && S_ISDIR(dir.st_mode)) {
		exists = 1;
	}
	//if it doesn't exist, create a new directory
	if (!exists) {
		if (mkdir(dumpDir, 0777) < 0) {
				printf("Failed to complete extraction operation\n");
				exit(-1);
		}
	}

	struct tarFileDesc object;
	struct tarStats tarStat;
	//read the tarFile information (file count, etc)
	if(read(tar, &tarStat, sizeof(struct tarStats)) != sizeof(struct tarStats)) {
		printf("Failed to complete extraction operation\n");
		exit(-1);
	}
	int found = 0; //to check if the file is arhcived in the tarfile
	while ((read(tar, &object, sizeof(struct tarFileDesc))) > 0) {
		char filename[300];
		makeFilePath(filename, dumpDir, object.name);
		int fd;
		if (!strcmp(file, object.name)) {
			found = 1;
			fd = open(filename, O_CREAT|O_WRONLY, 0644);
			if (fd < 0) {
				printf("Failed to complete extraction operation\n");
				exit(-1);
			}
		}
		//scan the current file content to reach the next file content
		if (found) {
			//write into fd if found = 1
			if (read512(tar, fd, found, object.size) < 0) {
				printf("Failed to complete extraction operation\n");
				exit(-1);
			}
		}
		else {
			//dont write into fd as found = 0
			if (read512(tar, fd, found, object.size) < 0) {
				printf("Failed to complete extraction operation\n");
				exit(-1);
			}
		}

		close(fd);
		if (found) {
			break;
		}
	};
	if (!found) printf("No such file present in tar file\n");
	close(tar);
	
}

void list (char tarFile[]) {
	int tar = open(tarFile, O_RDONLY);
	int index = -1;
	if (tar < 0) {
		printf("Failed to complete list operation\n");
		exit(-1);
	}
	//get directory of tar file
	for (int i = 0; i < strlen(tarFile); i++) {
		if (tarFile[i] == '/') {
			index = i;
		}
	}
	//get tarStructure path
	char listFile[300];
	strcpy(listFile, tarFile);
	listFile[index] = '\0';
	strcat(listFile, "/tarStructure");

	struct tarFileDesc object;
	struct tarStats tarStat;
	//read the fileCount
	if (read(tar, &tarStat, sizeof(struct tarStats)) != sizeof(struct tarStats)) {
		printf("Failed to complete list operation\n");
		exit(-1);
	}

	//use the stat function to get tarFile size
	struct stat tarDesc;
	stat(tarFile, &tarDesc);
	int fd = open(listFile, O_CREAT|O_WRONLY, 0644);
	char entry[50];
	//enter the tarFileSize and the fileCount
	sprintf(entry, "%ld\n", tarDesc.st_size);
	if (write(fd, entry, strlen(entry)) != strlen(entry)){
		printf("Failed to complete list operation\n");
		exit(-1);
	}

	sprintf(entry, "%d\n", tarStat.fileCount);
	if (write(fd, entry, strlen(entry)) != strlen(entry)) {
		printf("Failed to complete list operation\n");
		exit(-1);
	}
	while ((read(tar, &object, sizeof(struct tarFileDesc))) > 0) {
		//write the file statistics
		sprintf(entry, "%s %ld\n", object.name, object.size);
		if (write(fd, entry, strlen(entry)) != strlen(entry)) {
			printf("1Failed to complete list operation\n");
			exit(-1);
		}
		//read the file data to move to next file
		if (read512(tar, fd, 0, object.size) < 0) {
			printf("2Failed to complete list operation\n");
			exit(-1);
		}
	}
	close(tar);
}