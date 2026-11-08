# 本地 Precision 兼容性与 API 安全核查

路径：`E:/source/repos/Precision-main`。日期：2026-11-08。范围为当前 Mod 所需接口及相关公开 API 的生命周期与加锁行为；未修改 Precision 源码，未构建或交付 Precision DLL。

## 当前目录不是 V5 提供方

`CMakeLists.txt:3-6` 标明版本 2.0.6；`src/PrecisionAPI.h:12-18` 只有 V1–V4；`src/ModAPI.h:22` 实现 V4。`src/main.cpp:125-145` 对这些版本返回接口 singleton，对 V5 返回 `nullptr`。

本地全文未找到 `GetCachedExtraData`、`AddExtraParameterName` 或 V5 命中额外数据缓存实现。前一轮对照的是 max-su-2019 的 V5 分支，不能把那个分支的缓存指针问题归到本目录。

ModernStaggerLock 请求 V5 并检查空接口，这里不会误把 V4 当 V5 调用。结果是基础 Mod 处理可以继续，Precision 特殊硬直载荷联动无法使用。1.2.2 增加缺少 V5 时的日志警告。若需要该联动，必须使用兼容的 V5 提供方；单纯修改版本枚举不能补出缺失的实现。

## 与本 Mod 无直接调用关系的 API 风险

以下均不在当前 ModernStaggerLock 的活动调用路径中，不能据此认定它们造成了本 Mod 崩溃。

### 回调内重新注册/移除回调可能死锁

`src/PrecisionHandler.cpp:1994-2104` 的 Run 回调方法持有 callbacksLock 的读锁时调用外部插件代码。Add/Remove 路径需要同一锁的写锁，如 `src/PrecisionHandler.cpp:1647-1656`、`:1755-1760`。当回调在同一执行链中调用这些注册/移除 API 时，形成读锁到写锁的重入问题。

建议注册变更在回调外执行；提供方可另行设计回调快照及注销生命周期协议，不能简单解锁后调用裸函数指针而忽略插件卸载和注销同步。

### 回调集合的无锁 size 查询

`src/PendingHit.cpp:50,218` 与 `src/Havok/ContactListener.cpp:209,237` 在未持 callbacksLock 时查询回调 map 的 `.size()`。若其他线程同时通过 API 注册或移除回调，读取与容器结构修改缺少同步。仅在启动时注册、之后不变的使用方式不会触发该条件。

可直接调用内部已有锁的 Run 方法并允许空集合，或在同一锁下判断；避免无锁读取容器结构。

### Clone 返回值是借用指针

`src/ModAPI.cpp:208-216` 导出的两个 `GetOriginalFromClone` 最终在 `src/PrecisionHandler.cpp:1968-1992` 查找并返回原始 node/body 指针。读锁只保护查找过程，不覆盖调用方之后的使用；actor 缓存移除和骨架更换可能影响其存活。

`src/ActiveActor.cpp:165-168` 的 rigid-body 重载还会直接解引用输入 `a_clone`，调用方必须传入有效非空对象。空值和已释放对象都不应交给该 API。建议提供方明确输入前置条件、返回值借用期限及允许调用的线程；若接口希望支持空输入，应增加空值返回路径。

没有复现游戏中的并发移除，也未把所有返回裸指针的接口一概判定为缺陷。当前 Mod 不调用 clone 查询，PreHit 注册代码也未启用。
