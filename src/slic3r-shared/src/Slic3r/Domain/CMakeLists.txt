set(SLIC3R3_DOMAIN_SOURCES
        DataManager.hpp
        DataManager.cpp
        Workbench.hpp
        Workbench.cpp
        Project.hpp
        Project.cpp
        ConfigContainer.hpp
        ConfigContainer.cpp
        Bed.hpp
        Bed.cpp
)

add_library(slic3r3_domain ${SLIC3R3_DOMAIN_SOURCES})
target_link_libraries(slic3r3_domain PUBLIC libslic3r)
