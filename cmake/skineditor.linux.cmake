include_guard (DIRECTORY)

list (APPEND skineditor_ccl_sources
	${CCL_DIR}/platform/linux/linuxmain.cpp
)

install (TARGETS skineditor RUNTIME DESTINATION ${VENDOR_APPLICATION_RUNTIME_DIRECTORY})

ccl_install_desktop_file (${CMAKE_CURRENT_LIST_DIR}/../packaging/linux/resource/skineditor.desktop.in "${skineditor_PACKAGE_ID}")
ccl_install_icon (skineditor ${CMAKE_CURRENT_LIST_DIR}/../packaging/linux/resource/skineditor.svg)

include (${CMAKE_CURRENT_LIST_DIR}/../packaging/linux/deb/skineditor.package.cmake)
