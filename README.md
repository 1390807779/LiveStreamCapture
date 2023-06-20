# LiveStreamCapture
## cpp+ffmpeg 练手demo
**项目组成：cpp ffmpeg qt sdl2 js**
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
  * 完善转码代码
  * 优化编码导致内存过高问题(1-2GB)  
  
### 已知问题：
  * 1.qt 选择文件夹函数偶尔导致窗口闪退
  * 2.界面逻辑基础，按钮无防暴击功能，待完善 