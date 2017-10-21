# DVFS Usser Guide

[参考文章DVFS User Guide](http://processors.wiki.ti.com/index.php/DVFS_User_Guide)

## What is DVFS

```
Dynamic Voltage and Frequency scaling is a framework to change the frequency
and/or operating voltage of a processor(s) based on system performance
requirements at the given point of time
```

## CPUFreq consists two elements

- The Governor - that makes decisions
- The Driver - acts based on the decisions made by the governor

## Usage

To list all available governors

	cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors

To see current active governor

	cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

To switch to a different governor(e.g. to switch to 'userspace' governor)

	echo -n "userspace" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

Show current frequency of a cpu

	cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq

Show All available frequencies

	cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies

when the frequency is changed, system voltage is also changed to meet the new
requirements as part of scaling: This is done in two ways

- when new frequency is higher (moving to high power state/opp) Voltage is increased first then the frequency,
- when new frequency is lower (moving to low power state/opp) Frequency is reduced first then the voltage.
