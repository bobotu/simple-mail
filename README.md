# Simple Mail  
Simple-Mail 是一个简单的命令行 SMTP & POP3 客户端工具   

##Build  
依赖:  
glib-2.0  
gmime-3.0  
OpenSSL-1.0.2l  

其余部分依赖已包含在源码 (vendor 目录) 中  

构建方式  
```shell
mkdir build && cd build
cmake ..
make simple-mail
```  

##Usage  

###发送邮件
```shell
./simple-mail send -u "邮箱" -p "密码" -H "邮件服务器" -t "接收人" -s "主题" <<EOF
正文
EOF
```  

###接收邮件   
```shell
./simple-mail recv -u "邮箱" -p "密码" -H "邮件服务器"
```  

详细说明参见帮助 `./simple-mail --help`   