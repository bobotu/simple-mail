# Simple Mail  
Simple-Mail 是一个简单的命令行 SMTP & POP3 客户端工具   

## Build  
### 依赖  
glib-2.0  
gmime-3.0  
OpenSSL-1.0.2l  

其余部分依赖已包含在源码 (vendor 目录) 中  

### 依赖安装  
对于 macOS 可以使用 [Homebrew](https://brew.sh/index_zh-cn.html) 进行便捷的安装  

```shell
brew install gmime glib openssl
```

### 构建方式  
构建需要的 CMake 以及 pkg-config 也可以通过 Homebrew 安装
```shell
brew install cmake pkg-config
``` 

*对于 Homebrew 安装的 OpenSSL 需要指定 OpenSSL 的目录*
```shell
mkdir build && cd build
cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl ..
make simple-mail
```  

## Usage  

### 发送邮件
```shell
./simple-mail send -u "邮箱" -p "密码" -H "邮件服务器" -t "接收人" -s "主题" <<EOF
正文
EOF
```  

### 接收邮件   
```shell
./simple-mail recv -u "邮箱" -p "密码" -H "邮件服务器"
```  

详细说明参见帮助 `./simple-mail --help`   