# MultiStreamAI

This is a AI Demo Project on QT that support x86 and embedded (ARM) platform with Hailo AI Accelerator, this project supports QT version 5.12 or latest, although it may still be possible to support 5.10 but it is not tested.

NOTE: In this branch 'StreamingOnly' the hailo AI integration portion is removed, this is intended for a pure streaming test on different platform before adding Hailo related code/API/library


# Platform Config

For specific platform such as Nvidia Jetson please add defines in [MultiStreamAI.pro](https://github.com/Yalee104/MultiStreamAI/blob/main/MultiStreamAI.pro) as shown below, this will compile for target specific that uses gstreamer with HW decoder of specific platform for optimim performance with multiple streaming.

```
DEFINES += QT_ON_JETSON
```

# Environment Setup

Currently the project only supports QT 5.x.x, make sure this version is used when compiling the project

__Linux__

- For x86 simply go to [QT OpenSource](https://www.qt.io/download-qt-installer?hsCtaTracking=99d9dd4f-5681-48d2-b096-470725510d34%7C074ddad0-fdef-4e53-8aa8-5e8a876d6ab4) and download either online or offline installs. For Ubuntu 18.04 suggested to download the offline file which only support up to QT 5.15.
- OpenCV 4.x, this is required for some analytic application although it will be optional in the future, how to install instruction please refer to this [Link](https://linuxize.com/post/how-to-install-opencv-on-ubuntu-18-04/)


__Windows__

- For x86 simply go to [QT OpenSource](https://www.qt.io/download-qt-installer?hsCtaTracking=99d9dd4f-5681-48d2-b096-470725510d34%7C074ddad0-fdef-4e53-8aa8-5e8a876d6ab4) and download either online or offline installs. 
- Please note that for windows you may need to install additional codec to play .mp4 or other format. You can install codec pack such as K-Lite (https://www.codecguide.com/download_k-lite_codec_pack_basic.htm)
- OpenCV 4.x, this is required for some analytic application although it will be optional in the future, you can simply download pre-build libraries for windows from this [Link](https://docs.opencv.org/4.x/d3/d52/tutorial_windows_install.html). Make sure the path for the vc5 lib and bin is set accordingly for the windows OpenCV path under MultiStreamAI.pro file 

__ARM__

- For ARM embedded system it depends on which platform but this page [QT on Embedded](https://github.com/Yalee104/QtOnEmbedded) provides instruction for cross compilation for some embedded system.
- OpenCV 4.x, this is required for some analytic application although it will be optional in the future, for install instruction please follow Linux guide.

