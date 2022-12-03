media-ctl -d /dev/media0 --set-v4l2 '"ov13850 1-0036":0[fmt:SBGGR10_1X10/2112x1568]'
media-ctl -d /dev/media0 --set-v4l2 '"rkisp1-isp-subdev":0[fmt:SBGGR10_1X10/2112x1568]'
media-ctl -d /dev/media0 --set-v4l2 '"rkisp1-isp-subdev":0[crop:(0,0)/2112x1568]'
media-ctl -d /dev/media0 --set-v4l2 '"rkisp1-isp-subdev":2[fmt:YUYV8_2X8/2112x1568]'
media-ctl -d /dev/media0 --set-v4l2 '"rkisp1-isp-subdev":2[crop:(0,0)/2112x1568]'
v4l2-ctl -d /dev/video0 \
	--set-selection=target=crop,top=336,left=432,width=1920,height=1080 \
	--set-fmt-video=width=1280,height=720,pixelformat=NV21 \
	--stream-mmap=3 --stream-to=/tmp/mp.out --stream-count=20 --stream-poll
