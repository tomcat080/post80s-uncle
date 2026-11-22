# 02 · 启动链：shim boot.img 与 USB 上电修复

## 分区与刷写入口

| 分区 | 块设备 | 大小 | 备注 |
|---|---|---|---|
| `boot` | `/dev/block/sde45` | `0x3FFC000`（约 64 MB） | shim 启动镜像落点 |
| `userdata` | `/dev/block/sda23` | 237,158,360 扇区 | iOS 根文件系统（sparse HFS+） |
| `logo` | — | `0x1FFC000` | 已刷自制白苹果首屏；**该分区至今没有 dump**，ABL 不支持 `fastboot fetch` |

冷启动、`fastboot boot`、TWRP 三者之间的状态残留很脏：从 TWRP 回 iOS 必须冷启动；`fastboot reboot` 会回落 fastboot 且 USB PID 变 `0xd00d`；`fastboot reboot normal` 直接报 unknown target。

## boot.img 结构（beryllium 风格，header v1）

```
header v1 (1648 B, page 4096)
kernel:  out/Image.gz-dtb      ← shim 二进制 gzip + 追加 DTB
ramdisk: payload.bin           ← SWEETXNU：xnu Mach-O + ADT + cmdline
second:  空
cmdline: console=null          ← 真正的内核参数在 payload 内部
```

重打包参数（缺一不可）：

```
mkbootimg --header_version 1 --kernel Image.gz-dtb --ramdisk payload.bin \
  --base 0 --kernel_offset 0x8000 --ramdisk_offset 0x1000000 \
  --tags_offset 0x100 --pagesize 4096 --os_version 16.0.0 --os_patch_level 2025-06 \
  --cmdline "console=null" -o shim-boot.img
```

## payload（SWEETXNU）布局

```
0x000  magic[8] = "SWEETXNU"
0x008  u32 version, u32 nsegs
0x010  u64 virt_base      kernel VA ↔ phys_base
0x018  u64 entry_va       LC_UNIXTHREAD pc
0x020  u64 adt_off, adt_size
0x030  u64 kernel_end_va  最高段结束（页对齐）
0x038  u64 phys_base      与 ADT 的 dram-base 一致
0x048  char cmdline[608]
0x2a8  seg 表
```

内核是带 `LC_DYLD_CHAINED_FIXUPS` 的 PIE，没有 iBoot 帮忙解链，`mkpayload.py` 在打包时就把每条链上的指针改写成未滑动目标地址，shim 只负责平铺复制。

## 当前内核参数（payload 内 cmdline）

```
debug=0xc serial=3 cpus=8 keepsyms=1 -noprogress rd=ufsroot0 unentitled_ios_sim_launch=1
ignition_level=1 wdt_test=no_user_checkin ufsrw=1 rootrw=1 launchd_unsecure_cache=1 wdt=-1
sweetfb_noconsole=1 sweetfb_hz=30 sweetfb_vsync=1 sweetfb_scale=1 sweetfb_flip=1
sweetusb=2 sweetbattery=1 sweetsample=-
sweetwlan=2 sweetbt=1 sweetbtsk=1 sweetbtlog=200
```

BT 相关开关语义：

| 参数 | 作用 |
|---|---|
| `sweetwlan=2` | WCN3990 上电 + 固件下载 + UART 3 Mbaud |
| `sweetbt=1` | 注册 `com.apple.uart.bluetooth` kctl（依赖 `sweetwlan=2`） |
| `sweetbtsk=1` | 额外注册 `com.apple.uart.sk.bluetooth` |
| `sweetbtlog=200` | 每 200 条 HCI 事件打一行统计 |
| `sweetbtaddr=1` / `sweetbtpoweroff=1` / `sweetbtfw` / `sweetbtbaud` / `sweetbtibs` / `sweetbtprobe` | 地址改写、下电、固件名、波特率、IBS、探测 |

## USB 断链的根因与修复

表面现象是 fastboot 能进系统但网络断、SSH 连不上。用 TWRP 持久日志逐条比对，根因是两处硬件状态没被拉起来：

```
GDSC usb30_prim off
clk master off
```

即在 ABL 交接之后，USB3 prim GDSC 与时钟主控都处于关闭状态，而 stock 内核是在自己的 USB PHY 初始化里补上的。修复放在 shim 里：在跳进 xnu 之前把这两处按 SoC 寄存器序列上电，然后再 `sweetusb=2` 交给内核的 ECM gadget。修完 ECM 才真的起来。

## 调试用镜像一览（都在 Releases）

| 镜像 | 用途 | SHA256 |
|---|---|---|
| `shim-boot-aks2.img` | **当前验证版**（AKS WrapKey 内核 + 全 BT 参数） | `fa6996f76611ba22a3aad3463dbe10410f2fb99b0e151ce904425ab35c366052` |
| `shim-boot-bt2.img` | 蓝牙链已验证版，AKS 修复前，**回滚首选** | `d17959fa646f90213d158c916d39edf2b7ce709cf3145bdf6ed59f04cbca3ac9` |
| `shim-boot-bt1.img` | 只带 `sweetwlan=2 sweetbt=1` 的早期版 | `52085fe0152d0fc7d6ce76b5bfd2aa761348ae0cdbed935873feb2d04e993d0d` |
| `shim-boot-usbpow2.img` | 不带 BT，panic / 显示异常时的最小回滚 | `fec9e9731a02da9f63d4eb7877e77f83b18e6aa88a029500bd9de556a8a7f74b` |

注意：这些镜像目前只在 `fastboot boot` 临时生效，**boot 分区里仍是旧的 `shim-boot.img`（不带 BT 参数）**，尚未固化。
