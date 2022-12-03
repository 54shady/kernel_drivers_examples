# FIQ-Debugger

[参考文章:Linux/Android常用调试工具](http://blog.csdn.net/azloong/article/details/45768633)

## 代码实现

参考 kernel/drivers/staging/android/fiq_debugger/fiq_debugger.c

## 内核配置

	CONFIG_FIQ_DEBUGGER                         // 使能fiq debugger
	CONFIG_FIQ_DEBUGGER_CONSOLE                 // fiq debugger与console可以互相切换
	CONFIG_FIQ_DEBUGGER_CONSOLE_DEFAULT_ENABLE  // 启动时默认串口在console模式

## 进入FIQ Debugger

使用minicom进入FIQ Debug mode
(在RK3399上按完ctrl+a+f后还要按fiq才能进入debug模式,在函数fiq_debugger_handle_uart_interrupt中设置的)

	ctrl+a+z+f 或 ctrl+a+f

退出debug模式

	输入console

## Process/Thread状态

    "R (running)",  /*   0 */
    "S (sleeping)",  /*   1 */
    "D (disk sleep)", /*   2 */
    "T (stopped)",  /*   4 */
    "t (tracing stop)", /*   8 */
    "Z (zombie)",  /*  16 */
    "X (dead)",  /*  32 */
    "x (dead)",  /*  64 */
    "K (wakekill)",  /* 128 */
    "W (waking)",  /* 256 */

通常一般的Process处于的状态都是S(sleeping)，而如果一旦发现处于如D(disk sleep)、T(stopped)、Z(zombie)等就要认真审查
