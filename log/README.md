# 关于LOG模块的功能设计



## VERSION 0.1

### 模块要求：

1. 线程安全
2. 单例模式，全局只能存在一个LOG类
3. 要求能够实现异步写入文件，也能同步写入文件



### 需求分析：

1. 能够实时地将软件运行的状态写成日志，及时地写入到文件中
2. 对日志进行分级，分为INFO、DEBUG、ERROR、WARING级别
3. 生成日志时，记录当前时间、当前函数名、当前线程号，以及发生了什么事情
4. 要求程序能够格式化进行日志的生成
5. 日志文件以天数作为划分，日志名以时间为名。如果当前的日志行数超过最大设置，则另起一个文件，日志名后面加上编号
6. 每当运行一次程序，都需要重新生成一次日志文件，记录本次运行开始的时间。如果文件名已存在，另起一个文件，日志名后面加上编号



### 类的设计：

成员变量：



- 当前操作文件名
- 文件序号
- 根目录名
- 文件指针FILE
- 临时存储日志信息的缓存
- 缓存指针
- 互斥锁
- 日志最大行数
- 是否异步
- 阻塞队列

成员函数

（想将写入文件和制作日志分开来处理，却出现了不少线程安全问题（吐血），问题就是如何在模块化（粒度小）与线程安全作出均衡）

- 写入文件 需要上锁。检查文件状态，是否已满，是否隔日，是否新建文件，设置文件指针，之后写入日志。

  小细节：制作日志和写入文件设计上是分开的，因此有可能本次写入文件的时候缓冲区已经空了，这时候就需要进行判断，如果缓冲区已经空了就不需要写入文件。

  分为同步写和异步写

  - 同步写：直接将缓存内的数据写入文件，并清空缓存
  - 异步写：将缓存的字符串封装成string，加入阻塞队列，并清空缓存

- 初始化，新建一个日志文件

- 制作日志 根据格式化输入的内容生成字符串在缓冲区，并写入文件

**如果缓冲区是公用的话，很有可能在写入文件前就积攒了多条记录，导致日志计数困难，这里暂时想不到好的解决办法，只好放宽对记录数量的限制，多一点少一点没有关系**

**可以将缓冲区设置为临时变量，在每次调用函数的时候都生成一块缓冲区，这样就不用担心多线程抢夺一个缓冲区的问题，但是每次调用函数都需要分配内存，一定程度上**

- 生成新文件名 根据日期和序号自动生成文件名

**这b玩意也有线程安全问题，**

- 刷新文件流缓冲区
- 强制将当前缓存中的数据写入文件中，用于更换文件时起作用：当发现行数满了的时候，将之前留在缓存中的数据直接写入文件。**这时又出现了线程安全问题**，制作日志会加锁，写文件也会加锁，对上锁的锁加锁会造成死锁问题
- 析构，注意回收线程和内存空间、关闭文件



最后是宏定义，写成最好使用的方式



## VERSION 0.2

采用陈硕大佬的muduo库的结构，log模块单独使用一个线程，对外维护一个阻塞队列，往阻塞队列里添加日志串，线程负责取出日志写入文件中。

### 模块要求：

1. 单例模式，全局只能存在一个LOG类
2. 维护一个专用线程，用于处理日志相关事物
3. 维护一个阻塞队列，由用户线程作为生产者，log线程作为消费者，用户线程需要对日志进行封装推入阻塞队列中
4. 由于只用一个线程处理log，因此不必担心线程安全的问题（访问阻塞队列的时候仍需要注意）



### 需求分析：

1. 能够实时地将软件运行的状态写成日志，及时地写入到文件中
2. 对日志进行分级，分为INFO、DEBUG、ERROR、WARING级别
3. 生成日志时，记录当前时间、当前函数名、当前线程号，以及发生了什么事情
4. 要求程序能够格式化进行日志的生成
5. 日志文件以天数作为划分，日志名以时间为名。如果当前的日志行数超过最大设置，则另起一个文件，日志名后面加上编号
6. 每当运行一次程序，都需要重新生成一次日志文件，记录本次运行开始的时间。如果文件名已存在，另起一个文件，日志名后面加上编号





### 类的设计：

成员变量：

- 当前操作文件名
- 文件序号
- 根目录名
- 文件指针FILE
- 互斥锁
- 日志最大行数
- 阻塞队列
- 是否停止

成员函数：

- 创建文件，根据当前日期和序号创建一个新文件，如果存在，则序号+1，再创建一个
- 写入文件，同步写
- 线程工作函数，负责不断地取出阻塞队列的string，不停地写入文件
- 初始化
- 制作日志，写入当前线程、时间、以及格式化的字符内容，封装成string类放入阻塞队列中。

