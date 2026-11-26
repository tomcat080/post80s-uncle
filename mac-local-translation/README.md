# Mac 本机本地翻译模型搭建方案

> 一行命令出双语字幕：`wsp 音频.mp3`
>
> 关键词：faster-whisper `large-v3` 识别 + Qwen2.5-7B GGUF **进程内**翻译 + 词级时间戳 + 自动校准
>
> 全链路离线：不装 Ollama、不起服务、不占端口、跑完即退；两台 Linux 服务器完全不参与。

---

## 一、这套方案解决什么问题

常见的「本地翻译」方案要么把素材推给服务器、要么起一个常驻的 Ollama 服务占着 11434 端口。这套方案走第三条路：

| 需求 | 传统做法 | 本方案 |
|---|---|---|
| 翻译模型从哪来 | Ollama `pull` 拉到私有仓库 | GGUF 单文件直接放 `~/Whisper/models/` |
| 谁来推理 | 常驻 `ollama serve` + HTTP API | `llama_cpp` 在管线**进程内**加载，跑完即退 |
| 占用 | 端口 11434 + 后台进程 | 零端口、零常驻 |
| 服务器依赖 | 断网/服务器停机就翻不了 | 完全不需要服务器 |
| 命令长度 | `cd` + Python 全路径 + 一堆参数 | `wsp 音频.mp3` |

**一句话定性**：Ollama 在别处是「模型仓库 + 推理服务器」二合一；在本机这两层都被跳过了——下载归各家库（faster-whisper / funasr / ModelScope / HF），推理归 `llama_cpp`。

## 二、先分清「耳朵」和「嘴巴」

这是最容易混淆的地方，也是决定该装什么模型的关键：

| 角色 | 干什么 | 模型 | 能不能被别的模型替代 |
|---|---|---|---|
| **耳朵**（ASR） | 把日语/英语等语音听成文字 | faster-whisper `large-v3` | 同族 `small`/`turbo` 功能相同，只有精度/速度差异 |
| **嘴巴**（翻译） | 把识别出的文字翻成中文 | Qwen2.5-7B-Instruct-Q4_K_M.gguf | 本机只有这一颗 7B；`Q4_K_M` 是量化精度，不是尺寸 |

**本机的两个关系式**：

```text
耳朵：faster-whisper small / large-v3-turbo / large-v3  →  已收敛为只留 large-v3
嘴巴：Qwen 0.5B / 7B GGUF  →  0.5B 实测复读被否，只留 7B
```

`--whisper` 调的是耳朵，`--gguf-model` 调的是嘴巴，两者互不相干。

## 三、环境地基

### 3.1 Python 运行时

本机有两套 Python，只有一套能跑全流程：

| 环境 | 已装 | 用途 |
|---|---|---|
| `~/Whisper/venv` | faster_whisper 1.2.1 / av / ctranslate2 | 识别 + 校准；翻译要借服务器 |
| **`~/miniforge3/envs/comfy/bin/python`** | 上述全部 + **`llama_cpp 0.3.36`** + torch 2.7.1 + transformers | **本机全流程独立** |

验证命令：

```bash
~/miniforge3/envs/comfy/bin/python -c "import llama_cpp, faster_whisper; print(llama_cpp.__version__)"
# 期望输出：0.3.36
```

### 3.2 为什么不用 Homebrew 装 Ollama

这台是 Intel Mac / macOS 14 Sonoma，Homebrew 7.0 已把 Intel 与 Sonoma 降为 Tier 3——不再产预编译 bottle，`brew install ollama` 变成 Go 源码本地编译，16G 内存被挤爆，进程被信号 137 掐断。

**结论：不修 Brew、不装 Ollama。** 本机字幕链路用不到它；`llama_cpp` 已随 `comfy` 环境装好，模型文件也早就在位。

### 3.3 依赖获取的两个坑

- `llama-cpp-python` 在清华源是 76.6MB **源码包**、需本地编译 → 直接复用已装好的 `comfy` 环境，不要重装。
- ComfyUI 目录里那颗 `qwen2.5-0.5b-instruct-q4_k_m.gguf`（469M）是给绘图提示词做中英转换的，**翻译质量不合格**（实测复读、自问自答），不能拿来做字幕。

## 四、模型清单与下载

当前本机 `~/Whisper/models/` 只剩三个（约 8.1G）：

| 模型 | 体积 | 加载者 | 用途 |
|---|---|---|---|
| `faster-whisper-large-v3/` | 2.9G | faster-whisper 直接读目录 | 语音识别（本机唯一 ASR 档） |
| `SenseVoiceSmall/` | 897M | funasr `AutoModel` | 旁路富标签（语种/情感/BGM/事件） |
| `Qwen2.5-7B-Instruct-Q4_K_M.gguf` | 4.3G | `llama_cpp` 进程内加载 | 翻译 |

### 4.1 翻译模型（7B GGUF）

从 ModelScope 下载单文件：

```bash
DST="$HOME/Whisper/models/Qwen2.5-7B-Instruct-Q4_K_M.gguf"
curl -L --fail --retry 2 --speed-time 30 --speed-limit 500000 \
  -o "${DST}.part" \
  "https://www.modelscope.cn/api/v1/models/lmstudio-community/Qwen2.5-7B-Instruct-GGUF/repo?Revision=master&FilePath=Qwen2.5-7B-Instruct-Q4_K_M.gguf"
mv "${DST}.part" "${DST}"
```

**必须放在 `$HOME/Whisper/models/` 且保持这个文件名** —— 那是管线的 `MODELS_DIR` 默认查找位，放对了就不必写 `--gguf-model`。

### 4.2 识别模型（large-v3）

```bash
HF_ENDPOINT=https://hf-mirror.com HF_HUB_ENABLE_HF_TRANSFER=1 \
  hf download Systran/faster-whisper-large-v3 \
  --local-dir "$HOME/Whisper/models/faster-whisper-large-v3" \
  --local-dir-use-symlinks False
```

⚠️ 目录名必须是 `faster-whisper-<档位>`——管线按这个前缀硬拼路径，写错会退化成联网下载。若目录里只有 `Downloads/model.bin`，需补齐 `config.json`、`preprocessor_config.json`（`feature_size: 128`）、`tokenizer.json`、`vocabulary.json`，否则会报 mel 维度不匹配或加载失败。

### 4.3 已清理的资产（别再装回来）

| 已移除 | 原因 |
|---|---|
| `faster-whisper-small` / `large-v3-turbo` | 与 large-v3 同族同功能，收敛到单档即可 |
| `Qwen 0.5B GGUF` | 实测复读、自问自答，翻译不合格 |
| 本机 NLLB-200 | 蒸馏版会丢人名，服务器留完整副本做兜底即可 |

## 五、核心改造：给管线加一颗 gguf 后端

`long_pipeline.py` 原本只认服务器 Ollama。改造点是新增 `--mt-backend gguf`：

```python
# 延迟导入，避免不用 GGUF 时也吃内存
from llama_cpp import Llama

llm = Llama(model_path=gguf_path, n_ctx=gguf_ctx, n_threads=gguf_threads, verbose=False)
resp = llm.create_chat_completion(
    messages=[{"role": "system", "content": hint_for(lang)},
              {"role": "user", "content": text}],
    temperature=0.2,
)
```

关键细节：

- **必须走 `create_chat_completion` + Qwen chat 模板**。误用 completion 模板会让 7B 自问自答复读（0.5B 上尤其明显）。
- 进程内加载、退出即释放，不监听任何端口。
- 翻译缓存写进 `work/<文件名>/mt.json`，重跑不重复翻；批量失败会严格按编号逐条回退。
- ffmpeg / ffprobe / silencedetect 的 `subprocess.run()` 一律加 `stdin=subprocess.DEVNULL`——否则在 IDE/管道环境里 ffmpeg 会监听 stdin 卡死。

## 六、启动器：把 5 行命令压成 1 行

`~/Whisper/local.sh`（已软链到 `/usr/local/bin/wsp`）：

```bash
#!/bin/bash
set -euo pipefail

# 顺着软链摸回真正的家（macOS BSD readlink 没有 -f）
SRC="${BASH_SOURCE[0]}"
while [ -L "${SRC}" ]; do
  DIR="$(cd "$(dirname "${SRC}")" && pwd)"
  LINK="$(readlink "${SRC}")"
  case "${LINK}" in /*) SRC="${LINK}" ;; *) SRC="${DIR}/${LINK}" ;; esac
done
HERE="$(cd "$(dirname "${SRC}")" && pwd)"
PY="${HOME}/miniforge3/envs/comfy/bin/python"
MODEL="${HERE}/models/Qwen2.5-7B-Instruct-Q4_K_M.gguf"

# 路径：相对路径转绝对；在任意目录敲命令时回落 Whisper 目录找同名文件
MEDIA="$1"; shift
case "${MEDIA}" in /*) ;; *) MEDIA="${PWD}/${MEDIA}" ;; esac
[ -f "${MEDIA}" ] || MEDIA="${HERE}/$(basename "${MEDIA}")"
cd "${HERE}"

# 未显式指定 --whisper 时补上 large-v3（本机只留这一档 ASR）
EXTRA=(--mt-backend gguf)
_wh=0
for a in "$@"; do [ "$a" = "--whisper" ] && _wh=1; done
[ "${_wh}" -eq 0 ] && EXTRA+=(--whisper large-v3)

exec "${PY}" long_pipeline.py "${MEDIA}" "${EXTRA[@]}" "$@"
```

三个设计要点：

1. **软链解析**：从 `/usr/local/bin/wsp` 启动时，脚本若把「家」认成 `/usr/local/bin`，模型就找不到了。循环 `readlink` 摸回真实路径。
2. **默认注入**：`--mt-backend gguf` 是唯一必写的开关（默认 `ollama` 留给服务器），封进启动器；本机只留 large-v3，所以未传 `--whisper` 时自动补上。
3. **`exec` 收尾**：进程替换 shell，跑完即退，不留后台。

建立全局命令：

```bash
chmod +x "$HOME/Whisper/local.sh"
ln -sfn "$HOME/Whisper/local.sh" /usr/local/bin/wsp
```

## 七、日常用法

```bash
wsp test-jp.mp3                              # 最简：large-v3 识别 + 7B 翻译
wsp "/Volumes/Share/MBRBI-015/xxx.mp4"       # 网络盘素材（空格/日文路径都吃）
wsp test-jp.mp3 --out 我的字幕.srt --batch 8 # 自定义输出与批大小
nohup wsp 大片.mp4 > "$HOME/Whisper/night.log" 2>&1 &   # 挂夜班，跑完自动退
```

输出：`~/Whisper/<文件名>_双语字幕.srt`
缓存：`~/Whisper/work/<文件名>/mt.json`（断点续跑）
默认已开：词级时间戳、时间轴校准、无对白检测、逐段语种标签。

### 参数旋钮速查

| 想调什么 | 参数 | 说明 |
|---|---|---|
| 换耳朵 | `--whisper large-v3` | 本机只剩这一档；手动传参仍优先 |
| 换嘴巴 | `--gguf-model <路径>` | 本机只有 7B；默认位已对准，不必写 |
| CPU 占用 | `--gguf-threads 4` | 0=自动；Intel 老机别贪多 |
| 内存/吞吐 | `--batch 8` | 默认 15，调小更省内存 |
| 字幕整体时间偏移 | `--shift 0.2` | 治「字幕偏早/偏晚」 |
| 单条最长显示 | `--max-sub-dur 5` | 治「字幕挂屏」 |
| 关闭校准 | `--no-refine` | 一般不用 |

**翻译质量没有「加个后缀就变好」的魔法开关**——想更准只能换更大的模型（14B/32B），这台 16G Intel 扛不动。

## 八、实测数据

| 时间 | 场景 | 结果 |
|---|---|---|
| 2026-11-05 | 首次 7B 全链路 | 约 49 秒（ASR 4.4s / 7B 加载 27.2s / 翻译 15s） |
| 2026-11-07 | `wsp test-jp.mp3` 全默认复核 | 2 分 03 秒（机器负载偏高），4 段双语字幕 |
| 2026-11-26 | 删 small/turbo 后默认档实跑 | 5.6 秒出片，校准 0 段、新重叠 0 处 |

译文对照：「こんにちは、ゆめじです」→「你好，我是叶舟。」（7B 人名按中文音译；服务器 3b 对照组保留片假名「我是ゆめじ」）。

验收标准：进程无残留、11434 端口无监听、字幕无重叠。

## 九、踩坑速查表

| 现象 | 原因与解法 |
|---|---|
| `brew install ollama` 被 137 掐断 | Intel/Sonoma 无 bottle，Go 源码编译爆内存。不用装，本机用 GGUF |
| `llama_cpp` 导入失败 | 清华源源码包需编译；直接用 `comfy` 环境 |
| 译文复读、自问自答 | 用了 0.5B 或 completion 模板；换 7B + `create_chat_completion` |
| 从 `/usr/local/bin/wsp` 启动找不到模型 | 软链解析没做；见第六节 `while [ -L ]` |
| 卡在「抽取 16k 单声道音频」 | ffmpeg 监听 stdin；`subprocess.run(..., stdin=DEVNULL)` |
| 删掉 small 后本地跑找不到模型 | `local.sh` 未显式指定时补 `--whisper large-v3` |
| 日文/SMB 路径取不到字幕 | macOS SMB 把 `じ` 存成 NFD 分解形；用「首词前缀 + `*` 通配」 |
| bash 里中文后面的变量值消失 | UTF-8 locale 下中文标点被吃进变量名；一律写 `${var}` |
| 时间轴挂屏/超前 | Whisper 段级窗口吞静音；默认词级时间戳 + 校准已修，旧字幕用 `srt_refine.py` |

## 十、维护与回滚

- **新增素材**：直接丢进 `~/Whisper/`，`wsp` 自动按文件名隔离缓存。
- **换机器**：拷走 `~/Whisper/models/Qwen2.5-7B-Instruct-Q4_K_M.gguf` 与 `local.sh`，在 `comfy` 环境装好 `llama_cpp` 即可；无需迁移服务器状态。
- **回滚到服务器翻译**：删掉 `/usr/local/bin/wsp` 软链，改用 `bash long_job.sh 素材.mp4`（走 Ollama 千问）。服务器文件从未被本方案改动。
- **磁盘**：模型目录约 8.1G；废纸篓里的 small/turbo 约 2G 清空后才真正释放（注意 Time Machine 本地快照会先 pin 住空间）。

---

**相关文件**：`local.sh`（启动器）、`long_pipeline.py`（主管线，33K）、`timing.py` / `srt_refine.py`（时间轴）、`sensevoice_tag.py`（旁路富标签）、`使用手册.md`（完整操作手册）。

**出处与边界**：本文记录的是本机（Intel Mac / macOS 14 Sonoma）的实搭方案；两台 Linux 服务器 `tom-pan` / `rock-tom` 的 Ollama 千问链路属于另一套体系，不在本文范围。文中命令均以本机实测为准，涉及删除/分区的操作请先备份。
