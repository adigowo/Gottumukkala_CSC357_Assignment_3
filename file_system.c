#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h> 
#include <unistd.h>

#define MAXINODE 1024
#define LENGTH 32

//inode

struct Inode {
    uint32_t current;
    uint32_t parent;
    char type;
    char name[LENGTH];
};

struct Inode inodes[MAXINODE];

uint32_t currentDirectory = 0; 

// declarations

void *checked_malloc(int len);
char *uint32_to_str(uint32_t i);
void loadInodes(const char* path);
void rootDirectory(void);
void command(void);
void lS(void);
void cD(const char* directoryName);
int findAvailableInode(void);
void mkdir(const char* name);
void updateParentDirectory(uint32_t parentInode, uint32_t childInode, const char* childName);
void writeDirectoryEntries(FILE *dirFile, uint32_t selfInode, uint32_t parentInode);
void touch(const char *name);
void saveInodesList(void);


int main(int argc, char* argv[]){

    if(argc < 2) {
        fprintf(stderr, "Usage: %s path\n", argv[0]);
        return 1; 
    }

     DIR* dir = opendir(argv[1]);
    if (dir) {
        printf("Directory '%s' exists.\n", argv[1]);
        closedir(dir);

    } else {
        fprintf(stderr, "Failed '%s'.\n", argv[1]);
    }

    loadInodes(argv[1]);

    rootDirectory();

    command();

    return 0;
}


//utility functions

void *checked_malloc(int len)
{
   void *p = malloc(len);
   if (p == NULL)
   {
      printf("Ran out of memory!\n");
      exit(1);
   }
   return p;
}

char *uint32_to_str(uint32_t i)
{
   // pretend to print to a string to get length
   int length = snprintf(NULL, 0, "%lu", (unsigned long)i);

   // allocate space for the actual string
   char* str = (char *)checked_malloc(length + 1);

   // print to string
   snprintf(str, length + 1, "%lu", (unsigned long)i);

   return str;
}


//checks for root directory: task 2


void rootDirectory(void){
    if (inodes[0].type == 'd'){
        printf("Root exists");
        
    }
    else {
        printf("directory does not exist");
        exit(EXIT_FAILURE);
    }
}

// parses inodes

void loadInodes(const char* path){

    if(chdir(path) != 0){
        printf("Failed");
        exit(EXIT_FAILURE);
    }

    memset(inodes, 0, sizeof(inodes));
    
    FILE *file = fopen("inodes_list", "rb");
    if (!file) {
        printf("could not open");
    }

     struct Inode node;
    while (fread(&node.current, sizeof(uint32_t), 1, file) == 1 && fread(&node.type, sizeof(char), 1, file) == 1) {

      //  fprintf(stderr, "THE NODE IS: %u\n", node.current);
        if (node.current >= MAXINODE) {
            fprintf(stderr, "error: inode index %u is out of bounds.\n", node.current);
            break; 
        }
    

        node.parent = 0;
        memset(node.name, 0, LENGTH); 
        inodes[node.current] = node;
    }
    fclose(file);

    int i = 0;
    while(i < MAXINODE){
        if (inodes[i].type == 'd') { 

            char filename[LENGTH];
            snprintf(filename, sizeof(filename), "%d", i);
            FILE *dirFile = fopen(filename, "rb");

            if (dirFile) {
                uint32_t child;
                char childName[LENGTH];

                fseek(dirFile, 2 * (sizeof(uint32_t) + LENGTH), SEEK_SET);

                while (fread(&child, sizeof(uint32_t), 1, dirFile) == 1 && fread(childName, LENGTH, 1, dirFile) == 1) {

                    inodes[child].parent = i;
                    strncpy(inodes[child].name, childName, LENGTH - 1);
                    inodes[child].name[LENGTH - 1] = '\0'; 

                }

                fclose(dirFile);
            }
        }
    i++;  

    }
}

// reads user input

void command(void) {
    char command[LENGTH + 1];
    printf("\n Command: ");
    while(fgets(command, sizeof(command), stdin) != NULL){
        
        if (strcmp(command, "ls\n") == 0) {
        lS();
        }
        else if(strcmp(command, "exit\n") == 0){
            break;
        }
        else if(strncmp(command, "mkdir ", 6) == 0){

            char* directoryName = command + 6; 
            printf(directoryName);
            directoryName[strcspn(directoryName, "\n")] = 0; 
            mkdir(directoryName);

        }
        else if(strncmp(command, "cd ", 3) == 0){
            char* directoryName = command + 3; 
            printf(directoryName);
            directoryName[strcspn(directoryName, "\n")] = 0; 
            cD(directoryName);
        }
        else if (strncmp(command, "touch ", 6) == 0) {
            char* fileName = command + 6;
            touch(fileName);
        }
        else{
            printf("\n invalid: %s",command);
        }
        printf("\nCommand: ");
    }
}

//ls

void lS(void) {

    char filename[LENGTH]; 
    snprintf(filename, sizeof(filename), "%u", currentDirectory); 
    
    FILE *dirFile = fopen(filename, "rb");

    if (!dirFile) {

        fprintf(stderr, "failed %s\n", filename);
        return;

    }
    
    uint32_t inode;

    char name[LENGTH + 1]; 

    while (fread(&inode, sizeof(inode), 1, dirFile) == 1) {

        if (fread(name, LENGTH, 1, dirFile) == 1) {
    
            name[LENGTH] = '\0';
            printf("%u %s\n", inode, name);

        } else {

            break;

        }
    }
    
    fclose(dirFile);
}

//cd

void cD(const char* directoryName) {
    char filename[LENGTH];
    snprintf(filename, sizeof(filename), "%u", currentDirectory); 

    FILE *dirFile = fopen(filename, "rb");
    if (!dirFile) {
        fprintf(stderr, "failed %s\n", filename);
        return;
    }

    uint32_t inode;
    char name[LENGTH + 1];
    bool found = false;

    while (fread(&inode, sizeof(inode), 1, dirFile) == 1) {
        if (fread(name, LENGTH, 1, dirFile) == 1) {
            name[LENGTH] = '\0'; 

            if (strcmp(name, directoryName) == 0) {
                
                found = true;
                break; 
            }
            printf("checking: name=%s, inode=%u, type=%c\n", name, inode, inodes[inode].type);
        } else {
            break; 
        }
    }

    fclose(dirFile);

    if (found) {

        if (inode < MAXINODE && inodes[inode].type == 'd') {
            currentDirectory = inode; 
            printf("Changed directory to %s\n", directoryName);
        } else {
            fprintf(stderr, "%s is not a directory\n", directoryName);
        }
    } else {
        fprintf(stderr, "not found\n", directoryName);
    }
}

//find inode that is available

int findAvailableInode() {
    int i = 0;
    while(i < MAXINODE) {
        if (inodes[i].type == 0) { 
            return i;
        }
        i++;
    }
    return -1; 
}

//mkdir

void mkdir(const char* name) {

    int newInodeIndex = findAvailableInode();
    if (newInodeIndex == -1) {
        fprintf(stderr, "0 inodes\n");
        return;
    }

    int i = 0;
    while (i < MAXINODE){
        if (inodes[i].parent == currentDirectory && strcmp(inodes[i].name, name) == 0) {
            fprintf(stderr, "Already Exists\n", name);
            return;
        }
        i++;
    }

    // new inode
    inodes[newInodeIndex].current = newInodeIndex;
    inodes[newInodeIndex].parent = currentDirectory;
    inodes[newInodeIndex].type = 'd';
    strncpy(inodes[newInodeIndex].name, name, LENGTH);
    inodes[newInodeIndex].name[LENGTH - 1] = '\0'; 


    char newDirFilename[LENGTH];
    snprintf(newDirFilename, sizeof(newDirFilename), "%d", newInodeIndex);
    FILE *newDirFile = fopen(newDirFilename, "wb");
    if (!newDirFile) {
        fprintf(stderr, "failed %s\n", newDirFilename);
        return;
    }

    writeDirectoryEntries(newDirFile, newInodeIndex, currentDirectory);

    fclose(newDirFile);

    updateParentDirectory(currentDirectory, newInodeIndex, name);

    saveInodesList();
    printf("Directory %s created\n", name);
}

//helper for touch and mkdir

void updateParentDirectory(uint32_t parentInode, uint32_t childInode, const char* childName) {

    char parentDirFilename[LENGTH];
    snprintf(parentDirFilename, sizeof(parentDirFilename), "%d", parentInode);
    FILE *parentDirFile = fopen(parentDirFilename, "ab"); 


    if (!parentDirFile) {
        fprintf(stderr, "failed to open parent %s\n", parentDirFilename);
        return;
    }


    fwrite(&childInode, sizeof(uint32_t), 1, parentDirFile);
    char formattedName[LENGTH];
    strncpy(formattedName, childName, LENGTH - 1);
    formattedName[LENGTH - 1] = '\0';
    fwrite(formattedName, LENGTH, 1, parentDirFile);

    fclose(parentDirFile);
}

//helper for mkdir and others

void writeDirectoryEntries(FILE *dirFile, uint32_t selfInode, uint32_t parentInode) {
 
    struct 
    {
        uint32_t inode;
        char name[LENGTH];
    }

    entries[] = {{selfInode, "."},{parentInode, ".."}};

    int i = 0;
    while (i < 2) {
        fwrite(&entries[i].inode, sizeof(uint32_t), 1, dirFile);
        fwrite(entries[i].name, LENGTH, 1, dirFile); 
        i++;
    }
}

// touch

void touch(const char *name) {

    int inodeIndex = findAvailableInode();
    if (inodeIndex == -1) {
        fprintf(stderr, "0 available inodes.\n");
        return;
    }

    int i = 0;

    while(i < MAXINODE) {
        if (inodes[i].parent == currentDirectory && strcmp(inodes[i].name, name) == 0) {
            fprintf(stderr, "File %s already exists.\n", name);
            return;
        }
        i++;
    }

    // new inode
    inodes[inodeIndex].current = inodeIndex;
    inodes[inodeIndex].parent = currentDirectory;
    inodes[inodeIndex].type = 'f'; 
    strncpy(inodes[inodeIndex].name, name, LENGTH - 1);
    inodes[inodeIndex].name[LENGTH - 1] = '\0';

    //extra file
    char filename[LENGTH];
    snprintf(filename, sizeof(filename), "%d", inodeIndex);
    FILE *file = fopen(filename, "w");
    if (file) {
        fprintf(file, "%s", name); 
        fclose(file);
    } else {
        fprintf(stderr, "failed", inodeIndex);
        return;
    }

    updateParentDirectory(currentDirectory, inodeIndex, name);
    saveInodesList();
}

// saving function

void saveInodesList() {

    FILE *file = fopen("inodes_list", "wb");

    if (!file) {
        printf("failed");
        return;
    }

    int i = 0;

    while (i < MAXINODE) {

        fwrite(&inodes[i].current, sizeof(inodes[i].current), 1, file);
        fwrite(&inodes[i].parent, sizeof(inodes[i].parent), 1, file);
        fwrite(&inodes[i].type, sizeof(inodes[i].type), 1, file);
        fwrite(inodes[i].name, sizeof(inodes[i].name), 1, file);

        i++;
    }

    fclose(file);
}