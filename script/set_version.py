import datetime
import os

# 应用版本号
PROJECT_VERSION = "1.0.2"
IS_RELEASE_MODE = True
IS_DEVELOPER_TEST = False

# 构建时间戳
BUILD_TIMESTAMP = datetime.datetime.now()

# 读取或生成构建版本号
def get_build_number():
    version_file_path = os.path.join(os.path.dirname(__file__), '..', 'version.txt')
    build_number = 0

    if os.path.exists(version_file_path):
        with open(version_file_path, 'r') as file:
            for line in file:
                if line.startswith("Build version:"):
                    build_number = int(line.strip()[-3:])  # 提取最后三位构建号

    # 询问是否增加构建号
    print(f"当前构建号: {build_number:03d}")
    user_input = input("是否增加构建号？(y/n): ").strip().lower()
    if user_input == 'y':
        build_number += 1

    return build_number

# 计算距离上次构建的时间
def time_since_last_build():
    version_file_path = os.path.join(os.path.dirname(__file__), '..', 'version.txt')
    if not os.path.exists(version_file_path):
        return "这是第一次构建！"

    with open(version_file_path, 'r') as file:
        for line in file:
            if line.startswith("Build time:"):
                last_build_time_str = line.strip().split(": ", 1)[1]
                last_build_time = datetime.datetime.strptime(last_build_time_str, "%Y-%m-%d_%H-%M-%S")
                delta = BUILD_TIMESTAMP - last_build_time
                days, seconds = delta.days, delta.seconds
                hours = seconds // 3600
                minutes = (seconds % 3600) // 60
                return f"距离上次构建过去了 {days} 天 {hours} 小时 {minutes} 分钟"

    return "无法解析上次构建时间！"

# 生成版本号
def generate_version(build_number):
    major, minor, *_ = PROJECT_VERSION.split('.')
    version_prefix = f"{major}{minor}"
    version_type = "A" if IS_RELEASE_MODE else "D"
    developer_flag = "5" if IS_DEVELOPER_TEST else ""
    full_version = f"{version_prefix}{version_type}{developer_flag}{build_number:03d}"
    return full_version

# 主函数
if __name__ == "__main__":
    # 获取构建号
    build_number = get_build_number()

    # 生成版本号
    full_version = generate_version(build_number)
    build_timestamp = BUILD_TIMESTAMP.strftime("%Y-%m-%d_%H-%M-%S")

    # 计算距离上次构建的时间
    time_since_last = time_since_last_build()

    print(time_since_last)
    print("project_version:", PROJECT_VERSION)
    print("build_version:", full_version)
    print("build_time:", build_timestamp)

    # 写入到 version.txt 文件
    version_file_path = os.path.join(os.path.dirname(__file__), '..', 'version.txt')
    with open(version_file_path, 'w', encoding='utf-8') as version_file:  # 显式指定 UTF-8 编码
        version_file.write(f"Firmware version: {PROJECT_VERSION}\n")
        version_file.write(f"Build version: {full_version}\n")
        version_file.write(f"Build time: {build_timestamp}\n")

    print("version信息已写入。")