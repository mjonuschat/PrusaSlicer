cmake_minimum_required(VERSION 3.13)
project(libvgcode)

add_library(libvgcode
    include/Slic3r/App/libvgcode/ColorRange.hpp
    include/Slic3r/App/libvgcode/GCodeNodeTag.hpp
    include/Slic3r/App/libvgcode/Types.hpp
    include/Slic3r/App/libvgcode/Viewer.hpp
    include/Slic3r/App/libvgcode/ViewerInputData.hpp

    src/Slic3r/App/libvgcode/ColorRange.cpp
    src/Slic3r/App/libvgcode/Bitset.hpp
    src/Slic3r/App/libvgcode/CogMarker.hpp
    src/Slic3r/App/libvgcode/CogMarker.cpp
    src/Slic3r/App/libvgcode/Extruders.hpp
    src/Slic3r/App/libvgcode/Extruders.cpp
    src/Slic3r/App/libvgcode/ExtrusionRoles.hpp
    src/Slic3r/App/libvgcode/ExtrusionRoles.cpp
    src/Slic3r/App/libvgcode/Layers.hpp
    src/Slic3r/App/libvgcode/Layers.cpp
    src/Slic3r/App/libvgcode/ObjExport.hpp
    src/Slic3r/App/libvgcode/ObjExport.cpp
    src/Slic3r/App/libvgcode/OptionTemplate.hpp
    src/Slic3r/App/libvgcode/OptionTemplate.cpp
    src/Slic3r/App/libvgcode/Range.hpp
    src/Slic3r/App/libvgcode/Range.cpp
    src/Slic3r/App/libvgcode/SegmentTemplate.hpp
    src/Slic3r/App/libvgcode/SegmentTemplate.cpp
    src/Slic3r/App/libvgcode/Settings.hpp
    src/Slic3r/App/libvgcode/ToolMarker.hpp
    src/Slic3r/App/libvgcode/ToolMarker.cpp
    src/Slic3r/App/libvgcode/Types.cpp
    src/Slic3r/App/libvgcode/Utils.hpp
    src/Slic3r/App/libvgcode/Utils.cpp
    src/Slic3r/App/libvgcode/Viewer.cpp
    src/Slic3r/App/libvgcode/ViewerImpl.hpp
    src/Slic3r/App/libvgcode/ViewerImpl.cpp
    src/Slic3r/App/libvgcode/ViewRange.hpp
    src/Slic3r/App/libvgcode/ViewRange.cpp
)

target_include_directories(libvgcode PUBLIC include)
target_include_directories(libvgcode PRIVATE src)

target_link_libraries(libvgcode PUBLIC libslic3r libpgcode slic3r-render slic3r-shared)
