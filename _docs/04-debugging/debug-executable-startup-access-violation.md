---
id: OTMS-DBG-001
title: Debug 可执行文件启动前访问冲突
document_type: debugging
module: build-runtime
status: resolved

verification_status: verified

related_documents:
  - ../03-detailed-design/overall-ui-design.md
config_refs:
  - ../../CMakeLists.txt
  - ../../CMakePresets.json

created: 2026-08-11
updated: 2026-08-11
summary: MSVC x64 Debug 可执行文件在进入 main 之前因导入地址异常发生 0xc0000005，通过 clean build 恢复。
---

# Debug 可执行文件启动前访问冲突

## 1. 环境

- 操作系统：Windows 10 x64，构建号 19045；
- 构建工具：Visual Studio 2022 / MSBuild 17.10 / MSVC 14.40；
- 界面框架：Qt 6.5.3 MSVC x64 Debug；
- CMake 预设：`msvc2022-x64` + `msvc-debug`；
- 运行目标：`out/build/msvc2022-x64/Debug/OpticalThicknessUi.exe`。

## 2. 现象和影响

2026-08-11 的一次 Debug 构建后，双击或启动程序时主窗口不出现，
进程随即退出。在此之前的构建曾因已运行的程序占用目标文件而报：

```text
LINK : fatal error LNK1168: 无法打开 .../OpticalThicknessUi.exe 进行写入
```

故障期间 Windows 应用程序事件连续记录 `0xc0000005` 访问冲突，
但 `logs/runtime.log` 没有产生新的“应用程序启动”记录。
这表明崩溃发生在应用日志初始化之前，不是业务工作流或设备初始化失败。

## 3. 诊断证据

Windows CrashDumps 中的转储显示：

```text
WinMainCRTStartup
  -> __scrt_common_main
  -> pre_c_initialization
  -> __scrt_initialize_type_info
  -> call [__imp_InitializeSListHead]
  -> 0x0000000000000000
```

- 异常为尝试执行空地址 `0x0`；
- 问题位于 MSVC CRT 预初始化，早于 `main()`；
- 崩溃时 `InitializeSListHead` 的导入调用未得到有效地址；
- `dumpbin /imports:KERNEL32.dll` 能在 PE 导入表中找到该符号，
  但崩溃转储中的运行时间接调用目标为空；
- 当时待修改的滚动条 QSS/XPM 资源尚未成功链接进该 EXE，
  因此它们不是这次启动崩溃的原因。

## 4. 原因结论

### 已确认的直接原因

当时生成的 Debug 可执行文件存在 CRT 启动导入地址异常，
`__scrt_initialize_type_info()` 通过空函数地址调用 `InitializeSListHead`，
导致程序在进入 `main()` 前发生 `0xc0000005`。

### 可能的生成诱因

故障前多次构建遭遇运行中的 EXE 文件锁，随后又使用原有增量链接中间产物生成了新 EXE。
结合 clean build 后问题消失，可推断增量链接中间产物或目标文件在这一过程中异常。
现有证据不足以将“文件锁”单独认定为根因，因此本项保持为有证据支持的推断。

## 5. 解决方案

1. 确认已没有 `OpticalThicknessUi.exe` 进程占用构建目标；
2. 使用已确认的 MSVC x64 Debug 预设执行完整清理重建：

   ```powershell
   cmake --build --preset msvc-debug --clean-first
   ```

3. 确认 MSBuild 返回 `0`，新的 EXE 和 PDB 时间戳一致；
4. 使用 `dumpbin /imports:KERNEL32.dll` 确认 PE 导入表包含
   `InitializeSListHead`；
5. 使用虚拟设备模式启动程序，确认主窗口正常出现。

2026-08-11 完成 clean build 后，用户已确认软件恢复正常启动。

## 6. 预防和复发处理

- 构建前先关闭正在运行的 Debug 程序；
- 出现 `LNK1168` 时不要连续重试链接，先确认并处理占用进程；
- 如果新 EXE 启动后在运行日志生成前退出，优先检查 Windows 应用程序事件和 CrashDumps；
- 如果转储显示 CRT 预初始化或导入地址异常，先停止目标进程并执行 clean build；
- 不通过禁用检查、更换编译器或随意复制其他版本运行库规避问题。

## 7. 其他观察

成功构建末尾仍出现了系统找不到 `pwsh.exe` 的提示，
但 MSBuild 返回 `0` 且 EXE/PDB 正常生成。
该提示未导致本次启动故障，可作为独立的构建环境问题后续追踪。
