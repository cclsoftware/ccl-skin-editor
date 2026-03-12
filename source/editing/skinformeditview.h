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
// Filename    : ccl/extras/skinediting/skinformeditview.h
// Description : Form Editing View
//
//************************************************************************************************

#ifndef _ccl_skinformeditview_h
#define _ccl_skinformeditview_h

#include "ccl/app/editing/editview.h"

#include "skinitempainter.h"

namespace CCL {

class SkinFormEditComponent;

//************************************************************************************************
// SkinFormEditView
//************************************************************************************************

class SkinFormEditView: public EditView
{
public:
	DECLARE_CLASS (SkinFormEditView, EditView)

	SkinFormEditView (SkinFormEditComponent* component = 0, RectRef size = Rect ());
	
	SkinItemPainter& getPainter () { return painter; }

	// EditView
	void attached (IView* parent) override;
	void draw (const DrawEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;

protected:
	SkinItemPainter painter;
};

} // namespace CCL

#endif // _ccl_skinformeditview_h
