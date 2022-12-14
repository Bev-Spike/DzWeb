# 数据库&数据库连接池模块的设计
这里是一个简单的数据库的设计文档，以及数据库连接池的设计说明
## 数据库的设计
### 基本信息表 General
通常只有一行，用于记录服务器运行的一些基本数据
- 网站人流量
- 注册用户量
- 图库存量

### User
象征性的建立一个用户表，用于保存注册的用户
- **user_id** 由数据库自动递增生成，用户不可见
- user_name  由用户自己指定，唯一
- password  密码
- power  权限， 分为normal 和 super

### DZmessage
重量级的丁真表，用于保存丁真的图片路径以及丁真的文案
- **dz_id** 保存的丁真图片ID
- dz_path 图片路径
- dz_describe  xx丁真
- dz_appraisal 鉴定为xxx

### 触发器
每新增一张丁真图片，就多一个图库存量
新增用户同理

### 索引
用户名、丁真描述

## 数据库连接池的设计
通常，web服务器是作为数据库的一个用户，负责访问数据库。在高并发多线程的环境下，如果只有一个数据库连接，将会造成数据库连接的竞争，同一时间只能有一个用户请求拥有数据库的连接，这样造成了效率低下。

这时，就需要有一个数据库连接池，用于存放多个数据库连接，由程序动态地对池中的连接进行使用、释放。当系统开始处理客户请求的时候，如果它需要相关的资源，可以直接从池中获取，无需动态分配；当服务器处理完一个客户连接后,可以把相关的资源放回池中，无需执行系统调用释放资源。

在程序初始化的时候，就可以集中创建多个数据库连接，将他们集中管理，供程序使用，保证较快的数据库读写速度。

### 功能需求
- RAII机制获取和释放数据库连接。允许用户请求使用完数据库连接后自动释放回连接池中
- 单例模式。整个程序只能有一个数据库连接池。
- 连接池的初始化、获取连接、释放连接、销毁连接池。

### 数据结构的选择
使用阻塞队列存放数据库连接（一招鲜吃遍天了属于是）。

阻塞队列中，使用条件变量进行线程的同步。当阻塞队列中没有空闲的数据库连接时，就阻塞试图获取数据库连接的线程，直到其他线程释放了数据库连接，才将等待的线程唤醒，这样就实现了线程的同步。

当用户获取数据库连接时，就将阻塞队列的元素pop出去，释放连接时，将元素push回来，期间需要使用lock加锁以保证访问阻塞队列的线程安全问题。




# 数据库操作模块
封装数据库操作而已，没什么好说的。