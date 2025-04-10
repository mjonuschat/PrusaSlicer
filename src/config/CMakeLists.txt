cmake_minimum_required(VERSION 3.10)
project(ConfigTest)


add_executable(config_test main.cpp)

target_link_libraries(config_test PUBLIC slic3r-shared slic3r-domain)
