set(SLIC3R_SHARED_FILES
        src/Slic3r/App/Render/Context.hpp
        src/Slic3r/App/Render/Context.cpp
        src/Slic3r/App/Render/Device.hpp
        src/Slic3r/App/Render/Device.cpp
        src/Slic3r/App/Render/Material.hpp
        src/Slic3r/App/Render/ShaderManager.hpp
        src/Slic3r/App/Render/ShaderManager.cpp
        src/Slic3r/App/Render/ModelManager.hpp
        src/Slic3r/App/Render/Model.hpp
        src/Slic3r/App/Render/Texture.hpp
        src/Slic3r/App/Render/Texture.cpp
        src/Slic3r/App/Render/Drawable.hpp
        src/Slic3r/App/Render/Buffer.hpp
        src/Slic3r/App/Render/Buffer.cpp
        src/Slic3r/App/Render/Shader.hpp
        src/Slic3r/App/Render/Shader.cpp
        src/Slic3r/App/Render/commonGL.hpp
        src/Slic3r/App/Render/commonGL.cpp
        src/Slic3r/App/Plater/ScenePresenter.hpp
        src/Slic3r/App/View/IPanelView.hpp
        src/Slic3r/App/Preview/View/DoubleSlider.cpp
        src/Slic3r/App/Preview/View/DoubleSlider.hpp
        src/Slic3r/App/Scene/Scene.cpp
        src/Slic3r/App/Scene/Scene.hpp
        src/Slic3r/Biz/PrintConfigInteractor.hpp
        src/Slic3r/Biz/ObservableDynamicPrintConfig.hpp
        src/Slic3r/Biz/PrintConfigInteractor.cpp
        src/Slic3r/Biz/Observable.hpp
        src/Slic3r/Biz/Plater/SceneInteractor.hpp
        src/Slic3r/Biz/Plater/ConfigViewModel.hpp
        src/Slic3r/Biz/Interactor/SceneInteractor.hpp
        src/Slic3r/Biz/Interactor/SceneInteractor.cpp
        src/Slic3r/App/TestRenderModule.hpp
        src/Slic3r/App/TestRenderModule.cpp
        src/Slic3r/App/Render/Geometry.hpp
        src/Slic3r/App/Render/Geometry.cpp
        src/Slic3r/App/Render/GeometryBuilder.hpp
        src/Slic3r/App/Render/GeometryBuilder.cpp
        src/Slic3r/App/Render/TextureManager.cpp
        src/Slic3r/App/Render/TextureManager.hpp
        src/Slic3r/App/Render/Image.hpp
        src/Slic3r/App/Render/Image.cpp
        src/Slic3r/App/Render/ImageCodec.cpp
        src/Slic3r/App/Render/ImageCodec.hpp
        src/Slic3r/Memory.hpp
)

if (NOT EMSCRIPTEN)
    set(SLIC3R_SHARED_FILE
            ${SLIC3R_SHARED_FILES}
            src/Slic3r/Domain/ConfigContainer.hpp
            src/Slic3r/Domain/DataManager.cpp
            src/Slic3r/Domain/Project.cpp
            src/Slic3r/Domain/Bed.hpp
            src/Slic3r/Domain/Workbench.cpp
            src/Slic3r/Domain/DataManager.hpp
            src/Slic3r/Domain/Project.hpp
            src/Slic3r/Domain/Bed.cpp
            src/Slic3r/Domain/Workbench.hpp
            src/Slic3r/Domain/ConfigContainer.cpp
    )
endif()

find_package(NanoSVG REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
add_library(slic3r-shared STATIC ${SLIC3R_SHARED_FILES})
target_link_libraries(slic3r-shared PUBLIC slic3r-platform libslic3r NanoSVG::nanosvg NanoSVG::nanosvgrast Boost::filesystem)
if (EMSCRIPTEN)
#    target_link_libraries(slic3r-shared PUBLIC png-mt )
else()
    find_package(PNG REQUIRED)
    target_link_libraries(slic3r-shared PUBLIC PNG::PNG)
endif ()
target_include_directories(slic3r-shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
