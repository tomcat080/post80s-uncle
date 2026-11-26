# 80后大叔 · 折腾记录

> 浙江台州，爱折腾旧电子设备的 80 后。这里是我第一手折腾记录的**原始档案库**——只记录、不教学，踩过的坑比成功更值得留底。

- 🌐 **展示主页**：[zhengba-lab.pages.dev](https://zhengba-lab.pages.dev)
- 📖 **在线教程**：[POCO F1 刷 iOS 16.7.16](https://zhengba-lab.pages.dev/tutorial-poco-f1-ios16.html)（站点项目卡 `#p/poco-f1-ios16` 是同源内容）· [Moto G5S Plus 刷 Ubuntu Touch](https://zhengba-lab.pages.dev/tutorial-g5s-plus-ubuntutouch.html)
- 📁 **本仓库**：过程记录、补丁、脚本、小体积素材（镜像走 Releases）
- 🔗 **参考资料**：[司徒大哥的教学站](https://steward-fu.github.io/website/index.htm)

## 目录约定

按**设备**分一级目录，每个设备内部再按**折腾方向**分二级目录；二进制一律走 Releases，仓库里只留"清单 + 链接 + 还原步骤"。

```
<设备>/                     # 一级：一台机器一个目录
├── README.md               # 这台机器做过什么
└── <方向>/                  # 二级：一个方向一个目录（ios16、linux、edl、…）
    ├── README.md           # 状态 / 时间线 / 结论 / 下一步
    ├── docs/               # 分主题的详细记录
    ├── patches/            # 自己写的补丁
    ├── scripts/            # 自己写的脚本
    ├── workflows/          # CI 工作流
    ├── images/             # 这台机器专属的实拍图、截图
    └── logs/               # 关键日志节选

firmware/                   # 只放跨机型通用的原厂包 / 工具链
images/                     # 只放跨项目复用的素材
```

为什么这样分：

- **镜像不进仓库**：GitHub 单文件硬上限 100 MB、历史永久保留二进制；机型专属的 boot.img / 内核 / 根镜像一律发 Release，README 里挂链接。
- **机型内部按方向分**：同一台机器可能先刷 Linux 再刷 iOS，方向混在一个 README 里会越来越难读。
- **firmware 只收通用件**：单机型专属的东西放回该机型目录，避免 `firmware/` 变成大杂烩。
- **只有一个方向的设备不套空壳**：像 iRiver D8、小米路由 4C 这种只干过一件事的机器，长文就是它的记录，`README.md` 直接放在设备目录下，不额外造一层空的方向目录；等出现第二个方向再拆。


## 目录结构

```
.
├── iriver-d8/      # iRiver D8 固件恢复（单方向，长文即记录）
│   ├── README.md
│   └── images/
├── tapion/         # BQ SURO CARBON 工程机（tapion / Aquaris X3）
│   ├── README.md   # 身份 / BL / root / EDL 备份 / OTA 排查 总记录
│   └── gsi/        # Android 9 GSI 方向：配套文件（开机动画、Open Camera、相机自启脚本）
├── poco-f1/        # POCO F1：两个方向各自成目录
│   ├── README.md   # 设备总览 + 两方向索引
│   ├── ios16/      # iOS16 档案：docs / patches / workflows / scripts / images / logs
│   └── linux/      # Kupfer Linux (Phosh)：过程记录待补
├── xiaomi-4c/      # 小米路由 4C 刷 OpenWrt（单方向）
├── moto-g5s-plus/  # Moto G5S Plus (sanders) 刷 Ubuntu Touch（单方向）
│   ├── README.md
│   └── tutorial-g5s-plus-ubuntutouch.html
├── firmware/       # 跨机型通用原厂包 / 工具链
└── images/         # 跨项目通用素材
```

## 折腾清单

| 设备 | 项目 | 状态 | 记录 |
|---|---|---|---|
| BQ Aquaris X3（工程机 tapion / SURO CARBON） | 身份鉴定 / BL 解锁 / root / EDL 全量备份 / Android 9 GSI | 已 root，6.8GB 备份完成；GSI 已点亮（中文+联网，相机黑屏真因已定位为物理开关） | [详情](tapion/README.md) |
| iRiver D8 | 固件恢复 / NAND 重写 | 刷机包自制成功，启动链排查中 | [详情](iriver-d8/README.md) |
| POCO F1 | 刷 Kupfer Linux (Phosh) | 已刷入 | [详情](poco-f1/linux/README.md) |
| POCO F1 | 刷 iOS 16.7.16（xnu-8796，纯 CPU） | 真机进桌面 + SSH 可操作；蓝牙 HCI 三条命令 status 0；音频 PAS_INIT 在 TZ 挂死、WiFi 封存；无音频 stage 的 `aud_stable` 已发 Release | [详情](poco-f1/ios16/README.md) |
| 小米路由 4C | Breed + OpenWrt | 已完成 | 整理中 |
| Moto G5S Plus（sanders） | 刷 Ubuntu Touch 24.04（系统出自第三方移植 [EEMuk/ubuntu-touch-motorola-sanders](https://github.com/EEMuk/ubuntu-touch-motorola-sanders)，已署名）/ SSH 打通 / Waydroid 跑 Android 13 / WiFi 假断线根治 + 静态 IP | 运行中，静态 IP `172.16.12.36` | [详情](moto-g5s-plus/README.md) |
| TS240 | 本地 Ollama 模型流水线 | 在跑，已优化 13 轮 | 整理中 |
| DOOV MT6771 | 安卓老机改家庭 NAS | 已落地 | 整理中 |

## 各机型 Release（刷机包都在这）

一个机型一份 Release，标题按机型定格；机型目录里也各自挂了链接，从任一入口都能进。

| 机型 | 方向 | Release | 归档目录 |
|---|---|---|---|
| POCO F1（beryllium / SDM845） | iOS 16.7.16 启动镜像 + AKS 内核 | [poco-f1-ios16-boot-v1](https://github.com/tomcat080/post80s-uncle/releases/tag/poco-f1-ios16-boot-v1) | [poco-f1/ios16](poco-f1/ios16/README.md) |
| POCO F1（beryllium / SDM845） | iOS 16.7.16 音频/WiFi 封存后的稳定镜像 | [poco-f1-ios16-aud-stable-v1](https://github.com/tomcat080/post80s-uncle/releases/tag/poco-f1-ios16-aud-stable-v1) | [poco-f1/ios16/docs/09](poco-f1/ios16/docs/09-音频线-实验索引.md) |
| BQ Aquaris X3（tapion / SURO CARBON） | EDL 全分区备份 6.8 GB（3 卷） | [tapion-edl-v1](https://github.com/tomcat080/post80s-uncle/releases/tag/tapion-edl-v1) | [tapion](tapion/README.md) |
| BQ Aquaris X3（tapion / SURO CARBON） | Android 9 GSI 实验包（phh v123 + Magisk boot） | [tapion-gsi-v1](https://github.com/tomcat080/post80s-uncle/releases/tag/tapion-gsi-v1) | [tapion/gsi](tapion/gsi/README.md) |
| iRiver D8（Dicple / JZ4755） | 原厂恢复包 + 4 GB NAND dump | [d8-restore-v1](https://github.com/tomcat080/post80s-uncle/releases/tag/d8-restore-v1) | [iriver-d8](iriver-d8/README.md) |

> 小米路由 4C、POCO F1 的 Kupfer Linux 方向暂不放 Release：前者固件直接用 OpenWrt 官方镜像，后者镜像单独冷备在服务器。

## 镜像备份存放位置（可回滚）

- **BQ Aquaris X3（tapion / SURO CARBON）**：EDL 全分区备份 6.8GB（64 分区 + 双 GPT，排除 userdata，含 SHA256 清单）已冷备到服务器：
  `rock-tom@172.16.14.177:/home/rock-tom/backups/tapion-suro-carbon-20261117/`
  - **GitHub 副本**：[Release tapion-edl-v1](https://github.com/tomcat080/post80s-uncle/releases/tag/tapion-edl-v1)，3 个分卷 `tapion-edl-full.z01` / `.z02` / `.zip`（合计约 5.2 GiB）。
  - 分卷合并：macOS / Linux 下 `zip -s 0 tapion-edl-full.zip --out merged.zip && unzip -t merged.zip && unzip merged.zip`；Windows 用 7-Zip 直接打开末卷 `.zip` 即可自动识别分卷。
  - 回滚方法：EDL 9008 下 `edl w <分区名> <镜像> --memory=emmc --sectorsize=512`，`*_a`/`*_b` 双槽分区两槽都要写。
  - 原厂 boot 在 `edl-tool/dumps/boot_a.img`（SHA256 `db359a0b…3f57f`），当前 Magisk 补丁 boot 为 `partitions/boot_a.bin`（SHA256 `869b266b…3720`），双通道都在。
  - 本机 Mac 工作区 `Doubao/chats/2026-11-17/new-chat-1/` 也留有一份（同源）。
- **iRiver D8**：成品恢复包 + 整片 NAND dump 分卷走 [Release d8-restore-v1](https://github.com/tomcat080/post80s-uncle/releases/tag/d8-restore-v1)。
- **POCO F1 / iOS 16**：启动镜像（shim boot.img ×4）+ AKS 内核走 [Release poco-f1-ios16-boot-v1](https://github.com/tomcat080/post80s-uncle/releases/tag/poco-f1-ios16-boot-v1)，附 `SHA256SUMS.txt`。
  - 音频/WiFi 封存后另发 [aud_stable](https://github.com/tomcat080/post80s-uncle/releases/tag/poco-f1-ios16-aud-stable-v1)：`sweetwlan=0`，不启动音频 stage；sha256 `b7475097…`，含 `SHA256SUMS.txt`。
  - 12.1GB 根文件系统与 21.4GB raw 镜像**不进 GitHub**（单文件上限 2GB，且体积/带宽不划算），冷备在 `rock-tom@172.16.14.177:~/backups/beryllium-poco-f1-ios16-20261119/`（67GiB / 165 文件，SHA 逐件核对通过）；重建方法见 [ios16/docs/04](poco-f1/ios16/docs/04-内核重建通道.md)。

## 关于这些记录

- **不保证能用**：都是我个人在特定设备上的操作记录，照做有风险，变砖自负。
- **图片随时补**：`images/` 里的占位说明会随实拍图逐步替换；机型专属图在各机型目录。
- **刷机包**：跨机型的放 `firmware/`，机型专属的走 Releases，详情页挂下载链接。

---

*最后更新：2026-11-22*
