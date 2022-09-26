# MultiStreamAI

This is a AI Demo Project on QT that support x86 and embedded (ARM) platform with Hailo AI Accelerator

NOTE: In this branch 'StreamingOnly' the hailo AI integration portion is removed, this is intended for a pure streaming test on different platform before adding Hailo related code/API/library


# Platform Config

For specific platform such as Nvidia Jetson please add defines in [MultiStreamAI.pro](https://github.com/Yalee104/MultiStreamAI/blob/main/MultiStreamAI.pro) as shown below, this will compile for target specific that uses gstreamer with HW decoder of specific platform for optimim performance with multiple streaming.

```
DEFINES += QT_ON_JETSON
```


