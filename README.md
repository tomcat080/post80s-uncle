# 80后大叔 · 折腾记录

> 浙江台州，爱折腾旧电子设备的 80 后。这里是我第一手折腾记录的**原始档案库**——只记录、不教学，踩过的坑比成功更值得留底。

- 🌐 **展示主页**：[zhengba-lab.pages.dev](https://zhengba-lab.pages.dev)（做成可点进去的详情页）
- 📁 **本仓库**：原始记录、图片、刷机包镜像、脚本
- 🔗 **参考资料**：[司徒大哥的教学站](https://steward-fu.github.io/website/index.htm)

## 目录结构

```
.
├── iriver-d8/      # iRiver D8 固件恢复（最完整的一篇）
│   ├── README.md   # 六步过程 + 踩坑
│   └── images/     # 实拍图、启动日志截图
├── tapion/         # BQ SURO CARBON 工程机（tapion / Aquaris X3）
│   └── README.md   # 身份鉴定 + BL 解锁 + root + EDL 全量备份记录
├── firmware/       # 自制刷机包、镜像（大文件走 Releases）
├── poco-f1/        # POCO F1 刷 Kupfer Linux
├── xiaomi-4c/      # 小米路由 4C 刷 OpenWrt
└── images/         # 通用图片素材
```

## 折腾清单

| 设备 | 项目 | 状态 | 记录 |
|---|---|---|---|
| BQ SURO CARBON (tapion) | 工程机身份鉴定 / BL 解锁 / root / EDL 全量备份 | 已 root，6.8GB 全分区备份完成 | [详情](tapion/README.md) |
| iRiver D8 | 固件恢复 / NAND 重写 | 刷机包自制成功，启动链排查中 | [详情](iriver-d8/README.md) |
| POCO F1 | 刷 Kupfer Linux (Phosh) | 已刷入 | 整理中 |
| 小米路由 4C | Breed + OpenWrt | 已完成 | 整理中 |
| TS240 | 本地 Ollama 模型流水线 | 在跑，已优化 13 轮 | 整理中 |
| DOOV MT6771 | 安卓老机改家庭 NAS | 已落地 | 整理中 |

## 镜像备份存放位置（可回滚）

- **tapion / SURO CARBON**：EDL 全分区备份 6.8GB（64 分区 + 双 GPT，排除 userdata，含 SHA256 清单）已冷备到服务器：
  `rock-tom@172.16.14.177:/home/rock-tom/backups/tapion-suro-carbon-20261117/`
  - **GitHub 副本**：[Release tapion-edl-v1](https://github.com/tomcat080/post80s-uncle/releases/tag/tapion-edl-v1)，4 个分卷 `tapion-edl-full.z01/.z02/.z03/.zip`（各 <2GB，合计 7.5GB）。
  - 分卷合并（4 卷放同一目录，末卷是 `.zip`）：macOS / Linux 下 `zip -s 0 tapion-edl-full.zip --out merged.zip && unzip -t merged.zip && unzip merged.zip`；Windows 用 7-Zip 直接打开末卷 `.zip` 即可自动识别分卷。
  - 回滚方法：EDL 9008 下 `edl w <分区名> <镜像> --memory=emmc --sectorsize=512`，`*_a`/`*_b` 双槽分区两槽都要写。
  - 原厂 boot 在 `edl-tool/dumps/boot_a.img`（SHA256 `db359a0b…3f57f`），当前 Magisk 补丁 boot 为 `partitions/boot_a.bin`（SHA256 `869b266b…3720`），双通道都在。
  - 本机 Mac 工作区 `Doubao/chats/2026-11-17/new-chat-1/` 也留有一份（同源）。
- **iRiver D8**：成品恢复包 + 整片 NAND dump 分卷走 [GitHub Releases](https://github.com/tomcat080/post80s-uncle/releases/tag/d8-restore-v1)。

## 关于这些记录

- **不保证能用**：都是我个人在特定设备上的操作记录，照做有风险，变砖自负。
- **图片随时补**：`images/` 目录里的占位说明会随实拍图逐步替换。
- **刷机包**：单独放 `firmware/` 或 GitHub Releases，详情页会挂下载链接。

---

*最后更新：2026-11-17*
