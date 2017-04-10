#ifndef h_string
#define h_string

#include <stdio.h>
#include <string.h>
	
//判断字符串line是否以part开头
int str_starts_with(char *line,char *part);

//从字符串line中获取sep后面的子串
void str_get_part_next(char *line,char *sep,char *part);
#endif
