/// Authors: Siddhant, Arun, Medha
/// Net ID: ssm200002, axj200010, mva170001
/// Course: CS 5348.001
/// Project 2 Part 2 - V6 FILE SYSTEM
///
/// In this project, we built off the modv6 program that we created in Project 2 Part 1 and implemented the additional
/// following commands: openfs, initfs, cpin, cpout, rm, mkdir, cd, pwd, rootdir and q/exit. 
/// Openfs is used to open/create an existing file. Initfs makes sure that all free blocks are
/// accessible from free[] array of the super block and that one of the data blocks contains the root
/// directory’s contents. Cpin will create a new file in the v6 file system and fill the contents of the
/// newly created file with the contents of the externalfile. The file externalfile is a file in the native unix machine
/// , the system where your program is being run. Cpout will  create externalfile and make the externalfile's contents
/// equal to the file generated in . Rm will delete a file and Remove all the data blocks of the file, free the i-node
/// and remove the directory entry. Mkdir should create the directory. Cd will change the working directory.
/// Q/exit will terminate the program.
///
/// How to run the code:
///
/// Compile this c file: gcc modv6.c -o (name of the generated exe)
/// Run it with the following format: ./(name of generated exe) (name of file)
/// Do the following commands
/// Terminate the program
///
/// Work that each person completed:
///
/// We all did research and discussed ways to approach this project in addition to debugging.
///
/// Siddhant: Worked on the initfs function in addition to getFreeBlock function. In addition he worked on the cpout and
///           mkdir commands.
/// Arun: Worked on main  and readInode and writeInode functions. In addition, he worked on the rm and cd functionality.
/// Medha: Worked on openfs and addFreeBlock functions. In addition, she worked on the cpin functionality
///        with the quit/exit.
/// Refactored code to work with linux system calls rather than using c library calls

//import required header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include<sys/stat.h>

//define constants
#define BLOCK_SIZE 1024
#define INODE_SIZE 64
#define FREE_ARRAY_SIZE 251

//superblock struct
typedef struct {
    int isize;
    int fsize;
    int nfree;
    unsigned int free[FREE_ARRAY_SIZE];
    char flock;
    char ilock;
    char fmod;
    unsigned int time;
} superblock_type;

//inode struct
typedef struct {
    unsigned short flags;
    unsigned short nlinks;
    unsigned int uid;
    unsigned int gid;
    unsigned int size0;
    unsigned int size1;
    unsigned int addr[9];
    unsigned int actime;
    unsigned int modtime;
} inode_type;

//dir struct
typedef struct {
    unsigned int inode;
    char filename[28];
} dir_type;

//global variables
superblock_type superBlock;
int fd;
int n_inode;
unsigned int currentInodeNumber;
char pwd[256];


void writeBlock(int blockNumber, int offset, void *buffer, int nBytes) { //to write a block
    lseek(fd, (BLOCK_SIZE * blockNumber) + offset, SEEK_SET); //go to the proper location along with the offset
    write(fd, buffer, nBytes); //write block at that pos
//    printf("wrote block number %d and offset %d at location %d\n", blockNumber, offset, BLOCK_SIZE*blockNumber+offset);
}


void addFreeBlock(int blockNumber) { //to add a block to free list
    if (superBlock.nfree == FREE_ARRAY_SIZE) { //if nfree becomes 251
        writeBlock(blockNumber, 0, superBlock.nfree, sizeof(superBlock.nfree));
        writeBlock(blockNumber, sizeof(superBlock.nfree), superBlock.free, sizeof(superBlock.free));
        superBlock.nfree = 0;
    }//else
    superBlock.free[superBlock.nfree] = blockNumber;
    superBlock.nfree++;
}

void readBlock(int blockNumber, int offset, void *buffer, int nBytes) { //to read a block
    lseek(fd, (BLOCK_SIZE * blockNumber) + offset, SEEK_SET); //go to the proper location 
    read(fd, buffer, nBytes); //read block into buffer
}

void writeInode(int inode_num, inode_type inode) { //to write inode
    int block_num = 2 + (((inode_num) * INODE_SIZE) / BLOCK_SIZE); //find block number
    int offset = (inode_num * INODE_SIZE) % BLOCK_SIZE; //find offset
//    printf("Inode number %d added to block number %d and offset %d. Loc %d\n", inode_num, block_num, offset, BLOCK_SIZE*block_num+offset);
    writeBlock(block_num, offset, &inode, INODE_SIZE); //write inode
}

inode_type getInode(int INumber) { //to get an inode
    inode_type iNode;
    int blockNumber = (INumber * INODE_SIZE) / BLOCK_SIZE; //find block number
    int offset = (INumber * INODE_SIZE) % BLOCK_SIZE; //find offset
    lseek(fd, (BLOCK_SIZE * blockNumber) + offset, SEEK_SET); //go to the correct location
    read(fd, &iNode, INODE_SIZE); //read inode
    return iNode; //return inode
}

inode_type readInode(int inode_num) { //to read inode
    inode_type tempInode;
    int block_num = 2 + ((inode_num * INODE_SIZE) / BLOCK_SIZE); //go to the proper location
    int offset = (inode_num * INODE_SIZE) % BLOCK_SIZE; //find offset
//    printf("Read block location: block %d offset %d. In bytes %d\n", block_num, offset, BLOCK_SIZE*inode_num+offset);
    readBlock(block_num, offset, &tempInode, INODE_SIZE); //read block into tempInode
    return tempInode; //return tempInode
}

int getInodeNumber() { //to get the inode number
    inode_type tempInode;
    int i;
    for (i = 2; i < superBlock.isize; i++) {
        tempInode = readInode(i); //read inode
        if (!(tempInode.flags & (1 << 15))) { //check flags to see if its allocated or not
            tempInode.flags = tempInode.flags | (1 << 15); //set as allocated
            writeInode(i, tempInode); //write inode;
            return i; //return inode number
        }
    }
    printf("No Inode available"); 
    return -1;
}

int getFreeBlock() { //to get a free block
    superBlock.nfree--;
    if (superBlock.nfree > 0) {//if nfree > 0
        if (superBlock.free[superBlock.nfree] == 0) { //if not blocks left
            printf("File system full\n");
            return -1;
        }
        return superBlock.free[superBlock.nfree]; //return a free block
    }
    //if nfree becomes 0
    int tempBlockNum = superBlock.free[0]; //read blocknumber present in free[0]
    lseek(fd, BLOCK_SIZE * tempBlockNum, SEEK_SET); //go to block number
    read(fd, superBlock.nfree, sizeof(superBlock.nfree)); //read nfree
    lseek(fd, (BLOCK_SIZE * tempBlockNum) + sizeof(superBlock.nfree), SEEK_SET);//go to the next of nfree location
    read(fd, superBlock.free, sizeof(superBlock.free)); //read new blocks into the free array
    return tempBlockNum; //return free[nfree]
}

int openfs(char *filename) { //to open v6fs
    if (access(filename, F_OK) != -1) { //file exists
        printf("\nFile system %s exists. Trying to open...\n", filename);
        fd = open(filename, O_RDWR); //open file

        lseek(fd, BLOCK_SIZE, SEEK_SET); //go to superblock location
        read(fd, &superBlock, sizeof(superBlock)); //read superblock into superlbock struct
        lseek(fd, 2 * BLOCK_SIZE, SEEK_SET); //go to inode block
        inode_type rootInode = getInode(1); 
        read(fd, &rootInode, sizeof(rootInode));//read root inode
        printf("File opened\n");
        struct stat buf;
        stat(filename, &buf);
        printf("The file size is %ld\n", buf.st_size);
        return 1;
    } else {
        printf("File system does not exists! create a new one using initfs command\n");
    }
}

int initfs(char *fsPath, int numBlocks, int numInodes) { //to initialize v6fs 
    printf("FS Initialization!\n");

    if ((fd = open(fsPath, O_RDWR | O_CREAT, 0600)) == -1) { //read filename
        printf("No file open. Use openfs command to open a file.\n");
        return -1;
    }
    printf("Initializing file system...\n");

    char buffer[BLOCK_SIZE] = {0}; //block of 1024 bytes
//    printf("Writing first empty block at last location.\n");
    writeBlock(numBlocks, 0, buffer, BLOCK_SIZE);

    //sb
    if (((numInodes * INODE_SIZE) % BLOCK_SIZE) == 0) { //find number of blocks required for inode
        superBlock.isize = (numInodes * INODE_SIZE) / BLOCK_SIZE;
    } else {
        superBlock.isize = (numInodes * INODE_SIZE) / BLOCK_SIZE + 1;
    }
    superBlock.fsize = numBlocks; //update fsize in superblock 
    //set other params to 0
    superBlock.nfree = 0;
    superBlock.flock = '0';
    superBlock.ilock = '0';
    superBlock.fmod = '0';
    superBlock.time = 0;     //time(NULL);

//    printf("Superblock isize: %d\n", superBlock.isize);
//    printf("Superblock fsize: %d\n", superBlock.fsize);
//    printf("Superblock flock: %c\n", superBlock.flock);
//    printf("Superblock ilock: %c\n", superBlock.ilock);
//    printf("Superblock fmod: %c\n", superBlock.fmod);
//    printf("Superblock time: %d\n", superBlock.time);

//    printf("writing super block\n");
    writeBlock(1, 0, &superBlock, BLOCK_SIZE);

    int blockNum, startFreeBlock;
    startFreeBlock= 2+superBlock.isize; //this is at root dir currently at this point

    //add free block
    for (blockNum = startFreeBlock+1; blockNum <= numBlocks; blockNum++) {
//        printf("\nCall addfreeblock for block: %i\n", blockNum);
        addFreeBlock(blockNum);
//        printf("nfree value is %d\n", superBlock.nfree);
    }
    printf("\n");

    inode_type tempInode;
    tempInode.flags = 0; //set flags to 0
    int i;
    for (i = 0; i < 9; i++) { //set all addr[] blocks to 0
        tempInode.addr[i] = 0;
    }

    for (i = 1; i <= numInodes; i++) { //write all inodes
        writeInode(i, tempInode);
    }

//    createRootDirectory();
    int blockNumber = startFreeBlock; //get blocknumber for root dir
    dir_type rootDir[2]; //make two entries - . and ..
    rootDir[0].inode = 1; //set inode number as 1
    strcpy(rootDir[0].filename, "."); //put filename as .

    rootDir[1].inode = 1; //set inode number as 1
    strcpy(rootDir[1].filename, ".."); //put filename as ..
//    printf("writing root dir\n");
    writeBlock(blockNumber, 0, &rootDir, sizeof(rootDir)); //write to block

    inode_type rootInode;
    rootInode.flags = 1 << 14 | 1 << 15; //set flags to show its a dir and allocated
    rootInode.nlinks = 1; //set links to 1
    rootInode.uid = 0;
    rootInode.gid = 0;
    rootInode.size0 = 0;
    rootInode.size1 = 2 * sizeof(dir_type); //update filesize
    rootInode.addr[0] = blockNumber; //put block number of root dir
    rootInode.actime = time(NULL); //put current time
    rootInode.modtime = time(NULL); //put current time

//    printf("writing inode 1  - root inode\n");
    writeInode(1, rootInode); //write root inode
    currentInodeNumber = 1; //set current inode number to 1
    strcpy(pwd, "~V6FS@root"); //set string as current working dir
    return 0;

}

int getInodeByFileName(char *fileName) { //to get inode from filename
    int iNodeNum;
    inode_type dirInode;
    dir_type dirEntry;

    dirInode = readInode(1); //read root inode
//    printf("Location: %d\n",BLOCK_SIZE * 2 + (sizeof(dirEntry) * 2));
    lseek(fd, (BLOCK_SIZE * 2 + (sizeof(dirEntry) * 2)), SEEK_SET); //go to the current location
    lseek(fd, (BLOCK_SIZE * dirInode.addr[0]), SEEK_SET);
    int noOfRecords = 0;
    noOfRecords = BLOCK_SIZE / sizeof(dirEntry); //find total number of records

    int i;

    for (i = 0; i < noOfRecords; ++i) { //iterate through all the records
        read(fd, &dirEntry, sizeof(dirEntry)); //read record
//        printf("Read record: %s\n", dirEntry.filename);
//        printf("Input filename was %s\n", fileName);
        if (strcmp(fileName, dirEntry.filename) == 0) //if match found
            return dirEntry.inode; //return inode number
    }
    printf("File %s not found!\n", fileName);
    return -1;
}

unsigned int getBlockSmallFile(int fileInodeNum, int currentBlock) { //to get small block of addr[]
    inode_type fileInode;
    fileInode = readInode(fileInodeNum); //read inode
    return fileInode.addr[currentBlock]; //return addr[blockNumber]
}

int addFileToDir(char *toFileName){ //add file to dir
//     printf("\nInside addFileToDir, create %s was requested\n",toFileName);

    inode_type dirInode, freeNode;
    dir_type dirEntry;
    int toFileInodeNum;

    int found = 0;
    toFileInodeNum=1;
    while(found==0){//get a free inode
        toFileInodeNum++;
        freeNode = readInode(toFileInodeNum);
//        printf("The 15th bit of inode number %d is %d\n",toFileInodeNum, (freeNode.flags>>(15-1))&1);
        if(((freeNode.flags >> (15-1)) & 1) == 0){
//            printf("Free Inode number is %d\n", toFileInodeNum);
            found = 1;
        }
    }
    dirInode = readInode(1); //read root inode
//    printf("Root inode number is 1 and its block[0] is at block number is %d\n", dirInode.addr[0]);
    lseek(fd, (BLOCK_SIZE * dirInode.addr[0] + dirInode.size1), SEEK_SET); //go to the proper location
//    printf("Size of root inode before adding a new record of 32 bytes (dir): %ld\n",dirInode.size1);
    //add record to dir
    dirEntry.inode = toFileInodeNum; //update inode number of record
    strcpy(dirEntry.filename, toFileName); //update filename of record
    write(fd, &dirEntry, sizeof(dirEntry)); //write changes0
    //Update Directory file inode to increment size by one record
    dirInode.size1+=sizeof(dirEntry); //update size
    writeInode(1,dirInode); //write inode
 
    return toFileInodeNum; //return inode number 
}
 
inode_type initFileInode(int toFileInodeNum, int fileSize){ //to initialize inode
//    printf("Inside init file inode\n");

    inode_type toFileInode;

    toFileInode.flags = 0; //set all flags to 0 initially
    toFileInode.flags |= 1<<15 | 0<<14 | 0<<13; //set as allocated
    if(fileSize <= BLOCK_SIZE*9){//small file
        toFileInode.flags |= 0<<12 | 0<<11; //bit d and e are 0 for small file
    }
    //set other params to 0
    toFileInode.nlinks = 0;
    toFileInode.uid = '0';
    toFileInode.gid = '0';
    toFileInode.size0 = 0;
    toFileInode.size1 = fileSize; //update file size
    int i;
    for(i=0; i<9; ++i){ //all addr[] to 0
        toFileInode.addr[i] = 0;
    }
    toFileInode.actime = time(NULL); //put current time
    toFileInode.modtime = time(NULL); //put current time

    return toFileInode; //return inode
}

void addBlockToInodeSmallFile(int blockSeq, int blockNum, int toFileInodeNum){ //to update block number into inode
        //only for small file
        inode_type fileInode;
        int temp = blockNum;
        fileInode = readInode(toFileInodeNum); //read inode
        fileInode.addr[blockSeq] = temp; //update block number into addr[]
        writeInode(toFileInodeNum, fileInode); //write inode 
        return;
}

int cpin(char *fromFileName, char *toFileName){ //to copy an external file inot v6fs
//    printf("Inside cpin, copying from %s file to %s file\n", fromFileName, toFileName);

    inode_type  toFileInode;
    int fromFilefd, fileSize, toFileInodeNum;
    char buffer[BLOCK_SIZE];
    FILE* debug;

    toFileInodeNum = getInodeByFileName(toFileName); //try to get the inode of the file

    if(toFileInodeNum != -1){ //if not present
        printf("File %s already exists! Choose a different file\n", toFileName);
    }
    printf("File not found! Creating %s ...",toFileName);
    //check access permission
    if(access(fromFileName, F_OK) != -1){ 
        //file exists
        printf("\nFile %s exists. Trying to open...\n",fromFileName);
        fromFilefd = open(fromFileName, O_RDWR); //open source external file
        struct stat buf;
        fstat(fromFilefd, &buf);
        fileSize = buf.st_size;
        printf("%s file size is %d\n", fromFileName, fileSize); //print size
        if (fileSize == 0){ // if size is 0
            printf("\nCopy from file %s doesn't exists. Enter correct filename\n",fromFileName);
            return -1;
        }
        else{//print size
            printf("Copy from file size is %i\n",fileSize);
//            lseek(fd, BLOCK_SIZE*(2+superBlock.isize+1), SEEK_SET);
        }
    }
    else{
        //file does not exists
        printf("Copy from file %s does not exists. Enter correct filename\n", fromFileName);
        return -1;
    }
    //adding file to dir
    toFileInodeNum = addFileToDir(toFileName);
    //init and load inode for this new file
    toFileInode = initFileInode(toFileInodeNum, fileSize);
    writeInode(toFileInodeNum, toFileInode); //write inode
    
    //Now copy from fromFile to toFile
    int numBlocksRead = 1024, freeBlockNum, totalNumBlocks = 0;
    lseek(fromFilefd, 0, SEEK_SET); //go to the beginning of the file
    int blockSeq = 0;
//    debug = fopen("debug.txt","w");
    while(numBlocksRead == 1024){ //iterate till all block all copied from external file into the internal file 
//         Read one block at a time from source file
        numBlocksRead = read(fromFilefd, &buffer,sizeof(buffer));
        totalNumBlocks+= numBlocksRead/BLOCK_SIZE;
        freeBlockNum = getFreeBlock();
        //Debug print into file for testing purpose only
//        fprintf(debug,"\nBlock allocated %i, Sequence/order num %i\n",freeBlockNum, totalNumBlocks);
//        fflush(debug);
        if (freeBlockNum == -1){ //if no free blocks left
            printf("\nNo free blocks left. Total blocks read: %i\n",totalNumBlocks);
            return -1;
        }
//        printf("\nBefore Add block %i\n",blockSeq);
//        fflush(stdout);
        addBlockToInodeSmallFile(blockSeq,freeBlockNum, toFileInodeNum); //update block into inode
        // Write one block at a time into target file
//        printf("free allocated block is %d\n", freeBlockNum);
//        printf("\nAfter Add block %i\n",blockSeq);
//        fflush(stdout);
        lseek(fd, freeBlockNum*BLOCK_SIZE, SEEK_SET); //go to the block
        write(fd, &buffer, sizeof(buffer)); //write contents of buffer to the current pos
        blockSeq++;
//        printf("End of loop %i\n",blockSeq);
//        fflush(stdout);
    }
    //close debug file
    struct stat buf0;
    struct stat buf1;
    int fromfd = open(fromFileName, O_RDWR);
    int tofd = open(toFileName, O_RDWR);
    fstat(fromfd,&buf0);
    fstat(tofd,&buf1);
    printf("\nExternal source file size is %d\n", buf0.st_size);
    //printf("Destination file size is %d\n\n", buf1.st_size);
//    fclose(debug);
    return 0;
}

int getInodeFileSize(int fileInodeNum){ //to get inode size
    inode_type toFileInode;
    toFileInode = readInode(fileInodeNum); //read inode
    return toFileInode.size1; //return size
}
int cpout(char *fromFile, char *toFile) { //to copy a file from v6fs to external file
//    printf("Inside cpout function, copying from %s file to %s external file\n", fromFile, toFile);
    int fileInodeNum, numOfBlocks;
    fileInodeNum = getInodeByFileName(fromFile); //try to get inode of the file
    if (fileInodeNum == -1) { //if not present
        printf("The file you are trying to copy from not found!\n");
        return -1;
    }
    char buffer[BLOCK_SIZE] = {0}; //declare a block of 1024 bytes
    int writeFile;
    writeFile = open(toFile, O_RDWR|O_CREAT,0600); //open external output file
    int fileSize;
//    struct stat buf;
    fileSize = getInodeFileSize(fileInodeNum); //get filesize
    printf("Internal file %s size is %d\n", fromFile, fileSize);

    int bytesLastBlock = fileSize % BLOCK_SIZE; //find number of bytes in the last block 
    char lastBuffer[bytesLastBlock];

    if (bytesLastBlock == (fileSize / BLOCK_SIZE)) { //if remainder 0
        numOfBlocks = fileSize / BLOCK_SIZE;
    } else numOfBlocks = fileSize / BLOCK_SIZE + 1; //take one extra block to store the remainder contents

//    printf("Number of blocks: %d\nLast block bytes are %d\n", numOfBlocks, bytesLastBlock);

    int currentBlock, nextBlockNum;
    currentBlock = 0;

    while (currentBlock < numOfBlocks) { //iterate all blocks of addr[]
        //only considering small files as of now (part 2)
        nextBlockNum = getBlockSmallFile(fileInodeNum, currentBlock); //get a block

        lseek(fd, nextBlockNum * BLOCK_SIZE, SEEK_SET); //go to the block loc
        if ((currentBlock < (numOfBlocks - 1)) || (bytesLastBlock == 0)) { //if the block is not the last block or remainder is 0
            read(fd, &buffer, sizeof(buffer));
            write(writeFile, &buffer, sizeof(buffer));
        } else {//if last block
            read(fd, &lastBuffer, sizeof(lastBuffer));
            write(writeFile, &lastBuffer, sizeof(lastBuffer));
        }

        currentBlock++;
    }
    close(writeFile); //close external output file fd
    return 0;
}

int makeNewDir(char *name) { //to make new dir
    inode_type dirInode, newDirInode;
    dir_type dirEntry;
    int toFileInodeNum, fileInodeNum;

    fileInodeNum = getInodeByFileName(name); //try to get inode of the ifle
    if (fileInodeNum != -1) {//if not present
        printf("Directory %s already exists!\n", name);
        return -1;
    }
    //get a free inode
    int found = 0;
    toFileInodeNum = 1;
    while (found == 0) {//get a free inode
        toFileInodeNum++;
        newDirInode = readInode(toFileInodeNum); //read inode corresponding to the inode number
//        printf("15th bit of inode number %d is %d\n", toFileInodeNum, (newDirInode.flags>>(15-1))&1);
        if (!(newDirInode.flags >> (15 - 1)) & 1) { //check flag to verify its allocated or not
//            printf("Free inode number is %d\n", toFileInodeNum);
            found = 1; //set flag to 1 if found one
        }
    }

    dirInode = readInode(1);
//    printf("Root inode number is 1 and its block[0] is at block number is %d\n", dirInode.addr[0]);
    lseek(fd, (BLOCK_SIZE * dirInode.addr[0] + dirInode.size1), SEEK_SET);
//    printf("Size of root inode before adding a new record of 32 bytes (dir): %ld\n",dirInode.size1);
    //add record to dir
    dirEntry.inode = toFileInodeNum;
    strcpy(dirEntry.filename, name);
    write(fd, &dirEntry, sizeof(dirEntry));

    //update dirInode params
    dirInode.size1 += sizeof(dirEntry);
    writeInode(1, dirInode);

//    struct stat buf;
//    fstat(fd, &buf);
    printf("Size of root inode after adding a new record of 32 bytes (dir): %ld\n",dirInode.size1);

    //update newDirInode params
    newDirInode.flags = 0;
    newDirInode.flags |= 1 << 15 | 1 << 14 | 0 << 13;
    writeInode(toFileInodeNum, newDirInode);
    return 0;
}

void removeFileFromDir(int fileInodeNum){ //remove file from dir
    // printf("Inside remove file from dir function\n");
    inode_type dirInode;
    dir_type dirEntry;

    dirInode = readInode(1); //read root inode
//    printf("Location: %d\n",BLOCK_SIZE * 2 + (sizeof(dirEntry) * 2));
    lseek(fd, (BLOCK_SIZE * 2 + (sizeof(dirEntry) * 2)), SEEK_SET); //go to the proper location
    lseek(fd, (BLOCK_SIZE * dirInode.addr[0]), SEEK_SET);
    int noOfRecords = 0;
    noOfRecords = BLOCK_SIZE / sizeof(dirEntry); //find total number of recordss
//    printf("Number of records are %d\n", noOfRecords);

    int i;

    for (i = 0; i < noOfRecords; ++i) { //iterate through all the records
        read(fd, &dirEntry, sizeof(dirEntry));
//        printf("Node number in directory is %d and filename is %s\n", dirEntry.inode, dirEntry.filename);
        if(dirEntry.inode == fileInodeNum){ //if match found
            lseek(fd,(-1)*sizeof(dirEntry), SEEK_CUR); //go one record back from the current pos
            dirEntry.inode = 0; //set inode as 0 as it is invalid now
            memset(dirEntry.filename,0,sizeof(dirEntry.filename)); //set filename to blank
            write(fd,&dirEntry,sizeof(dirEntry)); //write changes
            return;
        }
    }
    return;
}

int rm(char *fileName){ //remove file from v6fs
    // printf("Inside rm function\n");
    int fileInodeNum, fileSize, blockSeq, nextBlockNum;
    inode_type fileInode;

    fileInodeNum = getInodeByFileName(fileName); //get inode to file
    if(fileInodeNum == -1){ //if not present
        printf("File not found!\n");
    }
    fileInode = readInode(fileInodeNum); //read inode
    if(!(((fileInode.flags>>(12-1)) & 1) && ((fileInode.flags>>(11-1)) & 1))){ //check flags to verify its a small file
        //if bit 11 and 12 are 0 means it is a small file
//        printf("Removing small file\n");
        fileSize = getInodeFileSize(fileInodeNum); //get file size
        blockSeq = fileSize/BLOCK_SIZE; //find number of blocks
        if(fileSize%BLOCK_SIZE != 0){
            blockSeq++;
        }
        blockSeq--;
        while(blockSeq > 0){
            nextBlockNum = fileInode.addr[blockSeq]; //get addr[i] block
//            printf("add block %d to freeblock list\n", nextBlockNum);
            addFreeBlock(nextBlockNum); //add block to free list
            blockSeq--;
        }
    }
    fileInode.flags = 0;
    writeInode(fileInodeNum, fileInode); //write inode
    removeFileFromDir(fileInodeNum); //remove file listing from dir
    return 0;
}

int lastIndex(){ //to get index of "/"
    int i, index;

    for(i=0; i<256; ++i){//iterate through the contents till a "/" is found
        if(pwd[i] == '/'){ //found
            index = i; //store into index
            break;
        }
    }
    return index; //return index of "/"
}

void slicePath(char path[256], char *pwd, int start, int end){//to slice the path
    int i, index = 0;
    for(i=start; i<=end; ++i){
        pwd[index++] = path[i]; //copy contents till a "/" is encountered
    }
    pwd[index] = 0;
}

void printRootDir(){ //to print root dir
    inode_type dirInode, tempInode;
    dir_type dirEntry;

    dirInode = readInode(1); //read root inode

//    printf("Location: %d\n",BLOCK_SIZE * 2 + (sizeof(dirEntry) * 2));
    lseek(fd, (BLOCK_SIZE * 2 + (sizeof(dirEntry) * 2)), SEEK_SET); //go to the proper location
    lseek(fd, (BLOCK_SIZE * dirInode.addr[0]), SEEK_SET);
    int noOfRecords = 0;
    noOfRecords = BLOCK_SIZE / sizeof(dirEntry);
//    printf("Number of records are %d\n", noOfRecords);
    //list all records
    int i;
    for (i = 0; i < noOfRecords; ++i) {//iterate through all the records
        read(fd, &dirEntry, sizeof(dirEntry));
        printf("Node number: %d\tFilename: %s\n", dirEntry.inode, dirEntry.filename);
    }
}

void changeDir(char *path){ //to change dir
    inode_type dirInode, tempInode;
    int blockNumber, lastSlashPos;
    char temp[256];
    dir_type dirEntry;

    dirInode = readInode(1); //read root inode
//    printf("Location: %d\n",BLOCK_SIZE * 2 + (sizeof(dirEntry) * 2));
    lseek(fd, (BLOCK_SIZE * 2 + (sizeof(dirEntry) * 2)), SEEK_SET); //go to the proper location to get all records
    lseek(fd, (BLOCK_SIZE * dirInode.addr[0]), SEEK_SET);
    int noOfRecords = 0;
    noOfRecords = BLOCK_SIZE / sizeof(dirEntry);
//    printf("Number of records are %d\n", noOfRecords);

    int i;

    for (i = 0; i < noOfRecords; ++i) {//iterate through all the records
        read(fd, &dirEntry, sizeof(dirEntry)); //read content into dirEntry buffer
//        printf("Node number in directory is %d and filename is %s\n", dirEntry.inode, dirEntry.filename);
        if(strcmp(dirEntry.filename, path) == 0){ // if match is found
            tempInode = readInode(dirEntry.inode); //read inode
            if(tempInode.flags == (1<<15 | 1<<14)){ //check flags to check it's a dir
                if(strcmp(path, ".") == 0) return;
                else if(strcmp(path, "..") == 0){ //match found
                    currentInodeNumber = dirEntry.inode;
                    lastSlashPos = lastIndex(pwd); //get index of "/"
                    slicePath(pwd, temp, 0, lastSlashPos-1); //slice pwd accordingly
                    strcpy(pwd, temp); //copy new content to pwd 
                    printf("The present directory inode number is %d\n", currentInodeNumber);
                    printf("The current path is %s\n",pwd);
                }
                else{
                    currentInodeNumber = dirEntry.inode; //update current inode number
                    strcat(pwd,"/"); //append "/" to the path
                    strcat(pwd, path); //append path name to pwd
                    printf("The present directory inode number is %d\n", currentInodeNumber);
                    printf("The current path is %s\n",pwd);
                }
            }
            else{
                printf("\nNot a directory!\n"); //if input path is not a dir
            }
            return;
        }
    }
}

int main(int argc, char *argv[]) {
    printf("V6 File System:\n");
    char str[256];
    char *args_1, *args_2, *command, *fsPath;
    unsigned int status, numBlocks, numInodes;

    while (1) {

        printf(">> ");
        scanf(" %[^\n]s", str);

        command = strtok(str, " ");

        if (strcmp(command, "openfs") == 0) { //openfs function
            args_1 = strtok(NULL, " ");
            openfs(args_1);
        }
        else if (strcmp(command, "initfs") == 0) {//initfs function
            fsPath = strtok(NULL, " ");
            args_1 = strtok(NULL, " ");
            args_2 = strtok(NULL, " ");

            if (access(fsPath, X_OK) != -1) { //check permission
                printf("FS already exists. \n");
                printf("Same FS will be used\n");
            } else {
                if (!args_1 || !args_2) // insufficient arguments
                    printf(" Insufficient arguments!\n");
                else {
                    numBlocks = atoi(args_1);
                    numInodes = atoi(args_2);
                    status = initfs(fsPath, numBlocks, numInodes); //call initfs function
                    if (status == 0) {
                        printf("File system initialized successfully\n");
                    } else printf("File system initialization failed!\n");
                }
            }
            command = NULL; //clear the contents
        }
        else if (strcmp(command, "q") == 0 || strcmp(command, "exit") == 0) { //quit function
            printf("Exiting V6 File System\n");
            close(fd); //close v6fs
            exit(0);
        }
        else if (strcmp(command, "cpin") == 0) { //cpin function
//            printf("cpin command\n");
            args_1 = strtok(NULL, " ");
            args_2 = strtok(NULL, " ");
            status = cpin(args_1, args_2); //call cpin function
            if(status == 0){
                printf("Copying file successful\n");
            }
            else printf("Copying file failed!\n");
        }
        else if (strcmp(command, "cpout") == 0) { //cpout function
//            printf("cpout command\n");
            args_1 = strtok(NULL, " ");
            args_2 = strtok(NULL, " ");
            status = cpout(args_1, args_2); //call cpout function
            if (status == 0) {
                printf("File copied successfully\n");
            } else printf("File copying failed!\n");
        }
        else if (strcmp(command, "rm") == 0) { //rm file function
//            printf("rm command\n");
            args_1 = strtok(NULL, " ");
            status = rm(args_1); //call rm file function
            if(status == 0){
                printf("Removed file successfully\n");
            }
            else printf("Failed to remove file!\n");
        }
        else if (strcmp(command, "mkdir") == 0) {//mkdir function
//            printf("mkdir command\n");
            struct stat buf;
            fstat(fd, &buf);
            printf("Size before adding a new dir is %d\n", buf.st_size);
            args_1 = strtok(NULL, " ");
            status = makeNewDir(args_1); //call mkdir function
            struct stat buf1;
            fstat(fd, &buf1);
            printf("Size after adding a new dir is %d\n", buf1.st_size);
            if (status == 0) {
                printf("Created new directory successfully\n");
            } else printf("Creating new directory failed!\n");
        }
        else if (strcmp(command, "cd") == 0) { //change dir function
//            printf("cd command\n");
            args_1 = strtok(NULL, " ");
            changeDir(args_1); //call change dir function
        }
        else if (strcmp(command, "pwd") == 0) { // function to print current working dir - pwd
//            printf("pwd command\n");
           printf("%s\n", pwd);
        }
        else if (strcmp(command, "rootdir") == 0) { //function to print root dir
//            printf("rootdir command\n");
            printRootDir(); //call print root dir function
        }
        else printf("Please enter a valid command: initfs, openfs, mkdir, cd, pwd, rootdir, cpin, cpout or quit/exit!\n"); //print if input command matches nones
    }
}
