# httpd 
linux 平台下 http抓包工具，支持get和post请求方式

使用者必须拥有sudo权限，否则无法打开需要的网络套接字！！！

使用方式：
1.在源码目录下执行 sudo ./make命令，将生成httpd可执行文件
2. sudo ./httpd -h ip执行抓包操作，其中ip是运行httpd的本机ip 

其他参数使用说明 -h 当前主机ip,该参数为必选项,以下其他参数为可选项
-p 要过滤的端口号

-url 想要抓取的url,注意：此url不包含域名

-vurl 想要过滤掉的url,注意：此url不包含域名

-f 抓取内容保存的文件目录

例如：sudo ./httpd -h 192.168.1.1 -p 80 -url /test/api/t -f ./info.log -vurl /test/p
