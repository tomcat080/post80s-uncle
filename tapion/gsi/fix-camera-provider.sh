#!/bin/sh
# tapion GSI 相机服务开机自启修复
# 根因: vendor rc 里 camera provider 服务带 disabled, 且只在 FDE 解密属性触发时 start;
#       GSI 把 userdata 格式化为明文后无解密流程, 属性永不出现 -> 相机服务永不启动.
# 用法: adb root 后 adb push 到 /data/local/tmp/ 再 sh 执行, 会自动重启生效.
# 回滚: adb shell 'mv /vendor/etc/init/camera.rc.bak /vendor/etc/init/android.hardware.camera.provider@2.4-service.rc'

set -e
RC=/vendor/etc/init/android.hardware.camera.provider@2.4-service.rc

magisk --unlock-blocks 2>/dev/null || true
mount -o rw,remount /vendor

if [ ! -f "${RC}.bak" ]; then
    cp "$RC" "${RC}.bak"
    echo "[+] 已备份原 rc -> ${RC}.bak"
fi

if grep -q '^    disabled$' "$RC"; then
    sed -i '/^    disabled$/d' "$RC"
    echo "[+] 已移除 disabled, 相机服务将随 class hal 自启"
else
    echo "[=] disabled 已移除过, 跳过"
fi

mount -o ro,remount /vendor
echo "[*] 重启中..."
reboot
