# POCO F1（beryllium / SDM845）

同一台机器上先后折腾的两个方向，一个方向一个目录。

| 方向 | 内容 | 状态 | 记录 |
|---|---|---|---|
| iOS 16.7.16 移植（主线） | iOS 16 用户空间 + 自编 xnu-8796 内核，纯 CPU 渲染 | 真机进桌面 + SSH 可操作；蓝牙用户态推进中；内核重建通道已打通 | [ios16/](ios16/README.md) |
| Kupfer Linux (Phosh) | 把机器当 Linux 手机用，USB NCM 联网 | 已刷入、可起图形界面 | [linux/](linux/README.md) |

## 两个方向共用的基础

- 解锁 BL、TWRP 均已完成
- 分区：`boot` = `/dev/block/sde45`，`userdata` = `/dev/block/sda23`（237,158,360 扇区）
- 设备序列号 `2b8c9438`；另一台 `ad75c25f` 不参与折腾，操作一律 `-s 2b8c9438`

## 刷机产物

镜像不进仓库：boot.img ×4 + 内核包走 [Release poco-f1-ios16-boot-v1](../../releases/tag/poco-f1-ios16-boot-v1)，附 `SHA256SUMS.txt`。
12.1 GB 根文件系统与 21.4 GB raw 镜像冷备在服务器，重建方法见 [ios16/docs/04](ios16/docs/04-内核重建通道.md)。
