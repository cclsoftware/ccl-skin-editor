#************************************************************************************************
#
# This file is part of Crystal Class Library (R)
# Copyright (c) 2025 CCL Software Licensing GmbH.
# All Rights Reserved.
#
# Licensed for use under either:
#  1. a Commercial License provided by CCL Software Licensing GmbH, or
#  2. GNU Affero General Public License v3.0 (AGPLv3).
# 
# You must choose and comply with one of the above licensing options.
# For more information, please visit ccl.dev.
#
# Filename    : CMakeLists.txt
# Description : Skin Editor
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project ("CCL Skin Editor")
set (VENDOR_PRODUCT_NAME "CCL Skin Editor")
set (PROJECT_VENDOR ccl)

# Find dependencies
include (vendor OPTIONAL)
find_package (ccl ${CCL_VERSION_REQUIREMENT} REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclgui)
find_package (cclspy)
find_package (jsengine REQUIRED)
find_package (skins)

list (APPEND skineditor_license_files
	${CCL_IDENTITIES_DIR}/ccl/legal/CCL\ 3rd\ Party\ Licenses.txt
	${CCL_IDENTITIES_DIR}/ccl/eula/EULA.txt
)

# Add targets
ccl_add_app (skineditor GUI
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h"
	VERSION_PREFIX APP
)
ccl_set_product_name (skineditor ${skineditor_NAME})
ccl_set_startup_project (skineditor)

ccl_use_skin_packages (skineditor neutralbase neutraldesign devtools)
ccl_add_skin_package (skineditor ${CMAKE_CURRENT_LIST_DIR}/../skin)
ccl_add_script_package (skineditor ${CMAKE_CURRENT_LIST_DIR}/../scripts/skinedit.package.in)

ccl_add_assets (skineditor FILES ${skineditor_license_files} PATH "license")

list (APPEND skineditor_resources
	${CMAKE_CURRENT_LIST_DIR}/../resource/cclgui.config
	${CMAKE_CURRENT_LIST_DIR}/../resource/commands.xml
	${CMAKE_CURRENT_LIST_DIR}/../resource/menubar.xml
)

list (APPEND skineditor_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
	${CMAKE_CURRENT_LIST_DIR}/../source/skincodeexport.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/skincodeexport.h
	${CMAKE_CURRENT_LIST_DIR}/../source/skindoccontroller.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/skindoccontroller.h
	${CMAKE_CURRENT_LIST_DIR}/../source/skindocument.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/skindocument.h
	${CMAKE_CURRENT_LIST_DIR}/../source/skineditorapp.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/skineditorapp.h
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skineditcomponent.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skineditcomponent.h
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skineditdata.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skineditdata.h
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skineditfunctions.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skineditfunctions.h
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skinediticons.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skinediticons.h
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skinformeditmodel.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skinformeditmodel.h
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skinformeditor.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skinformeditor.h
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skinformeditview.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skinformeditview.h
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skiniteminspector.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skiniteminspector.h
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skinitempainter.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/editing/skinitempainter.h
)

list (APPEND skineditor_ccl_sources
	${CCL_DIR}/main/cclmain.cpp
)

source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/../ PREFIX "source" FILES ${skineditor_source_files})
source_group ("source\\ccl" FILES ${skineditor_ccl_sources})

list (APPEND skineditor_sources ${skineditor_source_files} ${skineditor_ccl_sources})

ccl_add_resources (skineditor ${skineditor_resources})
target_sources (skineditor PRIVATE ${skineditor_sources})
target_link_libraries (skineditor PRIVATE cclapp cclbase ccltext cclsystem cclgui)

ccl_add_dependencies (skineditor jsengine cclspy)
