LILYGO T-Display-S3 AMOLED video player

Files:
- TDisplayS3_AMOLED_VideoPlayer.ino  Arduino sketch
- data/video.mjpeg                    Converted video
- convert_video.bat                    Convert another MP4 using FFmpeg

Libraries:
- Arduino_GFX_Library
- JPEGDEC
- ESP32 Arduino core

Arduino IDE:
- Board: ESP32S3 Dev Module (or LILYGO T-Display-S3 AMOLED if available)
- Flash size: 16MB
- PSRAM: OPI PSRAM
- CPU: 240MHz
- USB CDC On Boot: Enabled

IMPORTANT:
The video is uploaded to LittleFS, not as a normal sketch file.
Use an ESP32 LittleFS filesystem uploader/tool to upload the data folder.
The final filesystem must contain:
    /video.mjpeg
