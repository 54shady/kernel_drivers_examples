# Linux PWM

[参考文章http://www.wowotech.net/comm/pwm_overview.html?utm_source=tuicool](http://www.wowotech.net/comm/pwm_overview.html?utm_source=tuicool)

# 测试方法

### 每一秒钟修改一次背光

```shell
# i=0;while (($i < 250)); do echo $i > /sys/devices/backlight.25/backlight/my_backlight/brightness; ((i+=20)); sleep 1;done
```

### 循环修改背光

```shell
# i=0;while (($i < 250)); do echo $i > /sys/devices/backlight.25/backlight/my_backlight/brightness; ((i+=20)); sleep 1;done
# while true; do if (($i > 250)) then;i=0 fi; echo $i; i+=1;done
```

```shell
i=0
while true
do
if(($i > 250)) then
i=0
fi
echo $i > /sys/devices/backlight.25/backlight/my_backlight/brightness;
((i+=10))
sleep 1
done
```
