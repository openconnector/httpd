#ifndef HTTP_H
#define HTTP_H
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "array_list.h"
#include "hash_map.h"
#include "h_string.h"
#include "logger.h"
#include "h_ungzip.h"
#include "h_time.h"

#define req_head_not 0
#define req_head_ok 1
#define res_head_not 2
#define res_head_ok 3
#define res_ok 4

#define chunk_blank_not  0
#define chunk_blank_ok  1


#define true 1
#define false 0

#define POST "POST"
#define GET "GET"

#define PATH_LEN 256


typedef struct req_res{
	char *key;
	char *method;
	int req_ip;
	int res_ip;
	array_list *req_head;
	array_list *res_head;		
	char *req_content;
	char *res_content;	
	int flag;				//0表示req头未读完，1表示req头已读完，2表示res头未读完，3表示res头已读完,4表示res内容读取完毕
	int res_flag;			//2表示res头未读完，3表示res头已读完
	int req_content_length;
	int req_read_length;	//已经读取的长度
	int transfer_encoding;  //0表示该字段未赋值，1表示chunked
	int res_content_length;	
	int res_read_length;
	int res_cur_length;

	//读取chunk相关
	int chunk_length;		//chunk大小
	int	chunk_total_length; //目前为止总共申请的空间
	int chunk_read_length;  //当前chunk已经读取的字节数

	long long start_time; 	//抓取请求时间
	long long end_time;		//抓取结束时间

	int res_content_zero;	//response content-length为0


}req_res;

//过滤请求使用
typedef struct filter{
	char ip[16];
	int port;			//过滤用端口号
	char url[PATH_LEN];	//过滤url使用,保留该url的数据
	char vurl[PATH_LEN];//过滤url使用,排除该url的数据
	char path[PATH_LEN];//文件路径
}filter;

void init_filter(filter *ft);

void processIP(struct iphdr *ip,char *packet,filter *ft);
//本机ip+本机port_远端ip+远端port
void get_key(struct iphdr *ip,struct tcphdr *tcp,char *key);

void init_req_res(char *key,req_res *rs);

int get_method(char *line,char *method);

void add_http_method(char *method,req_res *rs,struct iphdr *ip);

int is_req(struct iphdr *ip,req_res *rs);

int h_readline(char *packet,int tcp_len,char *line);

void add_to_req_head(char *line,req_res *rs);

void add_to_res_head(char *line,req_res *rs);

void add_req_content_length(req_res *rs);

int get_content_length(array_list *list);

void read_chunk(req_res *rs,char *packet,int tcp_len);

void read_with_closed(req_res *rs,char *packet,int tcp_len);

void read_with_content_length(req_res *rs,char *packet,int tcp_len);

//将content_length 或transfer_encoding 加入rs
void add_res_cl_te(req_res *rs);

void destroy_rs(req_res *rs);

void print_rs(req_res *rs,int flag);

void log_content(req_res *rs);

int get_param(int argc,char *argv[],filter *ft);


void ungzip(req_res *rs);

void get_header(array_list *list,char *head_name,char *head_value);

void get_recv_buf_size(int fd);

void set_recv_buf_size(int fd,int size);

//将整型ip地址转换成点分十进制形式
void ip_transfer(int ip,char *value);

void append_str(char *str,char *str1,char *str2);

//非200时无需处理response body
int not_200(req_res *rs);

void usage();
#endif
