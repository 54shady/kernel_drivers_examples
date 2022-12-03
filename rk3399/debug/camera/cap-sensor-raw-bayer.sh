media-ctl -d /dev/media0 --set-v4l2 '"ov13850 1-0036":0[fmt:SBGGR10_1X10/2112x1568]'
media-ctl -d /dev/media0 --set-v4l2 '"rkisp1-isp-subdev":0[fmt:SBGGR10_1X10/2112x1568]'
media-ctl -d /dev/media0 --set-v4l2 '"rkisp1-isp-subdev":0[crop:(0,0)/2112x1568]'
media-ctl -d /dev/media0 --set-v4l2 '"rkisp1-isp-subdev":2[fmt:SBGGR10_1X10/2112x1568]'
media-ctl -d /dev/media0 --set-v4l2 '"rkisp1-isp-subdev":2[crop:(0,0)/2112x1568]'
v4l2-ctl -d /dev/video0 --set-ctrl 'exposure=1216,analogue_gain=10' \
	--set-selection=target=crop,top=0,left=0,width=2592,height=1944 \
	--set-fmt-video=width=2112,height=1568,pixelformat=SBGGR10 \
	--stream-mmap=3 --stream-to=/tmp/mp.raw.out --stream-count=1 --stream-poll
