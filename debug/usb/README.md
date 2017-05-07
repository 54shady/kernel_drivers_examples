# USB

参考文章

[USB枚举过程](http://blog.csdn.net/myarrow/article/details/8270029)

[图解USB枚举过程](http://blog.csdn.net/myarrow/article/details/8270060)

[USB2.0速度识别](http://blog.chinaunix.net/xmlrpc.php?r=blog/article&uid=30497107&id=5759712)

## USB设备的6种状态

连接(Attached)

上电(Powered)

默认状态(Default)

地址(Address)

配置状态(Configured)

挂起状态(Suspend)

## 枚举过程

USB设备插入USB端口或给系统启动时设备上电

Hub监测它各个端口数据线上(D+/D-)的电压

Host了解连接的设备

Hub检测所插入的设备是全速还是低速设备

Hub复位设备

Host检测所连接的全速设备是否是支持高速模式

Hub建立设备和主机之间的信息通道

主机发送Get_Descriptor请求获取默认管道的最大包长度

主机给设备分配一个地址

主机获取设备的信息

主机给设备挂载驱动(复合设备除外)

设备驱动选择一个配置
