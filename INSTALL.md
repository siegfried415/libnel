
## NEL的编译和安装

NEL代码包含两个部分：libnel和libnlib，libnel包含了nel编译器的主体，libnlib包含了nel可以使用的函数库（比如字符串函数，x86 shellcode检测函数等）。


## 编译
为了编译libnel，需提前安装bison (libnel使用该库根据nel.y文件自动生成nel语言的语法分析c文件)，然后执行:

```console
make libnel
```

为了编译libnlib，需要：
```console
make libnlib
```

## 安装
通过如下的命令，将libnel、libnlib安装到系统库目录（/usr/local/lib/nel)下，以及将include文件安装到系统include目录（/usr/local/include/nel)下面：

```console
sudo make install-libnel
sudo make install-libnlib
```
