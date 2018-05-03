# PowerDomain

WIFI_PWR由GPIO4_D3控制,对应的电源域是APIO3,由datasheet中可以查到

![GPIO4_D3电源域APIO3](./power_domain01.png)

![GPIO4_D3电源域APIO3](./power_domain00.png)

APIO3_VDD电源由PMU上LDO(VCC_WL)提供电源,所以GPIO4_D3上电时序肯定是在VCC_WL之后
即使在代码中先操作GPIO4_D3的gpio,后使能regulator,但是用示波器量的结果始终是先有VCC_WL后有GPIO4_D3
