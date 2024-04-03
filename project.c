#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>


int checkForDirectories(int argc, char** argv){

    for(int i = 1; i < argc; i++){
        struct stat statbuf;
        lstat(argv[i], &statbuf);
        if(!S_ISDIR(statbuf.st_mode))
            return 0;

    }

    return 1;


}

void dirRead(char* filename){

    DIR *dir;

    if((dir = opendir(filename)) == NULL){
        perror("Input directory not found");
    }

    char filepath[512];

    struct dirent *file;
    while((file = readdir(dir))){

        if(file->d_name[0] != '.'){
            struct stat statbuf;
            snprintf(filepath, sizeof(filepath), "%s/%s", filename, file->d_name);
            if(stat(filepath, &statbuf) == -1){
                perror("Error at stat");
                continue;
            }

            if(S_ISDIR(statbuf.st_mode)){
                printf("%s\n  ", file->d_name);
                dirRead(filepath);
            }
            else printf("-%s\n", file->d_name);


        }


    }

    printf("  ");


}

void dirParse(char argc, char** argv){

    for(int i = 1; i < argc; i++){
        printf("%s\n  ", argv[i]);
        dirRead(argv[i]);
        putchar('\n');
    }


}


int main(int argc, char** argv){


    if(argc > 10){
        perror("Invalid number of arguments\n");
    }

    if(!checkForDirectories(argc, argv))
        perror("Only directories are allowed as arguments\n");

    
    dirParse(argc, argv);
    


    return 0;
}