cmake_minimum_required(VERSION 3.13)
project(libvgcode)

add_library(libvgcode
    include/libvgcode/ColorRange.hpp
    include/libvgcode/Types.hpp
    include/libvgcode/Viewer.hpp
    include/libvgcode/ViewerInputData.hpp

    src/Bitset.hpp
    src/CogMarker.hpp
    src/CogMarker.cpp
    src/ColorRange.cpp
    src/Extruders.hpp
    src/Extruders.cpp
    src/ExtrusionRoles.hpp
    src/ExtrusionRoles.cpp
    src/Layers.hpp
    src/Layers.cpp
    src/ObjExport.hpp
    src/ObjExport.cpp
    src/OptionTemplate.hpp
    src/OptionTemplate.cpp
    src/Range.hpp
    src/Range.cpp
    src/SegmentTemplate.hpp
    src/SegmentTemplate.cpp
    src/Settings.hpp
    src/ToolMarker.hpp
    src/ToolMarker.cpp
    src/Types.cpp
    src/Utils.hpp
    src/Utils.cpp
    src/Viewer.cpp
    src/ViewerImpl.hpp
    src/ViewerImpl.cpp
    src/ViewRange.hpp
    src/ViewRange.cpp
)

target_include_directories(libvgcode PUBLIC include)
target_include_directories(libvgcode PRIVATE src)

target_link_libraries(libvgcode PUBLIC libslic3r libpgcode slic3r-render)
