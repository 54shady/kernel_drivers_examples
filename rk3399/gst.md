# Gstreamer使用

## 视频播放

使用gstreamer播放本地视频

	export DISPLAY=:0.0
	gst-launch-1.0 uridecodebin uri=file:///usr/local/test.mp4 ! rkximagesink

如果是root用户执行则要切换到普通用户

	su linaro -c "gst-launch-1.0 uridecodebin uri=file:///usr/local/test.mp4 ! rkximagesink"

## 编解码测试

编解码测试[参考代码](./test_enc.sh)

	export DISPLAY=:0.0
	su linaro -c "gst-launch-1.0 videotestsrc num-buffers=512 ! video/x-raw,format=NV12,width=1920,height= 1080,framerate=30/1 ! queue ! mpph264enc ! queue ! h264parse ! mpegtsmux ! filesink location=/home/linaro/2k.ts"
	su linaro -c "gst-launch-1.0 uridecodebin uri=file:///home/linaro/2k.ts ! rkximagesink"


JPEG解码测试

	export DISPLAY=:0.0
	su linaro -c 'gst-launch-1.0 -v videotestsrc ! "video/x-raw,width=1920,height=1080" ! queue ! jpegenc ! queue ! jpegparse ! queue ! mppvideodec ! rkximagesink'
