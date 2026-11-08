# ModernStaggerLock — CommonLibSSE-NG

影天硬直锁定（Modern Stagger Lock）的 CommonLibSSE-NG 移植版，插件版本 **1.2.2**。一份 DLL 同时支持 Skyrim SE 1.5.97 与 AE 1.6.1170，保留原版的行为、动画、JSON/INI 配置与前置依赖，只替换插件本体。

## 安装

1. 装好对应版本的 SKSE 与 Address Library、Microsoft Visual C++ x64 运行库，以及原版 ModernStaggerLock 的行为/动画/配置资源。
2. 把包内 `Data` 合并到游戏目录，或用 MO2/Vortex 安装，覆盖 `Data/SKSE/Plugins/ModernStaggerLock.dll`。
3. 原有 INI/JSON 配置保留不动，本包不带默认配置，不会覆盖你的设置。

| 游戏 | SKSE | Address Library |
| --- | --- | --- |
| Steam 1.6.1170 | 2.2.6 | 对应 1.6.1170 的 AE 数据库 |
| 1.5.97 | 2.0.20 | 对应 1.5.97 的 SE 数据库 |

包内只有插件本体，不含行为、动画和配置资源。

## 兼容与验证

Precision V5 与 Open Animation Replacer Conditions V2 的集成接口继续保留。普通 Precision 2.0.6 仅提供 API V1–V4，无法提供特殊硬直联动所需的 V5 载荷接口；缺少 V5 时会记录警告并跳过该联动，普通硬直分级、恢复和跳跃限制等本体逻辑继续执行。此处 V5 是作者 Precision 分支增加的接口，不能根据 Precision 的发布日期或软件版本号推断它存在。上游来源及原版降级行为见 [Precision 上游核查](docs/PRECISION-UPSTREAM.md)。

支持范围限定为上表两个已核查的运行时；不声明支持 VR、GOG 1.6.1179 或其他 AE 版本。插件加载时检查游戏与 SKSE 版本，安装代码补丁前检查落地调用目标和硬直分支机器码。失败信息记录在 SKSE 日志目录的 `ModernStaggerLock.log`。

已通过 Release 构建、DLL 导出/版本信息检查，以及两版游戏二进制共 18 项地址核查。尚未进行游戏内加载和战斗实测，具体证据见 [偏移核查报告](docs/OFFSETS.md)。

## 变更

1.2.1 用事件过滤、图变量名缓存、配置快照、Precision 载荷解析缓存和连续转场数组压低回调耗时：已支持事件分类 55.16 → 17.56 ns/次，无关事件 12.88 → 1.51 ns/次，特殊受击 JSON 解析改缓存命中 1013 → 43.8 ns/次，转场列表整份复制 830 → 4.13 ns/次，恢复事件 256 项 1013 → 78.8 ns/次（i5-13600KF，7 轮中位数）。同步时序和回调顺序不变，详情、测试与基准见 [运行时优化记录](docs/OPTIMIZATION.md)。

1.2.2 修指针与内存安全边界：运行时数据改为初始化完成后原子发布，所有 Hook 先读到 ready 再访问插件数据，未就绪时只调用游戏原函数；计数器改为有符号溢出安全实现；JSON 与运行时输入加内存上限——单次运行时载荷 64 KiB、单个 JSON 配置 4 MiB、累计送入解析的内容 16 MiB，找 NUL 的 C 字符串扫描最多 64 KiB+1 字节而不再用无界 `strlen`，超限输入跳过而不是继续解析。详情见 [安全修复记录](docs/SAFETY-FIXES.md)，本地 Precision 的核查见 [Precision 审计](docs/PRECISION-AUDIT.md)。

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
