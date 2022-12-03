# Uart

[详细内容参考:Linux串口编程详解](https://www.cnblogs.com/jimmy1989/p/3545749.html)

## mark and space

计算机可以每次传送一个或者多个位(bit)的数据.
"串行"指的式每次只传输一位(1bit)数据
当需要通过串行通讯传输一个字(word)的数据时,
只能以每次一位的方式接收或者发送.
每个位可能是on(1)或者off(0).很多技术术语中经常用mark表示on,而space表示off.

## Partity

那个可选的parity位仅仅是所有传输位的和,这个和用以表示传输字符中有奇数个1还是偶数个1.
在偶数parity中,如果有传输字符中有偶数个1,那么parity位被设置成0,
而传输字符中有奇数个1,那么parity位被设置成1.在奇数parity中,位设置与此相反.
还有一些术语,比如space parity, mark parity和no parity.
Space parity是指parity位会一直被设置位0,而mark parity正好与此相反,
parity会一直是1.No parity的意思就是根本不会传输parity位. 剩余的位叫做stop位.
传输字符之间可以有1个,1.5个或者2个stop位,而且,它们的值总是1.传统上,
Stop位式用给计算机一些时间处理前面的字符的,但是它只是被用来同步接收数据的计算机和接受的字符.
异步数据通常被表示成"8N1","7E1", 或者与此类似的形式.这表示"8数据位,no parity和1个stop bit",
还有相应得,"7数据位,even parity和1个stop bit".

## Duplex

全双工(Full duplex)是说计算机可以同时接受和发送数据
也就是它有两个分开的数据传输通道(一个传入,一个传出).

半双工(Half duplex)表示计算机不能同时接受和发送数据,而在某一时刻它只能单一的传送或者接收.
这通常意味着,它只有一个数据通道.半双工并不是说RS-232的某些信号不能使用,
而是,它通常是使用了有别于RS-232的其他不支持全双工的标准.

## Flow control

两个串行接口之间的传输数据流通常需要协调一致才行.
这可能是由于用以通信的某个串行接口或者某些存储介质的中间串行通信链路的限制造成的.
对于异步数据这里有两个方法做到这一点.

### Software flow contnrol

这种方法采用特殊字符来开始(XON,DC1,八进制数021)或者结束(XOFF,DC3或者八进制数023)数据流.
而这些字符都在ASCII中定义好了.虽然这些编码对于传输文本信息非常有用,
但是它们却不能被用于在特殊程序中的其他类型的信息.

### Hardware flow control

这种方法使用RS-232标准的CTS和RTS信号来取代之前提到的特殊字符.
当准备就绪时,接受一方会将CTS信号设置成为space电压,而尚未准备就绪时它会被设置成为mark电压.
相应得,发送方会在准备就绪的情况下将RTS设置成space电压.
正因为硬件流控制使用了于数据分隔的信号,所以与需要传输特殊字符的软件流控制相比它的速度很快.
但是,并不是所有的硬件和操作系统都支持CTS/RTS流控制.

## BREAK

通常,直到有数据传输时,接收和传输信号会保持在mark电压.
如果一个信号掉到space电压并且持续了很长时间,
一般来说是1/4到1/2秒,那么就说有一个break条件存在了.
BREAK经常被用来重置一条数据线或者用来改变像调制解调器这样的设备的通讯模式.

## 串口编程

### 打开串口连接的时候

程序在open函数中除了Read+Write模式以外还指定了两个选项

```c
fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
```

标志O_NOCTTY可以告诉UNIX这个程序不会成为这个端口上的"控制终端".
如果不这样做的话,所有的输入,比如键盘上过来的Ctrl+C中止信号等等,会影响到你的进程.
而有些程序比如getty(1M/8)则会在打开登录进程的时候使用这个特性,
但是通常情况下,用户程序不会使用这个行为.

O_NDELAY标志则是告诉UNIX,这个程序并不关心DCD信号线的状态——也就是不关心端口另一端是否已经连接.
如果不指定这个标志的话,除非DCD信号线上有space电压否则这个程序会一直睡眠.

### 给端口上写数据

给端口上写入数据也很简单,使用write(2)系统调用就可以发送数据了

```c
write(fd, "ATZ\r", 4);
```

和写入其他设备文件的方式相同,
write函数也会返回发送数据的字节数或者在发生错误的时候返回-1.
通常,发送数据最常见的错误就是EIO,当调制解调器或者数据链路将
Data Carrier Detect(DCD)信号线弄掉了,就会发生这个错误.
而且,直至关闭端口这个情况会一直持续.

### 从端口上读取数据

从串口上读取数据的时候就得耍花招了.
因为,如果你在原数据模式(raw data mode)操作端口的话,
每个read(2)系统调用都会返回从串口输入缓冲区中实际得到的字符的个数.
在不能得到数据的情况下,read(2)系统调用就会一直等着,
只到有端口上新的字符可以读取或者发生超时或者错误的情况发生.
如果需要read(2)函数迅速返回的话,你可以使用下面这个方式

```c
fcntl(fd, F_SETFL, FNDELAY);
```

标志FNDELAY可以保证read(2)函数在端口上读不到字符的时候返回0.
需要回到正常(阻塞)模式的时候,需要再次在不带FNDELAY标志的情况下调用fcntl(2)函数

```c
fcntl(fd, F_SETFL, 0);
```

当然,如果你最初就是以O_NDELAY标志打开串口的,
你也可在之后使用这个方法改变读取的行为方式.

### 关闭串口

可以使用close(2)系统调用关闭串口

```c
close(fd);
```

关闭串口会将DTR信号线设置成low,这会导致很多调制解调器挂起.

### 配置串口

POSIX终端接口

很多系统都支持POSIX终端(串口)接口.
程序可以利用这个接口来改变终端的参数,
比如,波特率,字符大小等等.要使用这个端口的话,
你必须将<termios.h>头文件包含到你的程序中.
这个头文件中定义了终端控制结构体和POSIX控制函数.

与串口操作相关的最重要的两个POSIX函数可能就是tcgetattr(3)和tcsetattr(3).
顾名思义,这两个函数分别用来取得设设置终端的属性.
调用这两个函数的时候,你需要提供一个包含着所有串口选项的termios结构体

termios结构体成员

|成员       |描述
|-----------|--------
|c_cflag    |控制选项
|c_lflag    |行选项
|c_iflag    |输入选项
|c_oflag    |输出选项
|c_cc       |控制字符
|c_ispeed   |输入波特率(NEW)
|c_ospeed   |输出波特率(NEW)

### 控制选项

通过termios结构体的c_cflag成员可以控制波特率,
数据的比特数,parity,停止位和硬件流控制.

在传统的POSIX编程中,当连接一个本地的(不通过调制解调器)
或者远程的终端(通过调制解调器)时,这里有两个选项应当一直打开,
一个是CLOCAL,另一个是CREAD.这两个选项可以保证你的程序不会变成端口的所有者,
而端口所有者必须去处理发散性作业控制和挂断信号,
同时还保证了串行接口驱动会读取过来的数据字节.

波特率常数(CBAUD,B9600等等)通常指用到那些不支持c_ispeed和c_ospeed
成员的旧的接口上.后面文章将会提到如何使用其他POSIX函数来设置波特率.

千万不要直接用使用数字来初始化c_cflag(当然还有其他标志),
最好的方法是使用位运算的与或非组合来设置或者清除这个标志.
不同的操作系统版本会使用不同的位模式,
使用常数定义和位运算组合来避免重复工作从而提高程序的可移植性.

### 设置波特率

不同的操作系统会将波特率存储在不同的位置.
旧的编程接口将波特率存储在上表所示的c_cflag成员中,
而新的接口实装则提供了c_ispeed和c_ospeed成员来保存实际波特率的值.

程序中可是使用cfsetospeed(3)和cfsetispeed(3)函数在
termios结构体中设置波特率而不用去管底层操作系统接口.
下面的代码是个非常典型的设置波特率的例子.

### 设置字符大小

设置字符大小的时候,这里却没有像设置波特率那么方便的函数.
所以,程序中需要一些位掩码运算来把事情搞定.字符大小以比特为单位指定

```c
options.c_flag &= ~CSIZE; /* Mask the character size bits */
options.c_flag |= CS8;    /* Select 8 data bits */
```

### 设置奇偶校验

与设置字符大小的方式差不多,
这里仍然需要组合一些位掩码来将奇偶校验设为有效和奇偶校验的类型.
UNIX串口驱动可以生成even,odd和no parity位码.设置space奇偶校验需要耍点小手段.

No parity (8N1)
```c
options.c_cflag &= ~PARENB
options.c_cflag &= ~CSTOPB
options.c_cflag &= ~CSIZE;
options.c_cflag |= CS8;
```

Even parity (7E1)
```c
options.c_cflag |= PARENB
options.c_cflag &= ~PARODD
options.c_cflag &= ~CSTOPB
options.c_cflag &= ~CSIZE;
options.c_cflag |= CS7;
```
Odd parity (7O1)
```c
options.c_cflag |= PARENB
options.c_cflag |= PARODD
options.c_cflag &= ~CSTOPB
options.c_cflag &= ~CSIZE;
options.c_cflag |= CS7;
```

Space parity is setup the same as no parity (7S1)
```c
options.c_cflag &= ~PARENB
options.c_cflag &= ~CSTOPB
options.c_cflag &= ~CSIZE;
options.c_cflag |= CS8;
```

### 设置硬件流控制

某些版本的UNIX系统支持通过CTS(Clear To Send)
和RTS(Request To Send)信号线来设置硬件流控制.
如果系统上定义了CNEW_RTSCTS和CRTSCTS常量,
那么很可能它会支持硬件流控制.使用下面的方法将硬件流控制设置成有效

```c
options.c_cflag |= CNEW_RTSCTS;    /* Also called CRTSCTS
```

将它设置成为无效的方法与此类似

```c
options.c_cflag &= ~CNEW_RTSCTS;
```

### 经典输入和原始输入模式

成员变量c_lflag可以使用的常量

```c
ISIG 	Enable SIGINTR, SIGSUSP, SIGDSUSP, and SIGQUIT signals
ICANON 	Enable canonical input (else raw)
XCASE 	Map uppercase \lowercase (obsolete)
ECHO 	Enable echoing of input characters
ECHOE 	Echo erase character as BS-SP-BS
ECHOK 	Echo NL after kill character
ECHONL 	Echo NL
NOFLSH 	Disable flushing of input buffers after interrupt or quit characters
IEXTEN 	Enable extended functions
ECHOCTL 	Echo control characters as ^char and delete as ~?
ECHOPRT 	Echo erased character as character erased
ECHOKE 	BS-SP-BS entire line on line kill
FLUSHO 	Output being flushed
PENDIN 	Retype pending input at next read or input char
TOSTOP 	Send SIGTTOU for background output
```

### 选择经典输入

经典输入是以面向行设计的.
在经典输入模式中输入字符会被放入一个缓冲之中,
这样可以以与用户交互的方式编辑缓冲的内容,
直到收到CR(carriage return)或者LF(line feed)字符.

选择使用经典输入模式的时候,通常需要选择ICANON,ECHO和ECHOE选项

```c
options.c_lflag |= (ICANON | ECHO | ECHOE);
```

选择原始输入

原始输入根本不会被处理.输入字符只是被原封不动的接收.
一般情况中,如果要使用原始输入模式,程序中需要去掉ICANON,ECHO,ECHOE和ISIG选项

```c
options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
```

### 奇偶校验选项

```c
options.c_iflag |= (INPCK | ISTRIP);
```

IGNPAR是一个比较危险选项,即便有错误发生时,
它也会告诉串口驱动直接忽略奇偶校验错误给数据放行.
这个选项在测试链接的通讯质量时比较有用而通常不会被用在实际程序中.

PARMRK会导致奇偶校验错误被标志成特殊字符加入到输入流之中.
如果IGNPAR选项也是有效的,那么一个NUL(八进制000)字符会被加入到发生奇偶校验错误的字符前面.
否则,DEL(八进制177)和NUL字符会和出错的字符一起送出.

### 设置软件流控制

软件流控制可以通过IXON,IXOFF和IXANY常量设置成有效

```c
options.c_iflag |= (IXON | IXOFF | IXANY);
```

将其设置为无效的时候,很简单,只需要对这些位取反

```c
options.c_iflag &= ~(IXON | IXOFF | IXANY);
```
