//************************************************************************************************
//
// Skin Editor
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : skineditorapp.cpp
// Description : Skin Editor Application
//
//************************************************************************************************

#include "skineditorapp.h"
#include "skindocument.h"
#include "appversion.h"

#include "ccl/base/development.h"
#include "ccl/base/storage/url.h"

#include "ccl/app/debugmenu.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/options/mainoption.h"
#include "ccl/app/components/eulacomponent.h"

#include "ccl/public/plugins/iservicemanager.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

void ccl_app_init ()
{
	NEW SkinEditorApp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (SkinEditor, kFirstRun)
{
	DocumentManager::instance ().addDocumentClass (NEW SkinDocumentClass, true);
	DocumentManager::instance ().setMultipleDocuments (false);
	DocumentApplication::setupDocumentNavigation ();
	return true;
}

//************************************************************************************************
// SkinEditorApp
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinEditorApp, DocumentApplication)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditorApp::SkinEditorApp ()
: DocumentApplication (APP_ID, /*APP_COMPANY*/nullptr, APP_NAME, APP_PACKAGE_ID, APP_VERSION)
{
	setWebsite (APP_WEBSITE);
	setBuildInformation (APP_FULL_NAME);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditorApp::startup ()
{
	if(!SuperClass::startup ())
		return false;

	// init color scheme
	MainColorSchemeOption* appSchemeOption = UserOption::init<MainColorSchemeOption> ();
	appSchemeOption->addConfigurationSavers ();

	// load theme
	Url skinFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (skinFolder, CCL_APPLICATION_BASEDIR, "skin")
	if(!loadTheme (skinFolder))
		return false;

	// EULA
	Url eulaFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (eulaFolder, VENDOR_IDENTITY_DIR, "eula")
	if(!EULAComponent ().startup (&eulaFolder))
		return false;

	// create main window
	createWindow ();

	// scan plug-ins
	scanPlugIns ();

	// start scripting
	System::GetScriptingManager ().startup (APP_PACKAGE_ID, System::GetCurrentModuleRef ());
	System::GetScriptingManager ().setReporter (ScriptErrorReporter::getInstance (*this));

	// scan scripts
	Url scriptFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (scriptFolder, CCL_APPLICATION_BASEDIR, "scripts")
	scanScripts (scriptFolder);

	// update menu bar
	if(appearanceMenu.menu)
	{
		MenuInserter inserter (appearanceMenu);
		appSchemeOption->makeAppearanceMenu (*appearanceMenu.menu);
	}
	
	// start services
	System::GetServiceManager ().startup ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditorApp::shutdown ()
{
	// stop services
	System::GetServiceManager ().shutdown ();

	// stop scripting
	System::GetScriptingManager ().shutdown (System::GetCurrentModuleRef ());
	System::GetScriptingManager ().setReporter (nullptr);

	return SuperClass::shutdown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkinEditorApp::extendMenu (IMenu& menu, StringID name)
{
	SuperClass::extendMenu (menu, name);
	
	if(name == "Appearance")
		appearanceMenu = MenuPosition (menu);
}
