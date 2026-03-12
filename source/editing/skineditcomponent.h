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
// Filename    : skineditcomponent.h
// Description : Skin Editing Component
//
//************************************************************************************************

#ifndef _ccl_skineditcomponent_h
#define _ccl_skineditcomponent_h

#include "ccl/app/component.h"

#include "ccl/public/gui/commanddispatch.h"

#include "skineditdata.h"

namespace CCL {

class SkinDataTreeModel;
class SkinClassListModel;
class SkinFormEditComponent;
class SkinItemInspector;

//************************************************************************************************
// SkinEditComponent
//************************************************************************************************

class SkinEditComponent: public Component,
						 public CommandDispatcher<SkinEditComponent>
{
public:
	DECLARE_CLASS (SkinEditComponent, Component)

	SkinEditComponent ();
	~SkinEditComponent ();

	void setSkinData (SkinEditData* skinData);
	SkinEditData* getSkinData () const { return skinData; }

	void onItemFocused (SkinEditItem* focusItem);
	void performRemoval (Container& items);

	// Commands
	bool onAddForm (CmdArgs args);
	bool onAddImage (CmdArgs args);
	bool onAddFont (CmdArgs args);
	bool onAddStyle (CmdArgs args);

	// Component
	DECLARE_COMMANDS (SkinEditComponent)
	DECLARE_COMMAND_CATEGORY ("Skin", Component)
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	SkinEditData* skinData;

	SkinDataTreeModel* assetTreeModel;
	SkinItemInspector* assetInspector;
	
	SkinDataTreeModel* formTreeModel;
	SkinFormEditComponent* formEditor;
	SkinItemInspector* formInspector;

	SkinClassListModel* classListModel;
	IImageProvider* imageProvider;
	SkinEditItem* currentSourceItem;
	IParameter* sourceCodeParam;

	void setCurrentSourceItem (SkinEditItem* item);
	void updateSourceCode ();
};

} // namespace CCL

#endif // _ccl_skineditcomponent_h
