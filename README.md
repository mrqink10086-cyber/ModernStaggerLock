# ModernStaggerLock — CommonLibSSE-NG

影天韧性大修的 CommonLibSSE-NG 移植版，插件版本 **1.2.0**。

## 安装

本次交付是 DLL 更新包。将包内 `Data` 合并到游戏目录，或用 MO2/Vortex 安装，覆盖原来的 `Data/SKSE/Plugins/ModernStaggerLock.dll`。

| 游戏 | SKSE | Address Library |
| --- | --- | --- |
| Steam 1.6.1170 | 2.2.6 | 对应 1.6.1170 的 AE 数据库 |
| 1.5.97 | 2.0.20 | 对应 1.5.97 的 SE 数据库 |

保留原版的行为、动画、JSON/INI 配置及相关前置；本包不包含这些资源。原项目的 `Stagger_Lock_Behaviour` 是独立子模块。Precision V5 与 Open Animation Replacer Conditions V2 的集成接口继续保留。需要 Microsoft Visual C++ x64 运行库。

支持范围限定为上表两个已核查的运行时；不声明支持 VR、GOG 1.6.1179 或其他 AE 版本。插件加载时检查游戏与 SKSE 版本，安装代码补丁前检查落地调用目标和硬直分支机器码。失败信息记录在 SKSE 日志目录的 `ModernStaggerLock.log`。

已通过 Release 构建、DLL 导出/版本信息检查，以及两版游戏二进制共 18 项地址核查。尚未进行游戏内加载和战斗实测，具体证据见 [偏移核查报告](docs/OFFSETS.md)。

## 构建

需要 Visual Studio 2022 C++ 工具、Windows SDK、CMake 3.24+、vcpkg 和 C++23。

默认直接使用相邻目录 `../CommonLibSSE-NG-ng`；可通过 `-DCOMMONLIBSSE_NG_PATH=...` 指定。此次核对的本地源码与上游提交 `07262cc2ae75777a422ed0c9621310fadef7c382`（15.7.3）一致；已有的本地构建脚本差异予以保留。

DKUtil 使用 `external/DKUtil`，缺失时 CMake 获取固定提交 `19fcb2d2ee00a9b678bf08b24c53208ab7618fc6`。可用 `-DDKUTIL_PATH=...` 指定已有源码。

```powershell
$env:VCPKG_ROOT = 'E:/vcpkg'
cmake --preset NG
cmake --build --preset NG --parallel 8
cmake --install build/ng --config Release --component Plugin --prefix dist/package
```

`AE`、`SE` preset 可构建单一运行时版本；默认 `NG` 构建同时支持上表两版的 DLL。旧 `!Update.ps1` 不再参与构建。

本机实际构建复用了已安装的静态依赖，完整命令与迁移范围见 [迁移记录](docs/migration-commonlibsse-ng.md)。

## 来源

- [ModernStaggerLock 原项目](https://github.com/max-su-2019/ModernStaggerLock)
- [CommonLibSSE-NG](https://github.com/alandtse/CommonLibSSE-NG)
- [SKSE](https://skse.silverlock.org/)
- [原版行为资源](https://github.com/max-su-2019/Stagger_Lock_Behaviour)
