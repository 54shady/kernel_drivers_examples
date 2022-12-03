#!/bin/sh

export DISPLAY=:0.0
#export GST_DEBUG=*:5
#export GST_DEBUG_FILE=/tmp/2.txt

echo "Start UVC Camera M-JPEG Preview!"

su linaro -c " \
     gst-launch-1.0 v4l2src device=/dev/video0 ! mppvideodec ! rkximagesink sync=false \
"

# Fpr spefic size :

# v4l2-ctl --list-formats-ext
# gst-launch-1.0 v4l2src device=/dev/video0 ! "image/jpeg,width=640,height=480,framerate=30/1" ! \
# mppvideodec ! rkximagesink sync=false
