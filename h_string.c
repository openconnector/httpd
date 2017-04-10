#include "h_string.h"

int str_starts_with(char *line,char *part){
	if(line == NULL || part == NULL){
		return 0;
	}
	int line_len = strlen(line);
	int part_len = strlen(part);
	if(line_len < part_len){
		return 0;
	}
	int i;
	for(i=0;i<part_len;i++){
		if(line[i] != part[i]){
			return 0;
		}
	}
	return 1;
} 


void str_get_part_next(char *line,char *sep,char *part){
	if(line == NULL || sep == NULL || part == NULL){
		return;
	}
	char *p = strstr(line,sep);
	if(p == NULL){
		return;
	}
	int len = strlen(sep);
	strcpy(part,p+len);
}

