# LiveStreamCapture
## cpp+ffmpeg 练手demo
**项目组成：cpp ffmpeg qt sdl2 js**
  
  
***学习参考ffmplay 解码，播放等逻辑***
* 需配合Greasy Fork网站的油猴脚本：  

  * 学习自用 LiveStreamCapture 配套脚本 目前仅支持douyu  

  * 油猴脚本进行直播源地址分析以及打开本地LiveStreamCapture.exe 工具 标识为一个简单的record button
  * 界面粗糙请见谅， qt与js较少使用，此项目主要用于练手与学习cpp和ffmpeg
  
### 环境配置步骤
  * 1.编译的release版本在LiveStreamCapture文件夹中
  * 2.管理员权限运行env.bat配置window URL Protocol 支持
  * 3.LiveStreamCapture.exe添加读写权限或管理员权限，保证文件写入正常运行
  * 4.安装油猴脚本，进入直播间，目前douyu仅支持非活动直播间的地址解析，活动直播间及其他平台源地址解析待办
  
### 待办（TODO）
  * 整理代码
  * 整理fix bug代码，完善设计
  * 完善转码代码 （image部分已完成，音频声道部分TODO）
  * ~~优化编码导致内存过高问题(与视频源画质相关，需要到一定内存才不增长，应与编码器内存缓冲区相关)~~ (编码器优化，限制最高720p，4Mbps码率等)
  * 替换成硬解码/硬编码 减少CPU负载
  * 考虑不同类型直播采用不同参数优化编码器
  
### 已知问题：
  * ~~1.qt 选择文件夹函数偶尔导致窗口闪退~~ （已解决，使用QT自带资源管理器）
  * 2.界面逻辑基础，按钮无防暴击功能，待完善 