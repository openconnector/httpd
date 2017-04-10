#include "h_ungzip.h"

int inflate_read(char *source,int len,char *dest,int *dest_len)
{ 
	int   ret;
	z_stream   strm;
	
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in =	len;
	strm.next_in = source;
	strm.avail_out = *dest_len;
	strm.next_out = dest;

	ret = inflateInit2(&strm, 47);
	if(ret != Z_OK){
		printf("ret error ret=%d\n",ret);
		return   ret;
	}
	ret = inflate(&strm,   Z_NO_FLUSH);
	*dest_len = strm.total_out;
	inflateEnd(&strm);
	return ret;
}

