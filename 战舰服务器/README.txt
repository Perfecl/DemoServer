工程依赖:
* cmake 2.6+
* g++ 4.4以上版本
* rt
* python 3.0以上版本(用来把消息注册到消息原型工厂里面去)
 

初始化工程:
```
svn co svn://192.168.16.100/mbgame03/code/server_code
cd server_code
./init.sh
```

编译工程:
``` bash
mkdir build
cd build
cmake ..
make -j4
```

工程文件夹目录:
* deps是依赖库的文件夹
* deps/boost已经编译好的boost, 目前版本用的是1.58, 库全部为静态库
* deps/sdk底层库文件
* deps/google为预编译好的pb库, 自己开发机器上面需要安装一次, 里面有一个install.sh, 执行一次.
  编译时使用的是静态库, 所以目标机器上面不需要安装
