cmake_minimum_required(VERSION 3.12)
project(simnow_ctp_lab)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 库目录
link_directories(${PROJECT_SOURCE_DIR}/lib)

# 可执行文件
add_executable(simnow_ctp_lab src/main.cpp)

# 链接 CTP 库
target_link_libraries(simnow_ctp_lab
    thosttraderapi_se
    thostmduserapi_se
    pthread   # CTP 依赖 pthread
    dl        # 一般也需要 dl
)
