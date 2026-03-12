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
// Filename    : skinformeditmodel.cpp
// Description : Form Editing Model
//
//************************************************************************************************

#include "skinformeditmodel.h"
#include "skinformeditor.h"
#include "skinformeditview.h"
#include "skineditfunctions.h"

#include "ccl/app/editing/edithandler.h"
#include "ccl/app/editing/editdraghandler.h"
#include "ccl/app/editing/selectaction.h"
#include "ccl/app/editing/tasks/edittaskdescription.h"
#include "ccl/app/editing/tasks/edittaskhandler.h"
#include "ccl/app/utilities/multisprite.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// SkinEditTaskCandidate
//************************************************************************************************

class SkinEditTaskCandidate: public EditTaskCandidate
{
public:
	SkinEditTaskCandidate (SkinFormEditModel& model)
	: model (model)
	{}

	// EditTaskCandidate
	void prepare (EditTaskContext& context) override
	{
		AutoPtr<Iterator> iterator = model.getSelection ().newIterator<SkinViewItem> ();
		context.setObject ("iterator", iterator);

		if(auto focusItem = model.getFormEditor ()->getFocusViewItem ())
			context.setObject ("focusItem", focusItem);

		AutoPtr<SkinEditFunctions> functions = NEW SkinEditFunctions (*getSkinData ());
		context.setObject ("functions", functions);
	}

	IActionContext* getActionContext () override
	{
		return getSkinData ();
	}

protected:
	SkinFormEditModel& model;

	SkinEditData* getSkinData ()
	{
		auto formItem = model.getFormEditor ()->getFormItem ();
		ASSERT (formItem && formItem->getSkinData ())
		return formItem->getSkinData ();
	}
};

//************************************************************************************************
// SkinViewItemDragHandler
//************************************************************************************************

class SkinViewItemDragHandler: public EditDragHandler
{
public:
	DECLARE_CLASS_ABSTRACT (SkinViewItemDragHandler, EditDragHandler)

	SkinViewItemDragHandler (SkinFormEditView& editView);

	bool prepare (const DragEvent& event);

	// EditDragHandler
	tbool CCL_API dragEnter (const DragEvent& event) override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	void moveSprite (const DragEvent& event) override;
	tbool CCL_API afterDrop (const DragEvent& event) override;

protected:
	bool ownSelection;

	class ViewItemDrawable: public SolidDrawable
	{
	public:
		using SolidDrawable::SolidDrawable;
		PROPERTY_SHARED_AUTO (SkinViewItem, viewItem, ViewItem)
	};
};

//************************************************************************************************
// SkinViewItemResizeHandler
//************************************************************************************************

class SkinViewItemResizeHandler: public EditHandler
{
public:
	DECLARE_CLASS_ABSTRACT (SkinViewItemResizeHandler, EditHandler)

	SkinViewItemResizeHandler (EditView& editView, SkinItemHandle* handle);

	// EditHandler
	void onBegin () override;
	bool onMove (int moveFlags) override;
	void onRelease (bool canceled) override;

protected:
	SharedPtr<SkinItemHandle> handle;
	SkinFormEditComponent::GridSettings gridSettings;

	struct SizeCandidate
	{
		SharedPtr<SkinViewItem> viewItem;
		Rect oldSize;
	};
	Vector<SizeCandidate> candidates;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

// XSTRINGS_OFF - hint for xstring tool to skip this section

BEGIN_XSTRINGS ("SkinEditing")
	XSTRING (SkinEdit, "Skin")
	XSTRING (SkinLayout, "Layout")
END_XSTRINGS

CCL_KERNEL_INIT_LEVEL (SkinEditTask, kFirstRun)
{
	// define edit task category
	EditTaskDescription::defineCategory (SkinFormEditModel::kSkinEditCategory, XSTR_REF (SkinEdit));
	EditTaskDescription::defineMenuGroup ("SkinLayout", XSTR (SkinLayout));
	return true;
}

//************************************************************************************************
// SkinFormEditModel
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkinFormEditModel, EditModel)
const String SkinFormEditModel::kSkinEditCategory = CCLSTR ("SkinEdit");

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinFormEditModel::SkinFormEditModel (SkinFormEditComponent* component)
: EditModel (component)
{
	ASSERT (component != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinFormEditComponent* SkinFormEditModel::getFormEditor ()
{
	return static_cast<SkinFormEditComponent*> (component);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* SkinFormEditModel::resizeItems (EditView& editView, SkinItemHandle* handle)
{
	return NEW SkinViewItemResizeHandler (editView, handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditModel::containsAnyItems ()
{
	return getFormEditor ()->getFormItem () != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* SkinFormEditModel::findItem (EditView& editView, PointRef where)
{
	auto formEditView = ccl_cast<SkinFormEditView> (&editView);
	auto formItem = getFormEditor ()->getFormItem ();

	SkinViewItem* resultItem = nullptr;
	if(formEditView && formItem)
		resultItem = formEditView->getPainter ().findItem (formItem, formItem, where);

	#if (0 && DEBUG_LOG)
	if(resultItem)
	{
		MutableCString className;
		if(auto elementClass = resultItem->getElement ().getElementClass ())
			className = elementClass->getClassName ();
		CCL_PRINTF ("Found view of class: %s\n", className.str ())
	}
	else
		CCL_PRINTLN ("No view item found.")
	#endif

	return return_shared (resultItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* SkinFormEditModel::findItemPart (EditView& editView, Object* item, PointRef where)
{
	auto formEditView = ccl_cast<SkinFormEditView> (&editView);
	auto viewItem = ccl_cast<SkinViewItem> (item);
	if(formEditView && viewItem)
	{
		if(getSelection ().isSelected (viewItem))
			return formEditView->getPainter ().findHandle (viewItem, where).detach ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* SkinFormEditModel::findItemDeep (EditView& editView, PointRef where)
{
	// prefer selection
	auto formEditView = ccl_cast<SkinFormEditView> (&editView);
	auto& selection = getSelection ();
	if(formEditView && !selection.isEmpty ())
	{
		auto& painter = formEditView->getPainter ();
		if(auto viewItem = painter.findItemInSelection (selection, where))
		{
			auto handle = painter.findHandle (viewItem, where);
			if(handle)
				return handle.detach ();
			else
				return return_shared (viewItem);
		}
	}

	return SuperClass::findItemDeep (editView, where);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditModel::getItemSize (Rect& size, EditView& editView, Object* item)
{
	auto formEditView = ccl_cast<SkinFormEditView> (&editView);
	auto viewItem = ccl_cast<SkinViewItem> (item);
	if(formEditView && viewItem)
	{
		formEditView->getPainter ().getItemSize (size, viewItem);
		return true;
	}
	return SuperClass::getItemSize (size, editView, item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditModel::canSelectItem (Object* item)
{
	return ccl_cast<SkinViewItem> (item) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditModel::selectItems (EditView& editView, RectRef rect)
{
	auto formEditView = ccl_cast<SkinFormEditView> (&editView);
	auto formItem = getFormEditor ()->getFormItem ();
	if(!formItem || !formEditView)
		return false;

	formEditView->getPainter ().addItemsToSelection (getSelection (), formItem, rect);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditModel::setFocusItem (Object* item, EditView* editView)
{
	if(auto viewItem = ccl_cast<SkinViewItem> (item))
	{
		auto formEditor = getFormEditor ();
		formEditor->onViewItemFocused (viewItem, true);
	}

	SuperClass::setFocusItem (item, editView);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* SkinFormEditModel::createDragHandler (EditView& editView, const DragEvent& event)
{
	if(getFormEditor ()->getFormItem () != nullptr)
		if(auto formEditView = ccl_cast<SkinFormEditView> (&editView))
		{
			AutoPtr<SkinViewItemDragHandler> dragHandler = NEW SkinViewItemDragHandler (*formEditView);
			if(dragHandler->prepare (event))
				return dragHandler.detach ();
		}

	return SuperClass::createDragHandler (editView, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinFormEditModel::getSelectedOnce (Container& candidates)
{
	auto insertOnce = [&] (SkinViewItem* item)
	{
		ObjectArray toRemove;
		for(auto candidate : iterate_as<SkinViewItem> (candidates))
		{
			if(item->hasParent (candidate)) // parent already in list
				return;
			if(item->hasChild (candidate)) // item is parent, remove other
				toRemove.addOnce (candidate);
		}

		if(!toRemove.isEmpty ())
			for(auto tr : iterate_as<SkinViewItem> (candidates))
				candidates.remove (tr);

		candidates.addOnce (item);
	};

	IterForEach (getSelection ().newIterator<SkinViewItem> (), SkinViewItem, viewItem)
		if(ccl_cast<SkinFormItem> (viewItem) == nullptr) // exclude form!
			insertOnce (viewItem);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditModel::deleteSelected ()
{
	auto formItem = getFormEditor ()->getFormItem ();
	if(!formItem)
		return false;

	ObjectArray candidates;
	getSelectedOnce (candidates);
	if(!candidates.isEmpty ())
	{
		ASSERT (formItem->getSkinData ())
		SkinEditFunctions functions (*formItem->getSkinData ());
		functions.beginMultiple (ActionDescriptions::RemoveView (candidates.count () > 1));
		for(auto candidate : iterate_as<SkinViewItem> (candidates))
			functions.removeView (candidate->getParentViewItem (), candidate);
		functions.endMultiple ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditModel::deleteItem (Object* item)
{
	CCL_NOT_IMPL ("Do we need this???")
	return SuperClass::deleteItem (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* SkinFormEditModel::copySelected (bool shared, bool forDuplicate)
{
	if(!getSelection ().isEmpty ())
	{
		ObjectArray candidates;
		getSelectedOnce (candidates);

		auto candidatesCopy = static_cast<ObjectArray*> (candidates.clone ());
		candidatesCopy->objectCleanup (true); // FIX ME: should be done by container class during clone!
		return candidatesCopy;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditModel::canInsertData (Object* data)
{
	if(auto container = ccl_cast<Container> (data))
		for(auto object : *container)
			if(ccl_cast<SkinViewItem> (object)) // TODO: check skin model compatibility!
				return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditModel::insertData (Object* data)
{
	auto formItem = getFormEditor ()->getFormItem ();
	if(!formItem)
		return false;

	bool result = false;
	if(auto container = ccl_cast<Container> (data))
	{
		SkinViewItem* selectedParentItem = getFormEditor ()->getSelectedParentItem ();
		ASSERT (selectedParentItem != nullptr)

		ASSERT (formItem->getSkinData ())
		SkinEditFunctions functions (*formItem->getSkinData ());
		functions.beginMultiple (ActionDescriptions::AddView (container->count () == 1));
		for(auto object : *container)
			if(auto viewItem = ccl_cast<SkinViewItem> (object)) // TODO: check skin model compatibility!
			{
				functions.addView (selectedParentItem, viewItem);
				result = true;
			}
		functions.endMultiple ();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinFormEditModel::collectTaskCategories (StringList& taskCategories)
{
	CCL_NOT_IMPL ("Do we need this???")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditModel::canPerformTask (EditView& editView, const EditTaskDescription& task)
{
	return task.getCategory () == kSkinEditCategory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditModel::collectTaskCandidates (Container& resultList, EditView& editView, const EditTaskDescription& task)
{
	if(task.getCategory () == kSkinEditCategory)
	{
		resultList.add (NEW SkinEditTaskCandidate (*this));
		return true;
	}
	return false;
}

//************************************************************************************************
// SkinViewItemDragHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkinViewItemDragHandler, EditDragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewItemDragHandler::SkinViewItemDragHandler (SkinFormEditView& editView)
: EditDragHandler (editView),
  ownSelection (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinViewItemDragHandler::prepare (const DragEvent& event)
{
	if(Selection* selection = unknown_cast<Selection> (event.session.getItems ().getFirst ()))
	{
		ownSelection = true;
		IterForEach (selection->newIterator<SkinViewItem> (), SkinViewItem, viewItem)
			if(ccl_cast<SkinFormItem> (viewItem) == nullptr) // can't drag the form!
				data.add (viewItem->asUnknown (), true);
		EndFor
	}
	else
	{
		ownSelection = false;

		// TODO:
		// - convert SkinImageItem to SkinViewItem
		// - import Image if it doesn't exist yet

		ForEachUnknown (event.session.getItems (), unk)
			if(SkinViewClass* viewClass = unknown_cast<SkinViewClass> (unk))
			{
				AutoPtr<SkinViewItem> viewItem = viewClass->createViewItem ();
				if(viewItem)
				{
					Rect viewRect (viewItem->getSize ()); // could be set via default
					if(viewRect.isEmpty ())
						viewRect (0, 0, 200, 50);
					viewRect.moveTo (event.where);
					viewItem->setSize (viewRect);
					data.add (viewItem->asUnknown (), true);
				}
			}
		EndFor
	}
	return !data.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinViewItemDragHandler::dragEnter (const DragEvent& event)
{
	ITheme& theme = editView.getTheme ();
	Color spriteColor = theme.getThemeColor (ThemeElements::kAlphaSelectionColor);

	MultiSprite* multiSprite = NEW MultiSprite;
	ForEachUnknown (data, unk)
		if(auto viewItem = unknown_cast<SkinViewItem> (unk))
		{
			Rect viewRect;
			editView.getModel ().getItemSize (viewRect, editView, viewItem);
			SOFT_ASSERT (!viewRect.isEmpty (), "Empty view rect!?")
			if(viewRect.isEmpty ())
			{
				viewRect.setWidth (5);
				viewRect.setHeight (5);
			}

			CCL_PRINTF ("Drag item %p (%d,%d - %dx%d)\n", viewItem,
						viewRect.left, viewRect.top, viewRect.getWidth (), viewRect.getHeight ())

			ISprite* sprite = ccl_new<ISprite> (ClassID::FloatingSprite);
			AutoPtr<ViewItemDrawable> drawable = NEW ViewItemDrawable (SolidBrush (spriteColor));
			drawable->setViewItem (viewItem);
			sprite->construct (editView, viewRect, drawable);
			multiSprite->addSprite (sprite);
		}
	EndFor
	setSprite (multiSprite);

	Rect totalSpriteRect = multiSprite->getSize ();
	setSpriteOffset (Point (-(event.where.x - totalSpriteRect.left), -(event.where.y - totalSpriteRect.top)));

	return SuperClass::dragEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinViewItemDragHandler::dragOver (const DragEvent& event)
{
	if(ownSelection == false || event.keys.isSet (kCopyRealModifer|kCopySharedModifer))
		event.session.setResult (IDragSession::kDropCopyReal);
	else
		event.session.setResult (IDragSession::kDropMove);

	return SuperClass::dragOver (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItemDragHandler::moveSprite (const DragEvent& event)
{
	auto formEditor = unknown_cast<SkinFormEditComponent> (editView.getController ());
	ASSERT (formEditor)
	auto gridSettings = formEditor->getGridSettings ();

	Point p (event.where + spriteOffset);
	gridSettings.snap (p.x);
	gridSettings.snap (p.y);

	SuperClass::moveSprite (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinViewItemDragHandler::afterDrop (const DragEvent& event)
{
	auto formEditor = unknown_cast<SkinFormEditComponent> (editView.getController ());
	ASSERT (formEditor)
	auto formItem = formEditor->getFormItem ();

	auto selectedParentItem = formEditor->getSelectedParentItem ();
	ASSERT (selectedParentItem)

	bool mustClone = ownSelection == true && event.keys.isSet (kCopyRealModifer|kCopySharedModifer);

	ASSERT (formItem && formItem->getSkinData ())
	SkinEditFunctions functions (*formItem->getSkinData ());
	functions.beginMultiple (ActionDescriptions::DragView (false));

	SelectFunctions selectFunctions (*formEditor, functions);
	selectFunctions.beginMultiple (String::kEmpty);
	if(mustClone == true || ownSelection == false)
	{
		selectFunctions.saveSelection ();
		selectFunctions.selectExclusive (true);
		selectFunctions.focusFirstSelected (&getEditView ());
	}

	ObjectArray toSelect;
	if(auto multiSprite = unknown_cast<MultiSprite> (getSprite ()))
		for(auto sprite : multiSprite->getSprites ())
		{
			Rect newRect (sprite->getSize ());
			auto drawable = static_cast<ViewItemDrawable*> (sprite->getDrawable ());
			auto viewItem = drawable->getViewItem ();
			ASSERT (ccl_cast<SkinFormItem> (viewItem) == nullptr) // sanity check: must not be the form!

			if(mustClone == true)
			{
				AutoPtr<SkinViewItem> viewItemCopy = static_cast<SkinViewItem*> (viewItem->clone ());
				ASSERT (viewItemCopy)

				functions.addView (selectedParentItem, viewItemCopy);
				selectedParentItem->formToClient (newRect);
				functions.resizeView (viewItemCopy, newRect);
				toSelect.add (viewItemCopy);
			}
			else
			{
				if(auto parentItem = viewItem->getParentViewItem ())
				{
					parentItem->formToClient (newRect);
					functions.resizeView (viewItem, newRect);
				}
				else
				{
					functions.addView (selectedParentItem, viewItem);
					selectedParentItem->formToClient (newRect);
					functions.resizeView (viewItem, newRect);
					toSelect.add (viewItem);
				}
			}
		}

	// select new views
	selectFunctions.select (toSelect);
	selectFunctions.endMultiple ();

	functions.endMultiple ();

	return SuperClass::afterDrop (event);
}

//************************************************************************************************
// SkinViewItemResizeHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkinViewItemResizeHandler, EditHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewItemResizeHandler::SkinViewItemResizeHandler (EditView& editView, SkinItemHandle* handle)
: EditHandler (&editView),
  handle (handle)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItemResizeHandler::onBegin ()
{
	auto& selection = getEditView ()->getSelection ();
	IterForEach (selection.newIterator<SkinViewItem> (), SkinViewItem, viewItem)
		candidates.add ({viewItem, viewItem->getSize ()});
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinViewItemResizeHandler::onMove (int moveFlags)
{
	auto formEditor = unknown_cast<SkinFormEditComponent> (getEditView ()->getController ());
	ASSERT (formEditor)
	auto gridSettings = formEditor->getGridSettings ();

	Point delta = current.where - first.where;

	auto adjustSize = [&] (SizeCandidate& c)
	{
		Rect newSize (c.oldSize);
		switch(handle->getAlignment ().getAlignH ())
		{
		case Alignment::kLeft :
			newSize.left += delta.x;
			gridSettings.snap (newSize.left);
			break;
		case Alignment::kRight :
			newSize.right += delta.x;
			gridSettings.snap (newSize.right);
			break;
		}

		switch(handle->getAlignment ().getAlignV ())
		{
		case Alignment::kTop :
			newSize.top += delta.y;
			gridSettings.snap (newSize.top);
			break;
		case Alignment::kBottom :
			newSize.bottom += delta.y;
			gridSettings.snap (newSize.bottom);
			break;
		}

		newSize.setWidth (get_max (newSize.getWidth (), 0));
		newSize.setHeight (get_max (newSize.getHeight (), 0));

		c.viewItem->setSize (newSize);
	};

	for(auto& c : candidates)
		adjustSize (c);

	getEditView ()->invalidate ();

	if(auto editSupport = handle->getViewItem ()->getSkinData ()->getEditSupport ())
	{
		SkinValueChange sizeChange;
		editSupport->getSizeChange (sizeChange, &handle->getViewItem ()->getViewElement (), handle->getViewItem ()->getSize ());
		setEditTooltip (String () << VariantString (sizeChange.value) << " [" << delta.x << " / " << delta.y << "]");
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItemResizeHandler::onRelease (bool canceled)
{
	auto formEditor = unknown_cast<SkinFormEditComponent> (getEditView ()->getController ());
	ASSERT (formEditor)
	auto formItem = formEditor->getFormItem ();
	ASSERT (formItem && formItem->getSkinData ())

	if(canceled)
	{
		for(auto& c : candidates)
			c.viewItem->setSize (c.oldSize);
	}
	else
	{
		SkinEditFunctions functions (*formItem->getSkinData ());
		functions.beginMultiple (ActionDescriptions::ResizeView (candidates.count () == 1));
		for(auto& c : candidates)
		{
			Rect newSize = c.viewItem->getSize ();
			c.viewItem->setSize (c.oldSize); // reset first to make undo work correctly
			functions.resizeView (c.viewItem, newSize);
		}
		functions.endMultiple ();
	}

	getEditView ()->invalidate ();

	SuperClass::onRelease (canceled);
}
