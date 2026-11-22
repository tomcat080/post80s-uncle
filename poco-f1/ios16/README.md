# POCO F1（beryllium）刷 iOS 16.7.16 — 折腾档案

- 设备：Xiaomi POCO F1（beryllium / SDM845 / 1080×2246），fastboot serial `2b8c9438`（另一台 `ad75c25f` 不属于本批，全程未触碰）
- 移植谱系：`ry-diffusion/Bergamota` main —— iOS 16.7.16 用户空间（iPhone X d22ap，未改动）+ xnu-8796.141.3 开发内核，**纯 CPU 渲染，无 GPU 驱动**
- 状态：真机已进过锁屏与桌面，USB 网络 + SSH 可完全操作；蓝牙用户态 `bluetoothd` 未接管 HCI（会 SIGBUS 崩溃，其 `BlueTool -R` 不走我们的 kctl）；自编 `btscan` 已能真实扫到周边 BLE 设备；内核重建通道已打通
- 记录时间：2026-11-22（覆盖 2026-11-14 → 11-22 全过程）

> 铁律：**UI 只认真机屏幕**。同一镜像在别处（QEMU / 截图 / 别人机器）表现如何都不作数。

## 目录

```
poco-f1/ios16/
├── README.md                     ← 本页：状态、时间线、结论
├── docs/
│   ├── 00-20261119-备份清单.md      ← 67GiB 冷备清单与 SHA 核对结论
│   ├── 01-环境与工具链.md            ← Mac / 服务器 / 设备工具链现状
│   ├── 02-启动链-shim与USB.md        ← boot.img 结构、shim、payload、USB 上电修复
│   ├── 03-蓝牙与AKS.md              ← WCN3990 硬件链、bluetoothd 死亡链、AKS 修复
│   ├── 04-内核重建通道.md            ← GitHub Actions 编 xnu、产物下载、重打 boot.img
│   └── 05-复现步骤.md               ← 从零到进桌面 + 抓日志的完整操作序列
├── tutorial.html                    ← 自成一体的一页教程（可内联进站点）
├── patches/0142-sweet-aks-wrapkey.patch   ← 本轮新写的 AKS WrapKey/UnwrapKey 补丁
├── workflows/xnu-macos15.yml        ← 能用的 CI 工作流（macos-15）
├── scripts/mk_logo_apple.py         ← 开机 logo 生成脚本
├── images/                          ← 开机首屏、logo slot 预览
└── logs/                            ← 关键日志摘录
```

二进制镜像（boot.img / kernel / payload）走 Releases，不进仓库：
[`poco-f1-ios16-boot-v1`](https://github.com/tomcat080/post80s-uncle/releases/tag/poco-f1-ios16-boot-v1)

## 状态

| 能力 | 状态 | 说明 |
|---|---|---|
| 进锁屏 / 桌面 | ✅ | 真机屏幕确认，纯 CPU 渲染 |
| USB 网络（ECM） | ✅ | Mac 侧 `en13` ↔ 设备 `192.168.7.2` |
| SSH 登录 + 复合命令 | ✅ | dropbear + 自签 toybox；**scp 子系统不可用**，传文件走 `ssh 'toybox cat > …'` |
| Shell 可用性 | ✅ | toybox 无 `sh` applet，已用自编 `minish` V3 覆盖 `/usr/local/bin/sh`（支持 `$(...)`、glob、fd 重定向、16KiB 行缓冲） |
| 内核日志 | ✅ | 内核把 console 以 UDP 发到 `192.168.7.1:5140`，Mac 侧 python 收包即可 |
| 蓝牙硬件链 | ✅ | WCN3990 上电、`crbtfw21.tlv`/`crnv21.bin` 下载、3 Mbaud、BD_ADDR 改写全部成功；`hcitest` 实测 HCI Reset/Read Local Version/Read BD_ADDR 全 `status 0` |
| 蓝牙用户态 | ⚠️ | `bluetoothd` 已不再秒退、会拉起 `BlueTool`，但自己约 53 秒一次 SIGBUS（`EXC_BAD_ACCESS` @ dyld private memory+8）；它派生的 `BlueTool -R` 常驻却从不打开 kctl —— 它的传输不是我们这条通道 |
| BLE 扫描（自编 `btscan`） | ✅ | LE Set Scan Params / Enable 全 `status 0`；12 秒收到 **203 条广播 / 6 台设备**，含设备名 `Galaxy Fit2 (0E14)`，RSSI −41…−82 dBm |
| Wi-Fi | ⏸ | `sweetwlan=1` 探针跑过；P3（MSS 内存池 + hyp-assign + MBA）未开工，风险 high |
| 音频 | ❌ | 无驱动；`VirtualAudio` 已移入 `HAL.disabled` 绕过崩溃 |
| 内核重建 | ✅ | 自建 `tomcat080/ios-xnu` + Actions，**约 10 分钟出内核**；补丁集与在跑内核同源 |

## 时间线（2026-11-14 → 11-22）

| 日期 | 进展 | 结论 |
|---|---|---|
| 11-14 | 分区 dump、GPT 备份、BL/EDL 资料收集 | 18/18 分区 `sha256sum -c` 通过 |
| 11-17 | root.simg 写入 userdata；首版 boot.img 尝试 | 需要 shim 才能让 xnu 在 ABL 之后接管 |
| 11-18 | 双布局 IOMFB 补丁（0160）黑屏排查 | `0x514` vs iOS16 `0x46c` 错配 → 每帧 `kIOReturnBadArgument`；**从未进过 UI** |
| 11-19 | 换上游 main 谱系；USB 断链根因定位 | `GDSC usb30_prim off` + `clk master off`；shim 注入上电后 ECM 通 |
| 11-19 | `minish` V1→V3 | toybox 无 `sh`，自编 shell 逐步修 bug，19/19 + 29/29 回归全绿 |
| 11-20 | `shim-boot-bt1/bt2.img`，WCN3990 全链打通 | HCI 命令实测全 0，内核 kctl 注册成功 |
| 11-21 | `bluetoothd` 死亡链深挖 | 死在"打开传输之前"；AKS `sel 10` 返回 `kIOReturnUnsupported` |
| 11-22 | 内核重建通道打通；AKS WrapKey 补丁上机 | `sel 10` 由 `0xe00002c7` → `0x0`；`bluetoothd` 不再秒退 |

## 三条硬结论

1. **不要设 `BT_DEVICE_ADDRESS`。** iOS 16.7 的 `bluetoothd` 会 `sscanf` 该值再清零 → 直接 `No bluetooth on this device` 退出。传输用 `BT_HCI_TRANSPORT=H4BC:POSIX:<kctl>,<baud>`。
2. **不要 fork 错仓库。** `dipa-studio/ios-sweet` 是 sweet 线（补丁止于 `0081`），本机在用的是含 beryllium 专属补丁（`0104`–`0141`）的 135 个补丁谱系。构建前先比 `sweetbt`/`sweetwlan` 字符串条数，否则会得到"编译成功但设备全无"的假内核。
3. **AKS 是全局缺口，不只是蓝牙。** 未实现 `kAKSWrapKey` 时，每个启动周期有 132 次 `sel 10 -> 0xe00002c7`（chronod、CloudKeychainProxy、adprivacyd…全在列）。补上后这批错误消失。

## 下一步（按优先级）

1. 在 kctl `open` 路径打进程名日志，确认 `bluetoothd` / `BlueTool` 是否真的尝试打开传输、卡在 open 前还是 open 后；
2. 补 `kAKSLoadBag`（`sel 8`，当前仍 `0xe00002c7`）；
3. 放行 `bluetoothd` / `BlueTool` 的沙盒特权（`12000`、`1002`），观察是否越过当前等待点；
4. 蓝牙这条线收敛后转 Wi-Fi P3（第一次真正的 TZ 动作）与音频。
