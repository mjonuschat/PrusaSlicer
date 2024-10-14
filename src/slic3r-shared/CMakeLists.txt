set(SLIC3R_SHARED_FILES
        include/Slic3r/App/Render/Types.hpp
        src/Slic3r/App/Render/Types.cpp
        src/Slic3r/App/Render/Init.cpp
        include/Slic3r/App/Render/Init.hpp
        include/Slic3r/App/Render/Context.hpp
        src/Slic3r/App/Render/Context.cpp
        include/Slic3r/App/Render/Device.hpp
        src/Slic3r/App/Render/Device.cpp
        include/Slic3r/App/Render/Material.hpp
        include/Slic3r/App/Render/ShaderManager.hpp
        src/Slic3r/App/Render/ShaderManager_gl.cpp
        include/Slic3r/App/Render/ModelManager.hpp
        include/Slic3r/App/Render/Model.hpp
        include/Slic3r/App/Render/Texture.hpp
        src/Slic3r/App/Render/Texture_gl.cpp
        include/Slic3r/App/Render/Drawable.hpp
        include/Slic3r/App/Render/Buffer.hpp
        src/Slic3r/App/Render/Buffer_gl.cpp
        include/Slic3r/App/Render/Shader.hpp
        src/Slic3r/App/Render/Shader_gl.cpp
        include/Slic3r/App/Render/UniformValue.hpp
        src/Slic3r/App/Render/UniformValue.cpp
        include/Slic3r/App/Render/Geometry.hpp
        src/Slic3r/App/Render/Geometry_gl.cpp
        include/Slic3r/App/Render/GeometryBuilder.hpp
        src/Slic3r/App/Render/GeometryBuilder.cpp
        src/Slic3r/App/Render/TextureManager.cpp
        include/Slic3r/App/Render/TextureManager.hpp
        include/Slic3r/App/Render/Image.hpp
        src/Slic3r/App/Render/Image.cpp
        src/Slic3r/App/Render/ImageCodec.cpp
        include/Slic3r/App/Render/ImageCodec.hpp
        include/Slic3r/App/Render/CommandBuffer.hpp
        src/Slic3r/App/Render/CommandBuffer_gl.cpp
        src/Slic3r/App/Render/VertexAttribDesc.cpp
        include/Slic3r/App/Render/VertexAttribDesc.hpp
        src/Slic3r/App/Render/MathUtils.cpp
        include/Slic3r/App/Render/MathUtils.hpp
        include/Slic3r/App/Render/ImguiRender.hpp
        src/Slic3r/App/Render/ImguiRender.cpp
        include/Slic3r/App/Render/WithInternal.hpp
        src/Slic3r/App/Render/GL/GLTypes.cpp
        include/Slic3r/App/Render/GL/GLTypes.hpp
        include/Slic3r/App/Render/GL/commonGL.hpp
        src/Slic3r/App/Render/GL/commonGL.cpp
        include/Slic3r/App/Render/GL/GLTextureInternal.hpp
        include/Slic3r/App/Render/GL/GLBufferInternal.hpp
        include/Slic3r/App/Render/GL/GLGeometryInternal.hpp
        include/Slic3r/App/Render/GL/GLShaderInternal.hpp
        include/Slic3r/App/Render/GL/GLDeviceInternal.hpp
        src/Slic3r/App/Render/GL/GLDeviceInternal.cpp
        include/Slic3r/App/Render/ResourceManager.hpp
        include/Slic3r/App/Render/GeometryManager.hpp

        include/Slic3r/App/Plater/ScenePresenter.hpp

        include/Slic3r/App/View/IPanelView.hpp

        src/Slic3r/App/Preview/View/DoubleSlider.cpp
        include/Slic3r/App/Preview/View/DoubleSlider.hpp

        src/Slic3r/App/Scene/Scene.cpp
        include/Slic3r/App/Scene/Scene.hpp
        src/Slic3r/App/Scene/Node.cpp
        include/Slic3r/App/Scene/Node.hpp
        include/Slic3r/App/Scene/NodeInfo.hpp
        include/Slic3r/App/Scene/IRenderLayerObject.hpp
        include/Slic3r/App/Scene/IRenderNodeComponent.hpp
        include/Slic3r/App/Scene/Camera.hpp
        src/Slic3r/App/Scene/Camera.cpp
        include/Slic3r/App/Scene/Transform.hpp
        src/Slic3r/App/Scene/MeshRenderNodeComponent.cpp
        include/Slic3r/App/Scene/MeshRenderNodeComponent.hpp
        src/Slic3r/App/Scene/Material.cpp
        include/Slic3r/App/Scene/Material.hpp
        include/Slic3r/App/Scene/NodeBuilder.hpp
        src/Slic3r/App/Scene/NodeBuilder.cpp
        include/Slic3r/App/Scene/IRaycastNodeComponent.hpp
        include/Slic3r/App/Scene/AabbRaycastNodeComponent.hpp
        src/Slic3r/App/Scene/AabbRaycastNodeComponent.cpp
        include/Slic3r/App/Scene/Ray.hpp
        include/Slic3r/App/Scene/NodeVisitorTypes.hpp
        include/Slic3r/App/Scene/NodeVisitor.hpp
        src/Slic3r/App/Scene/NodeVisitor.cpp
        include/Slic3r/App/Scene/CameraTrackballController.hpp
        src/Slic3r/App/Scene/CameraTrackballController.cpp
        include/Slic3r/App/Scene/IImguiRenderNodeComponent.hpp
        include/Slic3r/App/Scene/INodeTransformModifier.hpp
        include/Slic3r/App/Scene/ScreenSpaceSizedTransformModifier.hpp
        src/Slic3r/App/Scene/ScreenSpaceSizedTransformModifier.cpp

        include/Slic3r/App/TestRenderModule.hpp
        src/Slic3r/App/TestRenderModule.cpp
        src/Slic3r/App/Color.cpp
        include/Slic3r/App/Color.hpp
        src/Slic3r/App/Init.cpp
        include/Slic3r/App/Init.hpp

        src/Slic3r/App/I18N/LanguageInfo.cpp
        include/Slic3r/App/I18N/LanguageInfo.hpp
        src/Slic3r/App/I18N/I18N.cpp
        include/Slic3r/App/I18N/I18N.hpp
        src/Slic3r/App/I18N/Translation.cpp
        include/Slic3r/App/I18N/Translation.hpp

        include/Slic3r/Biz/PrintConfigInteractor.hpp
        src/Slic3r/Biz/PrintConfigInteractor.cpp
        include/Slic3r/Biz/ObservableDynamicPrintConfig.hpp
        include/Slic3r/Biz/Observable.hpp
        include/Slic3r/Biz/Plater/SceneInteractor.hpp
        include/Slic3r/Biz/Plater/ConfigViewModel.hpp

        include/Slic3r/Biz/Scene/SceneInteractor.hpp
        src/Slic3r/Biz/Scene/SceneInteractor.cpp
        include/Slic3r/Biz/Scene/SceneChangeListener.hpp
        include/Slic3r/Biz/Scene/SceneSelectionChangeListener.hpp
        include/Slic3r/Biz/Scene/Selection.hpp
        include/Slic3r/Biz/Scene/SceneInteractorProjectContext.hpp

        include/Slic3r/Biz/Preset/IBedPresetValueChangedListener.hpp
        include/Slic3r/Biz/Preset/IBedPresetSwitchedListener.hpp
        include/Slic3r/Biz/Preset/IConfigInteractor.hpp
        src/Slic3r/Biz/Preset/PresetInteractor.cpp
        include/Slic3r/Biz/Preset/PresetInteractor.hpp
        include/Slic3r/Biz/Preset/PresetInteractorProjectContext.hpp
        include/Slic3r/Biz/Preset/PresetInteractorConfigContainerContext.hpp
        src/Slic3r/Biz/Preset/PresetInteractorConfigContainerContext.cpp
        include/Slic3r/Biz/Preset/PresetState.hpp
        include/Slic3r/Biz/Preset/PresetRuntime.hpp
        include/Slic3r/Biz/Preset/PresetBundleRuntime.hpp
        include/Slic3r/Biz/Preset/PresetHints.hpp
        src/Slic3r/Biz/Preset/PresetHints.cpp
        include/Slic3r/Biz/Preset/AbstractConfigManipulation.hpp
        src/Slic3r/Biz/Preset/AbstractConfigManipulation.cpp
        src/Slic3r/Biz/Preset/PresetBundleRuntime.cpp
        include/Slic3r/Biz/Preset/PresetBundleRuntime.hpp

        include/Slic3r/Biz/SelectionId.hpp
        include/Slic3r/Biz/ListenerList.hpp
        include/Slic3r/Biz/IProjectsChangedListener.hpp
        include/Slic3r/Biz/ISelectedConfigContainerChangedListener.hpp
        include/Slic3r/Biz/ISelectedProjectChangedListener.hpp
        src/Slic3r/Biz/ProjectInteractor.cpp
        include/Slic3r/Biz/ProjectInteractor.hpp

        include/Slic3r/Memory.hpp
        include/Slic3r/StringUtils.hpp

        src/Slic3r/Domain/ElementId.cpp
        include/Slic3r/Domain/ElementId.hpp
        src/Slic3r/Domain/Project.cpp
        include/Slic3r/Domain/Project.hpp
        include/Slic3r/Domain/Bed.hpp
        src/Slic3r/Domain/Bed.cpp
        src/Slic3r/Domain/Workbench.cpp
        include/Slic3r/Domain/Workbench.hpp
        src/Slic3r/Domain/ConfigContainer.cpp
        include/Slic3r/Domain/ConfigContainer.hpp
)

find_package(NanoSVG REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
add_library(slic3r-shared STATIC ${SLIC3R_SHARED_FILES})
target_link_libraries(slic3r-shared PUBLIC
        slic3r-base
        slic3r-platform
        libslic3r
        NanoSVG::nanosvg
        NanoSVG::nanosvgrast
        Boost::filesystem
        fmt::fmt
)
if (EMSCRIPTEN)
#    target_link_libraries(slic3r-shared PUBLIC png-mt )
else()
    find_package(PNG REQUIRED)
    target_link_libraries(slic3r-shared PUBLIC PNG::PNG)
endif ()
target_include_directories(slic3r-shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)


# Tests
add_executable(slic3r-shared-tests
        test/main.cpp
        test/Slic3r/App/Scene/Node.cpp
)
target_include_directories(slic3r-shared-tests PRIVATE test)

find_package(Catch2 2.9 REQUIRED)
target_link_libraries(slic3r-shared-tests PRIVATE slic3r-shared Catch2::Catch2 )


