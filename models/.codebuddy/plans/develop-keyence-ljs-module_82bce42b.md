---
name: develop-keyence-ljs-module
overview: 基于现有 `Hd_CameraModule_3DKeyence3` 的模块接口，新增/完善 LJS 系列 3D 基恩士相机模块，并对接上级目录 `Sample_ImageAcquisition/src/CPP/LJS8_ImageAcquisitionSample` demo 中的 LJS8 SDK 调用方式。
todos:
  - id: explore-interfaces
    content: 使用 [subagent:code-explorer] 核对 LJX 接口、CMake 与 LJS SDK
    status: completed
  - id: create-lfs-module-files
    content: 新增 Hd_CameraModule_3DKeyenceLJS3 模块文件
    status: completed
    dependencies:
      - explore-interfaces
  - id: port-ljs-sdk
    content: 使用 [skill:C/C++ Comprehensive Cheat Sheets] 移植 LJS8 通信采集逻辑
    status: completed
    dependencies:
      - create-lfs-module-files
  - id: wire-exports-ui
    content: 实现导出函数、参数 JSON、取图队列和测试控件
    status: completed
    dependencies:
      - port-ljs-sdk
  - id: fix-cmake-linking
    content: 修正 CMake 链接 keyence3DLJS8 和 LJS8_IF
    status: completed
    dependencies:
      - create-lfs-module-files
  - id: review-and-verify
    content: 使用 [skill:C++ Code Review Master] 审查并验证构建风险
    status: completed
    dependencies:
      - wire-exports-ui
      - fix-cmake-linking
---

## User Requirements

- 在当前 `models` 目录下开发一个 LJS 系列 3D 基恩士相机模块。
- 新模块需要按现有 `Hd_CameraModule_3DKeyence3` 的外部接口、动态库导出函数和宿主调用方式实现。
- LJS 采集逻辑参考上级目录 `Sample_ImageAcquisition/src/CPP/LJS8_ImageAcquisitionSample` 中的官方 demo。
- 保持相机模块可被现有模块加载机制自动编译、加载、创建、销毁、获取参数、触发采集、获取图像和注册回调。

## Product Overview

新增一个 LJS8 3D 基恩士相机适配模块，使宿主系统可以像使用现有 LJX 模块一样使用 LJS 系列设备。

## Core Features

- 提供与 `Hd_CameraModule_3DKeyence3` 一致的模块接口和导出函数。
- 支持 LJS8 SDK 初始化、以太网连接、高速通信预启动、采集触发、停止通信和关闭设备。
- 支持配置文件参数加载与保存，包括 IP、端口、超时、触发方式、图像尺寸、PC 图像滤波等。
- 支持高度图和亮度图采集输出，高度图按 16-bit、亮度图按 8-bit 处理。
- 支持软触发、允许/禁止取图、队列取图和回调出图。
- 保留现有相机参数编辑和图像预览控件交互方式。

## Tech Stack Selection

- 语言与框架：C++17、Qt 5.15.2、Qt Widgets、Qt 信号槽。
- 图像处理：OpenCV，沿用现有模块中 `cv::Mat` 数据结构与显示转换工具。
- 构建系统：CMake，沿用 `models/CMakeLists.txt` 自动遍历子目录的模块构建方式。
- SDK：Keyence LJS8 SDK，使用 `env/keyence3DLJS8/include/LJS8_IF.h`、`LJS8_ErrorCode.h` 和 `env/keyence3DLJS8/lib/LJS8_IF.lib`。
- 接口基类：沿用 `PbGlobalObject`、`PBGLOBAL_CALLBACK_FUN`、`ThreadSafeQueue`、`AlgParmWidget`、`ImageViewer` 等现有全局组件。

## Implementation Approach

- 以 `Hd_CameraModule_3DKeyence3` 为模板，在空目录 `Hd_CameraModule_3DKeyenceLJS3` 中新增同构模块，保持宿主侧导出函数和 `PbGlobalObject` 方法语义一致。
- 将 LJX8 SDK 调用替换为 LJS8 demo 中验证过的 LJS8 调用链：`LJS8IF_EthernetOpen`、`LJS8IF_InitializeHighSpeedDataCommunicationSimpleArray`、`LJS8IF_PreStartHighSpeedDataCommunication`、`LJS8IF_StartHighSpeedDataCommunication`、`LJS8IF_Trigger`、`LJS8IF_StopHighSpeedDataCommunication`、`LJS8IF_FinalizeHighSpeedDataCommunication`。
- 优先采用非阻塞/回调式高速通信，与现有 LJX 模块的数据队列和回调模式保持一致，避免将 demo 的同步阻塞采集直接暴露给宿主调用链。
- 全局状态、回调函数、设备数组使用 LJS 专属命名或匿名命名空间，避免与现有 LJX 模块在同进程动态加载时出现符号冲突。
- 亮度图严格按 LJS demo 的 `BYTE`/`unsigned char` 处理为 `CV_8UC1`，高度图按 `WORD`/`unsigned short` 处理为 `CV_16UC1`，避免照搬 LJX 亮度图 16-bit 归一化逻辑。

## Implementation Notes

- 需遵守已读取的 C/C++ 安全规范：避免不安全字符串函数、避免可变长栈数组、检查指针和内存分配结果、每个失败分支释放资源并返回明确错误。
- LJS demo 的 `malloc/free` 可在模块中保留但必须集中封装释放路径；更推荐在回调中尽快 clone 到 `cv::Mat` 或使用 `std::vector` 管理临时缓存，降低泄漏风险。
- `data()` 继续使用 `ThreadSafeQueue::wait_for_pop(timeout_ms, ImgS)`，保持调用方行为不变。
- `setData()` 中软触发应使用 `LJS8IF_Trigger(deviceId)`，外触发模式不主动触发。
- `getCameraSnList()` 可复用 LJX 的 ARP/IP 文件策略，但目录名应改为 LJS 专属目录，例如 `./3DKeyenceLJS/Ip.txt`，避免污染 LJX 设备缓存。
- 顶层 CMake 当前 `keyence3DLJS8` 可能被 `keyence3D` 匹配分支误设为 `LJX8_IF`；需要在 LJS 模块 CMake 中明确链接 `LJS8_IF`，或调整顶层匹配顺序先匹配 `keyence3DLJS8`。
- 由于 `env/keyence3DLJS8/dll` 当前未发现 dll，构建后运行前需确认运行目录存在 `LJS8_IF.dll` 或相关运行时依赖。

## Architecture Design

- 宿主动态加载层：通过导出函数 `create`、`destroy`、`getCameraWidgetPtr`、`getCameraPtr`、`getCameraSnList` 管理模块实例。
- 模块对象层：`Hd_CameraModule_3DKeyenceLJS3` 继承 `PbGlobalObject`，负责参数、生命周期、触发、取图和回调注册。
- SDK 封装层：LJS 专属 `CameraFunSDKfactoryCls` 负责 LJS8 SDK 初始化、连接、通信、回调数据转换和资源释放。
- UI 辅助层：`mPrivateWidget` 复用现有软触发、允许取图、禁止取图、图像显示和参数编辑交互。
- 数据流：宿主触发或外部触发 → LJS8 高速回调 → 转换 `cv::Mat` → 回调发送或写入线程安全队列 → 宿主 `data()` 获取。

## Directory Structure

本次实现主要新增 LJS 模块文件，并可能小范围调整顶层 CMake 的 SDK 链接映射。

```
e:/work/3.0Modules/
├── CMakeLists.txt
│   # [MODIFY] 顶层依赖映射。优先识别 keyence3DLJS8 并设置目标库 LJS8_IF，避免被 keyence3D 分支错误链接为 LJX8_IF。
└── models/
    └── Hd_CameraModule_3DKeyenceLJS3/
        ├── CMakeLists.txt
        │   # [NEW] LJS 模块构建脚本。引用 keyence3DLJS8、opencv、全局 include，生成共享库并链接 LJS8_IF。
        ├── Hd_CameraModule_3DKeyenceLJS3.h
        │   # [NEW] LJS 模块头文件。声明 PbGlobalObject 派生类、SDK 封装类、参数结构、UI Widget 和 C 导出函数。
        └── Hd_CameraModule_3DKeyenceLJS3.cpp
            # [NEW] LJS 模块实现。实现参数加载、设备扫描、SDK 初始化、触发采集、回调转换、队列取图、资源释放和测试 Widget。
```

## Key Code Structures

- 主类保持与现有模块等价的方法集合：`setParameter`、`parameters`、`init`、`setData`、`data`、`registerCallBackFun`、`cancelCallBackFun`。
- LJS 参数结构需要覆盖 demo 参数：`timeout_ms`、`useExternalTrigger`、`usePcImageFilter`，并在模块 JSON 中与现有配置命名兼容。
- 导出函数名称保持不变，确保宿主无需修改动态加载协议。

## Agent Extensions

### Skill

- **C/C++ Comprehensive Cheat Sheets**
- Purpose: 辅助实现 C++17、资源管理、回调与内存安全相关代码。
- Expected outcome: LJS 模块实现符合现代 C++ 与项目安全规范，减少内存泄漏和未定义行为。

- **C++ Code Review Master**
- Purpose: 在实现后对新增 C++ 模块进行专项代码审查。
- Expected outcome: 发现并修复 LJS SDK 调用、资源释放、线程安全、指针和数组边界方面的问题。

### SubAgent

- **code-explorer**
- Purpose: 继续核对现有模块接口、CMake 依赖映射和 LJS demo API 细节。
- Expected outcome: 确认新增文件、依赖库、函数签名和宿主加载协议完全匹配现有工程。