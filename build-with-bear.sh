#!/usr/bin/env bash
# 获取当前脚本文件所在的目录
SHELL_FOLDER="$(cd "$(dirname "$0")" && pwd)"

# 删除旧的compile_commands.json
rm -f compile_commands.json

# 使用bear包装整个构建过程
bear bash build.sh

# 移动compile_commands.json到项目根目录
cp compile_commands.json "${SHELL_FOLDER}/" 2>/dev/null || true

# 定义所有需要合并的子目录（包含新加入的 qemu 路径）
SUB_DIRS=("os" "opensbi-1.2" "trusted_domain" "qemu-8.0.2/build")

# 合并所有子目录中的compile_commands.json
for dir in "${SUB_DIRS[@]}"; do
    echo "Merging $dir..."
    if [ -f "${SHELL_FOLDER}/${dir}/compile_commands.json" ]; then
        jq -s 'add' "${SHELL_FOLDER}/compile_commands.json" "${SHELL_FOLDER}/${dir}/compile_commands.json" > temp.json
        mv temp.json "${SHELL_FOLDER}/compile_commands.json"
    else
        echo "Warning: $dir/compile_commands.json not found, skipping..."
    fi
done