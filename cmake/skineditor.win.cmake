include_guard (DIRECTORY)

list (APPEND skineditor_ccl_sources
	${CCL_DIR}/platform/win/winmain.cpp
)

list (APPEND skineditor_icons
	1	${CMAKE_CURRENT_LIST_DIR}/../packaging/win/resource/skineditor.ico
)

ccl_embed_manifest (skineditor "${CCL_DIR}/packaging/win/application.manifest")

ccl_nsis_package (skineditor "${CMAKE_CURRENT_LIST_DIR}/../packaging/win/nsis/skineditor.nsi" "/DCCL_APPLICATIONS_DIR=${CMAKE_CURRENT_LIST_DIR}/../..")