#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

char outputPath[128] = "\0";

int checkArguments(int argc, char** argv){

    for(int i = 1; i < argc; i++){
        
        if(strcmp(argv[i], "-o") == 0){
            if(i == argc-1){
                perror("Option arguments cannot be at the end of the command line");
            }

            snprintf(outputPath, sizeof(outputPath), "%s", argv[++i]);

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

void printIndent(int indent) {
    while (indent--) {
        printf("|   ");
    }
}

void fillSnapshot(char* snapPath, struct stat statbuf, struct dirent* dp) {
    int out;
    char metadata[1000];

    // Open the file in append mode or create it if it doesn't exist
    if ((out = open(snapPath, O_WRONLY | O_CREAT | O_TRUNC , 0644)) < 0) {
        perror("Error at opening the snapshot");
        return;
    }

    snprintf(metadata, sizeof(metadata), "Entry: %s\nSize: %ld bytes\nLastModified: %lo\nPermissions: %o\n\n",
             dp->d_name, statbuf.st_size, statbuf.st_mtime, statbuf.st_mode);

    write(out, metadata, strlen(metadata));
    close(out);

}


char *getDirName(char* path){
    char* dirName = strrchr(path, '/');
                if (dirName != NULL) {
                    dirName++; // Skip the '/'
                } else {
                    dirName = path; // Use the whole path if no '/'
                }
    return dirName;

}

void dirRead(char* basePath, int depth) {
    DIR *dir;
    struct dirent *dp;
    char path[1000], snapPath[1000];
    struct stat statbuf;

    if ((dir = opendir(basePath)) == NULL) {
        perror("Cannot open directory");
        return;
    }



    if(strcmp(outputPath, "\0") != 0){
        snprintf(snapPath, sizeof(snapPath), "%s/%s_Snapshot.txt", outputPath, getDirName(basePath));
    }
    else
        snprintf(snapPath, sizeof(snapPath), "%s/Snapshot.txt", basePath);
    

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") && strcmp(dp->d_name, "Snapshot.txt")!= 0) {

            snprintf(path, sizeof(path), "%s/%s", basePath, dp->d_name);
            if (stat(path, &statbuf) == -1){
               continue;
            }

            
            fillSnapshot(snapPath, statbuf, dp);

            // Check if it's a directory or file
            if (S_ISDIR(statbuf.st_mode)) {
                printIndent(depth);
                printf("|_ %s\n", dp->d_name);
                // Recursively call with increased depth
                dirRead(path, depth + 1);
            } else {
                printIndent(depth);
                printf("|_ %s\n", dp->d_name);
            }
        }
    }

    closedir(dir);
}


void dirParse(char argc, char** argv){

    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-o") == 0){
            i++;
            continue;

        }
        printf("%s\n", argv[i]);
        dirRead(argv[i], 0);
        putchar('\n');
    }


}


int main(int argc, char** argv){


    if(argc > 10){
        perror("Invalid number of arguments\n");
    }

    if(!checkArguments(argc, argv))
        perror("Only directories are allowed as arguments\n");

    
    dirParse(argc, argv);
    


    return 0;
}