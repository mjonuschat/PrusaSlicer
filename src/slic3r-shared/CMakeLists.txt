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
        include/Slic3r/App/Plater/SimplifyGizmo.hpp
        src/Slic3r/App/Plater/SimplifyGizmo.cpp
        include/Slic3r/App/Plater/SimplifyDialog.hpp
        src/Slic3r/App/Plater/SimplifyDialog.cpp
        include/Slic3r/App/Plater/ArrangeDialog.hpp
        src/Slic3r/App/Plater/ArrangeDialog.cpp
        include/Slic3r/App/Plater/ArrangeGizmo.hpp
        src/Slic3r/App/Plater/ArrangeGizmo.cpp
        include/Slic3r/App/Plater/PlaterSceneLayer.hpp
        include/Slic3r/App/Plater/PlaterRenderLayout.hpp
        src/Slic3r/App/Plater/PlaterRenderLayout.cpp
        include/Slic3r/App/Plater/History.hpp
        src/Slic3r/App/Plater/History.cpp
        include/Slic3r/App/Plater/SidebarPlaterActionButtons.hpp
        src/Slic3r/App/Plater/SidebarPlaterActionButtons.cpp
        include/Slic3r/App/Plater/PaintOnSupportsGizmo.hpp
        src/Slic3r/App/Plater/PaintOnSupportsGizmo.cpp
        include/Slic3r/App/Plater/PaintOnSupportsDialog.hpp
        src/Slic3r/App/Plater/PaintOnSupportsDialog.cpp
        include/Slic3r/App/Plater/TextGizmo.hpp
        src/Slic3r/App/Plater/TextGizmo.cpp
        include/Slic3r/App/Plater/TextDialog.hpp
        src/Slic3r/App/Plater/TextDialog.cpp
        include/Slic3r/App/Plater/MeasureGizmo.hpp
        src/Slic3r/App/Plater/MeasureGizmo.cpp
        include/Slic3r/App/Plater/MeasureGizmoHelper.hpp
        include/Slic3r/App/Plater/MeasureDialog.hpp
        src/Slic3r/App/Plater/MeasureDialog.cpp
        include/Slic3r/App/Plater/ThumbnailRenderer.hpp
        src/Slic3r/App/Plater/ThumbnailRenderer.cpp
        include/Slic3r/App/Plater/IBedVisuallyChangedListener.hpp
        include/Slic3r/App/Plater/BedThumbnailTextureRequest.hpp
        include/Slic3r/App/Plater/ScopedBedThumbnailSceneCustomizer.hpp
        src/Slic3r/App/Plater/ScopedBedThumbnailSceneCustomizer.cpp
        include/Slic3r/App/Plater/Scoped3mfThumbnailSceneCustomizer.hpp
        src/Slic3r/App/Plater/Scoped3mfThumbnailSceneCustomizer.cpp
        include/Slic3r/App/Plater/ScopedGCodeThumbnailSceneCustomizer.hpp
        src/Slic3r/App/Plater/ScopedGCodeThumbnailSceneCustomizer.cpp
        include/Slic3r/App/Plater/BedThumbnailTexture.hpp
        include/Slic3r/App/Plater/ThumbnailImageGenerator.hpp
        src/Slic3r/App/Plater/ThumbnailImageGenerator.cpp
        include/Slic3r/App/Plater/SurfaceMesh.hpp
        include/Slic3r/App/Plater/MeasureUtils.hpp
        include/Slic3r/App/Plater/Measure.hpp
        src/Slic3r/App/Plater/Measure.cpp
        include/Slic3r/App/Plater/SinkingContours.hpp
        src/Slic3r/App/Plater/SinkingContours.cpp
        include/Slic3r/App/Plater/SinkingContoursHelper.hpp
        include/Slic3r/App/Plater/PlaterScenePresenterProjectContext.hpp

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
        include/Slic3r/App/ObjectListWindow.hpp
        src/Slic3r/App/ObjectListWindow.cpp
        include/Slic3r/App/ObjectList.hpp
        src/Slic3r/App/ObjectList.cpp
        include/Slic3r/App/CubeView.hpp
        src/Slic3r/App/CubeView.cpp
        include/Slic3r/App/SidebarBed.hpp
        src/Slic3r/App/SidebarBed.cpp
        include/Slic3r/App/SidebarPrint.hpp
        src/Slic3r/App/SidebarPrint.cpp
        include/Slic3r/App/SidebarObject.hpp
        src/Slic3r/App/SidebarObject.cpp
        include/Slic3r/App/SidebarActionButtons.hpp
        src/Slic3r/App/SidebarActionButtons.cpp
        include/Slic3r/App/SidebarGraphicsSettings.hpp
        src/Slic3r/App/SidebarGraphicsSettings.cpp
        include/Slic3r/App/LightSetting.hpp
        src/Slic3r/App/LightSetting.cpp
        include/Slic3r/App/ExportPathSelect.hpp
        src/Slic3r/App/ExportPathSelect.cpp
        include/Slic3r/App/IDialogManager.hpp
        include/Slic3r/App/PrintSettingsDialog.hpp
        src/Slic3r/App/PrintSettingsDialog.cpp
        include/Slic3r/App/MaterialSettingsDialog.hpp
        src/Slic3r/App/MaterialSettingsDialog.cpp
        include/Slic3r/App/LogicalPrinterSettingsDialog.hpp
        src/Slic3r/App/LogicalPrinterSettingsDialog.cpp
        include/Slic3r/App/PrinterAdvancedSettingsDialog.hpp
        src/Slic3r/App/PrinterAdvancedSettingsDialog.cpp
        include/Slic3r/App/PhysicalPrinterSettingsDialog.hpp
        src/Slic3r/App/PhysicalPrinterSettingsDialog.cpp
        include/Slic3r/App/PhysicalPrinterSettingsButton.hpp
        src/Slic3r/App/PhysicalPrinterSettingsButton.cpp
        include/Slic3r/App/LogicalPrinterSettingsButton.hpp
        src/Slic3r/App/LogicalPrinterSettingsButton.cpp
        include/Slic3r/App/PhysicalPrinter.hpp
        src/Slic3r/App/PhysicalPrinter.cpp
        include/Slic3r/App/PrinterAddDialog.hpp
        src/Slic3r/App/PrinterAddDialog.cpp
        include/Slic3r/App/ConfigSubcategoryListView.hpp
        src/Slic3r/App/ConfigSubcategoryListView.cpp
        include/Slic3r/App/ConfigSubcategoryItem.hpp
        src/Slic3r/App/ConfigSubcategoryItem.cpp
        include/Slic3r/App/ConfigRowItem.hpp
        src/Slic3r/App/ConfigRowItem.cpp
        include/Slic3r/App/ConfigRowItems.hpp
        src/Slic3r/App/ConfigRowItems.cpp
        include/Slic3r/App/SidebarToolHeadRow.hpp
        src/Slic3r/App/SidebarToolHeadRow.cpp
        include/Slic3r/App/PrinterNozzleRow.hpp
        src/Slic3r/App/PrinterNozzleRow.cpp
        src/Slic3r/App/MaterialSelectionDialog.cpp
        include/Slic3r/App/MaterialSelectionDialog.hpp
        include/Slic3r/App/MaterialSelectionRow.hpp
        src/Slic3r/App/MaterialSelectionRow.cpp
        include/Slic3r/App/Search.hpp
        src/Slic3r/App/Search.cpp
        include/Slic3r/App/OverrideItemRow.hpp
        include/Slic3r/App/OverrideOptionGroup.hpp
        src/Slic3r/App/OverrideItemRow.cpp
        src/Slic3r/App/OverrideOptionGroup.cpp
        include/Slic3r/App/ObjectConfigItem.hpp
        src/Slic3r/App/ObjectConfigItem.cpp
        include/Slic3r/App/OverrideSettingsDialog.hpp
        src/Slic3r/App/OverrideSettingsDialog.cpp
        include/Slic3r/App/OverrideCategoryRow.hpp
        include/Slic3r/App/OverrideCategoryButton.hpp
        src/Slic3r/App/OverrideCategoryButton.cpp
	include/Slic3r/App/RenderModuleHelper.hpp
        src/Slic3r/App/RenderModuleHelper.cpp
        include/Slic3r/App/DialogNavigation.hpp
        src/Slic3r/App/DialogNavigation.cpp
        include/Slic3r/App/SearchBar.hpp
        src/Slic3r/App/SearchBar.cpp
        include/Slic3r/App/SearchPopup.hpp
        src/Slic3r/App/SearchPopup.cpp
        include/Slic3r/App/SearchObservableList.hpp
        src/Slic3r/App/SearchObservableList.cpp
        include/Slic3r/App/SearchResultRow.hpp
        src/Slic3r/App/SearchResultRow.cpp
        include/Slic3r/App/ConfigSettingsDialog.hpp
        src/Slic3r/App/ConfigSettingsDialog.cpp

        include/Slic3r/App/Config/ConfigItemTextField.hpp
        src/Slic3r/App/Config/ConfigItemTextField.cpp
        include/Slic3r/App/Config/ConfigItemCheckBox.hpp
        src/Slic3r/App/Config/ConfigItemCheckBox.cpp
        include/Slic3r/App/Config/ConfigItemComboBox.hpp
        src/Slic3r/App/Config/ConfigItemComboBox.cpp
        include/Slic3r/App/Config/ConfigItemColorPicker.hpp
        include/Slic3r/App/Config/ConfigItemPoints.hpp
        src/Slic3r/App/Config/ConfigItemColorPicker.cpp
        src/Slic3r/App/Config/ConfigItemPoints.cpp
        src/Slic3r/App/Config/ConfigItemTextFields.cpp
        include/Slic3r/App/Config/ConfigItemTextFields.hpp
        include/Slic3r/App/Config/ConfigItemSpinBox.hpp
        src/Slic3r/App/Config/ConfigItemSpinBox.cpp
        include/Slic3r/App/Config/ObservableCategorizer.hpp
        include/Slic3r/App/Config/CategoryPageTransformer.hpp
        src/Slic3r/App/Config/CategoryPageTransformer.cpp
        src/Slic3r/App/Config/ObservableCategorizer.cpp
        include/Slic3r/App/Config/CategoryUtils.hpp
        src/Slic3r/App/Config/CategoryUtils.cpp
        include/Slic3r/App/Config/ConfigItemSpinBoxes.hpp
        include/Slic3r/App/Config/ConfigItemComboBoxes.hpp
        src/Slic3r/App/Config/ConfigItemSpinBoxes.cpp
        src/Slic3r/App/Config/ConfigItemComboBoxes.cpp
        include/Slic3r/App/Config/ConfigItemCheckBoxes.hpp
        src/Slic3r/App/Config/ConfigItemCheckBoxes.cpp
        include/Slic3r/App/Config/ConfigItemControl.hpp
        src/Slic3r/App/Config/ConfigItemControl.cpp
        include/Slic3r/App/Config/ObservableOverrideCategorizer.hpp
        src/Slic3r/App/Config/ObservableOverrideCategorizer.cpp
        include/Slic3r/App/Config/ConfigItemSubstitutions.hpp
        src/Slic3r/App/Config/ConfigItemSubstitutions.cpp

        include/Slic3r/App/SharedThumbnailImageGenerator.hpp
        include/Slic3r/App/ThumbnailStore.hpp
        src/Slic3r/App/ThumbnailStore.cpp
        include/Slic3r/App/ThumbnailStoreUpdater.hpp
        src/Slic3r/App/ThumbnailStoreUpdater.cpp
        
        include/Slic3r/App/Browser/BrowserLogicCommand.hpp
        include/Slic3r/App/Browser/AbstractBrowserLogicCommandHandler.hpp
        include/Slic3r/App/Browser/AbstractBrowserLogic.hpp
        include/Slic3r/App/Browser/BrowserLogicLogIn.hpp
        src/Slic3r/App/Browser/BrowserLogicLogIn.cpp
        include/Slic3r/App/Browser/BrowserLogicConnectSelect.hpp
        src/Slic3r/App/Browser/BrowserLogicConnectSelect.cpp
        include/Slic3r/App/Browser/AbstractConnectRequestHandler.hpp
        src/Slic3r/App/Browser/AbstractConnectRequestHandler.cpp
        include/Slic3r/App/Browser/BrowserLogicConnectPage.hpp
        src/Slic3r/App/Browser/BrowserLogicConnectPage.cpp
        include/Slic3r/App/Browser/BrowserLogicPrintables.hpp
        src/Slic3r/App/Browser/BrowserLogicPrintables.cpp
        include/Slic3r/App/Browser/BrowserLogicLogInRedirect.hpp
        src/Slic3r/App/Browser/BrowserLogicLogInRedirect.cpp
        include/Slic3r/App/Browser/BrowserLogicPrintablesToConnect.hpp
        src/Slic3r/App/Browser/BrowserLogicPrintablesToConnect.cpp

        src/Slic3r/App/Scene/Scene.cpp
        include/Slic3r/App/Scene/Scene.hpp
        src/Slic3r/App/Scene/Node.cpp
        include/Slic3r/App/Scene/Node.hpp
        include/Slic3r/App/Scene/IRenderLayerObject.hpp
        include/Slic3r/App/Scene/IRenderNodeComponent.hpp
        include/Slic3r/App/Scene/Camera.hpp
        src/Slic3r/App/Scene/Camera.cpp
        include/Slic3r/App/Scene/CameraHelper.hpp
        src/Slic3r/App/Scene/CameraHelper.cpp
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
        include/Slic3r/App/Scene/GizmoKeyEventContext.hpp
        include/Slic3r/App/Scene/AbstractCameraGizmo.hpp
        src/Slic3r/App/Scene/AbstractCameraGizmo.cpp
        include/Slic3r/App/Scene/GizmoManager.hpp
        src/Slic3r/App/Scene/GizmoManager.cpp
		include/Slic3r/App/Scene/GizmoCommandRegistry.hpp
		src/Slic3r/App/Scene/GizmoCommandRegistry.cpp
        include/Slic3r/App/Scene/MouseDragDetector.hpp
        src/Slic3r/App/Scene/MouseDragDetector.cpp
        include/Slic3r/App/Scene/GeometryDataFactory.hpp
        src/Slic3r/App/Scene/GeometryDataFactory.cpp
        include/Slic3r/App/Scene/ISceneProvider.hpp
        include/Slic3r/App/Scene/IProjectSceneProvider.hpp
        include/Slic3r/App/Scene/ScenePresenterProjectContext.hpp
        src/Slic3r/App/Scene/Lights.cpp
        include/Slic3r/App/Scene/Lights.hpp
        include/Slic3r/App/Scene/LightingHelper.hpp
        src/Slic3r/App/Scene/LightingHelper.cpp
        include/Slic3r/App/Scene/PrintVolumeData.hpp
        src/Slic3r/App/Scene/PrintVolumeData.cpp
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
        include/Slic3r/App/Scene/ISceneChangedListener.hpp
        include/Slic3r/App/Scene/GraphicsSettings.hpp
        src/Slic3r/App/Scene/GraphicsSettings.cpp
        include/Slic3r/App/Scene/CameraFrustumUpdater.hpp
        src/Slic3r/App/Scene/CameraFrustumUpdater.cpp
        include/Slic3r/App/Scene/Clipper.hpp
        src/Slic3r/App/Scene/Clipper.cpp
        include/Slic3r/App/Scene/ClipperPresenter.hpp
        include/Slic3r/App/Scene/ClipperPresenterHelper.hpp
        src/Slic3r/App/Scene/ClipperPresenter.cpp

        include/Slic3r/App/TestRenderModule.hpp
        src/Slic3r/App/TestRenderModule.cpp
        src/Slic3r/App/Init.cpp
        include/Slic3r/App/Init.hpp
        include/Slic3r/App/AppServices.hpp
        src/Slic3r/App/AppServices.cpp

        include/Slic3r/Biz/IMessageDialogProvider.hpp

        src/Slic3r/Biz/I18N/LanguageInfo.cpp
        include/Slic3r/Biz/I18N/LanguageInfo.hpp
        src/Slic3r/Biz/I18N/I18N.cpp
        include/Slic3r/Biz/I18N/I18N.hpp
        src/Slic3r/Biz/I18N/Translation.cpp
        include/Slic3r/Biz/I18N/Translation.hpp
        src/Slic3r/App/Localization.cpp
        include/Slic3r/App/Localization.hpp
        include/Slic3r/App/ILanguageChangedListener.hpp

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
        include/Slic3r/App/Yoga/GizmoDialog.hpp
        src/Slic3r/App/Yoga/GizmoDialog.cpp
        include/Slic3r/App/Yoga/GizmoDialogHelp.hpp
        src/Slic3r/App/Yoga/GizmoDialogHelp.cpp
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
        src/Slic3r/App/Yoga/Slider.cpp
        include/Slic3r/App/Yoga/Slider.hpp
        src/Slic3r/App/Yoga/ProgressBar.cpp
        include/Slic3r/App/Yoga/ProgressBar.hpp
        src/Slic3r/App/Yoga/SliderWithInput.cpp
        include/Slic3r/App/Yoga/SliderWithInput.hpp
        src/Slic3r/App/Yoga/RadioButton.cpp
        include/Slic3r/App/Yoga/RadioButton.hpp
        src/Slic3r/App/Yoga/RadioExtruder.cpp
        include/Slic3r/App/Yoga/RadioExtruder.hpp
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
        src/Slic3r/App/Yoga/ProjectButton.cpp
        include/Slic3r/App/Yoga/ProjectButton.hpp
        src/Slic3r/App/Yoga/ProjectButtonBackground.cpp
        include/Slic3r/App/Yoga/ProjectButtonBackground.hpp
        src/Slic3r/App/TopBar.cpp
        include/Slic3r/App/TopBar.hpp
        include/Slic3r/App/Yoga/ScrollArea.hpp
        src/Slic3r/App/Yoga/ScrollArea.cpp
        include/Slic3r/App/Yoga/InputText.hpp
        src/Slic3r/App/Yoga/InputText.cpp
        src/Slic3r/App/Yoga/InputTextField.cpp
        include/Slic3r/App/Yoga/InputTextField.hpp
        include/Slic3r/App/Yoga/ImGuiUtils.hpp
        src/Slic3r/App/Yoga/ImGuiUtils.cpp
        include/Slic3r/App/Yoga/SpinButton.hpp
        src/Slic3r/App/Yoga/SpinButton.cpp
        include/Slic3r/App/Yoga/InputTextWithSpin.hpp
        src/Slic3r/App/Yoga/InputTextWithSpin.cpp
        include/Slic3r/App/Yoga/Validator.hpp
        src/Slic3r/App/Yoga/Validator.cpp
        include/Slic3r/App/Yoga/Scaler.hpp
        src/Slic3r/App/Yoga/Scaler.cpp
        include/Slic3r/App/Yoga/ComboBox.hpp
        src/Slic3r/App/Yoga/ComboBox.cpp
        include/Slic3r/App/Yoga/ItemEvents.hpp
        src/Slic3r/App/Yoga/ItemEvents.cpp
        include/Slic3r/App/Yoga/RootItem.hpp
        src/Slic3r/App/Yoga/RootItem.cpp
        include/Slic3r/App/Yoga/StackLayout.hpp
        src/Slic3r/App/Yoga/StackLayout.cpp
        include/Slic3r/App/PageEntryButton.hpp
        src/Slic3r/App/PageEntryButton.cpp
        include/Slic3r/App/Yoga/Popup.hpp
        src/Slic3r/App/Yoga/Popup.cpp
        include/Slic3r/App/Yoga/AlignmentButtons.hpp
        src/Slic3r/App/Yoga/AlignmentButtons.cpp
        include/Slic3r/App/Yoga/AbstractSettingsDialog.hpp
        src/Slic3r/App/Yoga/AbstractSettingsDialog.cpp
        include/Slic3r/App/Yoga/RevertableControl.hpp
        src/Slic3r/App/Yoga/RevertableControl.cpp
        include/Slic3r/App/Yoga/ComboBoxListView.hpp
        include/Slic3r/App/Yoga/ComboBoxListViewSelection.hpp
        include/Slic3r/App/Yoga/SegmentedControl.hpp
        src/Slic3r/App/Yoga/SegmentedControl.cpp

        src/Slic3r/App/Imgui/DoubleSlider.cpp
        include/Slic3r/App/Imgui/DoubleSlider.hpp
        src/Slic3r/App/Imgui/RulerForDoubleSlider.cpp
        include/Slic3r/App/Imgui/RulerForDoubleSlider.hpp
        src/Slic3r/App/Imgui/ImguiExtension.cpp
        include/Slic3r/App/Imgui/ImguiExtension.hpp
        src/Slic3r/App/Imgui/NavigationCube.cpp
        include/Slic3r/App/Imgui/NavigationCube.hpp

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
        src/Slic3r/App/Preview/GCodeDisplay.cpp
        include/Slic3r/App/Preview/GCodeDisplay.hpp
        src/Slic3r/App/Preview/GCodeWindow.cpp
        include/Slic3r/App/Preview/GCodeWindow.hpp
        src/Slic3r/App/Preview/Legend.cpp
        include/Slic3r/App/Preview/Legend.hpp
        src/Slic3r/App/Preview/LegendWindow.cpp
        include/Slic3r/App/Preview/LegendWindow.hpp
        src/Slic3r/App/Preview/AbstractViewerWrapper.cpp
        include/Slic3r/App/Preview/AbstractViewerWrapper.hpp

        src/Slic3r/App/AbstractRenderLayout.cpp
        include/Slic3r/App/AbstractRenderLayout.hpp
        include/Slic3r/App/ResourceResolver.hpp
        src/Slic3r/App/ResourceResolver.cpp

        include/Slic3r/App/PopNotification/PopNotificationCenter.hpp
        include/Slic3r/App/PopNotification/PopNotificationData.hpp
        include/Slic3r/App/PopNotification/PopNotificationView.hpp
        include/Slic3r/App/PopNotification/PopNotificationListView.hpp
        include/Slic3r/App/PopNotification/PopNotificationDataVariant.hpp
        include/Slic3r/App/PopNotification/PopNotificationLayout.hpp
        include/Slic3r/App/PopNotification/PopNotificationObservableList.hpp
        src/Slic3r/App/PopNotification/PopNotificationCenter.cpp
        src/Slic3r/App/PopNotification/PopNotificationView.cpp
        src/Slic3r/App/PopNotification/PopNotificationObservableList.cpp

        include/Slic3r/Biz/PrintConfigInteractor.hpp
        src/Slic3r/Biz/PrintConfigInteractor.cpp
        include/Slic3r/Biz/ObservableDynamicPrintConfig.hpp
        include/Slic3r/Biz/Observable.hpp
        include/Slic3r/Biz/Plater/ConfigViewModel.hpp
        include/Slic3r/Biz/IObservableList.hpp
        include/Slic3r/Biz/ObservableList.hpp
        include/Slic3r/Biz/IListObserver.hpp
        include/Slic3r/Biz/DataObserver.hpp
        include/Slic3r/Biz/ConfigBoxObservableList.hpp
        include/Slic3r/Biz/CBIObservableList.hpp
        src/Slic3r/Biz/CBIObservableList.cpp
        src/Slic3r/Biz/ConfigBoxObservableList.cpp
        include/Slic3r/Biz/ConfigBoxInteractor.hpp
        src/Slic3r/Biz/ConfigBoxInteractor.cpp
        include/Slic3r/Biz/ObservableListSortFilter.hpp
        include/Slic3r/Biz/ObservableListTransformer.hpp
        include/Slic3r/Biz/BatchObservableList.hpp
        include/Slic3r/Biz/ObservableListWithSelection.hpp
        src/Slic3r/Biz/Utils/CopyFile.cpp
        include/Slic3r/Biz/Utils/CopyFile.hpp
        include/Slic3r/Biz/ObjectSettingsInteractor.hpp
        src/Slic3r/Biz/ObjectSettingsInteractor.cpp
        include/Slic3r/Biz/ObjectSettingsObservableList.hpp
        src/Slic3r/Biz/ObjectSettingsObservableList.cpp
        include/Slic3r/Biz/OverrideItem.hpp
        src/Slic3r/Biz/Utils/MeshClipper.cpp
        include/Slic3r/Biz/Utils/MeshClipper.hpp

        include/Slic3r/Biz/Scene/SceneInteractor.hpp
        src/Slic3r/Biz/Scene/SceneInteractor.cpp
        include/Slic3r/Biz/Scene/Selection.hpp
        src/Slic3r/Biz/Scene/Selection.cpp
        include/Slic3r/Biz/Scene/SceneInteractorProjectContext.hpp
        include/Slic3r/Biz/Scene/BedPlacement.hpp
        src/Slic3r/Biz/Scene/BedPlacement.cpp
        include/Slic3r/Biz/Scene/BedGeometry.hpp
        src/Slic3r/Biz/Scene/BedGeometry.cpp
        include/Slic3r/Biz/Scene/BedTracking.hpp
        src/Slic3r/Biz/Scene/BedTracking.cpp
        src/Slic3r/Biz/Scene/BedFactory.hpp
        src/Slic3r/Biz/Scene/BedFactory.cpp

		include/Slic3r/Biz/Preset/IPresetChangedListener.hpp
        include/Slic3r/Biz/Preset/IBedPresetSwitchedListener.hpp
        include/Slic3r/Biz/Preset/IConfigInteractor.hpp
        src/Slic3r/Biz/Preset/PresetInteractor.cpp
        include/Slic3r/Biz/Preset/PresetInteractor.hpp
        include/Slic3r/Biz/Preset/PresetInteractorProjectContext.hpp
		src/Slic3r/Biz/Preset/PresetInteractorProjectContext.cpp
        include/Slic3r/Biz/Preset/PresetInteractorConfigContainerContext.hpp
        src/Slic3r/Biz/Preset/PresetInteractorConfigContainerContext.cpp
        include/Slic3r/Biz/Preset/PresetHints.hpp
        src/Slic3r/Biz/Preset/PresetHints.cpp
		include/Slic3r/Biz/Preset/ProjectPresetView.hpp
		src/Slic3r/Biz/Preset/ProjectPresetView.cpp
        #include/Slic3r/Biz/Preset/AbstractConfigManipulation.hpp
        #src/Slic3r/Biz/Preset/AbstractConfigManipulation.cpp

#		include/Slic3r/Biz/Preset/Bundle.hpp
#		src/Slic3r/Biz/Preset/Bundle.cpp

        include/Slic3r/Biz/Preset/IO/PresetLoader.hpp
        src/Slic3r/Biz/Preset/IO/PresetLoader.cpp
        include/Slic3r/Biz/Preset/IO/HwConfigLoader.hpp
        src/Slic3r/Biz/Preset/IO/HwConfigLoader.cpp
        include/Slic3r/Biz/Preset/IO/PresetMetadataLegacyLoader.hpp
        src/Slic3r/Biz/Preset/IO/PresetMetadataLegacyLoader.cpp
        include/Slic3r/Biz/Preset/IO/BundleLoader.hpp
        src/Slic3r/Biz/Preset/IO/BundleLoader.cpp
        src/Slic3r/Biz/Yaml/Yaml.hpp
        src/Slic3r/Biz/Yaml/Yaml.cpp
        src/Slic3r/Biz/Yaml/YamlAdapter.hpp
        src/Slic3r/Biz/Yaml/YamlSlic3rTypes.hpp
		src/Slic3r/Biz/Yaml/YamlSlic3rTypes.cpp

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
        include/Slic3r/Biz/ObservableProjectList.hpp
        src/Slic3r/Biz/ObservableProjectList.cpp
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
        include/Slic3r/Biz/UserAccount/UserAccountConnectMessageHandler.hpp
        include/Slic3r/Biz/UserAccount/ConnectUtils.hpp
        src/Slic3r/Biz/UserAccount/UserAccountInteractor.cpp
        src/Slic3r/Biz/UserAccount/UserAccountCommunication.cpp
        src/Slic3r/Biz/UserAccount/UserAccountSession.cpp
        src/Slic3r/Biz/UserAccount/UserAccountSessionDispatchBase.cpp
        src/Slic3r/Biz/UserAccount/UserAccountAction.cpp
        src/Slic3r/Biz/UserAccount/UserAccountCommunicationTokenBase.cpp
        src/Slic3r/Biz/UserAccount/UserAccountTokenStore.cpp
        src/Slic3r/Biz/UserAccount/UserAccountCodeChallengeGenerator.cpp
        src/Slic3r/Biz/UserAccount/UserAccountConnectMessageHandler.cpp
        src/Slic3r/Biz/UserAccount/ConnectUtils.cpp

        include/Slic3r/Biz/SHA256.hpp

        include/Slic3r/Biz/AppInstance/AbstractAppInstanceMessageHandler.hpp
        include/Slic3r/Biz/AppInstance/IAppInstanceMessageContentListener.hpp
        include/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerFactory.hpp
        include/Slic3r/Biz/AppInstance/AppInstanceUtils.hpp
        src/Slic3r/Biz/AppInstance/AbstractAppInstanceMessageHandler.cpp
        src/Slic3r/Biz/AppInstance/AppInstanceUtils.cpp
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
        src/Slic3r/Biz/Config/ConfigLegacy.cpp
        src/Slic3r/Biz/Config/Legacy/3mf_legacy.cpp
        include/Slic3r/Biz/Config/3mf_legacy.hpp

        include/Slic3r/Biz/Config/ConfigLoad.hpp
        src/Slic3r/Biz/Config/ConfigLoad.cpp
        src/Slic3r/Biz/Config/ConfigJson.hpp
        src/Slic3r/Biz/Config/ConfigJson.cpp

		src/Slic3r/Biz/JsonValueJson.cpp
		src/Slic3r/Biz/JsonValueJson.hpp

		src/Slic3r/Biz/ProjectMetadataJson.hpp
		src/Slic3r/Biz/ProjectMetadataJson.cpp
        src/Slic3r/Biz/Config/HwConfigJson.hpp
        src/Slic3r/Biz/Config/HwConfigJson.cpp
        src/Slic3r/Biz/Config/SelectedPresetJson.hpp
        src/Slic3r/Biz/Config/SelectedPresetJson.cpp
		src/Slic3r/Biz/Config/GCodeMetadataJson.hpp
		src/Slic3r/Biz/Config/GCodeMetadataJson.cpp
		src/Slic3r/Biz/Config/FeatureStructurizer.cpp
		src/Slic3r/Biz/Config/FeatureStructurizer.hpp
        include/Slic3r/Biz/Format/3mf.hpp
        include/Slic3r/Biz/Format/Metadata.hpp
        include/Slic3r/Biz/Format/ResultLoad3mf.hpp
        src/Slic3r/Biz/Format/3mf.cpp
        #src/Slic3r/Biz/Format/3mf/BuildTicket.cpp
        #src/Slic3r/Biz/Format/3mf/BuildTicket.hpp
        src/Slic3r/Biz/Format/3mf/Metadata.cpp
        src/Slic3r/Biz/Format/3mf/Model3mf.cpp
        src/Slic3r/Biz/Format/3mf/Model3mf.hpp
        src/Slic3r/Biz/Format/3mf/ModelMap.hpp
        src/Slic3r/Biz/Format/3mf/PrusaFile.cpp
        src/Slic3r/Biz/Format/3mf/PrusaFile.hpp
        src/Slic3r/Biz/Format/3mf/Relations.cpp
        src/Slic3r/Biz/Format/3mf/Relations.hpp
        src/Slic3r/Biz/Format/3mf/ResultLoad3mf.cpp

        include/Slic3r/Biz/FileLoadingLogic.hpp
        src/Slic3r/Biz/FileLoadingLogic.cpp
        include/Slic3r/Biz/Format/STL.hpp
        src/Slic3r/Biz/Format/STL.cpp

        include/Slic3r/Biz/ArrangeInteractor.hpp
        src/Slic3r/Biz/ArrangeInteractor.cpp

        include/Slic3r/Biz/PresetUpdater/IPresetUpdaterResultListener.hpp
        include/Slic3r/Biz/PresetUpdater/PresetUpdaterInteractor.hpp
        include/Slic3r/Biz/PresetUpdater/PresetUpdaterReconfigurationList.hpp
        include/Slic3r/Biz/PresetUpdater/PresetUpdaterRepositoryDescriptor.hpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterProcessStatus.hpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterProcessStatus.cpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterRepository.hpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterRepository.cpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterRepositorySync.hpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterRepositorySync.cpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterRepositoryDatabase.hpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterRepositoryDatabase.cpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterIndex.hpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterIndex.cpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterInteractor.cpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterReconfigurationList.cpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterUtils.hpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterUtils.cpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterFileHash.hpp
        src/Slic3r/Biz/PresetUpdater/PresetUpdaterFileHash.cpp

        include/Slic3r/App/PresetUpdaterUI.hpp
        src/Slic3r/App/PresetUpdaterUI.cpp
        include/Slic3r/App/AppConfig.hpp
        src/Slic3r/App/AppConfig.cpp

        include/Slic3r/Biz/RemovableDrive/RemovableDriveService.hpp
        include/Slic3r/Biz/RemovableDrive/IRemovableDriveMonitor.hpp
        include/Slic3r/Biz/RemovableDrive/RemovableDriveData.hpp
        include/Slic3r/Biz/RemovableDrive/IRemovableDriveStatusListener.hpp

        include/Slic3r/App/DisplayStrings.hpp
        src/Slic3r/App/DisplayStrings.cpp
)

if (WIN32)
    list(APPEND SLIC3R_SHARED_FILES
        src/Slic3r/Biz/SHA256Win32.cpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerWin32.hpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerWin32.cpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerFactoryWin32.cpp
        src/Slic3r/Biz/RemovableDrive/RemovableDriveServiceWin32.cpp
        src/Slic3r/Biz/RemovableDrive/RemovableDriveMonitorWin32.hpp
        src/Slic3r/Biz/RemovableDrive/RemovableDriveMonitorWin32.cpp
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
        src/Slic3r/Biz/RemovableDrive/RemovableDriveServiceMac.cpp
        src/Slic3r/Biz/RemovableDrive/RemovableDriveMonitorMac.hpp
        src/Slic3r/Biz/RemovableDrive/RemovableDriveMonitorMac.cpp
        src/Slic3r/Biz/RemovableDrive/RemovableDriveMonitorInnerMac.h
        src/Slic3r/Biz/RemovableDrive/RemovableDriveMonitorInnerMac.mm
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND SLIC3R_SHARED_FILES
        src/Slic3r/Biz/SHA256Linux.cpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerLinux.hpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerLinux.cpp
        src/Slic3r/Biz/AppInstance/AppInstanceMessageHandlerFactoryLinux.cpp
        include/Slic3r/Biz/AppInstance/SingleInstanceCheckerLock.hpp
        src/Slic3r/Biz/AppInstance/SingleInstanceCheckerLock.cpp
        src/Slic3r/Biz/RemovableDrive/RemovableDriveServiceLinux.cpp
        src/Slic3r/Biz/RemovableDrive/RemovableDriveMonitorLinux.hpp
        src/Slic3r/Biz/RemovableDrive/RemovableDriveMonitorLinux.cpp
    )
endif ()

find_package(NanoSVG REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(yoga CONFIG REQUIRED)
find_package(nlohmann_json 3.11 REQUIRED)
find_package(EXPAT CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)


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
find_package(range-v3 REQUIRED)

slic3r_remap_configs("pugixml::static;yoga::yogacore;${YAML_LIB}" RelWithDebInfo Release)

add_library(slic3r-shared STATIC ${SLIC3R_SHARED_FILES})

target_link_libraries(slic3r-shared PUBLIC
        slic3r-platform
        slic3r-render
        libvgcode
        libslic3r
        libpgcode
        fmt::fmt
        yoga::yogacore
        slic3r-biz-algorithms
        slic3r-biz-arrange
        nlohmann_json::nlohmann_json
        ${YAML_LIB}
        magic_enum::magic_enum
        fastfloat
        expat::expat
        TBB::tbb
        TBB::tbbmalloc
        pugixml::pugixml
		range-v3::range-v3
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
if (WIN32)
        target_link_libraries(slic3r-shared PUBLIC shlwapi)
endif ()
if (APPLE)
    FIND_LIBRARY(DISKARBITRATION_LIBRARY DiskArbitration)
    target_link_libraries(slic3r-shared PUBLIC ${DISKARBITRATION_LIBRARY})
endif()

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

if (SLIC3R_ENABLE_WEBKIT)
   target_compile_definitions(slic3r-shared PRIVATE SLIC3R_HAS_WEBKIT) 
endif()

add_target_to_doxygen(slic3r-shared)

if(SLIC3R_BUILD_TESTS AND NOT EMSCRIPTEN)
    add_executable(slic3r-shared-tests
        test/main.cpp

        test/Slic3r/App/DialogNavigationTests.cpp

        test/Slic3r/App/Scene/Node.cpp
        test/Slic3r/App/Scene/Ray.cpp

        test/Slic3r/App/Plater/SurfaceMesh.cpp

        test/Slic3r/App/Yoga/ListViewTests.cpp
        test/Slic3r/App/Yoga/LayoutTests.cpp
        test/Slic3r/App/Yoga/ItemTreeTests.cpp
        test/Slic3r/App/Yoga/ImGuiFixture.hpp
        test/Slic3r/App/Yoga/ButtonGroupTests.cpp
        test/Slic3r/App/Yoga/ValidatorTests.cpp
        test/Slic3r/App/Yoga/TestRootItem.hpp
        test/Slic3r/App/Yoga/SegmentedControlTests.cpp

        test/Slic3r/Biz/ProjectInteractorTests.cpp
        test/Slic3r/Biz/Scene/SceneInteractorTests.cpp
        test/Slic3r/Biz/WeakerPointerTests.cpp

        test/Slic3r/Biz/Expr/ParserTests.cpp
        test/Slic3r/Biz/Expr/EvalTests.cpp

        test/Slic3r/Biz/Yaml/YamlTests.cpp
        test/Slic3r/Biz/Preset/IO/HwConfigDefLoaderTests.cpp
        test/Slic3r/Biz/Preset/IO/PresetLoaderTests.cpp
		test/Slic3r/Biz/Preset/IO/PresetMetadataLegacyLoaderTests.cpp
        test/Slic3r/Biz/Preset/PresetCollectionEvaluatorTests.cpp
        test/Slic3r/Biz/Preset/PresetEvaluatorTests.cpp
        test/Slic3r/Biz/Preset/HwConfigEvaluatorTests.cpp
        test/Slic3r/Biz/Preset/ProjectPresetViewTests.cpp

        test/Slic3r/Biz/Slicing/TestUtils.cpp
        test/Slic3r/Biz/Slicing/GCodeUtils.cpp
        test/Slic3r/Biz/Slicing/ActionsTests.cpp
        test/Slic3r/Biz/Slicing/ListenersTests.cpp

        test/Slic3r/Biz/Config/ConfigLegacyTests.cpp
        test/Slic3r/Biz/Config/ConfigLoadTests.cpp
        test/Slic3r/Biz/Config/ConfigRoundtripTests.cpp

        test/Slic3r/Biz/Config/HwConfigJsonTests.cpp
        test/Slic3r/Biz/Config/FeatureStructurizerTests.cpp

        test/Slic3r/Biz/ArrangeInteractorTests.cpp
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
