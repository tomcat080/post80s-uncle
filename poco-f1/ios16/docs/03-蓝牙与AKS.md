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

## 六、2026-11-22 现场回归（归档镜像实测）

用仓库 Release 里的 `shim-boot-aks2.img`（SHA256 `fa6996f7…c366052`）在本机重新跑了一遍，确认归档产物可复现：

| 阶段 | 实测 |
|---|---|
| `fastboot -s 2b8c9438 boot shim-boot-aks2.img` | 0.57 s 发完，正常起 |
| ECM 网卡 | 12 s 出现 `en13`（192.168.7.1），USB 枚举名 `Sweet iOS (USB Ethernet)` |
| SSH 22 端口 | 66 s 可用，`ping 192.168.7.2` 往返 1.1–3.4 ms |
| HCI 三条命令 | 全 `status 0`：`0x0c03`、`0x1001`（manufacturer `0x001d` / LMP `0x02be`）、`0x1009`（BD_ADDR `02:00:5e:77:b0:01`） |
| 本轮 AKS | `sel 10`（securityd 5×，`0x0`）、`sel 17` 74×、`sel 35` 29×，全部 `0x0`；`sel 8` 未出现在本轮日志 |

**新踩的坑（重要判据）**：这次第一次跑 `hcitest` 直接失败：

```
CTLIOCGINFO com.apple.uart.bluetooth: No such file or directory (is sweetbt=1 up? look for 'SWEET BT: transport up' in the kernel log)
```

原因不是内核参数没生效，而是 **WCN3990 的 BT 固件还在分段下载**——console 里 `SWEET WLAN: BT fw patch: segment N/729` 一路爬到 50% 才过半，本轮大约 **2 分钟**才出现 `SWEET BT: transport up`。`sweetbt=1` 只保证后面会注册 kctl，不保证"现在就绪"。

所以复现时判据按顺序来：先等 console 出现 `SWEET BT: transport up: UART 3000000 baud … BD_ADDR 02:00:5e:77:b0:01`，再去跑 `hcitest`；否则拿到的一定是 `CTLIOCGINFO … No such file or directory`。

本轮 console 里 `bluetoothd` 侧只见到 1 次 `exec BlueTool`，**没有**出现上一轮那两条 `SWEET Sandbox: bluetoothd privilege 12000/1002 denied`；设备上没有 `ps`，存活与否无法直接确认，仍需先做"kctl open 打进程名"那个补丁才能定论。

## 七、2026-11-22 深夜：bluetoothd 的真死因与 BLE 扫描突破

### 7.1 `bluetoothd` 不是"退出"，是 **SIGBUS 崩溃**

从设备上翻到的崩溃报告（`/var/mobile/Library/Logs/CrashReporter/bluetoothd-*.ips`）：

```
"exception": {"codes":"0x0000000000000005, 0x0000000100f7c008",
              "type":"EXC_BAD_ACCESS", "signal":"SIGBUS",
              "subtype":"UNKNOWN_0x5 at 0x0000000100f7c008"},
"termination": {"code":10, "indicator":"Bus error: 10"},
"procLaunch": "21:38:11.6846", "captureTime": "21:39:04.1544",   ← 活了 ~53 s
"consecutiveCrashCount": 3, "throttleTimeout": 10,
"vmRegionInfo": "0x100f7c008 is in 0x100f7c000-0x100fbc000 … dyld private memory"
```

即：**进程活得比想象久（约 53 秒），然后崩在 dyld 自己的私有内存页（+8 字节）上**，不是之前那种 0.2 秒 `rc=1` 的干净退出。launchd 按 `consecutiveCrashCount=3 / throttleTimeout=10` 每 10 秒重启它，于是形成"反复拉起 → 约 1 分钟崩一次"的循环。

注意报告里 `usedImages` 全空、`thread_get_state` 三个端口都 invalid —— 这个移植的崩溃采集器读不到尸体栈，所以拿不到 backtrace，只能靠地址区间判断。

### 7.2 它到底在干什么：`BlueTool -R`

设备上没有 `ps`/`lsof`/`dtrace`，但可以把 `BlueTool` 换成同名包装器（adhoc 签名的 arm64 小程序，记 argv 后 `execv` 真身），零成本看清调用：

```c
/* /usr/sbin/BlueTool 换成包装器后抓到的 */
pid=746 argc=2 [/usr/sbin/BlueTool] [-R]
pid=762 argc=2 [/usr/sbin/BlueTool] [-R]
pid=775 argc=2 [/usr/sbin/BlueTool] [-R]
```

- 参数就是 **`-R`**，96 秒内 3 次；换成包装器之前一轮 5 分钟里是 **83 次**（几乎每 0.6 秒一次）。
- 手动跑 `/usr/sbin/BlueTool.real -R`：**不退出、也不打开我们的 kctl**（`SWEET BT: HCI: … connects` 计数一直停在 1，而那 1 次是我们自己的 `hcitest`）。它是常驻模式，卡在某处等响应。
- 反过来验证：`echo help | /usr/sbin/BlueTool` 会打印 `Welcome to BlueTool... Rev 6.0` 后等 stdin —— 它是交互式 CLI，`-R` 是常驻子模式。

**结论**：`bluetoothd` 的初始化链是 `bluetoothd → BlueTool -R（常驻） → 由它去建传输`。而 `BlueTool -R` 走的不是我们注册的 `com.apple.uart.bluetooth`（大概率是 Skywalk / mach service 那条），所以"kctl 明明能用、bluetoothd 就是不用"不是日志错觉，是路径不同。

### 7.3 反面证据：我们自己扫到了真实 BLE 设备

既然 `bluetoothd` 这条路卡着，就退一步验证"传输到底能不能跑真实业务"。自编 `btscan`（源码在 [`scripts/btscan.c`](../scripts/btscan.c)），走同一个 kctl：

1. HCI Reset `0x0c03` → status 0
2. LE Set Scan Parameters `0x200b`（active / 60 ms interval / 30 ms window / public addr / 不过滤）
3. LE Set Scan Enable `0x200c` → status 0
4. 读 12 秒 LE Meta 事件（`0x3e` / subevent `0x02`），解析地址、RSSI、AD 结构里的名字

结果（原始输出见 [`logs/20261122-ble-scan.txt`](../logs/20261122-ble-scan.txt)）：

```
unique devices: 6      total reports: 203
  10:39:17:37:0e:14  type0   -59 dBm  x86  "Galaxy Fit2 (0E14)"
  6f:cb:48:4b:44:00  type1   -41 dBm  x41
  05:ca:1f:a5:b6:23  type1   -41 dBm  x41
  7b:6d:3f:75:33:c5  type1   -41 dBm  x31
  e1:82:ab:fb:32:3c  type1   -42 dBm  x2
  63:61:19:5f:da:9e  type1   -82 dBm  x2
```

- BLE 从"HCI 命令能过"升级成**真实射频收包**：广播里带完整的 AD 结构，能在 Scan Response 里读出设备名。
- 踩到的坑：H4 事件布局是 `04 | evt | plen | payload`，subevent 在 `payload[0]`（`e[3]`）而不是 `e[2]`；一开始按 `e[2]` 解析，命令全 status 0 但打印 0 条，而内核侧 `SWEET BT: RX evt 0x3e` 早就在刷屏——说明**包到了、我们解析错了**。

### 7.4 下一步（已收窄）

1. `BlueTool -R` 到底在等谁：给 `bluetoothd`/`BlueTool` 加沙盒放行后看是否变化，或在 kctl `.sk.bluetooth` 的 open 路径打点，确认它有没有碰 Skywalk 那条；
2. `kAKSLoadBag`（`sel 8`）仍 `0xe00002c7`，`bluetoothd` 侧未见调用，优先级降低；
3. 若确认 `BlueTool -R` 只认 Skywalk：只有两条路 —— 把我们的 kctl 伪装成 Skywalk 的 mach service，或改 `bluetoothd` 的传输选择；两条都比现在的补丁重。


