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
// Filename    : skineditfunctions.h
// Description : Skin Editing Functions
//
//************************************************************************************************

#ifndef _ccl_skineditfunctions_h
#define _ccl_skineditfunctions_h

#include "ccl/app/actions/actionexecuter.h"

#include "skineditdata.h"

namespace CCL {

//************************************************************************************************
// ActionDescriptions
//************************************************************************************************

namespace ActionDescriptions
{
	StringRef ModifyAttribute (bool singular = true);
	StringRef AddAttribute (bool singular = true);
	StringRef RemoveAttribute (bool singular = true);
	StringRef ResizeView (bool singular = true);
	StringRef DragView (bool singular = true);
	StringRef AddView (bool singular = true);
	StringRef RemoveView (bool singular = true);
	StringRef AddForm ();
	StringRef RemoveForm (bool singular = true);
	StringRef AddImage (bool singular = true);
	StringRef RemoveImage (bool singular = true);
	StringRef RemoveItem (bool singular = true);
	StringRef AddFont ();
	StringRef AddStyle ();
}

//************************************************************************************************
// SkinEditFunctions
//************************************************************************************************

class SkinEditFunctions: public ActionExecuter
{
public:
	DECLARE_CLASS_ABSTRACT (SkinEditFunctions, ActionExecuter)
	DECLARE_METHOD_NAMES (SkinEditFunctions)

	SkinEditFunctions (SkinEditData& skinData);

	bool modifyAttribute (SkinEditItem* item, const SkinValueChange& valueChange, StringRef description = 0);
	bool removeAttribute (SkinEditItem* item, StringID name);
	bool resizeView (SkinViewItem* viewItem, RectRef newSize);

	bool addView (SkinViewItem* parentItem, SkinViewItem* childItem, int index = -1);
	bool removeView (SkinViewItem* parentItem, SkinViewItem* childItem);

	SkinFormItem* addForm (StringID formName, StringRef sourceFile, StringRef comment = 0, int index = -1);
	SkinImageItem* addImage (StringID imageName, StringRef imagePath, IImage* image, int index = -1);
	SkinEditItem* addFont (StringID fontName, int index = -1);
	SkinEditItem* addStyle (StringID styleName, int index = -1);
	bool removeItem (SkinEditItem* item, StringRef description = 0);

protected:
	SkinEditData& skinData;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_skineditfunctions_h
