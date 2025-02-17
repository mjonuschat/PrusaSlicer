cmake_minimum_required(VERSION 3.13)
project(libvgcode)

add_library(libvgcode
    include/Slic3r/Biz/libvgcode/ColorRange.hpp
    include/Slic3r/Biz/libvgcode/Types.hpp
    include/Slic3r/Biz/libvgcode/Viewer.hpp
    include/Slic3r/Biz/libvgcode/ViewerInputData.hpp

    src/Slic3r/Biz/libvgcode/Bitset.hpp
    src/Slic3r/Biz/libvgcode/CogMarker.hpp
    src/Slic3r/Biz/libvgcode/CogMarker.cpp
    src/Slic3r/Biz/libvgcode/ColorRange.cpp
    src/Slic3r/Biz/libvgcode/Extruders.hpp
    src/Slic3r/Biz/libvgcode/Extruders.cpp
    src/Slic3r/Biz/libvgcode/ExtrusionRoles.hpp
    src/Slic3r/Biz/libvgcode/ExtrusionRoles.cpp
    src/Slic3r/Biz/libvgcode/Layers.hpp
    src/Slic3r/Biz/libvgcode/Layers.cpp
    src/Slic3r/Biz/libvgcode/ObjExport.hpp
    src/Slic3r/Biz/libvgcode/ObjExport.cpp
    src/Slic3r/Biz/libvgcode/OptionTemplate.hpp
    src/Slic3r/Biz/libvgcode/OptionTemplate.cpp
    src/Slic3r/Biz/libvgcode/Range.hpp
    src/Slic3r/Biz/libvgcode/Range.cpp
    src/Slic3r/Biz/libvgcode/SegmentTemplate.hpp
    src/Slic3r/Biz/libvgcode/SegmentTemplate.cpp
    src/Slic3r/Biz/libvgcode/Settings.hpp
    src/Slic3r/Biz/libvgcode/ToolMarker.hpp
    src/Slic3r/Biz/libvgcode/ToolMarker.cpp
    src/Slic3r/Biz/libvgcode/Types.cpp
    src/Slic3r/Biz/libvgcode/Utils.hpp
    src/Slic3r/Biz/libvgcode/Utils.cpp
    src/Slic3r/Biz/libvgcode/Viewer.cpp
    src/Slic3r/Biz/libvgcode/ViewerImpl.hpp
    src/Slic3r/Biz/libvgcode/ViewerImpl.cpp
    src/Slic3r/Biz/libvgcode/ViewRange.hpp
    src/Slic3r/Biz/libvgcode/ViewRange.cpp
)

target_include_directories(libvgcode PUBLIC include)
target_include_directories(libvgcode PRIVATE src)

target_link_libraries(libvgcode PUBLIC libslic3r libpgcode slic3r-render)
