# USB通讯协议

[参考文章1:USB协通讯议](http://blog.csdn.net/myarrow/article/details/8484113)

[参考文章2:USB协通讯议](https://wenku.baidu.com/view/172255d0ce2f0066f533222d.html)

## 基本概念

USB采用LittleEndian字节顺序,在总线上先传输最低有效位,最后传输最高有效位

采用NRZI编码,若遇到连续6个1要求插入一个0

## 传输类型,事务,包的关系

USB的4种传输类型

- 控制传输(Control Transfer)
- 批量传输(Bulk Transfer)
- 中断传输(Interrupt Transfer)
- 同步传输(Isochronous)

每种传输都由若干个事务(Transation)组成

每个事务由一个或多个包(Packet)组成

## 包(Packet)

包(Packet)是USB系统中信息传输的基本单位,其组成如下

- 同步字段(SYNC),用来产生同步作用
- 包标识符字段(PID),表示数据分包类型
- 数据字段(可以包含ADDR, ENDP, FrameNumber, DATA)
- 循环冗余校验字段(CRC)
- 包结尾字段(EOP)

|SYNC|PID|DATA|CRC
|--|--|--|--
|8bit:FS/LS,32bit:HS||ADDR,ENDP,FrameNumber,Data|

### 包(Packet)类型

根据PID的不同,可以将包分为

- 令牌(Token)
- 数据(Data)
- 握手(Handshake)
- 特殊(Special)

重要的包类型如下

#### Token Packets

|Filed|PID|ADDR|ENDP|CRC5|
|--|--|--|--|--
|Bits|8|7|4|5|

此格式适用于IN,OUT,SETUP,PING

#### SOF Packets

SOF包由Host发送给Device

- 对于full-speed(FS)总线,每隔1.00 ms±0.0005ms发送一次
- 对于high-speed(HS)总线,每隔125 μs±0.0625μs发送一次

|Field|PID|FrameNumber|CRC5|
|--|--|--|--
|Bits|8|11|5|

#### Data Packets

|Filed|PID|DATA|CRC16|
|--|--|--|--
|Bits|8|0-8192|16|

数据包类型有4种(DATA0,DATA1,DATA2,MDATA),由PID来区分

## 事务(Transation)

在USB上数据信息的一次接收或发送的处理过程称为事务处理

一个事务由一系列的Packets组成

- 一个token packet
- 可选的data pcket
- 可选的handshake packet
- 可选的special packet

### 输入(IN)事务处理

- 表示USB主机从总线上的某个USB设备接收一个数据包的过程

正常的输入事务处理

![NORMAL](./tx_in_normal.png)

设备忙时的输入事务处理

![BUSY](./tx_in_busy.png)

设备出错时的输入事务处理

![ERROR](./tx_in_error.png)

### 输出(OUT)事务处理

- 表示USB主机把一个数据包输出到总线上的某个USB设备接收的过程

正常的输出事务处理

![NORMAL](./tx_out_normal.png)

设备忙时的输出事务处理

![BUSY](./tx_out_busy.png)

设备出错时的输出事务处理

![ERROR](./tx_out_error.png)

### 设置(SETUP)事务处理

正常的设置事务处理

![NORMAL](./tx_setup_normal.png)

设备忙时的设置事务处理

![BUSY](./tx_setup_busy.png)

设备出错时的设置事务处理

![ERROR](./tx_setup_error.png)

## 传输类型

- 控制传输(Control Transfer)
- 批量传输(Bulk Transfer)
- 中断传输(Interrupt Transfer)
- 同步传输(Isochronous)

### 控制传输

制传输由2到3个阶段组成

- 建立阶段(Setup)
- 数据阶段(无数据控制没有此阶段)(DATA)
- 状态阶段(Status)

每一个阶段都由一个或多个事务传输组成

### 批量传输(Bulk Transfer)

- 用于传输大量数据,要求传输不能出错,但对时间没有要求,适用于打印机,存储设备等
- 批量传输是可靠的传输,需要握手包来表明传输的结果

### 中断传输(Interrupt Transfer)

- 中断传输由IN或OUT事务组成
- 中断传输在流程上除不支持PING 之外,其他的跟批量传输是一样的
- 中断传输方式总是用于对设备的查询,以确定是否有数据需要传输.因此中断传输的方向总是从USB设备到主机

### 同步传输(Isochronous Transfer)

它由两种包组成

- token
- data

同步传输不支持handshake和重传能力,所以它是不可靠传输

实时传输一般用于麦克风,喇叭,UVC Camera等设备
