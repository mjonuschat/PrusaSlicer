set(SLIC3R_SHARED_FILES
        include/Slic3r/App/Plater/ScenePresenter.hpp
        include/Slic3r/App/Plater/ScenePresenterProjectContext.hpp
        include/Slic3r/App/Plater/PlaterRenderModule.hpp
        src/Slic3r/App/Plater/PlaterRenderModule.cpp
        include/Slic3r/App/Plater/GizmoManager.hpp
        src/Slic3r/App/Plater/GizmoManager.cpp
        include/Slic3r/App/Plater/IGizmo.hpp
        include/Slic3r/App/Plater/GizmoEventContext.hpp
        include/Slic3r/App/Plater/CameraGizmo.hpp
        src/Slic3r/App/Plater/CameraGizmo.cpp
        include/Slic3r/App/Plater/GizmoNodeTag.hpp
        include/Slic3r/App/Plater/SceneNodeTag.hpp
        include/Slic3r/App/Plater/QuickSelectGizmo.hpp
        src/Slic3r/App/Plater/QuickSelectGizmo.cpp
        include/Slic3r/App/Plater/QuickDragGizmo.hpp
        src/Slic3r/App/Plater/QuickDragGizmo.cpp

        include/Slic3r/App/View/IPanelView.hpp

        src/Slic3r/App/Preview/View/DoubleSlider.cpp
        include/Slic3r/App/Preview/View/DoubleSlider.hpp

        src/Slic3r/App/Scene/Scene.cpp
        include/Slic3r/App/Scene/Scene.hpp
        src/Slic3r/App/Scene/Node.cpp
        include/Slic3r/App/Scene/Node.hpp
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
        include/Slic3r/App/Scene/SceneChangeSession.hpp
        src/Slic3r/App/Scene/SceneChangeSession.cpp

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

        include/Slic3r/Biz/ListenerList.hpp
        include/Slic3r/Biz/IProjectsChangedListener.hpp
        include/Slic3r/Biz/ISelectedConfigContainerChangedListener.hpp
        include/Slic3r/Biz/ISelectedProjectChangedListener.hpp
        src/Slic3r/Biz/ProjectInteractor.cpp
        include/Slic3r/Biz/ProjectInteractor.hpp

        include/Slic3r/StringUtils.hpp

        include/Slic3r/Domain/SelectionId.hpp
        include/Slic3r/Domain/ElementRef.hpp
        src/Slic3r/Domain/ElementId.cpp
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
        slic3r-render
        libslic3r
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


# Doxygen
add_target_to_doxygen(slic3r-shared)

# Tests
add_executable(slic3r-shared-tests
        test/main.cpp
        test/Slic3r/App/Scene/Node.cpp
)
target_include_directories(slic3r-shared-tests PRIVATE test)

find_package(Catch2 2.9 REQUIRED)
target_link_libraries(slic3r-shared-tests PRIVATE slic3r-shared Catch2::Catch2 )


