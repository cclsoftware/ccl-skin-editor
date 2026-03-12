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
// Filename    : skinformeditor.h
// Description : Form Editing Component
//
//************************************************************************************************

#ifndef _ccl_skinformeditor_h
#define _ccl_skinformeditor_h

#include "ccl/app/editing/editor.h"

#include "skineditdata.h"

namespace CCL {

class EditTaskCollection;
class SkinFormTreeModel;
class SkinItemInspector;

//************************************************************************************************
// SkinFormEditComponent
//************************************************************************************************

class SkinFormEditComponent: public EditorComponent,
							 public CommandDispatcher<SkinFormEditComponent>

{
public:
	DECLARE_CLASS (SkinFormEditComponent, EditorComponent)

	SkinFormEditComponent ();
	~SkinFormEditComponent ();

	PROPERTY_POINTER (SkinItemInspector, itemInspector, ItemInspector)

	void setFormItem (SkinFormItem* formItem);
	SkinFormItem* getFormItem () const { return formItem; }
	
	struct GridSettings
	{
		bool snapActive;
		int gridSize;		

		GridSettings ();
		void snap (Coord& c) const;
	};
	
	GridSettings getGridSettings () const;
	int getPaintOptions () const;
	float getZoomFactor () const;
	SkinViewItem* getSelectedParentItem () const;

	void onViewItemFocused (SkinViewItem* viewItem, bool fromEditModel);
	void onViewItemFlagsEdit (SkinViewItem* changedItem);
	SkinViewItem* getFocusViewItem () const;

	void updateEditViews ();

	// Commands
	bool onSelectVariant (CmdArgs args);

	// EditorComponent
	DECLARE_COMMANDS (SkinFormEditComponent)
	DECLARE_COMMAND_CATEGORY ("Select Variant", EditorComponent)
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;

protected:
	SkinFormTreeModel* treeModel;
	EditTaskCollection* skinTasks;
	SkinFormItem* formItem;

	// EditorComponent
	EditModel* createModel () override;
};

} // namespace CCL

#endif // _ccl_skinformeditor_h
