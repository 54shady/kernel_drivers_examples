# 关于ftrace

## trace函数的位置

编译后的ftrace函数在目录(include/trace/events)下的头文件中

## 例子1

1. 比如函数 static int napi_poll中的如下函数
	trace_napi_poll(n, work, weight);

其中trace_napi_poll在头文件(include/trace/events/napi.h)中

TRACE_EVENT(napi_poll,
	TP_STRUCT__entry(
		__field(	struct napi_struct *,	napi)
		__string(	dev_name, napi->dev ? napi->dev->name : NO_DEV)
		__field(	int,			work)
		__field(	int,			budget)
	),
	...
	TP_printk("napi poll on napi struct %p for device %s work %d budget %d",
		  __entry->napi, __get_str(dev_name),
		  __entry->work, __entry->budget)
	...

### 使用过滤

可以通过如下来过滤budget=6的

	echo 'budget == 6' > events/napi/napi_poll/filter

清掉过滤

	echo 0 > events/napi/napi_poll/filter

所以trace信息如下

	sshd-3984    [000] .N.s...   877.078111: napi_poll: napi poll on napi struct 00000000e62d086d for device can1 work 1 budget 6

## 例子2

2. 比如 static void __spi_pump_messages(struct spi_controller *ctlr, bool in_kthread)
	trace_spi_message_start(msg);

其中trace_spi_message_start在  (include/trace/events/spi.h)

	DEFINE_EVENT(spi_message, spi_message_start,

		TP_STRUCT__entry(
			__field(        int,            bus_num         )
			__field(        int,            chip_select     )
			__field(        struct spi_message *,   msg     )
		),

		TP_PROTO(struct spi_message *msg),

		TP_ARGS(msg)
	);
