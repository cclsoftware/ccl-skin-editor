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
// Filename    : skinitempainter.h
// Description : Skin Item Painter
//
//************************************************************************************************

#ifndef _ccl_skinitempainter_h
#define _ccl_skinitempainter_h

#include "skineditdata.h"

#include "ccl/public/gui/graphics/types.h"

namespace CCL {

interface IVisualStyle;
class Selection;

//************************************************************************************************
// SkinItemHandle
//************************************************************************************************

class SkinItemHandle: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (SkinItemHandle, Object)

	SkinItemHandle (SkinViewItem* _viewItem, AlignmentRef _alignment)
	: alignment (_alignment)
	{
		setViewItem (_viewItem);
	}

	PROPERTY_OBJECT (Alignment, alignment, Alignment)
	PROPERTY_SHARED_AUTO (SkinViewItem, viewItem, ViewItem)
};

//************************************************************************************************
// SkinItemPainter
//************************************************************************************************

class SkinItemPainter
{
public:
	SkinItemPainter ();

	PROPERTY_VARIABLE (Color, outlineColor, OutlineColor)
	PROPERTY_VARIABLE (Color, outlineColorDimmed, OutlineColorDimmed)
	PROPERTY_VARIABLE (Color, textColor, TextColor)
	PROPERTY_OBJECT (Font, textFont, TextFont)
	PROPERTY_VARIABLE (Color, selectionColor, SelectionColor)
	PROPERTY_VARIABLE (Coord, selectionWidth, SelectionWidth)
	
	PROPERTY_VARIABLE (float, zoomFactor, ZoomFactor)

	void updateStyle (const IVisualStyle& style);

	void getItemSize (Rect& size, const SkinViewItem* viewItem) const;
	AutoPtr<SkinItemHandle> findHandle (SkinViewItem* viewItem, PointRef where) const;
	SkinViewItem* findItem (SkinViewItem* parentItem, SkinFormItem* formItem, PointRef where) const;
	SkinViewItem* findItemInSelection (const Selection& selection, PointRef where) const;
	void addItemsToSelection (Selection& selection, SkinFormItem* formItem, RectRef rect) const;

	enum DrawOptions
	{
		kDrawBoundingBox = 1<<0
	};

	void drawRecursive (IGraphics& graphics, SkinViewItem* viewItem, SkinFormItem* formItem, int options) const;

	void drawSelection (IGraphics& graphics, const Selection& selection) const;

protected:
	static const Coord kHandleSize = 8;
	static const Coord kEdgeSize = 4;
	static const int handleAlignments[];
	static const int edgeAlignments[];

	Rect getHandleRect (RectRef itemSize, AlignmentRef alignment) const;
	Rect getEdgeRect (RectRef itemSize, AlignmentRef alignment) const;

	SkinViewItem* testFindItem (SkinViewItem* childItem, SkinFormItem* formItem, PointRef where) const;

	void drawSelectionRect (IGraphics& graphics, RectRef itemSize, PenRef pen) const;
};

} // namespace CCL

#endif // _ccl_skinitempainter_h
