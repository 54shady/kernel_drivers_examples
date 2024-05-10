Enable system panic timeout

	ONFIG_PANIC_TIMEOUT=3

config ramoops in dts

	ramoops@110000 {
			compatible = "ramoops";
			reg = <0x0 0x110000 0x0 0xf0000>;
			record-size = <0x20000>;
			console-size = <0x80000>;
			ftrace-size = <0x00000>;
			pmsg-size = <0x50000>;
	}

trigger test manually

	echo c > /proc/sysrq-trigger

the pstore location

	/var/lib/systemd/pstore/
