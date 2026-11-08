# CommonLibSSE-NG 迁移记录

日期：2026-11-08。原始代码快照：`c5c3a1f207291e88f42db4e45e8fba21fd0dfc44`。

## 用户要求与实现

1. 将 ModernStaggerLock 改为使用本地 `E:/source/repos/CommonLibSSE-NG-ng`，确认它是最新上游版本，否则更新。
2. 更新至支持 Skyrim 1.6.1170 及对应 SKSE。
3. 核查虚函数地址偏移是否正确。

本地 CommonLibSSE-NG 与 GitHub `ng` 分支提交 `07262cc2ae75777a422ed0c9621310fadef7c382` 的源码逐文件 Git blob SHA-1 比较通过，版本 15.7.3；仅 `scripts/build-all-presets.cmd` 有本地差异，保留该文件，未覆盖 CommonLib 本地内容。

CMake 直接编译上述 CommonLib，明确使用 SE/AE，关闭 VR。固定 DKUtil 版本并声明所需依赖；去除原构建中依赖环境变量和 `!Update.ps1` 的自动生成/复制流程。插件元数据、游戏检查、SKSE 最低版本检查、导出和 WinAPI 调用适配新版接口。日志使用 CommonLib 的 SKSE 日志目录；旧版分离到 cpp 的 constexpr 哈希函数改为普通函数以保证链接。

原有 1.5.97 支持保留，并同样用实际二进制核查。所有版本限定、代码补丁地址及加载前机器码检查集中在 `src/Runtime.h`。虚表槽位不需要改变；NPC/玩家落地调用和硬直条件分支的 AE 地址与 SE 不同，已分别指定。具体数值、指令语义与核查过程见 [OFFSETS.md](OFFSETS.md)。

## 构建验证

本机使用 MSVC 19.44.35229、Windows SDK 10.0.26100.0，x64 Release，动态 CRT；第三方库静态链接，CommonLib 从源代码完整构建。

```powershell
cmake -S . -B build/ng-local -G 'Visual Studio 17 2022' -A x64 `
  '-DCMAKE_PREFIX_PATH=E:/source/repos/valhallaCombat/build/vcpkg_installed/x64-windows-static-md;E:/vcpkg/packages/nlohmann-json_x64-windows-skse' `
  -DFETCHCONTENT_SOURCE_DIR_HDE64=E:/source/repos/valhallaCombat/build/_deps/hde64-src `
  -DCOMMONLIB_ENABLE_IPO=OFF
cmake --build build/ng-local --config Release --target ModernStaggerLock --parallel 8
cmake --install build/ng-local --config Release --component Plugin --prefix dist/package
```

上述复用依赖路径是本机的构建选择；正常环境使用根目录 README 的 vcpkg preset。构建期间保留了既有代码的 float 转换和 DKUtil 的 nodiscard 警告，没有编译或链接错误。

已检查产物为 AMD64 DLL，具有 `SKSEPlugin_Query`、`SKSEPlugin_Load`、`SKSEPlugin_Version` 三个导出。版本为 1.2.0，AE 兼容元数据包含 1.6.1170、SKSE 最低 2.2.6、Address Library 与 post-629 结构兼容标记。SE 使用 Query/Load 路径及 2.0.20 检查。

## 游戏内验收待执行

本次没有启动游戏或改动游戏安装目录。离线二进制核查证明了所列地址、槽位和调用目标，不能替代游戏内功能验收。

在已有原版行为/动画资源及前置的配置中，通过 SKSE 启动 1.6.1170，确认 `skse64.log` 正常加载插件、`ModernStaggerLock.log` 输出全部 hook 安装记录，再检查：NPC 与玩家硬直、快速恢复、硬直中跳跃限制、特殊硬直落地转换、Precision 命中参数与 OAR 条件。1.5.97 的兼容性也需游戏内回归。
