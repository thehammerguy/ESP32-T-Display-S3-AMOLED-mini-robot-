@echo off
REM Requires FFmpeg in PATH.
REM Converts an MP4 to a 536x240, 15 FPS MJPEG file for the LILYGO T-Display-S3 AMOLED.
ffmpeg -i "%~1" -vf "crop=854:381:0:49,scale=536:240:flags=lanczos,fps=15" -q:v 10 -an -f mjpeg "data\video.mjpeg"
pause
