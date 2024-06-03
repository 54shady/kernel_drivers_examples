# unplug usb cable (option) or just keep usb plugin when bootup

#mkdir /config
mount -t configfs none /config

mkdir /config/usb_gadget/g1/

# 使用Linux FunctionFS Gadget 的vendor id 和product id
# https://devicehunt.com/view/type/usb/vendor/1D6B
echo 0x1d6b > /config/usb_gadget/g1/idVendor
echo 0x0105 > /config/usb_gadget/g1/idProduct
echo 0x0310 > /config/usb_gadget/g1/bcdDevice
echo 0x0200 > /config/usb_gadget/g1/bcdUSB

mkdir -p /config/usb_gadget/g1/strings/0x409
echo 0123456789 > /config/usb_gadget/g1/strings/0x409/serialnumber

echo "myusb" > /config/usb_gadget/g1/strings/0x409/manufacturer
echo "myusbtest" > /config/usb_gadget/g1/strings/0x409/product

mkdir /config/usb_gadget/g1/configs/b.1
mkdir /config/usb_gadget/g1/configs/b.1/strings/0x409
echo "mytest" > /config/usb_gadget/g1/configs/b.1/strings/0x409/configuration
echo 500 > /config/usb_gadget/g1/configs/b.1/MaxPower

echo 0x1 > /config/usb_gadget/g1/os_desc/b_vendor_code
echo "MSFT100" > /config/usb_gadget/g1/os_desc/qw_sign
ln -s /config/usb_gadget/g1/configs/b.1 /config/usb_gadget/g1/os_desc/b.1

# 通过名字mydemo来绑定ffs和/dev/usb-ffs/对应的目录
mkdir /config/usb_gadget/g1/functions/ffs.mydemo
ln -s /config/usb_gadget/g1/functions/ffs.mydemo /config/usb_gadget/g1/configs/b.1/f1

mkdir -p /dev/usb-ffs/mydemo
mount -o rmode=0770,fmode=0660,uid=1024,gid=1024 -t functionfs mydemo /dev/usb-ffs/mydemo

# 在新窗口中手动执行
#./aio_multibuff /dev/usb-ffs/mydemo

# plug usb cable (option)

# connect usb (fc000000.usb is udc)
#echo `ls /sys/class/udc/` > /config/usb_gadget/g1/UDC
#echo device > /sys/kernel/debug/usb/fc000000.usb/mode

# 串口中会看到如下日志
#[  507.567791] android_work: sent uevent USB_STATE=CONNECTED
#[  507.572767] android_work: sent uevent USB_STATE=CONFIGURED

#此时在主机上可以通过lsusb看到枚举的设备
#Bus 003 Device 042: ID 1d6b:0105 Linux Foundation FunctionFS Gadget
# 在主机上执行测试
#sudo ffs-aio-example/host_app/multibuff_test

# disconnect usb
#echo  > /sys/kernel/debug/usb/fc000000.usb/mode
#echo > /config/usb_gadget/g1/UDC

# 串口中会看到如下日志
#[  701.327453] android_work: sent uevent USB_STATE=DISCONNECTED
