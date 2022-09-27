# MultiStreamAI

This is a AI Demo Project on QT that support x86 and embedded (ARM) platform with Hailo AI Accelerator, this project supports QT version 5.12 or latest, although it may still be possible to support 5.10 but it is not tested.

NOTE: In this branch 'StreamingOnly' the hailo AI integration portion is removed, this is intended for a pure streaming test on different platform before adding Hailo related code/API/library

# Platform Config

For specific platform such as Nvidia Jetson please add defines in [MultiStreamAI.pro](https://github.com/Yalee104/MultiStreamAI/blob/main/MultiStreamAI.pro) as shown below, this will compile for target specific that uses gstreamer with HW decoder of specific platform for optimim performance with multiple streaming.

```
DEFINES += QT_ON_JETSON
```

# Environment Setup

For x86 simply go to [QT OpenSource](https://www.qt.io/download-qt-installer?hsCtaTracking=99d9dd4f-5681-48d2-b096-470725510d34%7C074ddad0-fdef-4e53-8aa8-5e8a876d6ab4) and download either online or offline installs. For Ubuntu 18.04 suggested to download the offline file which only support up to QT 5.15.

For ARM embedded system it depends on which platform but this page [QT on Embedded](https://github.com/Yalee104/QtOnEmbedded) provides instruction for cross compilation for some embedded system.
