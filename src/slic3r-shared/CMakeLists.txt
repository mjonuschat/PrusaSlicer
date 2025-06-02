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
        include/Slic3r/App/Plater/SelectionHandler.hpp
        src/Slic3r/App/Plater/SelectionHandler.cpp
        include/Slic3r/App/Plater/TranslationGizmo.hpp
        src/Slic3r/App/Plater/TranslationGizmo.cpp
        include/Slic3r/App/Plater/RotationGizmo.hpp
        src/Slic3r/App/Plater/RotationGizmo.cpp
        include/Slic3r/App/Plater/PlaterSceneLayer.hpp
        include/Slic3r/App/Plater/PlaterRenderLayout.hpp
        src/Slic3r/App/Plater/PlaterRenderLayout.cpp
        include/Slic3r/App/Plater/History.hpp
        src/Slic3r/App/Plater/History.cpp
        include/Slic3r/App/Plater/SidebarPlaterActionButtons.hpp
        src/Slic3r/App/Plater/SidebarPlaterActionButtons.cpp
        include/Slic3r/App/Plater/PlaterIFDMResultCacheChangedListener.hpp
        include/Slic3r/App/Plater/PaintOnSupportsGizmo.hpp
        src/Slic3r/App/Plater/PaintOnSupportsGizmo.cpp
        src/Slic3r/App/Plater/PaintOnSupportsDialog.hpp
        src/Slic3r/App/Plater/PaintOnSupportsDialog.cpp

        include/Slic3r/App/View/IPanelView.hpp

        include/Slic3r/App/Preview/PreviewRenderModule.hpp
        src/Slic3r/App/Preview/PreviewRenderModule.cpp
        include/Slic3r/App/Preview/PreviewCameraGizmo.hpp
        include/Slic3r/App/Preview/PreviewScenePresenter.hpp
        src/Slic3r/App/Preview/PreviewScenePresenter.cpp
        include/Slic3r/App/Preview/PreviewSceneLayer.hpp
        include/Slic3r/App/Preview/PreviewSceneRenderCustomizer.hpp
        src/Slic3r/App/Preview/PreviewSceneRenderCustomizer.cpp
        include/Slic3r/App/Preview/PreviewRenderLayout.hpp
        src/Slic3r/App/Preview/PreviewRenderLayout.cpp
        include/Slic3r/App/Preview/SidebarAutoReslice.hpp
        src/Slic3r/App/Preview/SidebarAutoReslice.cpp
        include/Slic3r/App/Preview/SidebarPreviewActionButtons.hpp
        src/Slic3r/App/Preview/SidebarPreviewActionButtons.cpp

        include/Slic3r/App/MultiSelections.hpp
        src/Slic3r/App/MultiSelections.cpp
        include/Slic3r/App/ObjectList.hpp
        src/Slic3r/App/ObjectList.cpp
        include/Slic3r/App/CubeView.hpp
        src/Slic3r/App/CubeView.cpp
        include/Slic3r/App/SidebarBed.hpp
        src/Slic3r/App/SidebarBed.cpp
        include/Slic3r/App/SidebarPrint.hpp
        src/Slic3r/App/SidebarPrint.cpp
        include/Slic3r/App/SidebarActionButtons.hpp
        src/Slic3r/App/SidebarActionButtons.cpp
        include/Slic3r/App/LightSetting.hpp
        src/Slic3r/App/LightSetting.cpp
        include/Slic3r/App/ExportPathSelect.hpp
        src/Slic3r/App/ExportPathSelect.cpp
        include/Slic3r/App/IDialogManager.hpp
        src/Slic3r/App/BedStore.cpp
        include/Slic3r/App/BedStore.hpp
        include/Slic3r/App/MaterialState.hpp

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
        src/Slic3r/App/Scene/IGizmo.cpp
        include/Slic3r/App/Scene/GizmoEventContext.hpp
        include/Slic3r/App/Scene/AbstractCameraGizmo.hpp
        src/Slic3r/App/Scene/AbstractCameraGizmo.cpp
        include/Slic3r/App/Scene/GizmoManager.hpp
        src/Slic3r/App/Scene/GizmoManager.cpp
        include/Slic3r/App/Scene/GeometryDataFactory.hpp
        src/Slic3r/App/Scene/GeometryDataFactory.cpp
        include/Slic3r/App/Scene/ISceneProvider.hpp
        include/Slic3r/App/Scene/ScenePresenterProjectContext.hpp
        src/Slic3r/App/Scene/Lights.cpp
        include/Slic3r/App/Scene/Lights.hpp
        include/Slic3r/App/Scene/LightingHelper.hpp
        src/Slic3r/App/Scene/LightingHelper.cpp
        include/Slic3r/App/Scene/BedNodeBuilder.hpp
        src/Slic3r/App/Scene/BedNodeBuilder.cpp
        include/Slic3r/App/Scene/BedNodeTag.hpp
        include/Slic3r/App/Scene/BedMaterials.hpp
        src/Slic3r/App/Scene/BedMaterials.cpp
        include/Slic3r/App/Scene/BedRenderHelper.hpp
        src/Slic3r/App/Scene/BedRenderHelper.cpp
        include/Slic3r/App/Scene/BedRenderUpdater.hpp
        src/Slic3r/App/Scene/BedRenderUpdater.cpp
        include/Slic3r/App/Scene/AuxiliaryElementId.hpp

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

        include/Slic3r/App/IRenderModuleChangedListener.hpp
        src/Slic3r/App/Navigator.cpp
        include/Slic3r/App/Navigator.hpp

        src/Slic3r/App/Yoga/FlexSizer.cpp
        include/Slic3r/App/Yoga/FlexSizer.hpp
        src/Slic3r/App/Yoga/SplitterSizer.cpp
        include/Slic3r/App/Yoga/SplitterSizer.hpp
        include/Slic3r/App/Yoga/Toolbar.hpp
        src/Slic3r/App/Yoga/Toolbar.cpp
        include/Slic3r/App/Yoga/Namespace.hpp
        include/Slic3r/App/Yoga/WindowParams.hpp
        include/Slic3r/App/Yoga/Item.hpp
        src/Slic3r/App/Yoga/Item.cpp
        src/Slic3r/App/Yoga/Window.cpp
        include/Slic3r/App/Yoga/Window.hpp
        include/Slic3r/App/Yoga/LambdaItem.hpp
        src/Slic3r/App/Yoga/LambdaItem.cpp
        include/Slic3r/App/Yoga/LambdaWindow.hpp
        src/Slic3r/App/Yoga/LambdaWindow.cpp
        include/Slic3r/App/Yoga/AbstractButton.hpp
        src/Slic3r/App/Yoga/AbstractButton.cpp
        include/Slic3r/App/Yoga/ToolbarButton.hpp
        src/Slic3r/App/Yoga/ToolbarButton.cpp
        include/Slic3r/App/Yoga/RectangleButton.hpp
        src/Slic3r/App/Yoga/RectangleButton.cpp
        include/Slic3r/App/Yoga/LayoutButton.hpp
        src/Slic3r/App/Yoga/LayoutButton.cpp
        include/Slic3r/App/Yoga/Tooltip.hpp
        src/Slic3r/App/Yoga/Tooltip.cpp
        include/Slic3r/App/Yoga/Rectangle.hpp
        src/Slic3r/App/Yoga/Rectangle.cpp
        include/Slic3r/App/Yoga/Text.hpp
        include/Slic3r/App/Yoga/Icon.hpp
        src/Slic3r/App/Yoga/Icon.cpp
        src/Slic3r/App/Yoga/Text.cpp
        include/Slic3r/App/Yoga/Dialog.hpp
        src/Slic3r/App/Yoga/Dialog.cpp
        include/Slic3r/App/Yoga/AttachedWindow.hpp
        src/Slic3r/App/Yoga/AttachedWindow.cpp
        src/Slic3r/App/Yoga/Separator.cpp
        include/Slic3r/App/Yoga/Separator.hpp
        src/Slic3r/App/Yoga/Oval.cpp
        include/Slic3r/App/Yoga/Oval.hpp
        src/Slic3r/App/Yoga/Circle.cpp
        include/Slic3r/App/Yoga/Circle.hpp
        src/Slic3r/App/Yoga/Toggler.cpp
        include/Slic3r/App/Yoga/Toggler.hpp
        src/Slic3r/App/Yoga/ToggleButton.cpp
        include/Slic3r/App/Yoga/ToggleButton.hpp
        src/Slic3r/App/Yoga/MenuItem.cpp
        include/Slic3r/App/Yoga/MenuItem.hpp
        src/Slic3r/App/Yoga/Menu.cpp
        include/Slic3r/App/Yoga/Menu.hpp
        src/Slic3r/App/Yoga/PrinterSettingsButton.cpp
        include/Slic3r/App/Yoga/PrinterSettingsButton.hpp
        src/Slic3r/App/Yoga/MaterialSettingsButton.cpp
        include/Slic3r/App/Yoga/MaterialSettingsButton.hpp
        include/Slic3r/App/Yoga/ButtonGroup.hpp
        src/Slic3r/App/Yoga/ButtonGroup.cpp
	include/Slic3r/App/Yoga/ListView.hpp

        src/Slic3r/App/Imgui/DoubleSlider.cpp
        include/Slic3r/App/Imgui/DoubleSlider.hpp
        src/Slic3r/App/Imgui/RulerForDoubleSlider.cpp
        include/Slic3r/App/Imgui/RulerForDoubleSlider.hpp
        src/Slic3r/App/Imgui/ImguiExtension.cpp
        include/Slic3r/App/Imgui/ImguiExtension.hpp

        src/Slic3r/App/Preview/Types.cpp
        include/Slic3r/App/Preview/Types.hpp
        src/Slic3r/App/Preview/FdmViewerWrapper.cpp
        include/Slic3r/App/Preview/FdmViewerWrapper.hpp
        include/Slic3r/App/Preview/FdmViewerWrapperInputData.hpp
        src/Slic3r/App/Preview/SlaViewerWrapper.cpp
        include/Slic3r/App/Preview/SlaViewerWrapper.hpp
        include/Slic3r/App/Preview/SlaViewerWrapperInputData.hpp
        src/Slic3r/App/Preview/DoubleSliderForLayers.cpp
        include/Slic3r/App/Preview/DoubleSliderForLayers.hpp
        src/Slic3r/App/Preview/DoubleSliderForGCode.cpp
        include/Slic3r/App/Preview/DoubleSliderForGCode.hpp
        src/Slic3r/App/Preview/ExtrudersSequence.cpp
        include/Slic3r/App/Preview/ExtrudersSequence.hpp
        src/Slic3r/App/Preview/TickCodeManager.cpp
        include/Slic3r/App/Preview/TickCodeManager.hpp
        src/Slic3r/App/Preview/ActualSpeedPlotter.cpp
        include/Slic3r/App/Preview/ActualSpeedPlotter.hpp
        src/Slic3r/App/Preview/GCodeWindow.cpp
        include/Slic3r/App/Preview/GCodeWindow.hpp
        src/Slic3r/App/Preview/Legend.cpp
        include/Slic3r/App/Preview/Legend.hpp
        src/Slic3r/App/Preview/AbstractViewerWrapper.cpp
        include/Slic3r/App/Preview/AbstractViewerWrapper.hpp

        src/Slic3r/App/AbstractRenderLayout.cpp
        include/Slic3r/App/AbstractRenderLayout.hpp
        include/Slic3r/App/ResourceResolver.hpp
        src/Slic3r/App/ResourceResolver.cpp

        include/Slic3r/Biz/PrintConfigInteractor.hpp
        src/Slic3r/Biz/PrintConfigInteractor.cpp
        include/Slic3r/Biz/ObservableDynamicPrintConfig.hpp
        include/Slic3r/Biz/Observable.hpp
        include/Slic3r/Biz/Plater/ConfigViewModel.hpp
        include/Slic3r/Biz/IObservableList.hpp
        include/Slic3r/Biz/ObservableList.hpp
        include/Slic3r/Biz/IListObserver.hpp
        include/Slic3r/Biz/DataObserver.hpp

        include/Slic3r/Biz/Scene/SceneInteractor.hpp
        src/Slic3r/Biz/Scene/SceneInteractor.cpp
        include/Slic3r/Biz/Scene/Selection.hpp
        src/Slic3r/Biz/Scene/Selection.cpp
        include/Slic3r/Biz/Scene/SceneInteractorProjectContext.hpp
        include/Slic3r/Biz/Scene/BedPlacement.hpp
        src/Slic3r/Biz/Scene/BedPlacement.cpp
        include/Slic3r/Biz/Scene/BedGeometry.hpp
        src/Slic3r/Biz/Scene/BedGeometry.cpp
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

#		include/Slic3r/Biz/Preset/Bundle.hpp
#		src/Slic3r/Biz/Preset/Bundle.cpp

        include/Slic3r/Biz/Preset/IO/PresetLoader.hpp
        src/Slic3r/Biz/Preset/IO/PresetLoader.cpp
        include/Slic3r/Biz/Preset/IO/HwConfigLoader.hpp
        src/Slic3r/Biz/Preset/IO/HwConfigLoader.cpp
        include/Slic3r/Biz/Preset/IO/BundleLoader.hpp
        src/Slic3r/Biz/Preset/IO/BundleLoader.cpp
        src/Slic3r/Biz/Yaml/Yaml.hpp
        src/Slic3r/Biz/Yaml/Yaml.cpp
        src/Slic3r/Biz/Yaml/YamlAdapter.hpp
        src/Slic3r/Biz/Yaml/YamlSlic3rTypes.hpp

		include/Slic3r/Biz/Preset/PresetEvaluator.hpp
		src/Slic3r/Biz/Preset/PresetEvaluator.cpp
		src/Slic3r/Biz/Preset/PresetCollectionEvaluator.hpp
		src/Slic3r/Biz/Preset/PresetCollectionEvaluator.cpp
		include/Slic3r/Biz/Preset/HwConfigEvaluator.hpp
		src/Slic3r/Biz/Preset/HwConfigEvaluator.cpp
		src/Slic3r/Biz/Preset/ValueMapBuilder.hpp
		src/Slic3r/Biz/Preset/ValueMapBuilder.cpp

        include/Slic3r/Biz/Expr/Parser.hpp
        src/Slic3r/Biz/Expr/Parser.cpp
        include/Slic3r/Biz/Expr/Eval.hpp
        src/Slic3r/Biz/Expr/Eval.cpp

        include/Slic3r/Biz/IProjectsChangedListener.hpp
        include/Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp
        include/Slic3r/Biz/ISelectedConfigContainerChangedListener.hpp
        include/Slic3r/Biz/ISelectedProjectChangedListener.hpp
        src/Slic3r/Biz/ProjectInteractor.cpp
        include/Slic3r/Biz/ProjectInteractor.hpp
        include/Slic3r/Biz/ProjectScoped.hpp
        include/Slic3r/Biz/LastExportPathStorage.hpp

        include/Slic3r/StringUtils.hpp

        src/Slic3r/Domain/Project.cpp
        include/Slic3r/Domain/Project.hpp
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
        src/Slic3r/Biz/Slicing/BackgroundProcess.cpp
        src/Slic3r/Biz/Slicing/SlicingInteractor.cpp

        include/Slic3r/Biz/FDMResultCache.hpp
        src/Slic3r/Biz/FDMResultCache.cpp

        include/Slic3r/Biz/StatusCache.hpp
        src/Slic3r/Biz/StatusCache.cpp

        include/Slic3r/Biz/PrintHost/IPrintHost.hpp
        include/Slic3r/Biz/PrintHost/IPrintHostListener.hpp
        include/Slic3r/Biz/PrintHost/IPrintHostJobCallbacks.hpp
        include/Slic3r/Biz/PrintHost/PrintHostJob.hpp
        include/Slic3r/Biz/PrintHost/PrintHostJobManager.hpp
        include/Slic3r/Biz/PrintHost/PrintHostInteractor.hpp
        include/Slic3r/Biz/PrintHost/PrintHostConfig.hpp
        include/Slic3r/Biz/PrintHost/PrintHostFactory.hpp
        include/Slic3r/Biz/PrintHost/PrintHostLocal.hpp
        include/Slic3r/Biz/PrintHost/PrintHostOctoPrint.hpp
        include/Slic3r/Biz/PrintHost/PrintHostPrusaLink.hpp
        include/Slic3r/Biz/PrintHost/PrintHostPrusaLinkStorage.hpp
        include/Slic3r/Biz/PrintHost/PrintHostPrusaConnect.hpp
        include/Slic3r/Biz/PrintHost/PrintHostSL1Host.hpp
        include/Slic3r/Biz/PrintHost/PrintHostMoonraker.hpp
        include/Slic3r/Biz/PrintHost/PrintHostDuet.hpp
        include/Slic3r/Biz/PrintHost/PrintHostFlashAir.hpp
        include/Slic3r/Biz/PrintHost/PrintHostAstroBox.hpp
        include/Slic3r/Biz/PrintHost/PrintHostRepetier.hpp
        include/Slic3r/Biz/PrintHost/PrintHostMKS.hpp
        include/Slic3r/Biz/PrintHost/PrintHostDataFinalizer.hpp
        src/Slic3r/Biz/PrintHost/PrintHostJobManager.cpp
        src/Slic3r/Biz/PrintHost/PrintHostInteractor.cpp
        src/Slic3r/Biz/PrintHost/PrintHostJob.cpp
        src/Slic3r/Biz/PrintHost/PrintHostFactory.cpp
        src/Slic3r/Biz/PrintHost/PrintHostLocal.cpp
        src/Slic3r/Biz/PrintHost/PrintHostOctoPrint.cpp
        src/Slic3r/Biz/PrintHost/PrintHostPrusaLink.cpp
        src/Slic3r/Biz/PrintHost/PrintHostPrusaLinkStorage.cpp
        src/Slic3r/Biz/PrintHost/PrintHostPrusaConnect.cpp
        src/Slic3r/Biz/PrintHost/PrintHostSL1Host.cpp
        src/Slic3r/Biz/PrintHost/PrintHostMoonraker.cpp
        src/Slic3r/Biz/PrintHost/PrintHostDuet.cpp
        src/Slic3r/Biz/PrintHost/PrintHostFlashAir.cpp
        src/Slic3r/Biz/PrintHost/PrintHostAstroBox.cpp
        src/Slic3r/Biz/PrintHost/PrintHostRepetier.cpp
        src/Slic3r/Biz/PrintHost/PrintHostMKS.cpp
        src/Slic3r/Biz/PrintHost/PrintHostDataFinalizer.cpp

        include/Slic3r/Biz/Network/IHttp.hpp
        include/Slic3r/Biz/Network/HttpFactory.hpp
        include/Slic3r/Biz/Network/HttpCurl.hpp
        include/Slic3r/Biz/Network/Bonjour.hpp
        include/Slic3r/Biz/Network/TCPConsole.hpp
        include/Slic3r/Biz/Network/ServiceConfig.hpp
        include/Slic3r/Biz/Network/Jwt.hpp        
        src/Slic3r/Biz/Network/IHttp.cpp
        src/Slic3r/Biz/Network/HttpCurl.cpp
        src/Slic3r/Biz/Network/HttpCurlFactory.cpp
        src/Slic3r/Biz/Network/Bonjour.cpp
        src/Slic3r/Biz/Network/TCPConsole.cpp
        src/Slic3r/Biz/Network/ServiceConfig.cpp
        src/Slic3r/Biz/Network/Jwt.cpp

        include/Slic3r/Biz/SLAResultCache.hpp
        src/Slic3r/Biz/SLAResultCache.cpp
        include/Slic3r/Biz/SLAObjectCache.hpp
        src/Slic3r/Biz/SLAObjectCache.cpp

        include/Slic3r/Biz/UserAccount/IUserAccountListener.hpp
        include/Slic3r/Biz/UserAccount/IUserAccountSessionListener.hpp
        include/Slic3r/Biz/UserAccount/IUserAccountActionCallbacks.hpp
        include/Slic3r/Biz/UserAccount/UserAccountInteractor.hpp
        include/Slic3r/Biz/UserAccount/UserAccountCommunication.hpp
        include/Slic3r/Biz/UserAccount/UserAccountSession.hpp
        include/Slic3r/Biz/UserAccount/UserAccountSessionDispatchBase.hpp
        include/Slic3r/Biz/UserAccount/UserAccountAction.hpp
        include/Slic3r/Biz/UserAccount/UserAccountActionStore.hpp
        include/Slic3r/Biz/UserAccount/UserAccountCommunicationTokenBase.hpp
        include/Slic3r/Biz/UserAccount/UserAccountTokenStore.hpp
        include/Slic3r/Biz/UserAccount/UserAccountCodeChallengeGenerator.hpp
        include/Slic3r/Biz/UserAccount/ConnectMessageHandler.hpp
        src/Slic3r/Biz/UserAccount/UserAccountInteractor.cpp
        src/Slic3r/Biz/UserAccount/UserAccountCommunication.cpp
        src/Slic3r/Biz/UserAccount/UserAccountSession.cpp
        src/Slic3r/Biz/UserAccount/UserAccountSessionDispatchBase.cpp
        src/Slic3r/Biz/UserAccount/UserAccountAction.cpp
        src/Slic3r/Biz/UserAccount/UserAccountCommunicationTokenBase.cpp
        src/Slic3r/Biz/UserAccount/UserAccountTokenStore.cpp
        src/Slic3r/Biz/UserAccount/UserAccountCodeChallengeGenerator.cpp
        src/Slic3r/Biz/UserAccount/ConnectMessageHandler.cpp

        include/Slic3r/Biz/SHA256.hpp

        include/Slic3r/Biz/AppInstance/AbstractAppInstanceMessageHandler.hpp
        include/Slic3r/Biz/AppInstance/IAppInstanceMessageContentListener.hpp
        include/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerFactory.hpp
        src/Slic3r/Biz/AppInstance/AbstractAppInstanceMessageHandler.cpp
        include/Slic3r/Biz/Config/ConfigSerialize.hpp
        include/Slic3r/Biz/Config/ConfigLegacy.hpp
        src/Slic3r/Biz/Config/ConfigSerialize.cpp
        src/Slic3r/Biz/Config/Legacy/Config.hpp
        src/Slic3r/Biz/Config/Legacy/Config.cpp
        src/Slic3r/Biz/Config/Legacy/PrintConfig.hpp
        src/Slic3r/Biz/Config/Legacy/PrintConfig.cpp
        src/Slic3r/Biz/Config/Legacy/Point.hpp
        src/Slic3r/Biz/Config/Legacy/Point.cpp
        src/Slic3r/Biz/Config/Legacy/clonable_ptr.hpp
        src/Slic3r/Biz/Config/Legacy/Thumbnails.hpp
        src/Slic3r/Biz/Config/Legacy/Thumbnails.cpp
        src/Slic3r/Biz/Config/Legacy/enum_bitmask.hpp
        src/Slic3r/Biz/Config/ConfigLegacy.cpp
        src/Slic3r/Biz/Config/Legacy/3mf_legacy.cpp
        include/Slic3r/Biz/Config/3mf_legacy.hpp
)

if (WIN32)
    list(APPEND SLIC3R_SHARED_FILES
        src/Slic3r/Biz/SHA256Win32.cpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerWin32.hpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerWin32.cpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerFactoryWin32.cpp

    )
elseif (APPLE)
    list(APPEND SLIC3R_SHARED_FILES
        src/Slic3r/Biz/SHA256Mac.cpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerMac.hpp
        include/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerMacWrapper.h
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerMac.mm
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerFactoryMac.cpp
        include/Slic3r/Biz/AppInstance/SingleInstanceCheckerLock.hpp
        src/Slic3r/Biz/AppInstance/SingleInstanceCheckerLock.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND SLIC3R_SHARED_FILES
        src/Slic3r/Biz/SHA256Linux.cpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerLinux.hpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerLinux.cpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerFactoryLinux.cpp
        include/Slic3r/Biz/AppInstance/SingleInstanceCheckerLock.hpp
        src/Slic3r/Biz/AppInstance/SingleInstanceCheckerLock.cpp
    )
endif ()

find_package(NanoSVG REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(yoga CONFIG REQUIRED)
find_package(nlohmann_json 3.11 REQUIRED)
find_package(EXPAT CONFIG REQUIRED)


if ("${SLIC3R_YAML}" STREQUAL "libfyaml")
    find_package(fyaml REQUIRED)
    set(YAML_LIB fyaml::fyaml)
    set(SLIC3R_SHARED_FILES
            ${SLIC3R_SHARED_FILES}
            src/Slic3r/Yaml/YamlAdapterLibfyaml.hpp
            src/Slic3r/Yaml/YamlAdapterLibfyaml.cpp
    )
    set(SLIC3R_YAML_DEFINE "-DSLIC3R_YAML_LIBFYAML")
elseif ("${SLIC3R_YAML}" STREQUAL "yaml-cpp")
    find_package(yaml-cpp REQUIRED)
    set(YAML_LIB yaml-cpp::yaml-cpp)
    set(SLIC3R_SHARED_FILES
            ${SLIC3R_SHARED_FILES}
            src/Slic3r/Biz/Yaml/YamlAdapterYamlCpp.hpp
            src/Slic3r/Biz/Yaml/YamlAdapterYamlCpp.cpp
    )
    set(SLIC3R_YAML_DEFINE "-DSLIC3R_YAML_YAMLCPP")
else ()
    message(FATAL_ERROR "No YAML library selected, allowed values are \"libfyaml\", \"yaml-cpp\", passed \"${SLIC}\"")
endif ()


find_package(magic_enum REQUIRED)

slic3r_remap_configs("yoga::yogacore;${YAML_LIB}" RelWithDebInfo Release)

add_library(slic3r-shared STATIC ${SLIC3R_SHARED_FILES})
target_link_libraries(slic3r-shared PUBLIC
        slic3r-platform
        slic3r-render
        libvgcode
        libslic3r
        libpgcode
        Boost::filesystem
        fmt::fmt
        yoga::yogacore
        slic3r-biz-algorithms
        nlohmann_json::nlohmann_json
        ${YAML_LIB}
        magic_enum::magic_enum
        fastfloat
        expat::expat
)
target_compile_definitions(slic3r-shared PUBLIC ${SLIC3R_YAML_DEFINE})
if (EMSCRIPTEN)
#    target_link_libraries(slic3r-shared PUBLIC png-mt )
else()
    find_package(CURL REQUIRED)

    add_library(libcurl INTERFACE)
    target_link_libraries(libcurl INTERFACE CURL::libcurl)

    # Fixing curl's cmake config script bugs
    if (NOT WIN32)
        # Required by libcurl
        find_package(ZLIB REQUIRED)
        target_link_libraries(libcurl INTERFACE ZLIB::ZLIB)
    else()
        target_link_libraries(libcurl INTERFACE crypt32)
    endif()

    target_link_libraries(slic3r-shared PUBLIC PNG::PNG libvgcode libcurl)
endif ()
target_include_directories(slic3r-shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(DBus1 REQUIRED)
    target_link_libraries(slic3r-shared PUBLIC dbus-1)
endif()

if (NOT WIN32)
    if (SLIC3R_FHS)
        # CMAKE_INSTALL_FULL_DATAROOTDIR: read-only architecture-independent data root (share)
        set(SLIC3R_FHS_RESOURCES "${CMAKE_INSTALL_FULL_DATAROOTDIR}/PrusaSlicer")
        install(DIRECTORY ${SLIC3R_RESOURCES_DIR}/ DESTINATION ${SLIC3R_FHS_RESOURCES})
        target_compile_definitions(slic3r-shared PUBLIC SLIC3R_FHS SLIC3R_FHS_RESOURCES="${SLIC3R_FHS_RESOURCES}")
    else ()
        install(DIRECTORY "${SLIC3R_RESOURCES_DIR}/" DESTINATION "${CMAKE_INSTALL_PREFIX}/resources")
    endif ()
endif()

if (SLIC3R_REPO_URL)
    target_compile_definitions(slic3r-shared PRIVATE SLIC3R_REPO_URL="${SLIC3R_REPO_URL}")
endif()

add_target_to_doxygen(slic3r-shared)

if(SLIC3R_BUILD_TESTS AND NOT EMSCRIPTEN)
    add_executable(slic3r-shared-tests
        test/main.cpp

        test/Slic3r/App/Scene/Node.cpp
        test/Slic3r/App/Scene/Ray.cpp

        test/Slic3r/App/Yoga/ListViewTests.cpp
        test/Slic3r/App/Yoga/LayoutTests.cpp
        test/Slic3r/App/Yoga/ItemTreeTests.cpp
        test/Slic3r/App/Yoga/ImGuiFixture.hpp
        test/Slic3r/App/Yoga/ButtonGroupTests.cpp

        test/Slic3r/Biz/ProjectInteractorTests.cpp
        test/Slic3r/Biz/Scene/SceneInteractorTests.cpp

        test/Slic3r/Biz/Expr/ParserTests.cpp
        test/Slic3r/Biz/Expr/EvalTests.cpp

        test/Slic3r/Biz/Yaml/YamlTests.cpp
        test/Slic3r/Biz/Preset/IO/HwConfigDefLoaderTests.cpp
        test/Slic3r/Biz/Preset/IO/PresetLoaderTests.cpp
		test/Slic3r/Biz/Preset/PresetCollectionEvaluatorTests.cpp
		test/Slic3r/Biz/Preset/PresetEvaluatorTests.cpp
		test/Slic3r/Biz/Preset/HwConfigEvaluatorTests.cpp

		test/Slic3r/Biz/Slicing/TestUtils.cpp
        test/Slic3r/Biz/Slicing/GCodeUtils.cpp
        test/Slic3r/Biz/Slicing/ActionsTests.cpp
        test/Slic3r/Biz/Slicing/ListenersTests.cpp
        test/Slic3r/Biz/Slicing/ProcessActionsTests.cpp
    )
    target_include_directories(slic3r-shared-tests PRIVATE test src)

    target_link_libraries(slic3r-shared-tests PRIVATE slic3r-shared Catch2::Catch2)

    set(TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/data)
    file(TO_NATIVE_PATH "${TEST_DATA_DIR}" TEST_DATA_DIR)
    target_compile_definitions(slic3r-shared-tests PRIVATE TEST_DATA_DIR=R"\(${TEST_DATA_DIR}\)")

    add_test(NAME slic3r-shared-tests
        COMMAND slic3r-shared-tests ${CATCH_ALL_ARGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if (WIN32)
        prusaslicer_copy_dlls(slic3r-shared-tests)
    endif ()
endif()
