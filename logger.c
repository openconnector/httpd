#include "logger.h"

static int fd = -1;

void init_logger(char *path){
	if(strlen(path) > 0){
		fd = open(path,O_RDWR|O_CREAT |O_TRUNC|O_APPEND,0755);
	}else{
		fd =STDOUT_FILENO;
	}
}

void info(char *buf,size_t count){
	if(buf[count-1] == '\0'){
		count -= 1;
	}
	write(fd,buf,count);
}


