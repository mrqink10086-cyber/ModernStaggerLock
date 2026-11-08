# CommonLibSSE-NG 迁移审查

审查日期：2026-11-08。以原始代码快照 `c5c3a1f207291e88f42db4e45e8fba21fd0dfc44` 为基线，对照迁移提交 `4865bbbbd2037b7ddd66bee3bff3a6863a886f1b`，分别进行规范和需求审查。

## Standards：规范审查

未发现需要修复的规范或结构问题。审查覆盖源码、构建配置和地址核查工具；运行时地址集中在 `src/Runtime.h`。

## Spec：需求审查

发现并修复一项 P1 问题：新版 CommonLib 不再定义 `SKSEAPI`，DKUtil 因此会将 INI 配置目录默认为游戏根目录。提交 `7fab30b671ad0a5d03a7a39a5795a3c84f2d289e` 在引入 DKUtil 前显式设置 `CONFIG_ENTRY`，恢复 `Data/SKSE/Plugins/ModernStaggerLock.ini`。

复核确认所有源文件强制使用该 PCH，配置路径修复生效；其他 DKUtil 条件分支没有同类剩余影响。游戏与 SKSE 版本检查、虚函数槽位、次级虚表参数和 AE 地址分支未发现其他确定性问题。

修复后完整 x64 Release 构建成功。DLL 导出、插件版本与最低 SKSE 元数据通过检查，安装包中的 DLL 与构建产物一致；DLL 还包含恢复后的配置目录常量。两版运行时共 18 项离线地址检查通过，详见 [偏移核查](OFFSETS.md)。尚未进行游戏内实测。

审查合计：规范 0 项；需求 1 项，已修复并复核；未解决问题 0 项。
