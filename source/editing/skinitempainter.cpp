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
// Filename    : skinitempainter.cpp
// Description : Skin Item Painter
//
//************************************************************************************************

#include "skinitempainter.h"

#include "ccl/app/editing/selection.h"

#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/framework/ivisualstyle.h"

using namespace CCL;

//************************************************************************************************
// SkinItemHandle
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkinItemHandle, Object)

//************************************************************************************************
// SkinItemPainter
//************************************************************************************************

const int SkinItemPainter::handleAlignments[] =
{
	Alignment::kLeftTop,
	Alignment::kRightTop,
	Alignment::kLeftBottom,
	Alignment::kRightBottom
};

const int SkinItemPainter::edgeAlignments[] =
{
	Alignment::kLeft,
	Alignment::kRight,
	Alignment::kTop,
	Alignment::kBottom
};

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinItemPainter::SkinItemPainter ()
: outlineColor (Colors::kRed),
  outlineColorDimmed (Colors::kLtGray),
  textColor (Colors::kBlack),
  selectionColor (Colors::kYellow),
  selectionWidth (2),
  zoomFactor (1.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinItemPainter::updateStyle (const IVisualStyle& style)
{
	outlineColor = style.getColor ("outlinecolor", outlineColor);
	outlineColorDimmed = style.getColor ("outlinecolor.dimmed", outlineColorDimmed);
	textColor = style.getTextColor ();
	textFont = style.getTextFont ();
	selectionColor = style.getColor ("selectioncolor", selectionColor);
	selectionWidth = style.getMetric<Coord> ("selectionwidth", selectionWidth);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect SkinItemPainter::getHandleRect (RectRef itemSize, AlignmentRef alignment) const
{
	Rect handleRect (0, 0, kHandleSize, kHandleSize);
	handleRect.align (itemSize, alignment);
	return handleRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect SkinItemPainter::getEdgeRect (RectRef itemSize, AlignmentRef alignment) const
{
	Rect edgeRect = itemSize;
	if(alignment.getAlignH () != 0) // left or right
		edgeRect.setWidth (kEdgeSize);
	else
		edgeRect.setHeight (kEdgeSize);
	edgeRect.align (itemSize, alignment);
	return edgeRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinItemPainter::getItemSize (Rect& itemSize, const SkinViewItem* viewItem) const
{
	viewItem->getClientRect (itemSize);
	Point p;
	viewItem->clientToForm (p);
	itemSize.offset (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoPtr<SkinItemHandle> SkinItemPainter::findHandle (SkinViewItem* viewItem, PointRef where) const
{
	if(!viewItem->isSelectable ())
		return nullptr;

	Rect itemSize;
	getItemSize (itemSize, viewItem);

	for(int alignment : handleAlignments)
	{
		Rect handleRect = getHandleRect (itemSize, alignment);
		if(handleRect.pointInside (where))
			return NEW SkinItemHandle (viewItem, alignment);
	}

	for(int alignment : edgeAlignments)
	{
		Rect edgeRect = getEdgeRect (itemSize, alignment);
		if(edgeRect.pointInside (where))
			return NEW SkinItemHandle (viewItem, alignment);
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewItem* SkinItemPainter::findItem (SkinViewItem* parentItem, SkinFormItem* formItem, PointRef where) const
{
	if(parentItem->isVariantOrTabView ())
	{
		if(auto childItem = parentItem->getSelectedChildItem ())
			if(auto result = testFindItem (childItem, formItem, where))
				return result;
	}
	else
	{
		ForEachReverse (parentItem->getChildren (), SkinViewItem, childItem)
			if(auto result = testFindItem (childItem, formItem, where))
				return result;
		EndFor			
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewItem* SkinItemPainter::testFindItem (SkinViewItem* childItem, SkinFormItem* formItem, PointRef where) const
{
	if(!childItem->isSelectable ())
		return nullptr;

	Point where2 (where);
	RectRef size = childItem->getSize ();
	where2.offset (-size.left, -size.top);
	// TODO: determine visible client...
	Rect clientRect;
	childItem->getClientRect (clientRect);
	if(clientRect.pointInside (where2) || childItem == formItem) // allow items outside form...
	{
		SkinViewItem* result = findItem (childItem, formItem, where2);
		if(result)
			return result;
				
		if(childItem == formItem) // ...but return the form only when inside
			return clientRect.pointInside (where2) ? formItem : nullptr;
		else
			return childItem;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewItem* SkinItemPainter::findItemInSelection (const Selection& selection, PointRef where) const
{
	IterForEachReverse (selection.newIterator<SkinViewItem> (), SkinViewItem, viewItem)
		Rect itemSize;
		getItemSize (itemSize, viewItem);
		if(itemSize.pointInside (where))
			return viewItem;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinItemPainter::addItemsToSelection (Selection& selection, SkinFormItem* formItem, RectRef rect) const
{
	struct Helper
	{
		const SkinItemPainter& painter;
		Selection& selection;
		
		Helper (const SkinItemPainter& painter, Selection& selection)
		: painter (painter),
		  selection (selection)
		{}

		bool testAdd (SkinViewItem* item, RectRef rect)
		{
			if(!item->isSelectable ())
				return false;

			Rect itemSize;
			painter.getItemSize (itemSize, item);
			if(rect.rectInside (itemSize))
			{
				selection.select (item);
				return true;
			}
			return false;
		}

		void findAll (SkinViewItem* parentItem, RectRef rect)
		{			
			if(parentItem->isVariantOrTabView ())
			{
				if(auto childItem = parentItem->getSelectedChildItem ())
				{
					if(!testAdd (childItem, rect))
						findAll (childItem, rect);
				}
			}
			else for(auto childItem : iterate_as<SkinViewItem> (parentItem->getChildren ()))
			{
				if(!testAdd (childItem, rect))
					findAll (childItem, rect);
			}
		}
	};

	Helper (*this, selection).findAll (formItem, rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinItemPainter::drawRecursive (IGraphics& graphics, SkinViewItem* viewItem, SkinFormItem* formItem, int options) const
{
	if(viewItem->isHidden ())
		return;

	RectRef viewRect = viewItem->getSize ();
	//ClipSetter cs (graphics, viewRect);
	bool drawn = false;
	if(auto editSupport = formItem->getSkinData ()->getEditSupport ())
	{
		if(viewItem == formItem)
			drawn = editSupport->drawFormBackground (graphics, &viewItem->getViewElement ());
		else
		{
			drawn = editSupport->drawViewElement (graphics, &viewItem->getViewElement ());

			if(drawn == false) // check for reference to other form
				if(auto otherForm = formItem->getSkinData ()->getReferencedFormItem (viewItem))
				{
					Rect savedSize (otherForm->getSize ());
					Rect newSize (savedSize);
					newSize.moveTo (viewRect.getLeftTop ());
					otherForm->setSize (newSize);
							
					drawRecursive (graphics, otherForm, otherForm, options);
							
					otherForm->setSize (savedSize);
					drawn = true;
				}
		}
	}
			
	if((options & kDrawBoundingBox) != 0 || drawn == false)
	{
		Pen pen;
		pen.setColor (drawn ? outlineColorDimmed : outlineColor);
		graphics.drawRect (viewRect, pen);
		if(drawn == false)
			graphics.drawString (viewRect, String (viewItem->getElement ().getName ()), textFont, SolidBrush (textColor), Alignment::kLeftTop);
	}

	if(!viewItem->getChildren ().isEmpty ())
	{
		TransformSetter ts (graphics, Transform ().translate ((float)viewRect.left, (float)viewRect.top));
		if(viewItem->isVariantOrTabView ())
		{
			if(auto childItem = viewItem->getSelectedChildItem ())
				drawRecursive (graphics, childItem, formItem, options);
		}
		else
		{
			for(auto childItem : iterate_as<SkinViewItem> (viewItem->getChildren ()))
				drawRecursive (graphics, childItem, formItem, options);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinItemPainter::drawSelection (IGraphics& graphics, const Selection& selection) const
{
	IterForEach (selection.newIterator<SkinViewItem> (), SkinViewItem, viewItem)
		Rect itemSize;
		getItemSize (itemSize, viewItem);
		drawSelectionRect (graphics, itemSize, Pen (selectionColor, (float)selectionWidth));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinItemPainter::drawSelectionRect (IGraphics& graphics, RectRef itemSize, PenRef pen) const
{
	graphics.drawRect (itemSize, pen);
	
	ClipSetter cs (graphics, itemSize);
	Rect handleRect (0, 0, kHandleSize, kHandleSize);
	for(int alignment : handleAlignments)
		graphics.drawRect (getHandleRect (itemSize, alignment), pen);
}
