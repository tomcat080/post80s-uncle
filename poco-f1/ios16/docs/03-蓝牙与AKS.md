# 03 · 蓝牙（WCN3990）与 AKS

蓝牙和 Wi-Fi 共用同一颗 WCN3990，但蓝牙是唯一能在"一次重启"里看到结果的入口，所以先啃它。

## 一、硬件链：已经端到端打通

`shim-boot-bt2.img` 启动后，内核串口日志依次出现：

```
SWEET WLAN: BT probe: HCI Reset at 115200 baud
SWEET WLAN: HCI patch version ... 02 00 01 02 14 02 01 40
SWEET WLAN: BT fw NVM: 44 tags from offset 8, stopped at 3583
SWEET WLAN: HCI baud change event at the new rate: ff 02 92 01
SWEET WLAN: HCI Read Local Version (at new baud): manufacturer 0x001d, LMP subversion 0x02be
SWEET WLAN: HCI set BD_ADDR (0xfc14) reply: 0e 04 01 14 fc 00
SWEET BT: kernel control com.apple.uart.bluetooth registered: 0
SWEET BT: kernel control com.apple.uart.sk.bluetooth registered: 0
```

即：上电 → `crbtfw21.tlv` / `crnv21.bin` 下载 → 3 Mbaud → BD_ADDR 改写 → 两个 kctl 注册，全部成功。IORegistry 里 `bluetooth` 节点属性齐全（`transport-encoding=03`、`local-mac-address=02:00:5e:77:b0:01`、`transport-speed=3 Mbaud`）。

用自编 `hcitest` 从用户态直接验证（这是"内核 BT 栈真的能用"的决定性证据）：

```
connected to com.apple.uart.bluetooth (ctl id 12)
TIOCSETA 3000000 baud ok
TX 0c03 → 04 0e 04 01 03 0c 00   HCI Reset, status 0
TX 1001 → HCI 9, manufacturer 0x001d, LMP subversion 0x02be, status 0
TX 1009 → BD_ADDR 02:00:5e:77:b0:01, status 0
```

结论：**硬件与内核侧 100% 可用，问题全在用户态。**

## 二、`bluetoothd` 死亡链（修复前）

现象：手动运行 `/usr/sbin/bluetoothd`，0.2–0.3 秒退出，`rc=1`，stderr 只有 6 行：

```
com.apple.server.bluetooth: Bluetooth Super Server Robot Destroyer
could not find BT_DEVICE_ADDRESS env variable
marconi-bt: Unable to match bluetooth service in device tree.
marconi-bt: Not a valid MAC address (all 00s).
bluetooth: Got local-mac-address: 02:00:5e:77:b0:01
FindLocalMacAddress Deleting local address from plist
```

排查过程与否定证据：

| 假设 | 实验 | 结果 |
|---|---|---|
| 传输名字写错 | 把传输指向不存在的 `com.apple.uart.nonexistent` | 输出**一模一样** → 死在打开传输之前，Skywalk/`.sk.` 不是瓶颈 |
| 设备树判据不满足 | 反汇编 `main`（`__text` @0x10000693c，`main` 区间 0x1000268xx–0x100027700） | `0x100026ae4` 判地址非零 → 两次 `access()` → `marconi-bt` → `bluetooth`，命中即继续；唯一 `errx` 出口 0x100027050 未触发 → **它通过了设备检查** |
| 偏好子系统坏了 | `sweetprefs mobile com.apple.sweetprobe probe hello` 读回 | 正常（root 亦可） |
| plist 注入传输变量 | 写入 `BT_HCI_TRANSPORT=H4BC:POSIX:com.apple.uart.bluetooth,3000000` 后重启 | 行为不变，仍 6 行 rc=1 |
| 用 DYLD 钩 exit 定位 | `DYLD_INSERT_LIBRARIES=/usr/local/lib/exithook.dylib` | iOS **剥掉平台二进制的 `DYLD_*`**，钩子不生效（此路不通） |
| 环境变量直接给地址 | 设 `BT_DEVICE_ADDRESS` | 走另一条路，打印 `Local address from ENV` 后 `Not a valid MAC address (all 00s)` → `No bluetooth on this device.`（**绝对不能设**） |

真正的关联证据来自内核 console 的 `SWEET AKS` 日志：`bluetoothd` 依次 `sel 0`(Open) / `sel 7`(GetLockState) / `sel 17`(GetDeviceState)×3，紧接着

```
SWEET AKS: securityd sel 10 arg0 0 in 2/32 out 1/40 -> 0xe00002c7
```

`sel 10 = kAKSWrapKey`，而 `0038-sweet-applekeystore.patch` 的 switch 里没有实现它，落到 `default: kIOReturnUnsupported`（`0xe00002c7`）。这个错误在整个启动里出现 **132 次**，`chronod`、`CloudKeychainProxy`、`syncdefaultsd`、`adprivacyd` 全在列——说明这不是蓝牙专属问题，而是 **keychain 包裹功能全局缺失**。

## 三、修复：0142-sweet-aks-wrapkey.patch

做法：在 `iokit/Kernel/arm/SweetKeyStore.cpp` 里补三个函数，并在 switch 里加两个 case。

- `sweet_aks_wrap_key`：输出 8 字节头 `'S''W''K''1'` + klen 字节 + key 原文（总长 `len+8`），长度写回 `scalarOutput[0]`、`scalarOutputCount=1`；
- `sweet_aks_unwrap_key`：校验头并把 key 交回；
- 两者都沿用已有的 `sweet_aks_struct_out` 约定。

补丁 110 行，`cd /tmp/aks/a && patch -p1 --dry-run < …` 返回 `APPLY_OK`，已编进新内核（旧内核 `SWK1` 字符串计数 0，新内核 1）。

## 四、修复后的实测（2026-11-22）

同一台设备、同一套 cmdline，只换了内核：

| sel | 含义 | 次数 | 返回 |
|---|---|---|---|
| 0 | Open | 11 | 0x0 |
| 7 | GetLockState | 8 | 0x0 |
| **8** | **LoadBag** | 3 | **0xe00002c7（仍缺）** |
| **10** | **WrapKey** | 21 | **0x0** ✅ |
| 17 | GetDeviceState | 345 | 0x0 |
| 26 / 27 | AssertHold / AssertDrop | 5 | 0x0 |
| 35 | — | 46 | 0x0 |

`bluetoothd` 行为随之质变：

- 修复前：0.2 秒、6 行、`rc=1`；
- 修复后：**不再退出**（存活十几分钟），console 里能看到它 `exec xpcproxy` → **`exec BlueTool`（连续 5 次）**，真的开始推蓝牙初始化子流程；
- 但它**没有占用 HCI 传输**——在它运行期间跑 `hcitest` 仍能成功打开 `com.apple.uart.bluetooth` 并跑完 3 条命令（每次 open+close 会在 console 打一行 `SWEET BT: HCI: rx … tx … connects 1` 统计）。

新的卡点（console 原样）：

```
SWEET Sandbox: bluetoothd privilege 12000 denied
SWEET Sandbox: bluetoothd privilege 1002 denied
process vmd[229] thread 2154 caught burning CPU! (65% over ~137 s)
```

## 五、下一步

1. kctl `open` 路径加进程名/pid 日志 —— 不用再从统计行反推"谁开了传输"；
2. 补 `kAKSLoadBag`（`sel 8`）；
3. 放行 `bluetoothd` / `BlueTool` 的沙盒特权（`12000`、`1002`）；
4. 给 `vmd` 的 WrapKey 调用加计数，区分"真循环"和"正常高频"。

配套工具与文档：`Bergamota-main/docs/BT_IOS_TRANSPORT.md`（§1.3 是 bluetoothd 存在性判据原文，§8 明确禁止设 `BT_DEVICE_ADDRESS`）、`tools/sweetnet/{hcitest.c,bluetool-stub.c}`。
