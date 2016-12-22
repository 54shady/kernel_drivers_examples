# SPI

spi_transfer数据结构

	struct spi_transfer - a read/write buffer pair
	@tx_buf: data to be written (dma-safe memory), or NULL
	@rx_buf: data to be read (dma-safe memory), or NULL
	@len: size of rx and tx buffers (in bytes)
	@cs_change: affects chipselect after this transfer completes
