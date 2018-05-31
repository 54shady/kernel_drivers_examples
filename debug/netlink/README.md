# Linux (Generic) NetLink

一般netlink demo([demo1_user.c](demo1_user.c) [demo2_kernel.c](demo2_kernel.c))

![net link msg](./netlinkmsg.png)

## Generic Netlink

[generic_netlink_howto](https://wiki.linuxfoundation.org/networking/generic_netlink_howto#architectural-overview)

[Generic Netlink(genl)介绍与例子](https://blog.csdn.net/ty3219/article/details/63683698)

介绍Generic Netlink之前,不得不说Netlink.Netlink是一种灵活的并且健壮的通讯方式,
可以用于kernel to user,user to kernel,kernel to kernel甚至user to user的通讯
Netlink的通道是通过Family来组织的,但随着使用越来越多,
Family ID已经不够分配了,所以才有了Generic Netlink.
所以,Generic Netlink其实是对Netlink报文中的数据进行了再次封装

Generic Netlink 架构如下

![GenericNetlink](./genl.png)

Genl数据包结构如下图

![genl data struct](./genl_data.png)

- genl机制的数据包分了4层
- 用户的实际数据封装在attribute里
- 一个或多个attribute可以被封装在用户自定义的一个family报文里
- 一个family报文又被封装在genlmsg里,最后genlmsg被封装在nlmsg里,总共4层

### User2Kernel

user2kernel 工作流程([demo2_user.c](demo2_user.c) [demo2_kernel.c](demo2_kernel.c))

	对于从user to kernel的通讯,driver必须先向内核注册一个struct genl_family,
	并且注册一些cmd的处理函数.这些cmd是跟某个family关联起来的.
	注册family的时候我们可以让内核自动为这个family分配一个ID.
	每个family都有一个唯一的ID,其中ID号0x10是被内核的nlctrl family所使用.
	当注册成功以后,如果user program向某个family发送cmd,那么内核就会回调对应cmd的处理函数.
	对于user program,使用前,除了要创建一个socket并绑定地址以外,还需要先通过family的名字获取family的ID.
	获取方法,就是向nlctrl这个family查询.详细的方法可以看后面的例子.有了family的ID以后,才能向该family发送cmd

### Kernel2User

kernel2user 工作流程([demo3_user.c](demo3_user.c) [demo3_kernel.c](demo3_kernel.c))

	对于从kernel to user的通讯,采用的是广播的形式,
	只要user program监听了,都能收到.但是同样的,
	user program在监听前,也必须先查询到family的ID

测试方法

	insmod demo3_kernel.ko
	./demo3_user
	echo 1 > /sys/kernel/genl_test/genl_trigger
