# PSTORE

## Enable in kernel config and dts

Enable system panic timeout

	CONFIG_PSTORE=y
	CONFIG_PSTORE_CONSOLE=y
	CONFIG_PSTORE_RAM=y
	CONFIG_PANIC_TIMEOUT=3

config ramoops in dts

	ramoops@110000 {
			compatible = "ramoops";
			reg = <0x0 0x110000 0x0 0xf0000>;
			record-size = <0x20000>;
			console-size = <0x80000>;
			ftrace-size = <0x00000>;
			pmsg-size = <0x50000>;
	}

## Mount the filesystem

mount the pstore for example

	mount -t pstore -o kmsg_bytes=8000 - /sys/fs/pstore

on systemd as init system, the pstore is already mounted by servcie

	systemctl status systemd-pstore.service
	pstore on /sys/fs/pstore type pstore (rw,nosuid,nodev,noexec,relatime)

## Trigger and Test

trigger test manually

	echo c > /proc/sysrq-trigger

the pstore location(systemd as init)

	/var/lib/systemd/pstore/
