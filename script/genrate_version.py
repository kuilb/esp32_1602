import os

version_file_path = os.path.join(os.path.dirname(__file__), '..', 'version.txt')

if not os.path.exists(version_file_path):
    raise FileNotFoundError("version.txt 文件不存在，请先运行 set_version.py 设置版本号。")

with open(version_file_path, 'r') as version_file:
    lines = version_file.readlines()

project_version = ""
build_version = ""
build_time = ""

for line in lines:
    if line.startswith("Firmware version:"):
        project_version = line.strip().split(": ", 1)[1]
    elif line.startswith("Build version:"):
        build_version = line.strip().split(": ", 1)[1]
    elif line.startswith("Build time:"):
        build_time = line.strip().split(": ", 1)[1]

# 输出到 PlatformIO 的 build_flags 格式
print(f'-D PROJECT_VERSION=\\"{project_version}\\"')
print(f'-D BUILD_VERSION=\\"{build_version}\\"')
print(f'-D BUILD_TIMESTAMP=\\"{build_time}\\"')