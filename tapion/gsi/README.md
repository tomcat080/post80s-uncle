# tapion · GSI 折腾（Android 9 移植）

设备总览、身份鉴定、EDL 备份与 root 全过程见 [../README.md](../README.md)（第十二节为 GSI 全程记录）。本目录只放 GSI 这一方向的配套文件。

| 文件 | 用途 | 来源 / 说明 |
|---|---|---|
| `bootanimation-1080x2340.zip` | 全屏开机动画（1080×2340 / 15fps） | 自制，`desc.txt` 为 `1080 2340 15` + `p 1 0 part0`；装到 `/system/media/bootanimation.zip`（0644）。注意 bootloader/ABL 阶段的黑边无法用动画消除 |
| `opencamera-v1.56.2-fdroid.apk` | Open Camera（F-Droid 签名版） | v1.56.2 / versionCode 96，Android 6.0+。替代 GSI 自带必崩的 AOSP Camera2。取景器黑屏是 vendor HAL 死结，与本 app 无关 |
| `fix-camera-provider.sh` | 相机服务开机自启修复 | 见下 |

## 大文件镜像

以下镜像体积超过 GitHub 单文件 100MB 软限制，放在仓库 Releases：

- **EDL 全分区备份**（6.8GB / 64 分区）：Release `tapion-edl-v1`，分卷 `tapion-edl-full.z01/.z02/.z03/.zip`
- **GSI system 镜像 / Magisk patched boot**：待上传（本轮 d9 实验产物）

## 相机修复脚本用法

```bash
adb root
adb push fix-camera-provider.sh /data/local/tmp/
adb shell 'sh /data/local/tmp/fix-camera-provider.sh'   # 自动重启
adb reboot   # 若上面已重启则省略
adb shell 'getprop init.svc.vendor.camera-provider-2-4'  # 应为 running
```

回滚：`adb shell 'mv /vendor/etc/init/camera.rc.bak /vendor/etc/init/android.hardware.camera.provider@2.4-service.rc; reboot'`
