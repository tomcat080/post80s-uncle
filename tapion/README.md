# tapion / SURO CARBON（BQ Aquaris X3）折腾记录表

> 记录时间：2026-11-17，GSI 折腾更新 2026-11-22　设备 adb serial：`ad75c25f`　定位：Wi-Fi 玩具 / 收藏机
> 安全边界：仅做只读备份、root 与本机分析；**不提供写 IMEI / 改号 / 恢复订阅的任何操作指引**。

## 一、结论速览（TL;DR）

- 这台机器是 Mundo Reader SL（BQ）为加密通讯客户 **Suro** 定制的工程/小批量机型，代号 `tapion`，对外名 BQ Aquaris X3，机身标 **SURO CARBON**；从未正式零售。
- 系统版本 `1.0.0.6_20191129-1057`（Android 9，2019-11-29 构建）即终点版本；**官方 OTA 后端已随域名出售下线，无官方升级包可拿**。
- Bootloader 已解锁（orange），EDL 9008 可枚举、通用 SDM660 firehose 可读写；**已通过 Magisk 29.0 获取 root（双槽补丁镜像均已刷入）**。
- 公开渠道**不存在** tapion 原厂固件 / boot.img / TWRP / Lineage 适配；网上固件站（gofirmware、firmwareoficial 等）均为 SEO 垃圾页，无真实该机包。
- 基带 NV 已初始化但**未写号（IMEI 全 0）**；不具备正常通话条件，Wi-Fi / 蓝牙不受影响。
- **2026-11-22 更新**：Android 9 同代 GSI（phh v123 vanilla）已在 **b 槽完整点亮**，中文 / ADB / Wi-Fi 可用，重启持久化；相机因原厂 HAL 接口不兼容无法出图（详见第十二节）。设备当前停在 b 槽 GSI。

## 二、设备身份表

| 项目 | 值 |
|---|---|
| adb / fastboot serial | `ad75c25f` |
| brand / manufacturer | bq / bq |
| model | Aquaris X3 |
| device / product | tapion / tapion_bq |
| board / platform | sdm660 / sdm660 |
| SoC | Qualcomm SDM660（eMMC 版本） |
| Android 版本 | 9（API 28） |
| display.id | `1.0.0.6_20191129-1057` |
| fingerprint | `bq/tapion_bq/tapion:9/PKQ1.190908.001/3440:user/release-keys` |
| incremental | 3440 |
| 构建时间 | Fri Nov 29 11:09:54 CET 2019（内核 11:57:26 CET），host bqbot6 / jenkins |
| 安全补丁级别 | 2019-11-01 |
| 内核 | `4.4.153-perf+ #1 SMP PREEMPT Fri Nov 29 11:57:26 CET 2019 aarch64`（gcc 4.9.x） |
| 基带版本 | `S.AT.3.1-00826-SDM660_GEN_PACK-1` |
| ABI | arm64-v8a（兼容 armeabi-v7a / armeabi） |
| Treble | `ro.treble.enabled=true` |
| 活动槽位 | `_a`（A/B 双槽） |
| 时区 / 语言 | Asia/Shanghai / en-US |
| 别名 / 认证 | SURO CARBON；FCC ID `2AN87CARBON`；哥伦比亚 SIUST 注册名 BQ Carbon / BQ Aquaris X3 |

### 来历背景（据 HTCMania 考证帖）

- Mundo Reader SL（BQ）自 2019-06 起为 Suro 定制该机型，合作期约至 2020-07。
- 固件为 Encrochat 式"安全 fork"，设计上抑制用户通过 fastboot / recovery / EDL 刷写。
- 2020-06 Europol 联合行动拆解 Encrochat 加密通讯平台；BQ 2020-10 关停 JSON 更新服务，Mundo Reader SL 2021-02 破产清算。
- 另一台同机型 eMMC 为 KMDH6001DA-B422；本机为 SanDisk DH6DAB（eMMC 厂商在不同批次间有差异）。
- 硬件：USB-C、后置三摄、水滴前摄、三档硬件开关（可切断射频 / 摄像头 / 麦克风）、3600mAh 电池。

## 三、解锁 / Root 状态

| 项目 | 值 |
|---|---|
| flash.locked | 0（已解锁） |
| verifiedbootstate | orange |
| secureboot | 0 |
| veritymode | enforcing |
| ro.debuggable | 0 |
| ro.secure | 1 |
| 构建类型 | user / release-keys |
| Root 方案 | Magisk 29.0（29000），离线修补 boot 镜像后回刷 |
| magiskd | 以 root（uid=0）运行，PID 见本机进程表 |
| su | `/sbin/su -> ./magisk`；adb shell（com.android.shell）超级用户已授权 |
| Zygisk | 否（未启用） |
| Ramdisk | 是 |
| 复核命令结果 | `su -c id` → `uid=0(root) gid=0(root) context=u:r:magisk:s0`；`magisk -V` → 29000 |

## 四、本地备份清单（`edl-tool/dumps/`）

| 文件 | 大小 | 校验 / 备注 |
|---|---|---|
| `boot_a.img` | 67,108,864 B | 原厂 boot_a；SHA256 `db359a0ba23cced4715e5c573e8c8235f5da206a59c7556c3d60cde7a2c3f57f`；内核版本串与本机一致 |
| `boot_b.img` | 67,108,864 B | 与 boot_a 同哈希（双槽原厂镜像一致） |
| `magisk_patched.img` | 67,108,864 B | 当前已刷入的 Magisk 补丁镜像；SHA256 `869b266b321bbef65009cbd4c16d07653607d4e59ff46066719182d003ae3720` |
| `modemst1.img` | 2,097,152 B | NV 备份；md5 `757cdc608f0ed3bb1c56c4f25fafe82b`；几乎非零，无 15 位 IMEI 串 |
| `modemst2.img` | 2,097,152 B | NV 备份；md5 `86eb4b35d68ab150a80f6eeb77051957` |
| `persist.img` | 33,554,432 B | 98% 零字节 |
| `sec.img` | 16,384 B | 99% 零字节 |
| `storsec.img` | 131,072 B | 81% 零字节 |
| `gpt_tapion.txt` | 11,851 B | 完整 GPT 分区表（偏移 / 长度 / 标志 / UUID） |

### EDL 全分区离线备份（`dumps/partitions/`，2026-11-17 19:14）

- 方式：EDL 9008 下 `edl rl` 批量导出；合计 **6.8GB**，64 个分区镜像 + `gpt_main0.bin` / `gpt_backup0.bin`；**仅排除 `userdata`**（用户数据，不纳入刷机恢复备份）。
- 校验清单：`dumps/partitions/MANIFEST.sha256.txt`（每行 SHA256 + 字节数 + 文件名）。
- 主要大分区：`system_a.bin` / `system_b.bin` 各 2,684,354,560 B；`vendor_a.bin` / `vendor_b.bin` 各 536,870,912 B；`modem_a.bin` / `modem_b.bin` 各 115,343,360 B；`dsp_a/b.bin` 各 16,777,216 B；`mdtp_a/b.bin` 各 33,554,432 B；`persist.bin` 33,554,432 B；`logdump.bin` 67,108,864 B；`rawdump.bin` 134,217,728 B；`splash.bin` 34,226,176 B；`factory.bin` 33,554,432 B。
- boot 镜像对账：`partitions/boot_a.bin`、`partitions/boot_b.bin` 哈希 = 当前 Magisk 补丁镜像 `869b266b…3720`；原厂 boot `db359a0b…3f57f` 仍保留在 `dumps/boot_a.img`、`dumps/boot_b.img`，回滚双通道齐备。
- 备份完成后 `edl reset` 复位，设备正常回系统；复核 `su -c id` → uid=0，`magisk -V` → 29000，root 完好。
- 恢复思路：EDL `edl w <分区> <镜像> --memory=emmc --sectorsize=512` 逐分区回写；双槽分区（`*_a` / `*_b`）需两槽都写。

### NV / 基带状态结论

- modemst1/2 已初始化（非空、md5 不同），但未检出 IMEI 写入痕迹；系统侧 IMEI 全 0。
- persist / sec / storsec 基本为空。
- 跨机型 QCN 不通用（NV 项与 modem 固件绑定）；本机无同机型 QCN 可恢复。**写号不在操作范围内。**

## 五、OTA 通道排查结论（后端已死）

### 从 BqOta.apk（`/system/priv-app/BqOta`，versionName 8.3.7-12d32a）解包出的端点

| 端点 | 用途 | 实测状态 |
|---|---|---|
| `https://updates-apis.bq.com/v1/updates/devices/latest_versions` | 检查最新版本 | 不可达（HTTPS 000 / exit 35） |
| `https://apis.bq.com/v1/installedapps` | 上报已装应用 | 不可达 |
| `http://api.filters.ota.k8s.bq.com/get` | 过滤服务 | 不可达 |
| `http://devices.bq.com` | 设备信息 | 不可达 |
| `http://notification-messages.bq-services.k8s.bq.com/get` | 通知消息 | 不可达 |
| `https://bq-ota.firebaseio.com` | Firebase 同步 | 404 |

- HTTP 80 跳转至 `https://dn.com/sale/updates-apis.bq.com`（域名出售停放页，403）；DNS 多指向 GCP 停放地址 `35.173.228.115`。
- Wayback Machine（web.archive.org）CDX 核查：上述 6 个 API/通知主机均无 200 快照；`bq.com` / `www.bq.com` 主域仅有 1998 年无关旧页，`aquaris.bq.com` 无记录。即归档站未留存 OTA 接口响应或配置快照。
- 上报参数：serial / serialNumber / version / versionCode / versionName / model / deviceVersion / lastUpdateVersionCode / latestUpdate / updateType / updateTypeId / androidAppVersions。
- 已 `am force-stop com.bqreaders.ota`，停掉后台轮询死域名进程。

### 版本对照

| 机型 | 分支 / 版本 | 备注 |
|---|---|---|
| tapion（本机） | `1.0.0.6_20191129-1057` / incr 3440 | 独立分支，从未零售，预发布构建即终点 |
| Aquaris X2 Pro（zangyapro）Pie | `2.3.0_20191011-1351` / incr 1044 | 零售机型 |
| Aquaris X2 Pro（zangyapro）Q | `3.1.5_20201014-0835` / incr 666 | 零售机型 |
| Aquaris X2（zangya）Q | `3.1.5_20201014-0834` / incr 664 | 零售机型 |

零售 OTA 库中无 tapion 设备记录。

## 六、EDL 9008 评估与工具链

### 枚举与握手

- `adb reboot edl` 可进入 9008：`VID 0x05c6 / PID 0x9008`（Qualcomm QUSB_BULK，480Mb/s）；secure XBL 未拦截 EDL 枚举与 firehose 握手。
- 自动匹配通用加载器，TargetName `MSM8996`，DateTime `Feb 4 2018`。
- **无 tapion 专属 firehose / 编程器 / rawprogram / patch**；仅有 SDM660 通用 firehose（X2 / X2 Pro / Vsmart Active 1 应急包内 `prog_emmc_firehose_Sdm660_ddr_Z.elf` 等）。XDA 作者警告被篡改的 secure 定制固件与应急包不兼容，但本机实测通用加载器可完成握手与读写。

### 存储识别（getstorageinfo）

| 项目 | 值 |
|---|---|
| 存储类型 | eMMC |
| prod_name | DH6DAB |
| manufacturer_id | 21（SanDisk） |
| serial_num | 2910175839 |
| fw_version | "2" |
| block / page size | 512 / 512 字节 |
| num_physical | 4 |
| total_blocks | 122,142,720（约 64GB） |

### Mac 工具链（`edl-tool/`）

- 仓库：bkerler/edl V3.62 + Loaders 子模块；venv 为 Python 3.12，仅装核心依赖（cryptography 因 Rust 编译失败，属可选诊断链，不影响主流程）；keystone-engine 缺失（可选）。
- 运行前需存在 `logs/` 目录。
- 客户端默认误判为 UFS / 4096 扇区，必须显式指定 eMMC / 512。

可用命令形式：

```bash
# 读 GPT
./venv/bin/edl printgpt --vid=05c6 --pid=9008 --memory=emmc --sectorsize=512
# 读分区
./venv/bin/edl r <分区名> <输出文件> --memory=emmc --sectorsize=512
# 复位重启（reset 不接受 memory/sectorsize 参数）
./venv/bin/edl reset --vid=05c6 --pid=9008
```

## 七、Root 实施记录（Magisk 29.0）

1. EDL 只读备份原厂 `boot_a.img` / `boot_b.img`（哈希一致，内核串核对无误）。
2. `adb reboot` 回系统；推送 `boot_a.img` 到 `/sdcard/Download/`。
3. 安装 `magisk-v29.0.apk`（arm64-v8a）；授予存储权限。
4. Magisk 安装页：保持"强制加密"勾选 → 方式选"选择并修补一个文件" → 选 `boot_a.img`。
5. 补丁日志：识别原厂 boot → 修补 ramdisk → 重打包；产物 `/sdcard/Download/magisk_patched-29000_Takr3.img`，拉回 Mac 存为 `dumps/magisk_patched.img`。
6. fastboot 双槽回刷：
   ```bash
   fastboot flash boot_a dumps/magisk_patched.img
   fastboot flash boot_b dumps/magisk_patched.img
   fastboot reboot
   ```
7. 启动后 magiskd 以 root 运行；Magisk 主页识别"当前 29.0 (29000)"；超级用户列表中开启 Shell（com.android.shell）授权。

### 回滚 / 恢复原厂启动

```bash
# fastboot 方式（优先）
fastboot flash boot_a dumps/boot_a.img
fastboot flash boot_b dumps/boot_b.img
fastboot reboot

# 或在 Magisk App 内：卸载 Magisk → 恢复原厂 boot
# EDL 兜底（设备进 9008 后）
./venv/bin/edl w boot_a dumps/boot_a.img --memory=emmc --sectorsize=512
./venv/bin/edl w boot_b dumps/boot_b.img --memory=emmc --sectorsize=512
./venv/bin/edl reset --vid=05c6 --pid=9008
```

## 八、系统内与安全 fork 相关的预装组件

> 已 root，可离线解包分析；改动前必须先备份对应分区。

| 组件 / 包 | 作用判断 |
|---|---|
| `BqOta` | OTA 轮询（后端已死，已强制停止） |
| `BqFirstBoot` | 首次开机 / 激活流程，重点排查对象 |
| `BqTestSuite` | 产线测试套件 |
| `bqtool` | 厂商工具 |
| `BqMicIsolation` | 麦克风隔离（对应硬件静音开关） |
| `BqAccScan` | BqAccScan 软键盘（已禁用） |
| `FaceLock` | 面部解锁 |
| `SecureElement` | 安全元件服务 |
| `remoteSimLockAuthentication` / `remotesimlockservice` | 远程 SIM 锁 / 鉴权服务，重点排查对象 |
| `com.qualcomm.qti.services.secureui` | 高通安全 UI |
| `BlockedNumberProvider` | 号码拦截 |
| 第三方应用 | 仅 `com.topjohnwu.magisk` |

## 九、后续可折腾方向 · 可行性矩阵

| # | 方向 | 可行性 | 依据 / 约束 | 主要风险 | 建议下一步 |
|---|---|---|---|---|---|
| 1 | 整机分区离线备份（system_a / vendor_a / modem_a / xbl / abl / dsp / bluetooth / vbmeta 等） | **已完成** | EDL `rl` 全分区导出至 `dumps/partitions/`，含 SHA256 清单；仅排除 userdata | 占用 Mac 磁盘；误写分区可变砖 | 已作为后续动系统 / 移植的兜底回滚点 |
| 2 | 离线分析 secure fork 激活 / 远程锁逻辑 | 中 | root 后可读 `/system`、`/vendor`、init rc、`BqFirstBoot`、远程锁服务与 sepolicy | 误删系统组件可能导致无法启动；改动前先备份 | 解包 system/vendor，搜激活校验、远程鉴权、账号绑定相关包名与服务，判断是本地锁还是依赖已死后端 |
| 3 | GSI 通用系统镜像试用（Android 9 同代） | **已实测：Android 9 GSI 点亮**；Android 14 跨代 bootloop | `ro.treble.enabled=true`；vendor 为 Android 9 blob，**必须选同代 GSI**。Android 9 / phh v123 vanilla 成功 | 相机 HAL 与 GSI 深层不兼容（见第十二节）；userdata 需格式化为明文 | 已刷 b 槽验证；退路是 `set_active a` + EDL 全备份 |
| 4 | LineageOS 等第三方 ROM 移植 | 低 | 无 tapion 适配、无内核源码、社区基数为零；可参照 zangyapro（X2 Pro，同 SDM660），但设备树 / 分区布局 / 调制解调器差异大 | 工作量极大，变砖风险高 | 暂不投入；除非方向 1、2 产出可复用的分区与 blob 清单 |
| 5 | TWRP / 自定义 recovery | 低 | A/B 设备且 ramdisk 位于 boot；无现成适配 | 同上 | 暂缓；若 GSI 方向走通再评估 |
| 6 | 官方 OTA / 升级 Android 10+ | 死路 | OTA 后端域名出售下线，厂商破产，机型从未零售 | — | 不再追踪官方更新 |
| 7 | 获取原厂 stock ROM | 死路 | 公开渠道不存在；固件聚合站为 SEO 垃圾页，无真实 tapion 包 | 刷入来源不明的包有变砖 / 植入风险 | 不下载、不使用任何第三方站提供的"tapion 固件" |
| 8 | IMEI 修复 / 写号 | 不做 | 安全边界；NV 项与 modem 固件绑定，无同机型 QCN，跨机型 QCN 不通用 | 违规 / 变砖 | 仅保留只读 NV 备份，不生成写号步骤 |
| 9 | 基带通话 / 短信功能 | 低 | 本机 IMEI 全 0、NV 未写号，不具备正常通话条件；Wi-Fi / 蓝牙不受影响 | — | 按 Wi-Fi 玩具机使用 |
| 10 | 内核 / 驱动逆向 | 低（收藏向） | 无内核源码，成本极高 | 投入产出比低 | 仅在方向 1 备份完成后按需分析内核配置与驱动节点 |
| 11 | 硬件收藏 / 拆机记录 | 可行 | 三档硬件开关、机身 SURO CARBON 编号、FCC 认证齐全 | — | 记录硬件编号、主板 / 排线 / 电池标识，作为藏品档案 |

### 关于"是否定死在 Android 9"

- **官方路径**：是的，定死在 Android 9，且没有任何官方后续版本。
- **非官方路径**：唯一还沾边的升级可能是 GSI（方向 3），受 vendor Android 9 blob 与无适配限制，预期多数外设不可用；第三方 ROM / 内核移植在可预见条件下不具备社区与源码基础。
- 现实定位：在保住已 root 当前系统的前提下，价值最高的动作是**方向 1（整机离线备份）**与**方向 2（离线拆解锁机逻辑）**。

## 十、市场 / 流通情况

- price.com.hk 检索到一台二手挂牌（`https://www.price.com.hk/product.php?p=579474`），本次从本机网络抓取失败；据描述该机流通量极低，几乎没有二手固件或配件流转。
- HTCMania 同机型求固件帖（2021-05）最终结论：无公开 stock 固件，二手持有者可选退货、报警或留作收藏。

## 十补、刷机社区与固件资源普查（2026-11-17）

| 社区 / 渠道 | 检索关键词 | 结果 |
|---|---|---|
| XDA Forums | tapion / Aquaris X3 / Suro Carbon | 无对应机型帖，无补丁、附件或 recovery |
| 4PDA（4pda.to / w3bsit3-dns） | aquaris x3、tapion、suro、прошивка | 无机型帖；命中均为无关帖子与用户设备列表 |
| LineageOS 官方设备列表 | bq 全系列 | 收录 E5s / M5 / U / U Plus / X / X Pro / X2(zangya) / X2 Pro(zangyapro) / X5 / X5 Plus；**无 tapion**；zangya、zangyapro 已停止维护 |
| Needrom | aquaris x3、suro carbon | 仅有 X2 / X2 Pro 的 EDL/QFIL 应急包，无 X3 |
| Hovatek | bq aquaris x3、tapion | 无对应固件帖 |
| HalabTech | BQ 固件目录 | 仅 Aquaris X / X2 系列，无 X3 |
| GitHub | tapion device tree / boot / dump | 仅 zangyapro 占位设备树（androidblobs/device_bq_zangyapro），无 tapion 仓库 |
| 固件聚合站（gofirmware / firmwareoficial / easyfirmwarex 等） | suro carbon、aquaris x3 | SEO 垃圾页，无真实安装包；**不要下载此类站点提供的"tapion 固件"** |
| Telegram / 泄露渠道 | suro carbon dump leak | 未检索到存档或转存 |
| 规格库（Heureka、cena-vykon、DeviceAtlas、Camera FV-5） | aquaris x3 | 仅有硬件参数档案，无软件资源 |
| FCC / device.report | 2AN87CARBON | 认证测试报告 + 一份 Aquaris X3 用户手册 PDF；SAR 报告内列出的是工程测试机 IMEI，**不可写入本机** |
| 二手拍卖（bigvalleyauction.ca） | suro carbon | 一台零件机拍卖（parts only），无任何附件 |

**普查结论**：公开刷机社区不存在 tapion 可复用的固件、boot、recovery、设备树或 dump；与 OTA 后端下线、EDL 无专属编程器的结论互相印证。跨机型可参照的最近平台仍是 zangyapro（X2 Pro，SDM660），但设备树、分区布局、调制解调器与传感器配置差异显著，不能直接套用。

### 规格库交叉核对到的硬件参数

6.3" IPS 1080×2340、SDM660、4GB RAM / 64GB eMMC、3600mAh、Android 9、后置三摄 + 水滴前摄、后置指纹、NFC、双卡混合卡槽、USB-C。

## 十补二、与 Aquaris X / X2 / X2 Pro 硬件对比

| 维度 | Aquaris X（bardock） | Aquaris X2（zangya） | Aquaris X2 Pro（zangyapro） | tapion / Aquaris X3（本机） |
|---|---|---|---|---|
| 发布时间 | 2017-06 | 2018-05 | 2018-05 | 2019-11 构建，未零售 |
| SoC | SD626（MSM8953 Pro），8×A53 2.2GHz，Adreno 506 | SDM636，Kryo260 4×1.8+4×1.6，Adreno 509 | SDM660，Kryo260 4×2.2+4×1.8，Adreno 512 | SDM660，Adreno 512（本机实读 board.platform=sdm660） |
| RAM | 3/4GB LPDDR3 | 3/4GB LPDDR3 | 4/6GB LPDDR3 | 4GB |
| 存储 | 32/64GB，microSD≤256 混合槽 | 32/64GB，microSD≤256 | 64/128GB，microSD≤256 | 64GB eMMC SanDisk，microSD≤256 混合槽 |
| 屏幕 | 5.2" 1080×1920 16:9 IPS LTPS，423ppi | 5.65" 2160×1080 18:9 IPS LTPS，427ppi | 5.65" 2160×1080 2:1 IPS LTPS，427ppi | 6.3" 1080×2340 19.5:9 IPS，水滴前摄 |
| 后置相机 | 单摄 16MP f/2.0，双色调 LED | 12MP + 5MP 景深 | 12MP + 5MP 景深 | 12MP + 5MP + 8MP 三摄 |
| 前置相机 | 8MP f/2.0 | 8MP | 8MP | 13MP |
| 电池 | 3100mAh 不可拆 | 3100mAh | 3100mAh，QC4 18W | 3600mAh |
| 蓝牙 | 4.2 A2DP aptX | 5.0 aptX | 5.0 aptX | 系统属性 / 手册未标注版本 |
| Wi-Fi | a/b/g/n/ac 双频 | b/g/n/ac | b/g/n/ac | 2.4 / 5.5 / 5.8GHz 双频 |
| 3.5mm 耳机孔 | 有 | 有 | 有 | 无（内核无 h2w 开关节点，手册产品概览未列；未拆机复核） |
| USB | Type-C 2.0 OTG | Type-C 2.0 OTG | Type-C 2.0 OTG，支持 DP 输出 | Type-C 2.0 OTG |
| FM 收音机 | 有 | 有 | 有 | 手册未列 |
| NFC / 指纹 | 有 / 后置 | 有 / 后置 | 有 / 后置 | 有 / 后置 |
| 尺寸 / 重量 | 146.5×72.7×7.9mm | 150.7×72.3×8.25mm | 150.7×72.3×8.35mm | 158.92×75.49×8.31mm，176g |
| 出厂系统 | Android 7.1（可升 8.1） | Android 8.1 Android One | Android 8.1 Android One | Android 9 API 28 |

**要点**：tapion 与 X2 Pro 同属 SDM660 / Adreno 512 平台，性能高于 X2（636）和 X（626）；但 RAM 仅 4GB，低于 X2 Pro 的 6GB 顶配。屏幕尺寸与比例最新（6.3" 水滴全面屏），影像规格最高（三摄 + 13MP 前摄），电池大 500mAh，机身也最大最重；相对老三台砍掉了 3.5mm 耳机孔。平台同源是 EDL 通用 firehose 能在本机握手的硬件基础。

## 十一、参考链接

- HTCMania 考证帖：https://www.htcmania.com/archive/index.php/t-1597147.html
- FCC ID 图库（2AN87CARBON）：https://fccid.io/2AN87CARBON/External-Photos/External-photos-4663416
- 哥伦比亚 SIUST 注册 PDF：http://www.siust.gov.co/siic-archivos/modelos/pdf/2020513083.pdf
- Europol 拆解 Encrochat 公告：https://www.europol.europa.eu/newsroom/news/dismantling-of-encrypted-network-sends-shockwaves-through-organised-crime-groups-across-europe
- XDA Aquaris X2 / X2 Pro EDL 应急包指南：https://forums.xda-developers.com/t/howto-aquaris-x2-x2-pro-emergency-package-qfil-edl-unbrick.4239267/
- bkerler/edl 仓库：https://github.com/bkerler/edl
- Loaders 子模块：https://github.com/bkerler/Loaders

## 十二、GSI 折腾记（2026-11-22）

> 目标是验证"同代 GSI 能否点亮 + 中文 + 联网 + root"。结论：**Android 9 GSI 成功，相机是硬件级死结**。

### 12.1 两次尝试对比

| 轮次 | GSI | 结果 | 卡点 |
|---|---|---|---|
| 1 | TrebleDroid Android 14（`system-td-arm64-vanilla`，2.06GB） | ❌ bootloop | 版本跨太大（vendor Android 9 / system Android 14），直接起不来；已切回 a 槽 |
| 2 | phh v123 `system-arm64-ab-vanilla-nosu.img.xz`（AOSP 9.0，451MB 压 / 1441MB 解） | ✅ 完整点亮 | 卡开机动画 → 查出是 `/data` FDE 加密格式不匹配 |

### 12.2 Android 9 GSI 刷入步骤（已验证）

```bash
# 1. 镜像本身就是 sparse（magic ed26ff3a），免 img2simg 转换，直接刷
xz -dk phh-v123-arm64-ab-vanilla.img.xz        # 解压 1441MB
adb reboot bootloader
fastboot flash system_b phh-v123-arm64-ab-vanilla.img
fastboot flash boot_b  tapion_boot_a_magisk.img  # 用 a 槽已 Magisk patch 的 boot（两槽均 64MB）
fastboot erase vbmeta_b                          # 跳过 AVB 校验（本机 vbmeta 无 magic）
fastboot set_active b
fastboot reboot
```

**关键坑 1：`/data` 加密格式。** 首次启动卡开机动画——adb 能进、native HAL 全活（modem/audio/sensor），但无 zygote/system_server。查 footer 有 `aes-xts` / `DKMK` = 原厂 **FDE 全盘加密**，而 phh GSI 的 `/vendor/etc/fstab.qcom` 里 /data 行是明文（`wait,check,resize,crashcheck,quota,reservedsize=128M`），挂不上 → vold 报 `Read-only file system` → framework 起不来。
- `fastboot erase userdata` **不够**（全 0 后 fs_mgr 不认），必须 **`fastboot format:ext4 userdata`**（mke2fs 写空 ext4）。格式化后约 30s 点亮。
- `metadata` 分区不存在（erase 报 Check device console），无妨——本机用 FDE 不用 metadata 加密。
- userdata 是 a/b 共享分区：格式化后切回 a 槽原厂会因 FDE fstab 重新加密并走首次设置向导（EDL 全备份可完全恢复）。

**关键坑 2：dump boot 分区。** 本机 `dd` 是精简版，不认 `bs=1M`，必须用 `bs=1048576`。ramdisk 是 gzip 压缩的，`strings` 搜不到 magisk 标记属正常；boot_a ≠ boot_b 即证明 patch 过。

### 12.3 验收结果（重启后持久化）

| 项 | 状态 | 证据 |
|---|---|---|
| 系统 | ✅ | `ro.build.fingerprint=...treble_arm64_bvN/phhgsi_arm64_ab:9/PQ3B.190801.002/191210:userdebug/test-keys`，槽 `_b`，`boot_completed=1` |
| 中文 | ✅ | `persist.sys.locale=zh-CN` 重启后保持；锁屏"没有 SIM 卡/已充满电/12月11日周三"全简体中文 |
| 联网 | ✅ | wlan0 `172.16.11.239/21`；Google generate_204 探测被墙会误报"无网络"，实际内网通 |
| root | ⚠️ 半到位 | boot 内 Magisk 29.0（29000），`adb root` 可用；但 GSI 预置的 2019 老 `/system/xbin/su` 与 Magisk `/sbin/su` 并存，普通 shell `su` 报 `request rejected (2000)`，待修 |
| 相机 | ❌ 死结 | 见 12.4 |

### 12.4 相机：修好服务自启，剩下 HAL 死结

**问题 1（已根治）：相机 provider 服务不启动。** vendor rc `/vendor/etc/init/android.hardware.camera.provider@2.4-service.rc` 里有 `disabled`，且只在 `on property:vold.decrypt=trigger_restart_framework` 时 start。data 改为明文后该属性永不触发 → 服务永不启动。修法：

```bash
adb root
adb shell 'magisk --unlock-blocks; mount -o rw,remount /vendor'
adb shell 'cp /vendor/etc/init/android.hardware.camera.provider@2.4-service.rc /vendor/etc/init/camera.rc.bak'
adb shell 'sed -i "/^    disabled$/d" /vendor/etc/init/android.hardware.camera.provider@2.4-service.rc'
adb reboot   # 验证 init.svc.vendor.camera-provider-2-4 = running
```

摄像头硬件本身完全正常：`dumpsys media.camera` 识别 4 摄像头 / 8 逻辑设备，`/dev/video0-6,32,33` 齐全。

**问题 2（app 层绕过）：AOSP Camera2 必崩。** 自带 `com.android.camera2`（2.0.002）报 `CameraDeviceInfo.isFacingFront()` NPE。改用 F-Droid 版 **Open Camera**（`net.sourceforge.opencamera` v1.56.2 / vercode 96），app 稳定、中文界面、快门/切换/图库 UI 齐全。

**问题 3（死结）：取景器黑屏。** Open Camera 能连上摄像头（Active Camera Clients 有它），但底层报：
```
processCaptureRequest: Function not implemented (-38)
Camera3-Device: Unable to submit capture request to HAL device: Function not implemented (-38)
sensor_write_i2c_setting_array: failed
mm_stream_streamon: Module StreamON failed: rc=-1
```
已排除：① 强制 Camera1 API（写 `preference_camera_api=camera1`，底层仍走 Camera3 桥接）；② SELinux permissive（无相机相关 avc denied，排除 policy）。结论：原厂高通 camera HAL 3.x 缺 phh GSI framework 调用的 capture request 接口实现（ENOSYS -38），是 vendor HAL 与 GSI 的深层 ABI 不匹配，配置层面修不了。可能的破局方向：换 GSI 变体（gapps 版 / 其他 phh v1xx）或找厂商专用 camera blob。

**附带踩坑**：用 `sed -i 's/.../...\n.../'` 改 Open Camera 的 shared_prefs 时，本机 sed 不认 `\n`，会把 XML 写成一行损坏文件，app 启动后重建配置、注入的值丢失。改用本地生成完整 XML 再 `adb push` + `chown 10075:10075` + `chmod 660`。

### 12.5 当前状态与退路

- 设备停在 **b 槽 GSI 桌面**，SELinux enforcing，相机服务自启修复保留，Open Camera 已装（取景器黑屏）。
- **退路完好**：EDL 64 分区全备份（see 第四节）+ a 槽原厂 system/boot 完好；`fastboot set_active a` 可随时切回。
- 大文件与镜像存放见仓库 Releases（`tapion-edl-v1`）；本轮 GSI 镜像 / boot dump 可视需要补充上传。

## 十三、待办与遗留

- [x] K2P Wi-Fi：已连接。SSID `K2P`，BSSID `82:8f:d4:79:c2:69`，5GHz（5220MHz），IP `172.16.11.36/21`，网关 `172.16.10.1`，DNS `114.114.114.114`，外网 ping 0% 丢包；网络代理 `172.16.14.227:12334`。
- [x] 整机分区离线备份（方向 1）：`dumps/partitions/`，64 分区 + 双 GPT，清单 `MANIFEST.sha256.txt`；仅排除 userdata。
- [x] GSI 实测（方向 3）：Android 9 / phh v123 vanilla 点亮，中文 / ADB / Wi-Fi 持久化；Android 14 跨代 bootloop。详见第十二节。
- [ ] 相机死结：换 GSI 变体（gapps 版或其他 phh v1xx）试相机，或找厂商专用 camera blob。
- [ ] Magisk 普通 shell `su` 授权：GSI 预置 2019 老 su 与 Magisk 29 冲突，`su -c id` 报 request rejected (2000)；adb root 可用，待修。
- [ ] secure fork 锁机逻辑离线分析（方向 2）。
- [ ] Magisk App 提示可升 30.6：建议保持 29.0；若升级需重新走"修补文件"流程并备份当前镜像。
