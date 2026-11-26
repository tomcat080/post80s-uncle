# Moto G5S Plus（sanders）刷 Ubuntu Touch 与后续折腾全记录

> 设备代号 `sanders` · 序列号 `ZY323Z4KXG` · 骁龙 625 / 3GB+32GB
> 记录时间：2026-11-26 · 状态：Ubuntu Touch 24.04.5 LTS 稳定运行，SSH + Waydroid 可用

这是一台 2017 年的 Moto G5S Plus，从出厂安卓机一路折腾成局域网里一台能 SSH 进去、还能跑整套 Android 13 的 8 核小主机。本文按时间线记录它的"前世今生"、关键操作命令、踩过的坑与回滚方法。

---

## 一、这台机器的档案

| 项目 | 值 |
|---|---|
| 型号 / 代号 | Moto G5S Plus / `sanders` |
| 序列号 | `ZY323Z4KXG` |
| SoC | 高通骁龙 625，8× Cortex-A53 |
| 内存 / 存储 | 2.8GB RAM + 511MB swap / 29.1GB eMMC（系统 4G + `/home` 23.9G）|
| **出厂系统** | **Android 8.1**（`ro.product.first_api_level=27`）|
| 刷机前系统 | 被 OTA 抬到 **Android 11**（`ro.build.version.release=11`, `ro.vndk.version=30`）|
| **当前系统** | **UBports Ubuntu Touch 24.04.5 LTS**（noble / Halium 11 / 底层 lineage_halium_arm64 Android 11）|
| 内核 | `3.18.140-halium` aarch64 |
| 主机名 | `ubuntu-phablet` |
| SSH 用户 | `phablet`（sudo 密码可用；root 账户锁定）|
| 局域网 IP | `172.16.12.36/21`（**静态**，网关 `172.16.10.1`）|

**系统身份要点**：根分区 `/` 只读 3.7G（已用 2.8G），`/home` 与 `/userdata` 共 24G 可写。关键配置目录（`/etc/ssh`、`/etc/netplan`、`/etc/NetworkManager/system-connections`、`/etc/writable` 等）都是从 `/userdata`（`/dev/mmcblk0p54`）bind-mount 来的——**这就是为什么所有改动都能持久、且 OTA 升级不覆盖**。

---

## 二、来路：从出厂到 Ubuntu Touch

> ⚠️ **史料说明**：本机详细的折腾日志是从「系统已经刷好、开始配 SSH」那天（2026-11-04）记起的。**刷入 Ubuntu Touch 的安装动作本身没有留下逐步记录**，以下为可确证的来路脉络，安装步骤留待补充（见文末"待补"）。

1. **出厂 Android 8.1**（`first_api_level=27` 证实），一路 OTA 被抬到 Android 11。
2. 解锁 Bootloader → 通过 UBports Installer 刷入 **Ubuntu Touch 24.04.5 LTS**（noble 通道，Halium 11 底座）。
3. 系统落定后，网络与 SSH 默认都是关闭状态——接下来的折腾全部围绕"把它变成一台可远程操作的 Linux 小主机"展开。

**可查证的版本来源**：
- UBports 设备页：`https://devices.ubports.com/#/sanders`
- 系统 apt 源：`deb http://repo.ubports.com/ 24.04-1.x main`
- UBports devices 仓库（`sanders` 设备定义）：`https://github.com/ubports/devices`（本次检查未查到设备文件的近期提交，详见文末"GitHub 分享页核查"）

---

## 三、SSH 打通（2026-11-04）

### 为什么默认连不上

- `ssh.socket` / `ssh.service` 都是 disabled
- `/etc/ssh/sshd_config.d/50-lxc-android-config.conf` 写死 `PasswordAuthentication=no`
- UBports 另有救援单元 `usb-moded-ssh.service`，只监听 USB 网卡 `10.15.19.82:8022`，当前未启用

### 落地改动（三步，全部持久）

```bash
# 1) 写一个优先级更高的配置（00- 排序在 50- 之前，sshd 采用 first-value-wins）
sudo tee /etc/ssh/sshd_config.d/00-tom-local.conf <<'EOF'
PasswordAuthentication yes
KbdInteractiveAuthentication no
PermitRootLogin no
PubkeyAuthentication yes
EOF

# 2) 把 Mac 的公钥塞进 authorized_keys（Mac 侧：~/.ssh/id_ed25519.pub）
cat ~/.ssh/id_ed25519.pub | ssh phablet@<手机IP> 'mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys'

# 3) 启用 ssh.socket（开机自启）
sudo systemctl enable --now ssh.socket
```

### 三条实测通道

| 通道 | 命令 | 结果 |
|---|---|---|
| WiFi · 公钥免密 | `ssh phablet@<手机IP>` | ✅ |
| WiFi · 密码 | 密码 `667613` | ✅ |
| USB 转发 | `adb forward tcp:2222 tcp:22` 后 `ssh -p 2222 phablet@127.0.0.1` | ✅ |

会话内 `sudo` 提权 root 成功。开机自启在重启后复测通过：设备启动约 1~2 分钟较慢 + 重启后 `adbd` 未自动启用曾造成误判，实际 `ssh.socket` 已 enabled + active，无需人工干预。

### 回滚 SSH

```bash
sudo rm /etc/ssh/sshd_config.d/00-tom-local.conf
sudo systemctl disable --now ssh.socket
# 再删除 /home/phablet/.ssh/authorized_keys 里对应公钥
```

---

## 四、Waydroid：在 Ubuntu Touch 里跑 Android 13

### 好消息：地基全现成

- `waydroid 1.5.1-0ubports2` 系统自带，`waydroid-container.service` 在册
- 内核 `3.18.140-halium` 已带 `CONFIG_ANDROID_BINDER_DEVICES="binder,hwbinder,vndbinder,anbox-binder,anbox-hwbinder,anbox-vndbinder"`
- `/dev/anbox-binder`、`anbox-hwbinder`、`anbox-vndbinder` 开机已创建；ASHMEM / USER_NS / NAMESPACES / CGROUPS / OVERLAY_FS / SECCOMP 全为 y
- **没有 `/dev/kvm`** → QEMU/AVD/Genymotion 全虚拟化不可行；无 binderfs → Docker/redroid 不可行

### 镜像下载（走代理多线程）

系统镜像与 vendor 镜像托管在 SourceForge，直连仅 ~84KB/s，870MB 要 3 小时。按本机验证过的配方：**16 线代理并发**。

```bash
# 关键坑：必须先 HEAD 解析出带签名的最终地址，不能直接给 aria2 一个 /download 页面
URL=$(curl -sS -I -L -o /dev/null -w '%{url_effective}' "<sourceforge 页面>")
aria2c "$URL" --all-proxy=http://127.0.0.1:12334 -x16 -s16 -k1M --user-agent=Mozilla/5.0
```

- vendor：`lineage-18.1-20260402-HALIUM_11-waydroid_arm64-vendor.zip`（41MB / 17 秒）
- system：`lineage-20.0-20260927-VANILLA-waydroid_arm64-system.zip`（870MB / 121 秒，峰值 ~10MiB/s）
- 两个 zip 的 SHA256 与 OTA 清单完全一致，确认官方原包

### 部署

```bash
# 镜像放 /home（根分区只剩 686M，1.86GiB 的 system.img 放不下）
mkdir -p /home/phablet/waydroid-images/images
# 根分区临时 remount rw，建软链指向 /home，让 waydroid init 命中预置镜像、跳过 OTA
sudo mount -o remount,rw /
sudo ln -s /home/phablet/waydroid-images/images /etc/waydroid-extra/images
waydroid init
```

### 实测结果

`Session: RUNNING` / `Container: RUNNING`，**Android 13（LineageOS 20）**，`sys.boot_completed=1`，152 个包，`launcher3` / `system_server` / `surfaceflinger` / 双 `zygote` 全在跑。容器网桥 `192.168.240.x`。

**代价**：Waydroid 常驻约 484MB，总内存用 1.72G、可用 1.10G、swap 吃 191M、load 4.15。对 3GB 内存的机器算重的，但能跑。

---

## 五、WiFi 假断线根治 + 静态 IP（2026-11-26）

### 症状

熄屏一段时间后，WiFi 图标仍显示已连接，**实际数据流已断**。断够久 DHCP 租约失效 → 重连换 IP（本机就从 `172.16.9.29` 漂到 `172.16.12.36`）→ 按旧 IP SSH 全部超时。判断设备是否真在线，用 ARP 一眼定性：

```bash
arp -n <IP>     # at (incomplete) = 网络层根本没有这块网卡回应 = 设备离线
                # 能解析出 MAC 但 SSH 不通 = 设备在线、服务问题
```

### 根因

NetworkManager 全局省电：`/etc/NetworkManager/conf.d/default-wifi-powersave-on.conf` 写死 `wifi.powersave = 3`（启用）。该文件在只读根分区，直接改不了——但**可以在连接级覆盖**。

### 修复：静态 IP + 关省电，一次做完

```bash
# 配置都写在 /userdata bind-mount 的可写区，OTA 不覆盖
sudo nmcli connection modify "K2P" \
  ipv4.method manual \
  ipv4.addresses 172.16.12.36/21 \
  ipv4.gateway 172.16.10.1 \
  ipv4.dns "114.114.114.114" \
  wifi.powersave 2          # 2 = disable，覆盖全局的 3
sudo nmcli connection up "K2P"
```

**验证**：路由从 `proto dhcp` 变 `proto static`，`powersave=disable`，ping 0 丢包、SSH 重连成功。

**回滚**：`sudo nmcli connection modify "K2P" ipv4.method auto wifi.powersave 0 && sudo nmcli connection up "K2P"`（改前备份留在手机 `~/.k2p_backup_20261126.txt`）。

**代价提醒**：静态 IP 钉在设备上，若路由器 DHCP 日后把 `172.16.12.36` 分给别人会冲突。彻底稳妥的做法是在路由器做 DHCP 保留（本次按用户要求未动路由器）。

---

## 六、踩坑速查表

| 坑 | 现象 | 解法 |
|---|---|---|
| sshd 配置不生效 | 密码登录被拒 | `00-` 前缀配置文件，first-value-wins 覆盖 `50-` |
| 重启后以为系统没起来 | adb 无设备、ping 不通 | 实为启动慢 + adbd 未自启；`ssh.socket` 是好的，等 1~2 分钟 |
| Waydroid 镜像下载 84KB/s | 3 小时下载 | 16 线代理并发，121 秒完成 |
| SourceForge 403 | aria2 拿到 HTML | 先 `curl -I -L -w '%{url_effective}'` 解析带签名直链 |
| WiFi 假断线 | 图标亮着但断网、IP 漂移 | 连接级 `wifi.powersave 2` + 静态 IP |
| ARP incomplete | SSH 超时 | 设备整个离线，唤醒/查新 IP，别死磕 |
| 根分区只剩 686M | 大镜像放不下 | 镜像放 `/home`，用软链绕过 |

---

## 七、待补 / 后续

- **安装实录缺失**：从 Android 11 解锁 BL 到 UBports Installer 刷入 UT 24.04 的逐步命令与截图未留存，建议补记（解锁状态、刷入用的 ROM 包版本、Installer 选项）。
- **GitHub 分享页核查**：见下节，本次核查未发现设备定义的近期更新。
- Waydroid 触摸/图形界面、SadTalker 等是同一台设备的其他支线，不在本文范围。

---

## 附：GitHub 分享页核查（2026-11-26）

用户要求核查"原始 GitHub 分享页有没有更新"。核查结果：

- `https://devices.ubports.com/#/sanders` —— UBports 官方设备页仍在，为前端应用，标注 UT 24.04 系列。
- `https://github.com/ubports/devices` —— 查询 `sanders` 设备定义文件的近期提交，API 返回空/无匹配，未发现明确的最近更新记录。
- **不确定项**：用户记忆中的"GitHub 分享页"若指某个具体刷机教程仓库（而非 UBports 官方设备定义），本地史料未记录其 URL。若有链接可补查。

> 本记录由折腾日志整理而来，命令均以本机实测为准；涉及 sudo / 分区的操作请先备份。
