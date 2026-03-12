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
// Filename    : skindoccontroller.h
// Description : Skin Document Controller
//
//************************************************************************************************

#ifndef _skindoccontroller_h
#define _skindoccontroller_h

#include "ccl/app/component.h"

#include "ccl/public/gui/commanddispatch.h"

namespace CCL {

interface IColorParam;
class ListViewModel;
class ListViewItem;
class SkinDocument;
class SkinEditComponent;

//************************************************************************************************
// SkinDocumentController
//************************************************************************************************

class SkinDocumentController: public Component,
							  public CommandDispatcher<SkinDocumentController>
{
public:
	DECLARE_CLASS_ABSTRACT (SkinDocumentController, Component)

	SkinDocumentController (SkinDocument& document);

	SkinEditComponent* getSkinEditor () { return skinEditor; }

	// Commands
	bool onSkinSettings (CmdArgs args);
	bool onExportCode (CmdArgs args);

	// Component
	DECLARE_COMMANDS (SkinDocumentController)
	DECLARE_COMMAND_CATEGORY ("File", Component)
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

protected:
	SkinDocument& document;
	SkinEditComponent* skinEditor;
};

//************************************************************************************************
// SkinDocumentSettingsComponent
//************************************************************************************************

class SkinDocumentSettingsComponent: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (SkinDocumentSettingsComponent, Component)

	SkinDocumentSettingsComponent (SkinDocument& document);
	~SkinDocumentSettingsComponent ();

	void runDialog ();

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	DECLARE_STRINGID_MEMBER (kValueID)

	SkinDocument& document;
	ListViewModel* defaultsList;
	IColorParam* backColorParam;

	void onDefaultsListSelectionChanged (MessageRef msg);
	void onDefaultsListOpenItem (MessageRef msg);

	bool editDefaultsItem (ListViewItem& item, StringRef title = 0);
};

} // namespace CCL

#endif // _skindoccontroller_h
