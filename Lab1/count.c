#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define MAX_PATH 260

int main(int argc,char *argv[]){
    char in_file_path[MAX_PATH],out_file_path[MAX_PATH]; /* file path */
    FILE *in_file,*out_file; /* file pointer */
    char search_string[MAX_PATH]; 
    char *in_file_content;
    int in_file_len=0;
    unsigned long search_count=0;

    int ch;
    char *search_ptr;

    /* test command line format */
    if(argc!=4){
        fprintf(stderr,"Incorrect command line format.\n");
        fprintf(stderr,"USAGE: count input_filename search_string output_filename\n");
        return 1;
    }

    /* fetch command line auguments */
    strcpy(in_file_path,argv[1]);
    strcpy(out_file_path,argv[3]);
    strcpy(search_string, argv[2]);

    /* get in_file size without open it */
    unsigned long in_file_size = -1;
    struct stat in_file_stat;
    if(stat(in_file_path,&in_file_stat)>=0)
        in_file_size=in_file_stat.st_size;
    fprintf(stdout,"Size of file is %lu\n",in_file_size);


    /* open in_file */
    in_file=fopen(in_file_path,"r");
    if(in_file==NULL){
        fprintf(stderr,"Cannot open file %s\n",in_file_path);
        fprintf(stderr,"%s\n",strerror(errno));
        return 2;
    }

    /* open out_file */
    out_file=fopen(out_file_path,"w");
    if(out_file==NULL){
        fprintf(stderr,"Cannot open file %s\n",out_file_path);
        fprintf(stderr,"%s\n",strerror(errno));
        return 3;
    }
    if(strlen(search_string)==0){
        fprintf(stderr,"Invalid search string\n");
        return 4;
    }

    /* output file size to out_file */
    fprintf(out_file,"Size of file is %lu\n",in_file_size);

    /* read file content */
    in_file_content=(char *)malloc(in_file_size);
    in_file_len=0;
    ch=fgetc(in_file);
    while(ch!=EOF){
        in_file_content[in_file_len++]=(char)ch;
        ch=fgetc(in_file);
    }

    /* count search_strings in in_file_content
     * search_ptr: indicate where last string occurs */
    search_count=0;
    search_ptr=strstr(in_file_content,search_string);
    while(search_ptr!=NULL){
        search_count++;
        search_ptr++;
        search_ptr=strstr(search_ptr,search_string);
    }

    /* output search result */
    fprintf(stdout,"Number of matches = %lu\n",search_count);
    fprintf(out_file,"Number of matches = %lu\n",search_count);

    return 0;
}
