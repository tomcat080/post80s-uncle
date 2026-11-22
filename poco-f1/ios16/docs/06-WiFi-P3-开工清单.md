# Wi-Fi P3（MSS 地基）开工清单

蓝牙用户态线 2026-11-22 已停手（原因见 `03-蓝牙与AKS.md` 第八节）。重心转 Wi-Fi，按上游
`docs/WIFI_BT_PLAN.md` §5 的阶段表，下一站是 **P3：free pool + hyp-assign + MBA load + RMB 握手，
到 "MBA alive" 为止，不碰 MPSS**。风险评级 high —— 这是全项目第一次真正让 QHEE 超管介入的动作。

## 1. 目标与边界

- **要做**：把 `image/mba.mbn`（238,304 B）放进 MBA carve-out `0x96900000 + 0x200000`，
  hyp-assign 给 `MSS_MSA`，写 `RMB_MBA_IMAGE_REG`(0x00)，放开 Q6 reset，读到
  `RMB_PBL_STATUS == 1`、`RMB_MBA_STATUS == XPU_UNLOCKED(1)`。
- **不做**：metadata 认证（P4 第 7 步）、MPSS 各段（P4 第 8 步）、SMP2P/GLINK/QRTR（P4）、WLAN 射频（P5）、联网（P6）。
- **绝不写 flash**。只允许 READ(10)；UFS 写路径仍由 `ufsrw=1` 与分区边界闸着。

## 2. 已确认的事实（可直接用）

| 项 | 值 | 来源 |
|---|---|---|
| MBA carve-out | base `0x96900000`，size `0x200000` | live DT |
| WLAN MSA | base `0x8e300000`，size `0x100000` | live DT |
| MPSS | base `0x8e400000`，size `0x7800000` | beryllium 覆盖值，mainline 默认是错的 |
| SMEM | `0x86000000 + 0x200000`，表在 `base+size-0x1000`，hwlock = TCSR mutex 3 | live DT |
| `mba.mbn` | ELF，238,304 B，能放进 2 MiB carve-out | 主机侧 FAT 解析 |
| `modem.mdt` | 30 个 program header，entry `0x8e400000`，28 个可加载段到 `0x95c00000` | 主机侧 ELF 解析 |
| halt 寄存器 | TCSR `0x1f60000`：halt_q6 `0x1f63000`、halt_modem `0x1f65000`、halt_nc `0x1f64000` | live DT |
| QDSP6 / RMB | `0x4080000`（0x408 B）/ `0x4180000`（0x48 B） | live DT |

## 3. 风险闸门（每条都要有日志与退路）

1. **寄存器读悬挂**：MSS 未出复位前不读 QDSP6/RMB。探测阶段先只读 TCSR/GCC 这类必然可读的窗口。
2. **SMMU 全局 fault = SoC 复位**：MBA 只做 mem→mem + RMW，不挂 DMA 主设备，别在这步引入 CE/HTC。
3. **XPU / QHEE**：MBA carve-out 是否已被 XPU 保护、`hyp-assign` 是否被拒都是 [U]，第一次 SMC 返回码就是答案，不要连打。
4. **rail vote**：只允许 `ldoa5/7/17/23/25`；`s4a` 永不 vote；CX/MX/MSS 三个 RPMh power domain 用与触摸 L11C 同一个 client。
5. **一次一变**：每个 gate 单独编译、单独刷、单独回滚，不合并三个未知。

## 4. 第 0 步：只读预检（先做这个）

新增 `sweetwlan=4` 模式（现有 `=1` 是 P0 探针、`=2` 是蓝牙、`=3` 是 P1 清单），只做三件事：

1. 把 carve-out 窗口 `ml_io_map(0x96900000, 0x200000)` 之外**不**碰，仅映射并读 MBA 首 16 字节（未上电时应全 0 或读通不悬挂）；读失败即中止并打印。
2. 打印 TCSR halt 三寄存器的原始值、GCC 的 `gcc_mss_*` 窗口可读性（只读）。
3. 确认 QHEE 侧 `hyp-assign` 入口存在：打印我们内核里可用的 SCM/hyp 调用符号与调用约定（SMCCC `0x42000000 | svc<<8 | cmd` vs legacy），**不发 SMC**。

产物：一段 console 输出，落 `logs/`，作为 P3 第 1 步的基线。不刷 boot 之外任何分区。

## 5. 第 1 步（第 0 步通过后再动）：MBA load skeleton

- 顺序：给 MBA 窗口做 `hyp-assign(HLOS -> MSS_MSA)` → 拷 `mba.mbn`（从 `modem` 分区读，只读）→ 写 `RMB_MBA_IMAGE_REG` → 放开 Q6 reset → 轮询 `RMB_PBL_STATUS`/`RMB_MBA_STATUS`（上限 100000 次，学 topaz 的 resume loop 上限）。
- 每一步单独打日志前缀 `SWEET MSS:`，失败即停，不重试。
- 回滚：这步不写 flash，最坏是 SoC 自己复位；`fastboot boot` 回 shim 镜像即可。

## 6. 已知未决（第 0 步要顺手回答）

- 这台机器的 TZ 用 SMCCC 还是 legacy SCM 调用约定。
- MBA carve-out 当前是否已被 XPU 锁（读通但写被拒也能从返回码看出来）。
- QHEE 是否允许 HLOS 对 MBA 窗口做 assign（topaz 在 SM6225 上可以，SDM845 未验证）。

## 7. 第 0 步实测结果（2026-11-22 16:20，`sweetwlan=4`）

内核：CI run `45001173418`（commit `1b8c9c1`，patch `0143-beryllium-mss-preflight.patch`），
boot：`shim-boot-mss.img`（17,313,792 B，sha256 `5dba596efead5316…`），cmdline 用
`sweetwlan=4 sweetwlandelay=30` 替换原 `sweetwlan=2 sweetbt=1 sweetbtsk=1 sweetbtlog=200`。

console 原文（`SWEET MSS` 前缀，六行）：

```
SWEET MSS: preflight start (read-only): no hyp-assign, no reset release
SWEET MSS: SCM query fn=0x42000601 arg1=0x2000c16
SWEET MSS: SCM ASSIGN_MEM svc 0x0C cmd 0x16 available: status=0 result=1
SWEET MSS: MBA 0x96900000 +0: 00000000 00000000 00000000 00000000 (zero (not loaded))
SWEET MSS: TCSR halt_q6 00000000 halt_modem 00000000 halt_nc 00000000
SWEET MSS: preflight done (QDSP6 0x4080000 / RMB 0x4180000 still not read)
```

### 7.1 三个未决问题，两个有答案

1. **调用约定**：`fn=0x42000601`（`SCM_64 | SCM_SIP | SVC_INFO | CMD_INFO_AVAIL`）返回
   `status=0`，说明这台机器的 TZ 认 **SMCCC/SMC64 编码**，跟 GPU 那条用的
   `0x4200xxxx`（`0103` 补丁里 `scm_smccc()`）完全一致，不需要走 legacy SCM 约定。
2. **ASSIGN_MEM 是否存在**：`result=1` —— **可用**。也就是说 HLOS→MSS_MSA 的
   hyp-assign（`qcom_scm_assign_mem`，SIP svc `0x0C` cmd `0x16`）这一侧是受支持的，
   P3 第 1 步不缺这个入口。
3. **XPU 是否已锁 MBA 窗口**：读得到且全 0（读通、没悬挂），但"能读"不等于"能写"，
   是否被 XPU 拦写要等第 1 步真正 assign/写才见分晓。

### 7.2 顺带确认的

- MBA carve-out 在 modem 复位状态下**读不悬挂**（plan §5 里那条 "register read hangs the bus"
  的风险这条路径上没有出现）。
- TCSR halt 三寄存器（`0x1f63000/0x1f65000/0x1f64000`）全 0，说明 MSS 处于 halt 状态、
  没有被别的子系统拉起来过。
- 无 panic、无自动重启；冷启动后 SSH 照常可用。

### 7.3 第 1 步要动的东西（现在才需要慎重）

- `hyp-assign` MBA 窗口 → 从 `modem` 分区只读读 `image/mba.mbn`（238,304 B）→ 拷进
  `0x96900000` → 写 `RMB_MBA_IMAGE_REG`（`0x4180000 + 0x00`）→ 放开 Q6 reset →
  轮询 `RMB_PBL_STATUS`/`RMB_MBA_STATUS`。
- 这步开始才真正碰 QHEE 与 XPU：写被拒的返回码、SMMU fault 复位都要按闸门单独测。
