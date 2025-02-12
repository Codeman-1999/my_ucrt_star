#!/usr/bin/env bash
# 获取当前脚本文件所在的目录
SHELL_FOLDER="$(cd "$(dirname "$0")" && pwd)"

# 删除旧的compile_commands.json
rm -f compile_commands.json

# 使用bear包装整个构建过程
bear bash build.sh

# 移动compile_commands.json到项目根目录
mv compile_commands.json "${SHELL_FOLDER}/" 2>/dev/null || true

# 合并所有子目录中的compile_commands.json（如果存在的话）
for dir in os opensbi trusted_domain; do
    if [ -f "${SHELL_FOLDER}/${dir}/compile_commands.json" ]; then
        jq -s 'add' "${SHELL_FOLDER}/compile_commands.json" "${SHELL_FOLDER}/${dir}/compile_commands.json" > temp.json
        mv temp.json "${SHELL_FOLDER}/compile_commands.json"
    fi
done