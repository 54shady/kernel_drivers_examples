# shell测试脚本(android ksh)

## 查看目录下每个文件的内容

有下面这样一个目录

```shell
root@rk3288:/sys/devices/ff650000.i2c/i2c-0/0-005a/regulator # ls
regulator.1
regulator.10
regulator.11
regulator.12
regulator.2
regulator.3
regulator.4
regulator.5
regulator.6
regulator.7
regulator.8
regulator.9
```

每个目录下有下面这些文件

```shell
root@rk3288:/sys/devices/ff650000.i2c/i2c-0/0-005a/regulator # ls regulator.1
device
max_microvolts
microvolts
min_microvolts
name
num_users
opmode
power
state
subsystem
suspend_disk_microvolts
suspend_disk_state
suspend_mem_microvolts
suspend_mem_state
suspend_standby_microvolts
suspend_standby_state
type
uevent
```

查看每个文件的内容(直接复制到shell中)

```
i=1
while (( $i < 13 ))
do
for f in `ls regulator.$i/`
do
if [ -f regulator.$i/$f ]
then
echo "regulator.$i/$f =" `cat regulator.$i/$f`
fi
done
(( i+= 1 ))
done
```

## 查看某个目录下所有的文件的内容

```shell
for f in `ls`
do
if [ -f $f ]
then
echo "$f =" `cat $f`
fi
done
```

## 打印所有regulator.[1-12]下num_users里的信息

```shell
i=1
cat_info="num_users"
while (( $i < 13 ))
do
echo "regulator.$i/$cat_info =" `cat regulator.$i/$cat_info`
(( i+=1 ))
done
```
