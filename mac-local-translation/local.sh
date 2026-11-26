#!/bin/bash
# local.sh — Mac 本机独立翻译启动器（进程内 Qwen2.5-7B GGUF）
# 不装 Ollama、不起服务、不占端口、跑完即退；两台服务器完全不参与。
#
# 用法：local.sh <音频或视频> [其它 long_pipeline.py 参数...]
#   local.sh test-jp.mp3
#   wsp "/Volumes/Share/MBRBI-015/xxx.mp4" --whisper large-v3
#   local.sh test-jp.mp3 --out 我的字幕.srt --batch 8 --shift 0.2
set -euo pipefail

# 顺着软链摸回真正的家（兼容 macOS BSD readlink，没有 -f）
SRC="${BASH_SOURCE[0]}"
while [ -L "${SRC}" ]; do
  DIR="$(cd "$(dirname "${SRC}")" && pwd)"
  LINK="$(readlink "${SRC}")"
  case "${LINK}" in /*) SRC="${LINK}" ;; *) SRC="${DIR}/${LINK}" ;; esac
done
HERE="$(cd "$(dirname "${SRC}")" && pwd)"
PY="${HOME}/miniforge3/envs/comfy/bin/python"
MODEL="${HERE}/models/Qwen2.5-7B-Instruct-Q4_K_M.gguf"

if [ "$#" -lt 1 ]; then
  cat <<'USAGE'
用法: local.sh <音频或视频> [其它参数...]

  local.sh test-jp.mp3
  local.sh "/Volumes/Share/MBRBI-015/xxx.mp4" --whisper large-v3
  local.sh test-jp.mp3 --out 我的字幕.srt --batch 8

默认: large-v3 识别 · Qwen2.5-7B GGUF 进程内翻译 · 词级时间戳 · 时间轴校准 · 无对白检测（本机仅保留 large-v3）
输出: ~/Whisper/<文件名>_双语字幕.srt   缓存: ~/Whisper/work/<文件名>/mt.json（断点续跑）
long_pipeline.py 的所有参数都可透传，后写的覆盖默认值。
USAGE
  exit 1
fi

[ -x "${PY}" ]    || { echo "✗ 找不到本机 Python：${PY}" >&2; exit 1; }
[ -f "${MODEL}" ] || { echo "✗ 找不到 GGUF 模型：${MODEL}" >&2; exit 1; }

MEDIA="$1"; shift
case "${MEDIA}" in
  /*) ;;
  *)  MEDIA="${PWD}/${MEDIA}" ;;
esac
if [ ! -f "${MEDIA}" ]; then
  # 在任意目录敲 `wsp test-jp.mp3` 时，回落到 Whisper 目录找同名文件
  CAND="${HERE}/$(basename "${MEDIA}")"
  if [ -f "${CAND}" ]; then MEDIA="${CAND}"; fi
fi
[ -f "${MEDIA}" ] || { echo "✗ 找不到媒体文件：${MEDIA}" >&2; exit 1; }

cd "${HERE}"

# 本机仅保留 large-v3：未显式指定 --whisper 时默认用它，避免删 small 后默认档失效
EXTRA=(--mt-backend gguf)
_wh=0
for _a in "$@"; do [ "${_a}" = "--whisper" ] && _wh=1; done
[ "${_wh}" -eq 0 ] && EXTRA+=(--whisper large-v3)

exec "${PY}" long_pipeline.py "${MEDIA}" "${EXTRA[@]}" "$@"
