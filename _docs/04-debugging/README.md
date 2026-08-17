# 调试

## 职能

只说明代码为何没有按照预定逻辑执行，包括目标、预期与实际结果、问题及原因。

## 文档索引

- [OTMS-DBG-001 Debug 可执行文件启动前访问冲突](debug-executable-startup-access-violation.md)
  — `resolved / verified`，记录 `0xc0000005` CRT 启动崩溃的诊断证据、原因边界和 clean build 恢复方案。
- [OTMS-DBG-002 圆环覆盖表格滚动条和箭头不能持续显示](circle-override-scrollbar-visibility.md)
  — `investigating / unverified`，记录 Qt 滚动条、原生箭头及父布局裁剪问题；外层滚动区方案已撤回。
