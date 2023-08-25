# Query-flooding-based-Resource-Sharer
Course project of Computer Network in SCU

---

**Project 3. Query-flooding-based Resource Sharer**

  **Requirements:**

**◦**   **Each peer can share its resource (files or sub-directories) in a specially appointed directory**

**◦**   **Each peer can search the needed resource in the query flooding approach.**

**◦**   **Peers can download the resource from the first peer who sent query hit message in C/S mode**

  **Hardness Coefficient: 1.0**

---

~~拿的分挺高的，不过我最后考试成绩依托也没救~~

感谢chatgpt和new bing提供的问题帮助。

下面的部分复制于原课程报告，没有调整过格式。

~~不知道为什么过了个暑假我现在编译不成功了，提示__stack_chk_fail，有无好心人有空解决了提个issue给我看看~~

~~上传的项目结构可能有问题，没法直接用，我也不知道为啥（即使照着配了环境的话~~

不提供可执行程序，杜绝抄袭喵~

代码仅作参考，如果思路能帮助到你的课程项目或者其它工作，那自然最好啦~

---



# 引言

1. 目的

使用socket编程，采用UDP和TCP协议，基于查询洪泛和C/S模式下载，在本课程要求下实现一个集文件分享、资源下载的分享应用。

2. 需求收集

a)     基本需求

​            i.       每个节点可以在一个特定的指定目录中分享自己的资源（文件或子目录）。

​           ii.       每个节点可以使用洪泛查询的方式搜索所需的资源。

​          iii.       节点可以以客户端/服务器模式从第一个发送查询命中消息的节点下载资源。

b)    非功能性需求

​            i.       界面需求

1. 系统需要提供用户友好的图形交互界面，使用户能够轻松理解和使用系统。

2. 界面应具有简洁、直观的设计，使用户能够快速上手并进行操作。

3. 操作逻辑应清晰明了，用户能够方便地执行各种功能，如资源分享、查询和下载等。

​           ii.       健壮性需求

1. 系统应具有鲁棒性，能够应对网络拥塞、发送超时和多余的反复查询发送等不可预测的情况。

2. 在网络拥塞或不稳定的情况下，系统应能够优雅地处理，并具备自动恢复机制，以确保系统的稳定性和可靠性。

3. 系统应具备错误处理能力，能够正确地识别和处理错误情况，并提供相应的错误提示和恢复机制，以便用户能够及时采取措施或重新尝试操作。

​          iii.       性能需求：系统能处理一定量的并发请求。

​          iv.       可移植性：系统应具备良好的可移植性，能够在不同的操作系统和平台上运行，以满足不同用户的需求和环境要求。

# 系统分析

1. ## 用例分析

![img](.\README.assets\clip_image002.jpg)

 

文本规格化用例：

**2.**  **用例1：发起查询**

1. 描述：用户通过系统界面发起资源查询请求。

2. 参与者：用户

3. 前置条件： 用户进入系统并连接网络。

4. 触发事件：用户点击查询按钮。

5. 场景：

​     i.    用户进入系统界面，并选择要查询的资源类型。

​     ii.    用户在查询输入框中输入关键字。

​    iii.    用户点击查询按钮。

​     iv.    系统验证用户输入的关键字是否有效。

​     v.    如果关键字无效，系统显示错误消息，提示用户重新输入有效的关键字。

​     vi.    如果关键字有效，继续执行下一步。

​    vii.    系统发送查询请求到网络中的邻居节点。

   viii.    系统等待查询结果返回。

6. 后置场景：用户选择目标资源。

**3.**  **用例2：选择目标资源**

1. 描述： 用户从查询结果中选择感兴趣的资源。

2. 参与者： 用户

3. 前置条件：用户已发起查询并获取查询结果。

4. 触发事件： 用户查看查询结果，并选择感兴趣的资源。

5. 场景：

​     i.    系统收到查询结果并显示给用户。

​     ii.    用户查看查询结果列表。

​    iii.    用户选择感兴趣的资源（可以是默认第一个QueryHit）。

6. 后置场景：用户进行资源下载。

 

**
**

4. **用例3：下载资源**

1. 描述：用户下载所选资源。

2. 参与者： 用户

3. 前置条件：用户已选择目标资源。

4. 触发事件：用户选择下载所选资源。

5. 场景：

​     i.    用户选择下载所选资源。

​     ii.    系统连接到提供该资源的节点，并发起下载请求。

​    iii.    系统等待下载请求响应。

​     iv.    当系统收到下载响应时，系统开始下载资源。

​     v.    下载过程中可能会出现错误，系统会进行错误处理和提示。

​     vi.    下载完成后，系统通知用户下载已完成。

​    vii.    用户可以在下载列表选择查看下载资源，并情况列表。

6. 后置场景：用户进入资源选择或查询模式。

**5.**  **用例4：配置查看**

1. 描述：用户可以查看系统的设置。

2. 主要参与者：用户

3. 前置条件：用户进入系统。

4. 触发事件：用户点击配置按钮。

5. 场景：

​     i.    用户点击配置按钮，进入系统配置界面。

​     ii.    用户可以查看当前的系统设置信息。

6. 后置场景：用户进行其它活动。

   ---

   

2. ## 界面原型设计

|      |                                           |      |
| ---- | ----------------------------------------- | ---- |
|      | ![img](.\README.assets\clip_image005.gif) |      |
|      |                                           |      |
|      | ![img](.\README.assets\clip_image006.gif) |      |


                             ![img](.\README.assets\clip_image009.gif)                 ![img](.\README.assets\clip_image010.gif)    
 



#  

 



# 系统设计

1. 分层领域模型

![img](.\README.assets\clip_image012.gif)![img](.\README.assets\clip_image014.gif)![img](.\README.assets\clip_image016.gif)



 

2. 状态机设计

文字流程描述：

> 在这个query flooding情形中的主要实体有：节点、Query、QueryHit；
>
> 进入下载情形后，为p2p的C/S模式。对于每个peer，具有自己的ip（课程项目使用ipv4地址），一个udp监听端口和一个tcp监听端口。每个peer在网络中具有一定数量的邻居peer，只知道对方的ip和udp监听端口。每个peer有一个资源共享文件夹和一个下载文件夹，二者可以相同。当peer想要获取一个分享资源时，它会向它自己知道的邻居节点发送UDP的Query包。Query包的核心结构为：请求字段、请求id、ttl、发出者的ip和UDP监听端口。每个peer发送Query包后，立即进入监听模式，同时开启超时倒计时。每个peer监听到请求后，立刻进入处理模式：如果是Query，反序列化字节流为Query对象，提取关键字段在分享文件夹进行前缀匹配。若满足匹配，则立刻构造QueryHit对象：包含匹配得到的文件信息（文件名数组，可能有多个匹配）、本peer的ip地址和tcp监听端口，Query的请求id号。然后向Query包含的ip和端口号发送UDP QueryHit包（发送3次，防止实际网络情况下各种因素丢包）。如果Query不满足匹配，则将其ttl减1，转发给自己的邻居peer们：为了减少重复转发，peer会检查最近一次转发和此次转发的Query内容是否一致，控制转发策略。特别地，如果发现Query是由自己发出的（比对ip和端口），则会停止处理和转发该包。在转发过程中的Query包会一直留存直到ttl耗尽。在监听模式下，如果得到QueryHit请求，peer将进入QueryHit处理状态。首先它会检查QueryHit包含的请求id是否为自己最新发出的Query ID，防止处理到超时了的请求。接着它会将QueryHit中经过解析存储到特定数据结构列表中。并等待新的QueryHit包得到解析并加入列表。当超时计时器耗尽，peer停止存储后续收到的QueryHit（并通过控制器提交所有的QueryHit给UI层显示）等待对这些QueryHit的进一步处理。peer可以自由选择一轮收到的这些QueryHit中的文件进行下载。peer将通过tcp请求向目标peer tcp监听端口发起连接请求，发送下载对应文件的请求。此时这两个peer构成了一个典型的c/s模型。server端将会检查本地分享文件夹里是否存在对应文件，不存在则会发送错误请求并关闭连接。如若存在，server将会依次告知client文件大小和类型，以便client准备好下载时判断文件接收相关参数。相关错误可能会发生，由server或client决定关闭socket。这时server将会创建一条新的socket，将下载通信转移到该socket上，空出监听tcp端口进行并行监听。这种模式下允许并行下载。server开始分块传输二进制数据，client端会根据自己的设置进行文件分块接收，直到client确认收到文件大小指示接收完成，主动关闭socket连接。这个过程中可能会出错，client会在最大重试次数内尝试不断发起下载请求，直到重试次数耗尽。进行下载的同时，client仍可以处理其它query请求。
>
> 1. Client状态机：

![img](.\README.assets\clip_image018.jpg)

 

 

 



 

Query Processing：查询洪泛子状态机：

![img](.\README.assets\clip_image020.gif)

![img](.\README.assets\clip_image022.jpg)File Downloading：

 

2. ![img](.\README.assets\clip_image024.jpg)Server状态机：

 

3. UML类图设计

![img](.\README.assets\clip_image026.jpg)**核心领域类：**

![img](.\README.assets\clip_image028.jpg)

![img](.\README.assets\clip_image030.jpg)

![img](.\README.assets\clip_image032.jpg)![img](.\README.assets\clip_image034.jpg)

 

 

 

 

 



 

**关键数据结构：**

![img](.\README.assets\clip_image036.jpg)![img](.\README.assets\clip_image038.jpg)**Query****和QueryHit****都实现了序列化/****反序列化方法，便于操作：**

 

 

 

![img](.\README.assets\clip_image040.jpg)**工具类：**

![img](.\README.assets\clip_image042.jpg)



 

 

# 系统构建 

1. 开发环境与工具

1. IDE：CLion 2021.2.3

2. 开发套件：

a)     操作系统：Windows10 家庭中文版 22H2

b)    环境：MSYS2 2023-05-26 LATEST

c)     工具链：mingw-w64-x86_64-toolchain 13.1.0-6

d)    CMAKE 3.21.1

3. 开发语言：

a)     C++

b)    C17标准

4. 引用库（msys2软件包）

a)     mingw-w64-boost 1.81.0-7 及其依赖

​                  i.       提供asio异步socket编程支持

​                 ii.       提供序列化、配置解析等库函数功能

b)    mingw-w64-gtk4 4.10.4-1 及其依赖

​                  i.       提供跨平台GUI支持

c)     mingw-w64-libzip 1.9.2-1 及其依赖

​                  i.       提供zip文件解压缩支持



 

![img](.\README.assets\clip_image044.jpg)CMakeLists.txt：

 

2. 代码映射细节描述

主函数，启动Controller：

![img](.\README.assets\clip_image046.jpg)

![img](.\README.assets\clip_image048.jpg)Controller维护三个关键领域类：Client、Server、UI（由于GTK4限制，该类为静态类），维护两个线程：cs_thread（Client和Server公用一个boost::asio::io_context调度资源）、ui_thread（ui线程）。在构造函数进行相应初始化：

由于退出事件只被UI捕获，因此由UI线程负责结束程序：

![img](.\README.assets\clip_image050.jpg)Controller运行，加入线程等待结束：



 

![img](.\README.assets\clip_image052.jpg)![img](.\README.assets\clip_image054.jpg)    Client核心状态，洪范查询模式，对应状态机结构：



 

![img](.\README.assets\clip_image056.jpg)![img](.\README.assets\clip_image058.jpg)

![img](.\README.assets\clip_image060.jpg)

![img](.\README.assets\clip_image062.jpg)发送查询Query：

C/S文件下载，以Server为代表一窥流程：

![img](.\README.assets\clip_image064.jpg)开启tcp端口监听socket：

如果确认到连接，则新建一条通讯socket接管与Client的通信。



 

![img](.\README.assets\clip_image066.jpg)一些工作细节：

判断请求类型。

![img](.\README.assets\clip_image068.jpg)Server检测目标是否文件夹。如果为文件夹，采用先打包后整体发送的模式：



 

![img](.\README.assets\clip_image070.jpg)    继续告知Client必要信息，对方会对应创建相应的配置：

 

![img](.\README.assets\clip_image072.jpg)分块读取文件到内存，逐个发送：

基于boost:asio，这些过程得以并发地进行。



 

 

# 安装、使用与测试说明

1. 基本信息

将应用以如下形式安放即可：

本查询器由三部分构成：

1. 程序本体

2. peers配置文件

3. 下载文件夹和分享文件夹（可以相同）

在目录结构中，呈如下形态：

其中这两个可以为任意位置，为演示方便配置到程序根目录下。

![img](.\README.assets\clip_image074.jpg)

 

 

 

​    打开config.ini进行节点的基本配置：

![img](.\README.assets\clip_image076.jpg)    



 

关于config.ini的配置格式，请参照如下标准：

[Peer]

  IP = 你的IP地址

  TCP_Port = 你的TCP端口号

  UDP_Port = 你的UDP端口号

  Download_Dir = 你的下载目录

  Shared_Dir = 你的共享目录

 

  [Neighbour1]

  IP = 邻居的IP地址

  UDP_Port = 邻居的UDP端口号

 

  [Neighbour2...] 依此类推

  ...

  [NeighbourN]

其中需要注意的是，邻居节点的标记可以是任意名称，但自己节点必须以[Peer]标记。

2. 使用说明

双击Network_proj.exe，可以打开程序GUI。在我的系统（详见前文）上，通常会提示防火墙问题。请依次勾上允许专用网络和公用网络访问，以确保socket通讯的正常进行。

![img](.\README.assets\clip_image078.jpg)程序的主界面（查询页）如下：

在查询页，你可以在顶部搜索框输入查询文件名/文件夹名称的前缀名称。

点击查询按钮，等待2s即可获取查询结果。

中间两个列表，左侧会显示资源节点的地址和端口信息。

右侧会显示该资源节点所共享的文件列表。

选择你想要的文件，点击下方下载按钮，下载任务会被自动添加。

![img](.\README.assets\clip_image080.jpg)

程序的下载页

在下载页，你可以看到当前正在下载的任务列表，和其它状态（完成、失效）的下载任务。

你可以单击完成项进入目标目录。

单击下方按钮清除多余下载项。



 

![img](.\README.assets\clip_image082.jpg)

程序的配置页（很遗憾，本来希望实现配置更改功能，未果)：

在配置页，你可以查看配置文件信息。左侧列表选择peer，右侧是具体配置。
