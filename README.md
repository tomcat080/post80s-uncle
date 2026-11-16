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
├── firmware/       # 自制刷机包、镜像（大文件走 Releases）
├── poco-f1/        # POCO F1 刷 Kupfer Linux
├── xiaomi-4c/      # 小米路由 4C 刷 OpenWrt
└── images/         # 通用图片素材
```

## 折腾清单

| 设备 | 项目 | 状态 | 记录 |
|---|---|---|---|
| iRiver D8 | 固件恢复 / NAND 重写 | 刷机包自制成功，启动链排查中 | [详情](iriver-d8/README.md) |
| POCO F1 | 刷 Kupfer Linux (Phosh) | 已刷入 | 整理中 |
| 小米路由 4C | Breed + OpenWrt | 已完成 | 整理中 |
| TS240 | 本地 Ollama 模型流水线 | 在跑，已优化 13 轮 | 整理中 |
| DOOV MT6771 | 安卓老机改家庭 NAS | 已落地 | 整理中 |

## 关于这些记录

- **不保证能用**：都是我个人在特定设备上的操作记录，照做有风险，变砖自负。
- **图片随时补**：`images/` 目录里的占位说明会随实拍图逐步替换。
- **刷机包**：单独放 `firmware/` 或 GitHub Releases，详情页会挂下载链接。

---

*最后更新：2026-11-16*
