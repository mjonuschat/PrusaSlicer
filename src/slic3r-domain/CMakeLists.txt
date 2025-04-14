project(slic3r-domain)

find_package(NanoSVG REQUIRED)

add_library(slic3r-domain
    include/Slic3r/Domain/CustomGCode.hpp
    include/Slic3r/Domain/Line.hpp
    include/Slic3r/Domain/MultiPoint.hpp
    include/Slic3r/Domain/Polygon.hpp
    include/Slic3r/Domain/Polyline.hpp
    src/Slic3r/Domain/Line.cpp
    src/Slic3r/Domain/MultiPoint.cpp
    src/Slic3r/Domain/Polygon.cpp
    src/Slic3r/Domain/Polyline.cpp
    include/Slic3r/Domain/BoundingBox.hpp
    src/Slic3r/Domain/BoundingBox.cpp
    include/Slic3r/Domain/Point.hpp
    src/Slic3r/Domain/Point.cpp

    include/Slic3r/Domain/ExPolygon.hpp
    src/Slic3r/Domain/ExPolygon.cpp

    include/Slic3r/Domain/FacetsAnnotation.hpp
    src/Slic3r/Domain/FacetsAnnotation.cpp

    include/Slic3r/Domain/ObjectID.hpp
    src/Slic3r/Domain/ObjectID.cpp

    include/Slic3r/Domain/TriangleMesh.hpp
    src/Slic3r/Domain/TriangleMesh.cpp

    include/Slic3r/Domain/TriangleSelector.hpp
    src/Slic3r/Domain/TriangleSelector.cpp

    include/Slic3r/Domain/SLA/DrainHole.hpp
    src/Slic3r/Domain/SLA/DrainHole.cpp

    include/Slic3r/Domain/SLA/SupportPoint.hpp
    src/Slic3r/Domain/SLA/SupportPoint.cpp

    include/Slic3r/Domain/TextConfiguration.hpp
    src/Slic3r/Domain/TextConfiguration.cpp

    include/Slic3r/Domain/EmbossShape.hpp

    include/Slic3r/Domain/SelectionId.hpp

    include/Slic3r/Domain/ElementRef.hpp
    src/Slic3r/Domain/ElementId.cpp

    include/Slic3r/Domain/Bed.hpp
    src/Slic3r/Domain/Bed.cpp

    include/Slic3r/Domain/BedRef.hpp

    include/Slic3r/Domain/BedInstance.hpp
    src/Slic3r/Domain/BedInstance.cpp

    include/Slic3r/Domain/Transformation.hpp
    src/Slic3r/Domain/Transformation.cpp
)

target_include_directories(slic3r-domain PUBLIC include)
target_include_directories(slic3r-domain PRIVATE src)


target_link_libraries(slic3r-domain PUBLIC
    admesh # TODO, remove dependency on admesh by refactoring triangle mesh
    NanoSVG::nanosvg # TODO remove dependency on nanosvg by refactoring EmbossShape
    NanoSVG::nanosvgrast
)

target_link_libraries(slic3r-domain PUBLIC slic3r-base TBB::tbbmalloc)
