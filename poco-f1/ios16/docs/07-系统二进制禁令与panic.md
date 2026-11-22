# 系统二进制禁令：为什么不能再动 `/usr/sbin/BlueTool`

2026-11-22 下午的一次实验踩了两个硬坑，写下这条红线，避免以后重复。

## 1. 现象

把 `/usr/sbin/BlueTool`（9,760,944 B，平台签名）换成 68,256 B 的自签 `exit 0` 桩后：

- 冷启动偶发**开机早期 panic**，屏幕文字：

```
panic(cpu 0 caller 0xffffffe007b238d0): userspace panic:
unexpected exec of non-platform binary: /usr/sbin/BlueTool: com.apple.BlueTool
exec id [293]
...
handshake structure not initialized
```

  即 launchd 在"exec 握手结构"尚未建立的开机早期去 exec 它，发现不是平台二进制就主动 panic。
- 之后每次重启，`/usr/sbin/BlueTool` 会**回到桩的状态**（md5 `76fc968d…`），我写回的 9,760,944 B 原件（md5 `aef4ae92…`）不保留。
- 直接 `cp` 覆盖这个文件时，设备会在几秒内自己重启一次（该文件正是常驻 `BlueTool -R` 进程映射着的 inode）。

## 2. 结论（红线）

1. **`/usr/sbin` 下的系统二进制一律不动。** 自签/adhoc 二进制放到这里，开机早期 exec 就会被判非平台并 panic。
2. **调试工具只放 `/usr/local/bin`**（我们现有的 `hcitest`、`btscan`、`minish` 都在那里，没出过事）。
3. 真要替换系统路径上的文件，只能**换 inode**（写到同目录临时名再 `mv`），绝不在原地 `cp` 覆盖正在执行的文件；并且要知道这种写回在重启后不保证保留。

## 3. 本次恢复过程（可复用）

1. 原件始终留在 `/usr/local/bin/BlueTool.orig-20261122`（9,760,944 B，md5 `aef4ae9257d270466bdec11b719ff1ff`）。
2. 恢复用 rename：

```sh
toybox cp -p /usr/local/bin/BlueTool.orig-20261122 /usr/sbin/.BlueTool.new
toybox chmod 755 /usr/sbin/.BlueTool.new
toybox mv /usr/sbin/.BlueTool.new /usr/sbin/BlueTool
toybox md5sum /usr/sbin/BlueTool          # 应等于 aef4ae92…
```

3. 校验通过后设备稳定运行、无新 panic。桩文件 `/tmp/btstub` 保留在本机备查，不再往设备上装。

## 4. 与蓝牙那条线的关系

这次实验本来是想验证"没有 `BlueTool` 时 `bluetoothd` 会不会自己去开 uart kctl"，结论是**不会**（详见 `03-蓝牙与AKS.md` 8.3）。蓝牙用户态整体已停手，这条禁令只是收拾现场。
