#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define I2S_DEFAULT_FREQ	(11289600)

/* 描述I2S控制器的数据结构 */
struct rk_i2s_dev {
	struct device *dev;
	struct clk *clk; /* bclk */
	struct clk *mclk; /*mclk output only */
	struct clk *hclk; /*ahb clk */
};

static const struct of_device_id rockchip_i2s_match[] = {
	{ .compatible = "rockchip-i2s", },
	{},
};
MODULE_DEVICE_TABLE(of, rockchip_i2s_match);

int parse_from_dt(struct platform_device *pdev, struct rk_i2s_dev *i2s)
{
	struct device_node *node = pdev->dev.of_node;
	int ret;

	/* get i2s id form dts */
	ret = of_property_read_u32(node, "i2s-id", &pdev->id);
	if (ret < 0) {
		dev_err(&pdev->dev, "Property 'i2s-id' missing or invalid\n");
		ret = -EINVAL;
		goto EXIT;
	}

	/*
	 * 获取I2S控制器需要的时钟
	 * 在dts描述里需要有相关配置
	 */
	i2s->hclk = devm_clk_get(&pdev->dev, "i2s_hclk");
	if (IS_ERR(i2s->hclk)) {
		dev_err(&pdev->dev, "Can't retrieve i2s bus clock\n");
		ret = PTR_ERR(i2s->hclk);
		goto EXIT;
	}

	i2s->clk = devm_clk_get(&pdev->dev, "i2s_clk");
	if (IS_ERR(i2s->clk)) {
		dev_err(&pdev->dev, "Can't retrieve i2s clock\n");
		ret = PTR_ERR(i2s->clk);
		goto EXIT;
	}

	i2s->mclk = devm_clk_get(&pdev->dev, "i2s_mclk");
	if (IS_ERR(i2s->mclk)) {
		dev_info(&pdev->dev, "i2s%d has no mclk\n", pdev->id);
		ret = PTR_ERR(i2s->mclk);
		goto EXIT;
	}

EXIT:
	return ret;
}

void enable_clks(struct rk_i2s_dev *i2s)
{
	clk_prepare_enable(i2s->hclk);
	clk_set_rate(i2s->clk, I2S_DEFAULT_FREQ);
	clk_prepare_enable(i2s->clk);
	clk_prepare_enable(i2s->mclk);
}

static int rockchip_i2s_probe(struct platform_device *pdev)
{
	struct rk_i2s_dev *i2s;
	struct resource *res;
	void __iomem *regs;
	int ret;

	/* 给I2S控制器数据结构分配空间 */
	i2s = devm_kzalloc(&pdev->dev, sizeof(struct rk_i2s_dev), GFP_KERNEL);
	if (!i2s) {
		dev_err(&pdev->dev, "Can't allocate rk_i2s_dev\n");
		ret = -ENOMEM;
		goto EXIT;
	}

	ret = parse_from_dt(pdev, i2s);
	if (ret < 0)
		goto EXIT;

	/* enable hclk, clk, mclk */
	enable_clks(i2s);

	/*
	 * 获取dts里设置的寄存器起始和结束地址
	 * 根据该值来映射内存
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	printk("res: start 0x%x, end 0x%x, name %s\n", res->start, res->end, res->name);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto EXIT;
	}

	printk("%s, %d\n", __FUNCTION__, __LINE__);

EXIT:
	return ret;
}

static int rockchip_i2s_remove(struct platform_device *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct platform_driver rockchip_i2s_driver = {
	.probe  = rockchip_i2s_probe,
	.remove = rockchip_i2s_remove,
	.driver = {
		.name   = "rockchip-i2s",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(rockchip_i2s_match),
	},
};

static int rockchip_i2s_init(void)
{
	return platform_driver_register(&rockchip_i2s_driver);
}
module_init(rockchip_i2s_init);

static void rockchip_i2s_exit(void)
{
	platform_driver_unregister(&rockchip_i2s_driver);
}
module_exit(rockchip_i2s_exit);

MODULE_AUTHOR("zeroway");
MODULE_DESCRIPTION("Rockchip I2S Controller Driver");
MODULE_LICENSE("GPL v2");
