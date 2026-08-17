---
id: OTMS-DBG-002
title: 圆环覆盖表格滚动条和箭头不能持续显示
document_type: debugging
module: ui
status: investigating

verification_status: unverified

related_documents:
  - ../03-detailed-design/overall-ui-design.md
code_refs:
  - ../../src/view/automatic_operation_widget.cpp
  - ../../resources/styles/application.qss
  - ../../resources/ui_resources.qrc

created: 2026-08-11
updated: 2026-08-11
summary: 圆形阵列的已设置覆盖表格受按需滚动条、原生箭头和父布局裁剪影响，无法保证完整持续显示。
---

# 圆环覆盖表格滚动条和箭头不能持续显示

## 1. 目标和现象

目标控件为自动化操作页中圆形阵列的“已设置覆盖”表格，
对应 `AutomaticOperationWidget::circleRingOverrides_`。

预期行为：无论当前行数、是否可滚动或 Windows 界面风格如何，
表格都要完整显示垂直滚动条、顶部向上箭头和底部向下箭头。

实际现象：原始界面在部分数据量或系统样式下只显示部分滚动条内容，
上下箭头不能保证同时、完整可见。

首次修复已将表格设为 `ScrollBarAlwaysOn` 并增加固定箭头资源，
但运行界面仍出现底部向下箭头不可见，因此该问题按复发继续诊断。

## 2. 原因

### 滚动条显示策略

`circleRingOverrides_` 原先没有显式设置垂直滚动条策略，
因此使用 Qt 默认的 `Qt::ScrollBarAsNeeded`。
当内容未溢出时，Qt 可以隐藏整个滚动条，与“始终显示”的需求冲突。

### 箭头绘制依赖

应用原先没有为该滚动条定义专用样式和箭头图形，
上下按钮的尺寸和箭头绘制由 Qt/Windows 原生样式决定。
这种依赖无法在系统样式、DPI 或控件禁用状态变化时保证一致显示。

### 父布局裁剪

复发时 `circleRingOverrides_` 的最小高度为 300 像素，
但包含它的“任务配置”区域直接放在主页布局中，没有外层滚动容器。
当中央工作区可用高度小于任务配置的最小尺寸时，
父级可视区域会裁剪表格底部。滚动条和向下箭头虽已创建，
但位于被裁剪的表格几何区域内，因此 QSS 无法使其可见。

## 3. 解决方案

1. 为表格设置唯一对象名 `circleRingOverrides`，使样式仅作用于该控件；
2. 将垂直滚动条策略设为 `Qt::ScrollBarAlwaysOn`；
3. 在 `application.qss` 中固定垂直滚动条的宽度、轨道、滑块和上下按钮尺寸；
4. 将 `scrollbar-up.xpm` 和 `scrollbar-down.xpm` 编译到 Qt 资源，
   由 QSS 显式绘制上下箭头；
5. 不修改其他表格的滚动条，也不强制显示无需要的表格水平滚动条。

上述基础修复已实现，但没有解决父布局裁剪导致的底部箭头不可见问题。

### 已撤回方案

2026-08-11 曾尝试在左侧“任务配置”组外增加 `QScrollArea`，
计划在高度不足时滚动整个配置区。用户不接受该交互和布局改动，
该代码已撤回，未编译、未运行、不作为当前解决方案。

## 4. 实现位置

- `src/view/automatic_operation_widget.cpp`：对象名和 `ScrollBarAlwaysOn`；
- `resources/styles/application.qss`：表格专用的滚动条样式；
- `resources/styles/scrollbar-up.xpm`：向上箭头资源；
- `resources/styles/scrollbar-down.xpm`：向下箭头资源；
- `resources/ui_resources.qrc`：箭头资源登记。

## 5. 验证状态

- 2026-08-11：资源引用、QSS 选择器和代码引用静态检查通过；
- 2026-08-11：MSVC x64 Debug clean build 通过；
- 2026-08-11：确认首次修复后仍存在父布局裁剪；外层滚动容器方案未被接受并已撤回；
- 尚未单独记录“无数据、少量数据、可滚动数据”三种界面状态的人工可见性验证，
  因此文档的 `verification_status` 保持 `unverified`。

## 6. 复发检查

如果滚动条或箭头再次不完整，按以下顺序检查：

1. 确认表格对象名仍为 `circleRingOverrides`；
2. 确认垂直策略仍为 `Qt::ScrollBarAlwaysOn`；
3. 确认 `application.qss` 已成功从 `:/styles/application.qss` 加载；
4. 确认两个 XPM 资源已编入 `ui_resources.qrc`；
5. 检查表格实际几何高度和父级可视区域，确认底部是否被裁剪；
6. 在不改变整个任务配置区交互的前提下重新评估局部布局方案。
