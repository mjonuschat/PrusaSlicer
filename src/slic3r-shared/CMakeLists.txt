set(SLIC3R_SHARED_FILES
        include/Slic3r/App/Plater/PlaterScenePresenter.hpp
        src/Slic3r/App/Plater/PlaterScenePresenter.cpp
        include/Slic3r/App/Plater/PlaterRenderModule.hpp
        src/Slic3r/App/Plater/PlaterRenderModule.cpp
        include/Slic3r/App/Plater/PlaterCameraGizmo.hpp
        src/Slic3r/App/Plater/PlaterCameraGizmo.cpp
        include/Slic3r/App/Plater/GizmoNodeTag.hpp
        include/Slic3r/App/Plater/SceneNodeTag.hpp
        include/Slic3r/App/Plater/NodePredicates.hpp
        include/Slic3r/App/Plater/QuickSelectGizmo.hpp
        src/Slic3r/App/Plater/QuickSelectGizmo.cpp
        include/Slic3r/App/Plater/QuickDragGizmo.hpp
        src/Slic3r/App/Plater/QuickDragGizmo.cpp
        include/Slic3r/App/Plater/BedSelectGizmo.hpp
        src/Slic3r/App/Plater/BedSelectGizmo.cpp
        include/Slic3r/App/Plater/AuxiliaryElementId.hpp
        include/Slic3r/App/Plater/SelectionHandler.hpp
        src/Slic3r/App/Plater/SelectionHandler.cpp
        include/Slic3r/App/Plater/TranslationGizmo.hpp
        src/Slic3r/App/Plater/TranslationGizmo.cpp
        include/Slic3r/App/Plater/RotationGizmo.hpp
        src/Slic3r/App/Plater/RotationGizmo.cpp
        include/Slic3r/App/Plater/PlaterSceneLayer.hpp
        include/Slic3r/App/Plater/BedNodeTag.hpp
        include/Slic3r/App/Plater/BedRenderUpdater.hpp
        src/Slic3r/App/Plater/BedRenderUpdater.cpp
        include/Slic3r/App/Plater/BedRenderHelper.hpp
        src/Slic3r/App/Plater/BedRenderHelper.cpp
        include/Slic3r/App/Plater/BedMaterials.hpp
        src/Slic3r/App/Plater/BedMaterials.cpp
        include/Slic3r/App/Plater/ObjectList.hpp
        src/Slic3r/App/Plater/ObjectList.cpp

        include/Slic3r/App/View/IPanelView.hpp

        include/Slic3r/App/Preview/PreviewRenderModule.hpp
        src/Slic3r/App/Preview/PreviewRenderModule.cpp
        include/Slic3r/App/Preview/PreviewCameraGizmo.hpp
        include/Slic3r/App/Preview/PreviewScenePresenter.hpp
        src/Slic3r/App/Preview/PreviewScenePresenter.cpp
        include/Slic3r/App/Preview/PreviewAuxiliaryElementId.hpp
        include/Slic3r/App/Preview/PreviewSceneLayer.hpp
        include/Slic3r/App/Preview/PreviewSceneRenderCustomizer.hpp
        src/Slic3r/App/Preview/PreviewSceneRenderCustomizer.cpp

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
        src/Slic3r/App/Scene/InstancedMeshRenderNodeComponent.cpp
        include/Slic3r/App/Scene/InstancedMeshRenderNodeComponent.hpp
        include/Slic3r/App/Scene/NodeBuilder.hpp
        src/Slic3r/App/Scene/NodeBuilder.cpp
        include/Slic3r/App/Scene/IRaycastNodeComponent.hpp
        include/Slic3r/App/Scene/AabbRaycastNodeComponent.hpp
        src/Slic3r/App/Scene/AabbRaycastNodeComponent.cpp
        include/Slic3r/App/Scene/Ray.hpp
        src/Slic3r/App/Scene/Ray.cpp
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
        include/Slic3r/App/Scene/Plane.hpp
        src/Slic3r/App/Scene/Plane.cpp
        include/Slic3r/App/Scene/Frustum.hpp
        src/Slic3r/App/Scene/Frustum.cpp
        include/Slic3r/App/Scene/IGizmo.hpp
        include/Slic3r/App/Scene/GizmoEventContext.hpp
        include/Slic3r/App/Scene/AbstractCameraGizmo.hpp
        src/Slic3r/App/Scene/AbstractCameraGizmo.cpp
        include/Slic3r/App/Scene/GizmoManager.hpp
        src/Slic3r/App/Scene/GizmoManager.cpp
        include/Slic3r/App/Scene/GeometryDataFactory.hpp
        src/Slic3r/App/Scene/GeometryDataFactory.cpp
        include/Slic3r/App/Scene/ISceneProvider.hpp
        include/Slic3r/App/Scene/ScenePresenterProjectContext.hpp

        include/Slic3r/App/TestRenderModule.hpp
        src/Slic3r/App/TestRenderModule.cpp
        src/Slic3r/App/Init.cpp
        include/Slic3r/App/Init.hpp

        src/Slic3r/App/I18N/LanguageInfo.cpp
        include/Slic3r/App/I18N/LanguageInfo.hpp
        src/Slic3r/App/I18N/I18N.cpp
        include/Slic3r/App/I18N/I18N.hpp
        src/Slic3r/App/I18N/Translation.cpp
        include/Slic3r/App/I18N/Translation.hpp
        src/Slic3r/App/Localization.cpp
        include/Slic3r/App/Localization.hpp
        include/Slic3r/App/ILanguageChangedListener.hpp

        include/Slic3r/App/Yoga/Align.hpp
        src/Slic3r/App/Yoga/FlexSizer.cpp
        include/Slic3r/App/Yoga/FlexSizer.hpp
        src/Slic3r/App/Yoga/SplitterSizer.cpp
        include/Slic3r/App/Yoga/SplitterSizer.hpp
        src/Slic3r/App/Yoga/Toolbar/Item.cpp
        include/Slic3r/App/Yoga/Toolbar/Item.hpp
        src/Slic3r/App/Yoga/Toolbar/Toolbar.cpp
        include/Slic3r/App/Yoga/Toolbar/Toolbar.hpp
        include/Slic3r/App/Yoga/Toolbar.hpp
        src/Slic3r/App/Yoga/MiddleSizer.cpp
        include/Slic3r/App/Yoga/MiddleSizer.hpp

        src/Slic3r/App/Imgui/DoubleSlider.cpp
        include/Slic3r/App/Imgui/DoubleSlider.hpp
        src/Slic3r/App/Imgui/RulerForDoubleSlider.cpp
        include/Slic3r/App/Imgui/RulerForDoubleSlider.hpp
        src/Slic3r/App/Imgui/ImguiExtension.cpp
        include/Slic3r/App/Imgui/ImguiExtension.hpp

        src/Slic3r/App/LibvgcodeWrapper/Types.cpp
        include/Slic3r/App/LibvgcodeWrapper/Types.hpp
        src/Slic3r/App/LibvgcodeWrapper/Wrapper.cpp
        include/Slic3r/App/LibvgcodeWrapper/Wrapper.hpp
        src/Slic3r/App/LibvgcodeWrapper/WrapperImpl.cpp
        include/Slic3r/App/LibvgcodeWrapper/WrapperImpl.hpp
        include/Slic3r/App/LibvgcodeWrapper/WrapperInputData.hpp
        src/Slic3r/App/LibvgcodeWrapper/DoubleSliderForLayers.cpp
        include/Slic3r/App/LibvgcodeWrapper/DoubleSliderForLayers.hpp
        src/Slic3r/App/LibvgcodeWrapper/DoubleSliderForGCode.cpp
        include/Slic3r/App/LibvgcodeWrapper/DoubleSliderForGCode.hpp
        src/Slic3r/App/LibvgcodeWrapper/ExtrudersSequence.cpp
        include/Slic3r/App/LibvgcodeWrapper/ExtrudersSequence.hpp
        src/Slic3r/App/LibvgcodeWrapper/TickCodeManager.cpp
        include/Slic3r/App/LibvgcodeWrapper/TickCodeManager.hpp
        src/Slic3r/App/LibvgcodeWrapper/ActualSpeedPlotter.cpp
        include/Slic3r/App/LibvgcodeWrapper/ActualSpeedPlotter.hpp
        src/Slic3r/App/LibvgcodeWrapper/GCodeWindow.cpp
        include/Slic3r/App/LibvgcodeWrapper/GCodeWindow.hpp
        src/Slic3r/App/LibvgcodeWrapper/Legend.cpp
        include/Slic3r/App/LibvgcodeWrapper/Legend.hpp

        src/Slic3r/App/TestRenderLayout.cpp
        include/Slic3r/App/TestRenderLayout.hpp

        include/Slic3r/Biz/PrintConfigInteractor.hpp
        src/Slic3r/Biz/PrintConfigInteractor.cpp
        include/Slic3r/Biz/ObservableDynamicPrintConfig.hpp
        include/Slic3r/Biz/Observable.hpp
        include/Slic3r/Biz/Plater/ConfigViewModel.hpp
        include/Slic3r/Biz/Plater/BedGeometry.hpp
        src/Slic3r/Biz/Plater/BedGeometry.cpp

        include/Slic3r/Biz/Scene/SceneInteractor.hpp
        src/Slic3r/Biz/Scene/SceneInteractor.cpp
        include/Slic3r/Biz/Scene/Selection.hpp
        src/Slic3r/Biz/Scene/Selection.cpp
        include/Slic3r/Biz/Scene/SceneInteractorProjectContext.hpp
        include/Slic3r/Biz/Scene/BedPlacement.hpp
        src/Slic3r/Biz/Scene/BedPlacement.cpp
        src/Slic3r/Biz/Scene/BedTracking.hpp
        src/Slic3r/Biz/Scene/BedTracking.cpp

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

        include/Slic3r/Biz/IProjectsChangedListener.hpp
        include/Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp
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
        include/Slic3r/Domain/BedRef.hpp
        include/Slic3r/Domain/BedInstance.hpp
        src/Slic3r/Domain/BedInstance.cpp
        include/Slic3r/Domain/BedContainer.hpp
        src/Slic3r/Domain/BedContainer.cpp
        src/Slic3r/Domain/Workbench.cpp
        include/Slic3r/Domain/Workbench.hpp
        src/Slic3r/Domain/ConfigContainer.cpp
        include/Slic3r/Domain/ConfigContainer.hpp

        src/Slic3r/Biz/Units.cpp
        include/Slic3r/Biz/Units.hpp
        src/Slic3r/Biz/Units.cpp
        include/Slic3r/Biz/Units.hpp

        include/Slic3r/Biz/Slicing/BackgroundProcess.hpp
        include/Slic3r/Biz/Slicing/SlicingInteractor.hpp
        include/Slic3r/Biz/Slicing/ModelUtils.hpp
        src/Slic3r/Biz/Slicing/BackgroundProcess.cpp
        src/Slic3r/Biz/Slicing/SlicingInteractor.cpp
        src/Slic3r/Biz/Slicing/ModelUtils.cpp
)

find_package(NanoSVG REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(yoga CONFIG REQUIRED)

slic3r_remap_configs("yoga::yogacore" RelWithDebInfo Release)


add_library(slic3r-shared STATIC ${SLIC3R_SHARED_FILES})
target_link_libraries(slic3r-shared PUBLIC
        slic3r-base
        slic3r-platform
        slic3r-render
        libslic3r
        libpgcode
        Boost::filesystem
        fmt::fmt
        yoga::yogacore
)
if (EMSCRIPTEN)
#    target_link_libraries(slic3r-shared PUBLIC png-mt )
else()
    find_package(PNG REQUIRED)
    target_link_libraries(slic3r-shared PUBLIC PNG::PNG libvgcode)
endif ()
target_include_directories(slic3r-shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (NOT WIN32)
    if (SLIC3R_FHS)
        # CMAKE_INSTALL_FULL_DATAROOTDIR: read-only architecture-independent data root (share)
        set(SLIC3R_FHS_RESOURCES "${CMAKE_INSTALL_FULL_DATAROOTDIR}/PrusaSlicer")
        install(DIRECTORY ${SLIC3R_RESOURCES_NEXT_DIR}/ DESTINATION ${SLIC3R_FHS_RESOURCES})
        target_compile_definitions(slic3r-shared PUBLIC SLIC3R_FHS SLIC3R_FHS_RESOURCES="${SLIC3R_FHS_RESOURCES}")
    else ()
        install(DIRECTORY "${SLIC3R_RESOURCES_NEXT_DIR}/" DESTINATION "${CMAKE_INSTALL_PREFIX}/resources")
    endif ()
endif()

add_target_to_doxygen(slic3r-shared)

if(SLIC3R_BUILD_TESTS AND NOT EMSCRIPTEN)
    add_executable(slic3r-shared-tests
        test/main.cpp

        test/Slic3r/App/Scene/Node.cpp
        test/Slic3r/App/Scene/Ray.cpp
        test/Slic3r/Biz/ProjectInteractorTests.cpp
        test/Slic3r/Biz/Scene/SceneInteractorTests.cpp

        test/Slic3r/Biz/Slicing/TestUtils.cpp
        test/Slic3r/Biz/Slicing/GCodeUtils.cpp
        test/Slic3r/Biz/Slicing/ActionsTests.cpp
        test/Slic3r/Biz/Slicing/ListenersTests.cpp
        test/Slic3r/Biz/Slicing/ProcessActionsTests.cpp
        test/Slic3r/Biz/Slicing/ModelUtilsTests.cpp
    )
    target_include_directories(slic3r-shared-tests PRIVATE test)

    find_package(Catch2 3.8 REQUIRED)
    slic3r_remap_configs("Catch2::Catch2" RelWithDebInfo Release)
    target_link_libraries(slic3r-shared-tests PRIVATE slic3r-shared Catch2::Catch2)

    set(TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/data)
    file(TO_NATIVE_PATH "${TEST_DATA_DIR}" TEST_DATA_DIR)
    target_compile_definitions(slic3r-shared-tests PRIVATE TEST_DATA_DIR=R"\(${TEST_DATA_DIR}\)")

    set(_catch_args "exclude:[NotWorking] exclude:[Slow]")
    list(APPEND _catch_args "${CATCH_EXTRA_ARGS}")
    add_test(NAME slic3r-shared-tests
        COMMAND slic3r-shared-tests ${_catch_args}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if (WIN32)
        prusaslicer_copy_dlls(slic3r-shared-tests)
    endif ()
endif()
