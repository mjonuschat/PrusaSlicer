cmake_minimum_required(VERSION 3.10)
project(ConfigTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(libslicerconfig
    include/libslicerconfig/Config.hpp
    include/libslicerconfig/ConfigSerialize.hpp
    include/libslicerconfig/ConfigLegacy.hpp

    src/ConfigItemValue.hpp
    src/ConfigSerialize.cpp
    src/Config.cpp
    src/Legacy/Config.hpp
    src/Legacy/Config.cpp
    src/Legacy/PrintConfig.hpp
    src/Legacy/PrintConfig.cpp
    src/Legacy/Point.hpp
    src/Legacy/Point.cpp
    src/Legacy/clonable_ptr.hpp
    src/Legacy/Thumbnails.hpp
    src/Legacy/Thumbnails.cpp
    src/Legacy/enum_bitmask.hpp
    src/ConfigLegacy.cpp
)

find_package(nlohmann_json 3.11 REQUIRED)


add_executable(config_test main.cpp
                           ConfigFDM.hpp
                           ConfigFDM.cpp
)

target_include_directories(libslicerconfig PUBLIC include
                                           PRIVATE src)
target_link_libraries(libslicerconfig PUBLIC nlohmann_json::nlohmann_json slic3r-base)
target_link_libraries(libslicerconfig PRIVATE localesutils fastfloat)

find_package(Boost CONFIG REQUIRED COMPONENTS system filesystem algorithm nowide log)
find_package(Eigen3 CONFIG REQUIRED)
find_package(LibBGCode REQUIRED)

target_link_libraries(libslicerconfig PUBLIC Boost::system Boost::filesystem Boost::algorithm Boost::nowide Boost::log Eigen3::Eigen LibBGCode::bgcode_convert)





# Link the executable with the library
target_link_libraries(config_test PRIVATE libslicerconfig)
