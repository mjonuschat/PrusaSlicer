set(SLIC3R3_BIZ_SOURCES
        PrintConfigInteractor.cpp
        PrintConfigInteractor.hpp
        Observable.hpp
        ObservableDynamicPrintConfig.hpp
        Interactor/SceneInteractor.cpp
        Interactor/SceneInteractor.hpp
)
add_library(slic3r3_biz ${SLIC3R3_BIZ_SOURCES})
target_link_libraries(slic3r3_biz PUBLIC libslic3r)
