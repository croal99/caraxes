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

# 在容器内执行 make，挂载项目与宿主机的内核头文件/模块
docker run --rm \
  -v "$ROOT_DIR:/workspace" \
  -v /lib/modules:/lib/modules:ro \
  -v /usr/src:/usr/src:ro \
  -w /workspace \
  "$IMAGE" \
  bash -lc 'make clean && make'