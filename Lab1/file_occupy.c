#include <sys/file.h>
#include <stdlib.h>
#include <stdio.h>
int main(int argc,char *argv[]){
    FILE **files;
    int i;
    int fd,lock;
    if(argc<=1){
        fprintf(stderr,"USAGE: file_occupy FILE1 FILE2 ...");
    }
    files=(FILE **)malloc(sizeof(FILE *)*(argc+1));
    for(i=1;i<argc;i++){
        files[i]=fopen(argv[i],"r");
        if(files[i]==NULL){
            fprintf(stderr, "FILE %s does not exist.\n", argv[i]);
        }else{
            fd=fileno(files[i]);
            lock=flock(fd,LOCK_EX);
            fprintf(stderr, "FILE %s lock result:%d\n",argv[i],lock);
        }
    }
    getchar();
    for(i=1;i<argc;i++){
        fclose(files[i]);
    }
    return 0;
}
