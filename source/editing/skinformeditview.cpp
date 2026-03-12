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
// Filename    : skinformeditview.cpp
// Description : Form Editing View
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "skinformeditview.h"
#include "skinformeditor.h"

#include "ccl/public/gui/graphics/igraphics.h"

#if DEBUG_LOG
#include "ccl/app/editing/editmodel.h"
#endif

using namespace CCL;

//************************************************************************************************
// SkinFormEditView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinFormEditView, EditView)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinFormEditView::SkinFormEditView (SkinFormEditComponent* component, RectRef size)
: EditView (component, size)
{
	ASSERT (component != nullptr)
	painter.setZoomFactor (component->getZoomFactor ());
	wantsToolActivation (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinFormEditView::attached (IView* parent)
{
	SuperClass::attached (parent);

	painter.updateStyle (getVisualStyle ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinFormEditView::draw (const DrawEvent& event)
{
	SkinFormEditComponent* formEditor = static_cast<SkinFormEditComponent*> (component);
	auto formItem = formEditor->getFormItem ();
	if(!formItem)
		return;

	int options = formEditor->getPaintOptions ();
	float zoomFactor = formEditor->getZoomFactor ();
	auto& selection = getSelection ();

	ASSERT (formItem->getSkinData ())
	Color backColor = formItem->getSkinData ()->getEditSettings ().getFormEditBackColor ();
	event.graphics.fillRect (event.updateRgn.bounds, SolidBrush (backColor));

	// experimental zoom (incomplete, need to handle mouse input as well)
	//TransformSetter ts (event.graphics, Transform ().scale (painter.getZoomFactor (), painter.getZoomFactor ()));

	painter.drawRecursive (event.graphics, formItem, formItem, options);

	if(!selection.isEmpty ())
		painter.drawSelection (event.graphics, selection);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditView::onMouseDown (const MouseEvent& event)
{
	// test find items:
	#if (1 && DEBUG_LOG)
	auto& model = component->getModel ();
	AutoPtr<Object> item = model.findItemDeep (*this, event.where);
	if(auto handle = ccl_cast<SkinItemHandle> (item))
		CCL_PRINTF ("Found handle: item = %p alignment = 0x%02X\n", handle->getViewItem (), handle->getAlignment ().align)
	else
		CCL_PRINTF ("Found item: %p\n", static_cast<IObject*> (item))
	#endif

	return SuperClass::onMouseDown (event);
}
