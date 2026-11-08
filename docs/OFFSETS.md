# 虚表与代码偏移核查

## 证据范围

使用本机真实的 Steam 1.6.1170 和 SE 1.5.97 可执行文件、CommonLib 的 Address Library 测试数据库，以及本次实际编译的 `include/RE/Offsets_VTABLE.h`。原 exe 的 SteamStub 加密代码不能直接反汇编，因此只对 `build/audit` 中的副本用 Steamless 解包，再读取 PE 虚表指针、反汇编对应函数和调用目标。游戏目录原文件未被修改，游戏进程未启动。

原文件 SHA-256：

| 文件 | SHA-256 |
| --- | --- |
| SkyrimSE.exe 1.6.1170 | `c434208894f07f604b852f29b8edc3a58c4de63de783373733e72b2b73f33be9` |
| SkyrimSE.exe 1.5.97 | `693e5a51ea2680119a68620bcf5080e81745549872b5d06bbd3f51131b67abab` |
| skse64_1_6_1170.dll 2.2.6 | `c9a2c8a80df6bf2372c5f49468bb2e5ab67786157265b6f29ece9f4eac075d54` |

以下地址均为 RVA，即模块基址相对地址；槽位从 0 开始，x64 每项 8 字节。

## 六处虚表 hook

| hook | CommonLib 虚表 | 槽位 | SE / AE Address Library ID | SE 目标 RVA | 1170 目标 RVA |
| --- | --- | --- | --- | --- | --- |
| Character Update | Character[0] | `0xAD` | 261397 / 207886 | `0x5D6FB0` | `0x667D40` |
| Player Update | PlayerCharacter[0] | `0xAD` | 261916 / 208040 | `0x69E580` | `0x732660` |
| Character NotifyAnimationGraph | Character[3] | `0x01` | 261400 / 207892 | `0x60F240` | `0x6A35F0` |
| Player NotifyAnimationGraph | PlayerCharacter[3] | `0x01` | 261919 / 208046 | `0x60F240` | `0x6A35F0` |
| Character animation ProcessEvent | Character[2] | `0x01` | 261399 / 207890 | `0x60B470` | `0x69F450` |
| JumpHandler CanProcess | JumpHandler[0] | `0x01` | 263065 / 208731 | `0x70A670` | `0x7A08A0` |

结论：原槽位正确，1170 无需加减槽位。动画事件 sink 改为使用 `RE::VTABLE_Character[2]`，替代手写 ID。NotifyAnimationGraph 的原函数指针改为显式接收 `IAnimationGraphManagerHolder*` 的函数类型，匹配次级虚表传入的 this 指针。1170 的 Notify 入口 `0x6A35F0` 为跳转 thunk，其目标为 `0x54C450`。

## 三处代码 hook / patch

| 位置 | SE ID + 偏移 | SE 写入 RVA | 1170 ID + 偏移 | 1170 写入 RVA |
| --- | --- | --- | --- | --- |
| NPC 落地 Process 调用 | 36507 + `0x89D` | `0x5E511D` | 37507 + `0x9BB` | `0x67725B` |
| Player 落地 Process 调用 | 41271 + `0x241` | `0x705771` | 42350 + `0x179` | `0x79B6F9` |
| 硬直状态条件跳转 | 36700 + `0x55` | `0x5FA205` | 37710 + `0x54` | `0x68D124` |

NPC 函数通过 Actor 主虚表槽 `0xC8` 定位，再对比两版落地状态分支；玩家函数也通过该控制流的调用关系核对。两个落地 hook 均对应原有 ActionLand 分支，调用目标为 `TESActionData::Process`：SE ID 40551 / RVA `0x6E6120`，1170 ID 41557 / RVA `0x77AD30`。1170 同函数中的其他 Process 调用（NPC `+0xA70`、Player `+0x22E`）属于另一条分支，不是本插件应替换的位置。

| 指令 | SE 原字节 | 1170 原字节 |
| --- | --- | --- |
| NPC call rel32 | `E8 FE 0F 10 00` | `E8 D0 3A 10 00` |
| Player call rel32 | `E8 AA 09 FE FF` | `E8 32 F6 FD FF` |
| jne rel32 | `0F 85 D6 01 00 00` | `0F 85 D1 01 00 00` |

条件跳转前两版都有 `C1 E8 0D A8 01`，即取 actor 状态位后测试；NOP 六字节保留原插件绕过该硬直检查的意图。运行时在任何 hook 写入前验证该分支原字节，并验证两条 E8 的相对位移目标，避免把不匹配的代码当成已核查版本修改。虚表 hook 继续允许正常的其他插件链式 hook，没有在运行时强制要求虚表指针仍等于原 exe。

## 重复核查

`tools/audit_offsets.py` 从当前 CommonLib 头文件提取虚表 ID，逐项断言 PE 中的槽位目标、call 目标、分支与前置状态测试字节。每个运行时九项，共 18 项通过；记录保存在 [1597.json](audit/1597.json) 和 [1170.json](audit/1170.json)。

```powershell
python -m pip install -r tools/requirements.txt
python tools/audit_offsets.py --exe build/audit/SkyrimSE-1170.exe.unpacked.exe --library ../CommonLibSSE-NG-ng/tests/REL/versionlib-1-6-1170-0.bin --commonlib ../CommonLibSSE-NG-ng --runtime 1170 --output build/audit/1170.json
python tools/audit_offsets.py --exe build/audit/SkyrimSE-1597.exe.unpacked.exe --library ../CommonLibSSE-NG-ng/tests/REL/version-1-5-97-0.bin --commonlib ../CommonLibSSE-NG-ng --runtime 1597 --output build/audit/1597.json
```

工具需要用户自己持有的、相同版本的解包 exe 副本；项目与分发包均不包含游戏二进制。此处结论属于离线地址与 ABI 核查，游戏内加载、hook 协作和实际动画效果仍待实测。
