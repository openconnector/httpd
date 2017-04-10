#include "http.h"

#define BUF_SIZE  65536
#define LINE_SIZE 1024
static char pack_buf[BUF_SIZE];
static char tcp_buf[BUF_SIZE];
static int host;
static dict *hash_map;

int main(int argc,char *argv[]){
	filter *ft = malloc(sizeof(filter));
	init_filter(ft);
	int flag = get_param(argc,argv,ft);
	if(flag == 0){
		usage();
		return 0;
	}
	init_logger(ft->path);
	int recv_len;
	char *packet = pack_buf;
	struct iphdr *ip;
	host = inet_addr(ft->ip);
	int fd = socket(PF_PACKET,SOCK_DGRAM,htons(ETH_P_ALL));
	if(fd == -1){
		printf("fd error errno=%d\n",errno);
		return -1;
	}
	hash_map = malloc(sizeof(dict));
	init_hash_map(hash_map);	
	for(;;){
		memset(packet,'\0',BUF_SIZE);
		recv_len = recvfrom(fd,packet,BUF_SIZE,0,NULL,NULL);
		if(recv_len <= 0){
			printf("recv_len error errno=%d\n",errno);
			return 0;
		}
		ip = (struct iphdr *)packet;
		if(ip->protocol != IPPROTO_TCP){
			continue;
		}	
		processIP(ip,packet,ft);
	}
	return 0;	
}

void init_filter(filter *ft){
	ft->port = 0;
	memset(ft->url,'\0',PATH_LEN);
	memset(ft->path,'\0',PATH_LEN);
	memset(ft->vurl,'\0',PATH_LEN);
}

int get_param(int argc,char *argv[],filter *ft){
	char port_str[6];
	int i=0;
	for(i=1;i<argc-1;i+=2){
		char *line = argv[i];
		char *value = argv[i+1];
		if(strcmp(line,"-h") == 0){
			strcpy(ft->ip,value);
		}else if(strcmp(line,"-p") == 0){
			strcpy(port_str,value);
			ft->port = atoi(port_str);
		}else if(strcmp(line,"-url") == 0){
			strcpy(ft->url,value);
		}else if(strcmp(line,"-f") == 0){
			strcpy(ft->path,value);
		}else if(strcmp(line,"-vurl") == 0){
			strcpy(ft->vurl,value);
		}
	}
	if(strlen(ft->ip)>0){
		return 1;
	}
}

void processIP(struct iphdr *ip,char *packet,filter *ft){
	struct tcphdr *tcp;
	unsigned char *ipaddr = (unsigned char *)&ip->saddr;
	int ip_headlen = (ip->ihl) << 2;
	int ip_total = ntohs(ip->tot_len);
	tcp =(struct tcphdr*)((char *)ip + ip_headlen);	
	int tcp_headlen = (tcp->doff) << 2;
	int tcp_len = ip_total - ip_headlen - tcp_headlen;
	char* tcp_data = (char*)(packet + ip_headlen + tcp_headlen);
	u_int16_t fin = tcp->fin;//tcp中的值事先在此取出，否则后面有可能会出现内存访问错误
	//过滤端口
	int des_port = ntohs(tcp->dest);
	int source_port = ntohs(tcp->source);

	if(ft->port != 0){
		if(source_port != ft->port && des_port != ft->port){
			return;
		}
	}
	char key[32];
	memset(key,'\0',32);
	get_key(ip,tcp,key);
	if(strlen(key) == 0){
		return;
	}
	req_res *rs = (req_res*)h_get(hash_map,key);
	char line[LINE_SIZE];
	if(rs == NULL){
		int num = h_readline(tcp_data,tcp_len,line);
		if(num == 0){
			return;
		}
		char method[8];
		int f = get_method(line,method);
		if(f == true){
			 if(strlen(ft->url) > 0 && strstr(line,ft->url) == NULL){
				 return;
			 }
			 if(strlen(ft->vurl) > 0 && strstr(line,ft->vurl) != NULL){
				 return;
			 }
			rs = (req_res*)malloc(sizeof(req_res));
			init_req_res(key,rs);
			add_http_method(method,rs,ip);
			h_put(hash_map,key,rs);
			if(tcp_len <= 0){
				return;
			}
		}else{
			return;
		}
	}
	if(is_req(ip,rs)){
		if(rs->flag == req_head_not){
			int num = 0;
			while((num=h_readline(tcp_data,tcp_len,line)) > 0){
				tcp_len -= num;
				tcp_data = tcp_data + num;
				if(strcmp(line,"\r\n") == 0){
					rs->flag = req_head_ok;
					add_to_req_head(line,rs);
					add_req_content_length(rs);
					break;
				}
				add_to_req_head(line,rs);
			}
		}
		if(rs->flag == req_head_ok){
			if(strcmp(rs->method,GET) == 0){
				rs->flag = res_head_not;
				return;
			}else{
				int need = rs->req_content_length - rs->req_read_length;
				if(need <= tcp_len){
					memcpy(rs->req_content+rs->req_read_length,tcp_data,need);
					rs->req_read_length =rs->req_content_length;
					rs->flag = res_head_not;
				}else{
					memcpy(rs->req_content+rs->req_read_length,tcp_data,tcp_len);
					rs->req_read_length += tcp_len;
				}	
			}
		}
	}else{
		if(rs->res_flag == res_head_not){
			int num = 0;
			while((num = h_readline(tcp_data,tcp_len,line)) > 0){
				  tcp_len -= num;
				  tcp_data = tcp_data + num;
				  if(strcmp(line,"\r\n") == 0){
					  rs->res_flag = res_head_ok;
					  add_to_res_head(line,rs);
					  add_res_cl_te(rs);
					  break;
					}
					add_to_res_head(line,rs);
			}
		 }
		 if(rs->res_flag == res_head_ok){
			if(not_200(rs) || rs->res_content_zero){
				rs->flag = res_ok;
			}else if(rs->transfer_encoding){
				read_chunk(rs,tcp_data,tcp_len);
			}else if(rs->res_content_length){
				read_with_content_length(rs,tcp_data,tcp_len);
//				if(fin){
//					rs->flag = res_ok;
//				}else{
//					read_with_content_length(rs,tcp_data,tcp_len);
//				}
			}else{
				if(fin){
					rs->flag = res_ok;
				}else{
					read_with_closed(rs,tcp_data,tcp_len);
				}
			}
		}
		if(rs->flag == res_ok){
			if(rs->end_time  == 0){
				rs->end_time = get_time();
			}
			ungzip(rs);//如果Content-Encoding: gzip，则解压
			destroy_rs(rs);
		}
	}
}

//@获取rs对应的key
void get_key(struct iphdr *ip,struct tcphdr *tcp,char *key){
	char s_ip[8],s_port[8],d_ip[8],d_port[8];	
	if(ip->saddr == host){
		sprintf(s_ip,"%d",ip->saddr);
		sprintf(s_port,"%d",tcp->source);
		sprintf(d_ip,"%d",ip->daddr);
		sprintf(d_port,"%d",tcp->dest);
	}else if(ip->daddr == host){
		sprintf(s_ip,"%d",ip->daddr);
		sprintf(s_port,"%d",tcp->dest);
		sprintf(d_ip,"%d",ip->saddr);
		sprintf(d_port,"%d",tcp->source);
	}else{
		return;
	}
    strcpy(key,s_ip);
    strcat(key,s_port);
    strcat(key,"_");
    strcat(key,d_ip);
    strcat(key,d_port);	
}

void init_req_res(char *key,req_res *rs){
	rs->method = (char*)malloc(sizeof(char) * 8);
	if(key != NULL){
		rs->key = malloc(sizeof(char) * 30);
		strcpy(rs->key,key);
	}
	rs->req_ip = 0;
	rs->res_ip = 0;
	rs->flag = req_head_not;
	rs->req_content_length = 0;
	rs->req_read_length = 0;
	rs->res_read_length = 0;
	rs->res_head = malloc(sizeof(array_list));
	rs->req_head = malloc(sizeof(array_list));
	init_array_list(rs->req_head);
	init_array_list(rs->res_head);
	rs->req_content = NULL;
	rs->res_content = NULL;
	rs->chunk_length = 0;
	rs->chunk_total_length = 0;
	rs->chunk_read_length = 0;
	rs->res_cur_length = 0;
	rs->res_content_length = 0;
	rs->res_read_length = 0;
	rs->res_flag = res_head_not;
	rs->transfer_encoding = 0;
	rs->start_time = get_time();
	rs->end_time = 0;
	rs->res_content_zero = 0;
}

int get_method(char *line, char *method){
	if(line == NULL){
		return false;
	}
	int len = strlen(line);
	if(len > 8){
		len = 8;	
	}
	int i;
	for(i=0; i<len; i++){
		if(*line != ' '){
			method[i] = *line;
		}else{
			break;
		}	
		line++;
	}
	method[i] = '\0';
	if(strcmp(GET,method) != 0 && strcmp(POST,method) != 0){
		return false;
	}
	char *http10 = strstr(line,"HTTP/1.0\r\n");
	char *http11 = strstr(line,"HTTP/1.1\r\n"); 	
	if(http10 != NULL || http11 != NULL){
		return true;
	}
	return false;
	
}


void add_http_method(char *method,req_res *rs,struct iphdr *ip){
	strcpy(rs->method,method);
	rs->req_ip = ip->saddr;
	rs->res_ip = ip->daddr;
}

int is_req(struct iphdr *ip,req_res *rs){
	if(rs->req_ip == 0){
		return false;
	}
	if(ip->saddr == rs->req_ip){
		return true;
	}else{
		return false;
	}
}

int h_readline(char *packet,int tcp_len,char *line){
	if(tcp_len <= 0){
		return 0;
	}
	int num = 0;
	int i;
	for(i=0;i<tcp_len;i++){
		if(i >= LINE_SIZE){
//			printf("too long line,line=%s\n",line);
			return 0;
		}
		line[i] = packet[i];
		num++;
		if(packet[i] == '\n'){
			line[i+1] = '\0';
			break;	
		} 			
	}
	return num;
}


void add_to_req_head(char *line,req_res *rs){
	if(line == NULL){
		return;
	}
	int len = strlen(line);
	char *l = malloc(sizeof(char) * (len + 1));
	strcpy(l,line);
	a_add(rs->req_head,l);
}

void add_to_res_head(char *line,req_res *rs){
	if(line == NULL){
		return;
	}
	int len = strlen(line);
	char *l = malloc(sizeof(char) * (len + 1));
	strcpy(l,line);
	a_add(rs->res_head,l);
}

void add_req_content_length(req_res *rs){
	int content_length = get_content_length(rs->req_head);
	if(content_length > 0){
		rs->req_content_length = content_length;
		rs->req_content = malloc(sizeof(char) * content_length);
	}
}


int get_content_length(array_list *list){
	char *name="Content-Length:";
	char value[8] = {'\0'};
	get_header(list,name,value);
	if(strlen(value) > 0){
		return atoi(value);
	}else{
		return -1;
	}
}

void get_header(array_list *list,char *head_name,char *head_value){
	if(list == NULL){
		return;
	}
	int i;
	int len = a_size(list);
	for(i=0;i<len;i++){
		char *line = a_get(list,i);
		if(line == NULL){
			continue;
		}
		if(str_starts_with(line,head_name)){
			str_get_part_next(line,": ",head_value);
			int len = strlen(head_value);
			int j;
			for(j=0;j<len;j++){
				if(*(head_value+j) == '\r'){
					*(head_value + j) = '\0';
					return;
				}
			}
			return;
		}
	}
}

//判断是否是tranfer_encoding编码
//如果存在content-length则获取该长度
void add_res_cl_te(req_res *rs){
	if(rs == NULL || rs->res_head == NULL){
		return;
	}
	int i,len=a_size(rs->res_head);
	for(i=0;i<len;i++){
		char *line = a_get(rs->res_head,i);
		if(str_starts_with(line,"Transfer-Encoding:")){
			rs->transfer_encoding = 1;
			break;
		}
	}
	int length = get_content_length(rs->res_head);
	if(length == 0){
		rs->res_content_zero = 1;
	}else if(length > 0){
		rs->res_content_length = length;
		rs->res_read_length = 0;
		rs->res_content = (char *)malloc(sizeof(char) * length);
	}
}

//
void read_chunk(req_res *rs,char *tcp_data,int tcp_len){
	while(rs->flag!=res_ok && tcp_len>0){
		char line[LINE_SIZE];
		if(rs->chunk_length == 0){
			int num = h_readline(tcp_data,tcp_len,line);
			if(num == 0 || strlen(line) == 0){
				printf("###line is null,line=%s\n",line);
				break;
			}
			rs->chunk_length = strtol(line,NULL,16);
			if(rs->chunk_length == 0){
				rs->flag = res_ok;
				break;
			}
			rs->chunk_read_length = 0;
			tcp_data = tcp_data + num;
			tcp_len -= num;
			rs->chunk_total_length += rs->chunk_length;
			if(rs->res_content == NULL){
				rs->res_content = (char *)malloc(sizeof(char) * rs->chunk_total_length);
				if(rs->res_content == NULL){
					printf("chunk malloc fail\n");
					break;
				}
			}else{
				rs->res_content = (char *)realloc(rs->res_content,sizeof(char) * rs->chunk_total_length);
				if(rs->res_content == NULL){
					printf("chunk realloc fail,chun_total_length=%d\n",rs->chunk_total_length);
					printf("chunk errno=%d\n",errno);
					break;
				}
			}
		}else{
			int left = rs->chunk_length - rs->chunk_read_length;//一个chunk中尚未读取的字节数
			int start = rs->chunk_total_length - (rs->chunk_length-rs->chunk_read_length);
			if(left <= tcp_len){
				memcpy(rs->res_content + start,tcp_data,left);
				rs->chunk_length = 0;
				rs->chunk_read_length = 0;
				tcp_len -= left + 2;//chunk body后面的回车换行
				tcp_data = tcp_data + left + 2;
			}else{
				memcpy(rs->res_content + start,tcp_data,tcp_len);
				rs->chunk_read_length += tcp_len;
				tcp_len = 0;
			}
		}
	}
}

void read_with_closed(req_res *rs,char *tcp_data,int tcp_len){
	int pos = rs->res_cur_length;
	if(rs->res_cur_length == 0){
		rs->res_cur_length = tcp_len;
		rs->res_content = (char *)malloc(sizeof(char) * tcp_len);
	}else{
		rs->res_cur_length += tcp_len;
		rs->res_content = (char *)realloc(rs->res_content,sizeof(char) * rs->res_cur_length);
	}
	memcpy(rs->res_content+pos,tcp_data,tcp_len);
}

void read_with_content_length(req_res *rs,char *tcp_data,int tcp_len){
	int need = rs->res_content_length - rs->res_read_length;
	if(need <= tcp_len){
		memcpy(rs->res_content+rs->res_read_length,tcp_data,need);
		rs->res_read_length = rs->res_content_length;
		rs->flag = res_ok;
	}else{
		memcpy(rs->res_content+rs->res_read_length,tcp_data,tcp_len);
		rs->res_read_length += tcp_len;
	}
}

void destroy_rs(req_res *rs){
	if(rs == NULL){
		return;
	}
	//打印抓取到的信息
	print_rs(rs,0);
	if(rs->key != NULL){
		h_remove(hash_map,rs->key);
		free(rs->key);
	}
	if(rs->method != NULL){
		free(rs->method);
	}
	a_destroy(rs->req_head);
	a_destroy(rs->res_head);
	rs->req_head = NULL;
	rs->res_head = NULL;
	if(rs->req_content != NULL){
		free(rs->req_content);
		rs->req_content = NULL;
	}
	if(rs->res_content != NULL){
		free(rs->res_content);
		rs->res_content = NULL;
	}
	free(rs);
	rs = NULL;
}

void print_rs(req_res *rs,int flag){
	char req_ip[20];
	char res_ip[20];
	ip_transfer(rs->req_ip,req_ip);
	ip_transfer(rs->res_ip,res_ip);
	char pr[80];
	memset(pr,'\0',80);
	append_str(pr,"request_ip:",req_ip);
	append_str(pr," response_ip:",res_ip);
	char t[10];
	sprintf(t,"%lld",rs->end_time-rs->start_time);
	append_str(pr," ## time=",t);
	strcat(pr,"\n");

	char *line = NULL;
	size_t i;
	size_t req_head_size = a_size(rs->req_head);
	char rh[100] = "\n########## request header## ";
	strcat(rh,pr);

	if(flag){
		printf("%s",rh);
	}
	info(rh,strlen(rh)+1);
	for(i=0; i<req_head_size; i++){
		line = a_get(rs->req_head,i);
		if(line != NULL){
			if(flag){
				printf("%s",line);
			}
			info(line,strlen(line)+1);
		}
	}
	char req_body[100] = "\n########## request body## ";
	strcat(req_body,pr);
	if(flag){
		printf("%s",req_body);
	}
	info(req_body,strlen(req_body)+1);
	if(rs->req_content != NULL){
		info(rs->req_content,rs->req_content_length);
		if(flag){
			printf("%s",rs->req_content);
		}
	}
	if(flag){
		printf("\n");
	}
	char reqh[100] = "\n########## response header## ";
	strcat(reqh,pr);
	if(flag){
		printf("%s",reqh);
	}
	info(reqh,strlen(reqh) + 1);
	size_t res_head_size = a_size(rs->res_head);
	for(i=0; i<res_head_size; i++){
		line = a_get(rs->res_head,i);
		if(line != NULL){
			if(flag){
				printf("%s",line);
			}
			info(line,strlen(line) + 1);
		}
	}
	if(flag){
		printf("\n");
	}
	char res_body[100] = "\n########## response body## ";
	strcat(res_body,pr);
	info(res_body,strlen(res_body) + 1);
	if(flag){
		printf("%s",res_body);
	}
	if(rs->res_content != NULL){
		log_content(rs);
		if(flag){
			printf("%s",rs->res_content);
			printf("\r\n");
		}
	}
}

void log_content(req_res *rs){
	if(rs->transfer_encoding){
		info(rs->res_content,rs->chunk_total_length);
	}else if(rs->res_content_length){
		info(rs->res_content,rs->res_content_length);
	}else{
		info(rs->res_content,rs->res_cur_length);
	}
}

//如果Content-Encoding: gzip，则解压
void ungzip(req_res *rs){
	char *name = "Content-Encoding:";
	char value[32];
	get_header(rs->res_head,name,value);
	if(strlen(value) > 0 && strcmp("gzip",value) == 0){
		int dlen = 1024 * 1024 * 2;
		int *dest_len = &dlen;
		char *dest = malloc(sizeof(char) * (*dest_len));
		int flag = 0;
		if(rs->transfer_encoding){
			flag = inflate_read(rs->res_content,rs->chunk_total_length,dest,dest_len);
			rs->chunk_total_length = *dest_len;
		}else if(rs->res_content_length){
			flag = inflate_read(rs->res_content,rs->res_content_length,dest,dest_len);
			rs->res_content_length = *dest_len;
		}else{
			flag = inflate_read(rs->res_content,rs->res_cur_length,dest,dest_len);
			rs->res_cur_length = *dest_len;
		}
		free(rs->res_content);
		rs->res_content = NULL;
		rs->res_content = malloc(sizeof(char) * (*dest_len));
		memcpy(rs->res_content,dest,*dest_len);
		free(dest);
		dest_len = NULL;
	}
}

//获取接收缓冲区大小
void get_recv_buf_size(int fd){
	socklen_t optlen;
	int rcv_size = 0;
	optlen = sizeof(rcv_size);
	int err = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcv_size, &optlen);
}
void set_recv_buf_size(int fd,int size){
	 int rcv_size = size;
	 socklen_t optlen = sizeof(rcv_size);
	 int err = setsockopt(fd,SOL_SOCKET,SO_RCVBUF, (char *)&rcv_size, optlen);
}

void ip_transfer(int ip,char *value){
	struct in_addr inaddr;
	inaddr.s_addr = ip;
	char *ipaddr= inet_ntoa(inaddr);
	strcpy(value,ipaddr);
}

void append_str(char *str,char *str1,char *str2){
	strcat(str,str1);
	strcat(str,str2);
}

int not_200(req_res *rs){
	array_list *list = rs->res_head;
	int size = a_size(list);
	if(size > 0){
		char *line = a_get(list,0);
		if(strstr(line,"200 OK") != NULL){
			return false;
		}
	}
	return true;
}

void usage(){
	printf("-h          当前主机ip,该参数为必选项,以下其他参数为可选项\r\n\r\n");
	printf("-p          要过滤的端口号\r\n\r\n");
	printf("-url        想要抓取的url,注意：此url不包含域名\r\n\r\n");
	printf("-vurl       想要过滤掉的url,注意：此url不包含域名\r\n\r\n");
	printf("-f          抓取内容保存的文件目录\r\n\r\n");
}

