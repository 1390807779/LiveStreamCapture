@echo off
set exeDir = %~dp0LiveStreamCapture.exe
echo %~dp0LiveStreamCapture.exe
reg add "HKEY_CLASSES_ROOT\LiveStreamCaptureExe"
reg add "HKEY_CLASSES_ROOT\LiveStreamCaptureExe" /t REG_EXPAND_SZ /d LiveStreamCaptureExeProtocol
reg add "HKEY_CLASSES_ROOT\LiveStreamCaptureExe" /v "URL Protocol" /t REG_EXPAND_SZ /d %~dp0LiveStreamCapture.exe
reg add "HKEY_CLASSES_ROOT\LiveStreamCaptureExe\DefaultIcon"
reg add "HKEY_CLASSES_ROOT\LiveStreamCaptureExe\DefaultIcon" /t REG_EXPAND_SZ /d %~dp0LiveStreamCapture.exe
reg add "HKEY_CLASSES_ROOT\LiveStreamCaptureExe\shell"
reg add "HKEY_CLASSES_ROOT\LiveStreamCaptureExe\shell\open"
reg add "HKEY_CLASSES_ROOT\LiveStreamCaptureExe\shell\open\command"
reg add "HKEY_CLASSES_ROOT\LiveStreamCaptureExe\shell\open\command" /t REG_EXPAND_SZ /d "\"%~dp0LiveStreamCapture.exe\" \"%%L\""
pause