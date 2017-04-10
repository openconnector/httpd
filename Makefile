obj=http.o array_list.o hash_map.o h_string.o logger.o h_ungzip.o h_time.o
httpd:$(obj) 
	gcc -o httpd $(obj) -lz
http.o: http.c array_list.h hash_map.h h_string.h logger.h h_ungzip.h h_time.h
array_list.o: array_list.c array_list.h
hash_map.o: hash_map.c hash_map.h
h_string.o: h_string.c h_string.h
logger.o: logger.c logger.h
h_ungzip.o: h_ungzip.c h_ungzip.h
h_time.o: h_time.c h_time.h
clean:
	rm httpd $(obj) info.log debug.log
