# FFmpeg_video_encode_decode

安装x264

到http://www.videolan.org/developers/x264.html 下载x264的库，然后解压。

./configure

configure过程中可能会遇到没有yasm库的错误，yasm是一个汇编库，用于加快h264、ffmpeg执行的速度，可以直接使用--disable-yasm来忽略该选项。然后

make

make install

安装ffmpeg

到http://ffmpeg.org/download.html 下载ffmpeg源码，执行configure

./configure --enable-libx264 --enable-gpl --enable-decoder=h264 --enable-encoder=libx264 --enable-shared --enable-static

编译过程中同样可能遇到没有yasm的错误，同样可以使用--disable-yasm来忽略。然后

make

make install

h265同上



compile

1. source env.mk
2. make
