# record_pushStream_widget

录屏推流工具，可实现任意封装格式的录屏和rtsp格式推流，底层捕获音视频帧采用screen capturer recorder(gihub开源项目，需要单独安装)，编解码使用ffmpeg，通过ffmpeg解码捕获的音视频流后加入音视频帧队列，再通过ffmpeg复用封装(原始代码来自于官方示例)，界面由qt6 widget类实现

![image](picture/1.png)

![image](picture/2.png)
