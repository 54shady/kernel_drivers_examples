adb shell "echo 0 > /proc/sys/kernel/watchdog"
adb shell "echo 'ttyFIQ0,115200' > /sys/module/kgdboc/parameters/kgdboc"
adb shell "echo g > /proc/sysrq-trigger"
