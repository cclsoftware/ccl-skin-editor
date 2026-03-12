include_guard (DIRECTORY)

list (APPEND skineditor_ccl_sources
	${CCL_DIR}/platform/cocoa/cocoamain.mm
)

list (APPEND skineditor_bundle_resources
	${CMAKE_CURRENT_LIST_DIR}/../packaging/mac/resource/skineditor.xcassets
	${CCL_DIR}/packaging/mac/resource/MainMenu.xib
)

list (APPEND skineditor_resources
	${skineditor_bundle_resources}
)

list (APPEND skineditor_sources
	${skineditor_resources}
)
source_group ("Resources" FILES ${skineditor_resources})

set (skineditor_FRAMEWORKS
	ccltext
	cclsystem
	cclgui
)

set_target_properties (skineditor PROPERTIES
	RESOURCE "${skineditor_bundle_resources}"
	MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/../packaging/mac/Info.plist"
	XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"

	XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
)

if(${VENDOR_USE_PUBLISHER_CERTIFICATE})
	set_target_properties (skineditor PROPERTIES
		XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${SIGNING_CERTIFICATE_MAC}"
	)
endif()
