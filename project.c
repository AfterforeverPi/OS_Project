#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/wait.h>

char outputPath[128] = "\0";
char quarantinePath[128] = "\0";

int checkArguments(int argc, char** argv){

    for(int i = 1; i < argc; i++){
        //Check for optional arguments, if one is found, skip the next argument (it is the quarantine/output directory)
        if(strcmp(argv[i], "-o") == 0){
            if(i == argc-1){
                perror("Option arguments cannot be at the end of the command line");
            }

            snprintf(outputPath, sizeof(outputPath), "%s", argv[++i]);
        }
        else if(strcmp(argv[i], "-s") == 0){
            if(i == argc-1){
                perror("Option arguments cannot be at the end of the command line");
            }

            snprintf(quarantinePath, sizeof(quarantinePath), "%s", argv[++i]);
        }
        else{
            struct stat statbuf;
            lstat(argv[i], &statbuf);
            if(!S_ISDIR(statbuf.st_mode))
                return 0;
        }
    }

    return 1;


}

//Utility function for helping formatting the dir tree output
void printIndent(int indent){
    while (indent--) {
        printf("|   ");
    }
}

void fillSnapshot(char* snapPath, struct stat statbuf, struct dirent* dp, int *out){

    // Open the file if it is not already open
    if (*out == -1) {
        *out = open(snapPath, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644);
        if (*out < 0) {
            perror("Error at opening the snapshot");
            return;
        }
    }

    char metadata[1000];
    snprintf(metadata, sizeof(metadata), "Entry: %s\nSize: %ld bytes\nLastModified: %lo\nPermissions: %o\n\n",
             dp->d_name, statbuf.st_size, statbuf.st_mtime, statbuf.st_mode);

    write(*out, metadata, strlen(metadata));
}

//Function for getting the directory name for the snapshot naming
char *getDirName(char* path){
    char* dirName = strrchr(path, '/');
                if (dirName != NULL){
                    dirName++; // Skip the '/'
                } else {
                    dirName = path; // Use the whole path if no '/'
                }
    return dirName;

}

int checkPermissions(char* path, struct stat statbuf){

    if (stat(path, &statbuf) == 0){
        // Check individual permissions
        if (((statbuf.st_mode & S_IRUSR) == 0) && ((statbuf.st_mode & S_IWUSR) == 0) && ((statbuf.st_mode & S_IXUSR) == 0) &&
            ((statbuf.st_mode & S_IRGRP) == 0) && ((statbuf.st_mode & S_IWGRP) == 0) && ((statbuf.st_mode & S_IXGRP) == 0) &&
            ((statbuf.st_mode & S_IROTH) == 0) && ((statbuf.st_mode & S_IWOTH) == 0) && ((statbuf.st_mode & S_IXOTH) == 0)){
            return 1;
        }
    } else {
        perror("Error reading file information");
    }

    return 0;

}


int dirRead(char* basePath){

    DIR *dir;
    struct dirent *dp;
    char path[1000], snapPath[1000];
    struct stat statbuf;
    int suspFiles = 0;

    if ((dir = opendir(basePath)) == NULL){
        perror("Cannot open directory");
    }

    //If outputPath has been modified -> the optional argument is used -> change the paths accordingly
    if(strcmp(outputPath, "\0") != 0){
        snprintf(snapPath, sizeof(snapPath), "%s/%s_Snapshot.txt", outputPath, getDirName(basePath));
    }
    else
        snprintf(snapPath, sizeof(snapPath), "%s/Snapshot.txt", basePath);


    if(strcmp(quarantinePath, "\0") == 0){
        snprintf(quarantinePath, sizeof(quarantinePath), "quarantine");
    }
    
    int out = -1; //This variable tells the program if the snapshot file is opened or not. Without it would only store data for the last file scanned
    while ((dp = readdir(dir)) != NULL){
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") && strcmp(dp->d_name, "Snapshot.txt")!= 0){

            snprintf(path, sizeof(path), "%s/%s", basePath, dp->d_name);
            if (stat(path, &statbuf) == -1){
               continue;
            }

            if(checkPermissions(path, statbuf)){
                char command[2000];
                snprintf(command, sizeof(command), "./checkSafe.sh %s %s", path, quarantinePath);
                suspFiles++;
                system(command);
                continue;

            }else{
            fillSnapshot(snapPath, statbuf, dp, &out);
            }
            // Check if it's a directory or file
            if (S_ISDIR(statbuf.st_mode)) {
                // Recursively call with increased depth
                dirRead(path);
            }

        }
    }

    close(out);
    closedir(dir);
    printf("Snapshot created for directory %s\n", getDirName(basePath));
    return suspFiles;
}

//Utility function for displaying the directory tree, it skips special files like snapshots, output/quarantine dirs
void dirShow(char* basePath, int depth){

    DIR *dir;
    struct dirent *dp;
    char path[1000];
    struct stat statbuf;

    if ((dir = opendir(basePath)) == NULL){
        perror("Cannot open directory");
        return;
    }
    
    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") && strcmp(dp->d_name, "Snapshot.txt")!= 0){
            snprintf(path, sizeof(path), "%s/%s", basePath, dp->d_name);
            if (stat(path, &statbuf) == -1){
               continue;
            }

            // Check if it's a directory or file
            if (S_ISDIR(statbuf.st_mode)) {
                printIndent(depth);
                printf("|_ %s\n", dp->d_name);
                // Recursively call with increased depth
                dirShow(path, depth + 1);
            } else {
                printIndent(depth);
                printf("|_ %s\n", dp->d_name);
            }
        }
    }

    closedir(dir);
}


void dirParse(int argc, char** argv){
    int status;
    int suspFiles = 0;
    for (int i = 1; i < argc; i++){
        if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "-s") == 0)) {
            i++;
            continue;
        }

        int pid = fork();

        if (pid == 0) {
            suspFiles = dirRead(argv[i]);
            exit(0);
        } else {
            wait(&status);
            if(WIFEXITED(status)){
                int exitCode = WEXITSTATUS(status);
                printf("Child process %d has been executed with exit code %d and %d dangerous files\n", pid, exitCode, suspFiles);
            }
        }
    }

    printf("\n\n");

    for(int i = 1; i < argc; i++){
        if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "-s") == 0)) {
            i++;
            continue;
        }
        printf("%s\n", argv[i]);
        dirShow(argv[i], 0);
        putchar('\n');
    }

}


int main(int argc, char** argv){


    if(argc > 10 && argc < 2){
        perror("Invalid number of arguments\n");
    }

    if(!checkArguments(argc, argv))
        perror("Only directories are allowed as arguments\n");
    
    dirParse(argc, argv);
    
    return 0;
}