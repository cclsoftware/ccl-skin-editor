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
// Filename    : skiniteminspector.h
// Description : Skin Item Inspector
//
//************************************************************************************************

#ifndef _ccl_skiniteminspector_h
#define _ccl_skiniteminspector_h

#include "ccl/app/component.h"

#include "ccl/public/gui/commanddispatch.h"

#include "skineditdata.h"

namespace CCL {

class SkinPropertyListModel;

//************************************************************************************************
// SkinItemInspector
//************************************************************************************************

class SkinItemInspector: public Component,
						 public CommandDispatcher<SkinItemInspector>
{
public:
	DECLARE_CLASS_ABSTRACT (SkinItemInspector, Component)

	SkinItemInspector (StringRef name);
	~SkinItemInspector ();

	void setSkinItem (SkinEditItem* skinItem);
	SkinEditItem* getSkinItem () const { return skinItem; }

	void onListSelectionChanged ();

	// Commands
	bool onAddAttribute (CmdArgs args);
	bool onEditRemove (CmdArgs args);

	// Component
	DECLARE_COMMANDS (SkinItemInspector)
	DECLARE_COMMAND_CATEGORY ("Edit", Component)
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	SkinPropertyListModel* listModel;
	SkinEditItem* skinItem;
};

} // namespace CCL

#endif // _ccl_skiniteminspector_h
