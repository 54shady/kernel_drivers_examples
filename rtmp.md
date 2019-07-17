# Setup RTMP server on RK3399

## 配置RTMP服务器

全新安装nginx

	sudo apt-get update
	sudo apt-get -y install nginx
	sudo apt-get -y remove nginx
	sudo apt-get clean
	sudo rm -rf /etc/nginx/*
	sudo apt-get install -y curl build-essential libpcre3 libpcre3-dev libpcre++-dev zlib1g-dev libcurl4-openssl-dev libssl-dev
	sudo mkdir -p /var/www
	rm -rf nginx_src
	mkdir -p nginx_src
	cd nginx_src
	NGINXSRC=$PWD
	wget http://nginx.org/download/nginx-1.13.8.tar.gz
	git clone https://github.com/54shady/nginx-rtmp-module.git
	(cd nginx-rtmp-module/; git checkout -b ng1.13.8 791b6136f02bc9613daf178723ac09f4df5a3bbf)
	tar -zxvf nginx-1.13.8.tar.gz
	cd nginx-1.13.8
	./configure --prefix=/var/www --sbin-path=/usr/sbin/nginx --conf-path=/etc/nginx/nginx.conf --pid-path=/var/run/nginx.pid --error-log-path=/var/log/nginx/error.log --http-log-path=/var/log/nginx/access.log --with-http_ssl_module --without-http_proxy_module --add-module=$NGINXSRC/nginx-rtmp-module
	make
	sudo make install

## 配置nginx服务器(开发板192.168.1.122)

添加下面内容到(/etc/nginx/nginx.conf)

	rtmp {
		server {
			listen 1935;
			chunk_size 4096;
			application live {
				live on;
				record off;
			}
		}
	}

配置完后重启服务

	sudo systemctl restart nginx

## 测试功能

### 使用内建测试数据测试

在开发板上用测试数据测试

	gst-launch-1.0 videotestsrc is-live=true ! videoconvert ! x264enc bitrate=1000 tune=zerolatency ! video/x-h264 ! h264parse ! video/x-h264 ! queue ! flvmux streamable=true name=mux ! rtmpsink sync=false location='rtmp://192.168.1.122:1935/live/test'

在主机上接收RTMP数据流(使用vlc播放out.flv)

	rtmpdump -r "rtmp://192.168.1.122/live/test" -V -z -o out.flv

### 使用USB摄像头测试

查看USB摄像头对应的节点

	uvcdynctrl -l

查看USB摄像头能支持的格式

	sudo v4l2-ctl --list-formats-ext -d /dev/video8

使用开发板上的USB摄像头(HD Pro Webcam C920)进行推流测试

	gst-launch-1.0 v4l2src device=/dev/video8 !  "image/jpeg,width=1280,height=720,framerate=30/1" ! jpegdec ! videoconvert ! queue ! mpph264enc ! queue ! h264parse ! flvmux streamable=true ! queue !  rtmpsink sync=false location='rtmp://192.168.1.122:1935/live/test'

在开发板上配置,支持网页浏览

	mkdir -p ~/strobe_src
	cd ~/strobe_src
	wget http://downloads.sourceforge.net/project/smp.adobe/Strobe%20Media%20Playback%201.6%20Release%20%28source%20and%20binaries%29/StrobeMediaPlayback_1.6.328-full.zip
	unzip StrobeMediaPlayback_1.6.328-full.zip
	sudo cp -r for\ Flash\ Player\ 10.1 /var/www/html/strobe

修改(/var/www/html/index.html)内容如下(注意IP地址写开发板)
修改后即可在PC上通过浏览器里输入IP即可浏览摄像头数据

	<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
	<html xmlns="http://www.w3.org/1999/xhtml">
		<head>
		<title>NanoPi Live</title>
		<script type="text/javascript" src="strobe/lib/swfobject.js"></script>
		<script type="text/javascript">
			// Create a StrobeMediaPlayback configuration
			var parameters = {
			// src:
	"http://players.edgesuite.net/videos/big_buck_bunny/bbb_448x252.mp4",
			src: "rtmp://192.168.1.122/live/test",
			autoPlay: true,
				controlBarAutoHide: false,
				playButtonOverlay: true,
				showVideoInfoOverlayOnStartUp: false,
				optimizeBuffering : false,
				initialBufferTime : 0.1,
				expandedBufferTime : 0.1,
				minContinuousPlayback : 0.1,
				poster: "strobe/images/poster.png"
			};

			// Embed the player SWF:
			swfobject.embedSWF
			( "strobe/StrobeMediaPlayback.swf"
				, "strobeMediaPlayback"
				, 640
				, 480
				, "10.1.0"
				, {}
				, parameters
				, { allowFullScreen: "true"}
				, { name: "strobeMediaPlayback" }
			);
		</script>
		</head>
		<body>
			<div id="strobeMediaPlayback">
			  <p>Alternative content</p>
			</div>
		</body>
	</html>
