#!/usr/bin/env bash
set -euo pipefail

# 项目根目录
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# 镜像名称（可通过环境变量 IMAGE 覆盖）
IMAGE="${IMAGE:-caraxes-build:ubuntu24.04}"

# 若镜像不存在则构建
if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
  docker build -t "$IMAGE" -f "$ROOT_DIR/Dockerfile" "$ROOT_DIR"
fi

# 运行容器，映射 src 目录与项目根目录，以及宿主机的内核头文件与模块
docker run --rm -it \
  -v "$ROOT_DIR:/workspace" \
  -v "$ROOT_DIR/src:/workspace/src" \
  -v /lib/modules:/lib/modules:ro \
  -v /usr/src:/usr/src:ro \
  -w /workspace \
  "$IMAGE"