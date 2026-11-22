# POCO F1（beryllium / SDM845）

同一台机器上先后折腾的两个项目。

## 1. iOS 16.7.16 移植（当前主线）

在 POCO F1 上跑 iOS 16.7.16 用户空间 + xnu-8796 内核，纯 CPU 渲染，无 GPU 驱动。
真机已进过锁屏与桌面，USB 网络 + SSH 可完全操作；内核重建通道（GitHub Actions 编 xnu）已打通。

📁 **[ios16/](ios16/README.md)** — 状态、时间线、四条硬结论、补丁、脚本与复现步骤

## 2. Kupfer Linux (Phosh)

- 设备：POCO F1（骁龙 845 / SDM845）
- 状态：已解锁 BL、刷过 TWRP，已刷入 Kupfer Linux Phosh
- 要点：USB NCM 网卡联网、天马屏适配

### 过程记录

整理中。折腾完的细节和图片发我，按 D8 那篇的格式补上。

资料备份：Google Drive 上的镜像走服务器直连 rsync 迁移（44MB/s），不经 Mac 中转。
