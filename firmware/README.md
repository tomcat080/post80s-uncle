# 固件 / 通用刷机包

**这里只放跨机型通用的东西**：原厂完整包、通用 recovery、EDL/fastboot 工具链、驱动。
机型专属的启动镜像、内核、根文件系统**不进仓库**，一律走 Releases，在各机型 README 里挂链接。

## 为什么这么分

- GitHub 单文件硬上限 **100 MB**（超过直接拒收），> 50 MB 会警告；
- git 历史永久保留二进制，镜像进 git 会让 clone 越来越慢；
- Releases 单资产上限 **2 GB**、不占仓库历史、有 CDN 直链，适合放刷机包；
- 超过 2 GB 的整片 dump 用 `zip -s 1.8g` 分卷（tapion 就是这么做的）。

## 通用文件清单

| 文件 | 用途 | 状态 |
|---|---|---|
| （待上传）iRiver D8 自制刷机包 | iriver-d8 | 整理中，上传后在此挂 Releases 链接 |

> 来源声明：D8 刷机包基于司徒大哥（steward-fu）分享的固件包二次制作，致谢原作者。

## 机型专属镜像索引（文件都在各自 Release）

| 机型 | 内容 | 位置 |
|---|---|---|
| POCO F1 (beryllium) | iOS16 shim boot.img ×4 + AKS 内核 | [poco-f1-ios16-boot-v1](../../releases/tag/poco-f1-ios16-boot-v1) · 记录见 [poco-f1/ios16](../../poco-f1/ios16/README.md) |
| BQ SURO CARBON (tapion) | EDL 全分区 6.8 GB | [tapion-edl-v1](../../releases/tag/tapion-edl-v1) |
| iRiver D8 | 原厂恢复包 + NAND dump | [d8-restore-v1](../../releases/tag/d8-restore-v1) |
