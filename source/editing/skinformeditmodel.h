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
// Filename    : skinformeditmodel.h
// Description : Form Editing Model
//
//************************************************************************************************

#ifndef _ccl_skinformeditmodel_h
#define _ccl_skinformeditmodel_h

#include "ccl/app/editing/editmodel.h"

namespace CCL {

class SkinFormEditComponent;
class SkinItemHandle;

//************************************************************************************************
// SkinFormEditModel
//************************************************************************************************

class SkinFormEditModel: public EditModel
{
public:
	DECLARE_CLASS_ABSTRACT (SkinFormEditModel, EditModel)

	SkinFormEditModel (SkinFormEditComponent* component);

	static const String kSkinEditCategory; // edit task category

	SkinFormEditComponent* getFormEditor ();

	EditHandler* resizeItems (EditView& editView, SkinItemHandle* handle);

	// EditModel
	bool containsAnyItems () override;
	Object* findItem (EditView& editView, PointRef where) override;
	Object* findItemPart (EditView& editView, Object* item, PointRef where) override;
	Object* findItemDeep (EditView& editView, PointRef where) override;
	bool getItemSize (Rect& size, EditView& editView, Object* item) override;
	bool canSelectItem (Object* item) override;
	bool selectItems (EditView& editView, RectRef rect) override;
	//TODO: bool selectAll () override;
	bool setFocusItem (Object* item, EditView* editView) override;
	IDragHandler* createDragHandler (EditView& editView, const DragEvent& event) override;
	bool deleteSelected () override;
	bool deleteItem (Object* item) override;
	Object* copySelected (bool shared, bool forDuplicate = false) override;
	bool canInsertData (Object* data) override;
	bool insertData (Object* data) override;
	void collectTaskCategories (StringList& taskCategories) override;
	bool canPerformTask (EditView& editView, const EditTaskDescription& task) override;
	bool collectTaskCandidates (Container& resultList, EditView& editView, const EditTaskDescription& task) override;

protected:
	void getSelectedOnce (Container& candidates);
};

} // namespace CCL

#endif // _ccl_skinformeditmodel_h
