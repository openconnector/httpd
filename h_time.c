#include "h_time.h"

long long get_time(){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	long long k = 1000;
	long long msec = tv.tv_sec * k + tv.tv_usec/k;
	return msec;
}

void get_times(struct timeval *tv){
	gettimeofday(tv,NULL);
}


