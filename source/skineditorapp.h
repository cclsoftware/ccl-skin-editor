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
// Filename    : skineditorapp.h
// Description : Skin Editor Application
//
//************************************************************************************************

#ifndef _skineditorapp_h
#define _skineditorapp_h

#include "ccl/app/documents/documentapp.h"

namespace CCL {

//************************************************************************************************
// SkinEditorApp
//************************************************************************************************

class SkinEditorApp: public DocumentApplication
{
public:
	DECLARE_CLASS (SkinEditorApp, DocumentApplication)

	SkinEditorApp ();

	// Application
	bool startup () override;
	bool shutdown () override;
	void CCL_API extendMenu (CCL::IMenu& menu, CCL::StringID name) override;
	
private:
	CCL::MenuPosition appearanceMenu;
};

} // namespace CCL

#endif // _skineditorapp_h
