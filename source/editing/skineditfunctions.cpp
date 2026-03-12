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
// Filename    : skineditfunctions.cpp
// Description : Skin Editing Functions
//
//************************************************************************************************

#include "skineditfunctions.h"

#include "ccl/app/utilities/boxedguitypes.h"

#include "ccl/public/text/translation.h"

namespace CCL {

//************************************************************************************************
// SkinAttributeAction
//************************************************************************************************

class SkinAttributeAction: public Action
{
public:
	SkinAttributeAction (SkinEditItem* _item,
						 const SkinValueChange& _valueChange,
						 StringRef description)
	: Action (description),
	  item (_item),
	  valueChange (_valueChange)
	{
		ASSERT (item != nullptr)
		item->retain ();
		setDetailedDescription (String () << _valueChange.name);
	}

	PROPERTY_SHARED_AUTO (SkinEditItem, item, Item)
	PROPERTY_OBJECT (SkinValueChange, valueChange, ValueChange)

protected:
	// Action
	bool execute () override
	{
		Variant oldValue;
		item->getElement ().getAttributeValue (oldValue, valueChange.name);
		oldValue.share ();
		item->getElement ().setAttributeValue (valueChange.name, valueChange.value);
		valueChange.value = oldValue; // swap values for undo/redo

		ASSERT (item->getSkinData ())
		item->getSkinData ()->itemChanged (item, valueChange.name);
		return true;
	}
};

//************************************************************************************************
// SkinAttributeInsertAction
//************************************************************************************************

class SkinAttributeInsertAction: public SkinAttributeAction
{
public:
	SkinAttributeInsertAction (SkinEditItem* item, const SkinValueChange& valueChange, int index, bool mustAdd,
							   StringRef description)
	: SkinAttributeAction (item, valueChange, description),
	  index (index),
	  mustAdd (mustAdd)
	{}

protected:
	int index;
	bool mustAdd;

	// SkinAttributeAction
	bool execute () override
	{
		if(mustAdd)
		{
			item->getElement ().setAttributeValue (valueChange.name, valueChange.value, index);
			mustAdd = false;
		}
		else
		{
			item->getElement ().getAttributeValue (valueChange.value, valueChange.name);
			valueChange.value.share ();
			item->getElement ().removeAttribute (valueChange.name, &index);
			mustAdd = true;
		}

		ASSERT (item->getSkinData ())
		item->getSkinData ()->itemChanged (item, valueChange.name);
		return true;
	}
};

//************************************************************************************************
// SkinViewResizeAction
//************************************************************************************************

class SkinViewResizeAction: public SkinAttributeAction
{
public:
	SkinViewResizeAction (SkinViewItem* viewItem, const SkinValueChange& valueChange, RectRef newSize)
	: SkinAttributeAction (viewItem, valueChange, ActionDescriptions::ResizeView ()),
	  newSize (newSize)
	{
		setDetailedDescription (valueChange.value.asString ());
	}

protected:
	Rect newSize;

	// SkinAttributeAction
	bool execute () override
	{
		// TODO: this could happen as side effect somewhere...
		auto viewItem = static_cast<SkinViewItem*> (static_cast<SkinEditItem*> (item));
		Rect oldSize (viewItem->getSize ());
		viewItem->setSize (newSize);
		newSize = oldSize;

		return SkinAttributeAction::execute ();
	}
};

//************************************************************************************************
// SkinViewInsertAction
//************************************************************************************************

class SkinViewInsertAction: public Action
{
public:
	SkinViewInsertAction (SkinViewItem* parentItem, SkinViewItem* childItem, int index, bool mustAdd,
						  StringRef description)
	: Action (description),
	  parentItem (parentItem),
	  childItem (childItem),
	  index (index),
	  mustAdd (mustAdd)
	{
		ASSERT (parentItem && childItem)
	}

protected:
	SharedPtr<SkinViewItem> parentItem;
	SharedPtr<SkinViewItem> childItem;
	int index;
	bool mustAdd;

	// Action
	bool execute () override
	{
		if(mustAdd)
		{
			childItem->retain ();
			parentItem->addChild (childItem, index);
			ASSERT (parentItem->getSkinData ())
			childItem->setSourceFileDeep (parentItem->getSourceFile ());
			mustAdd = false;
		}
		else
		{
			parentItem->removeChild (childItem, &index);
			childItem->setSourceFileDeep (String::kEmpty);
			childItem->release ();
			mustAdd = true;
		}
		return true;
	}
};

//************************************************************************************************
// SkinItemInsertAction
//************************************************************************************************

class SkinItemInsertAction: public Action
{
public:
	SkinItemInsertAction (SkinEditData* skinData, SkinEditData::ContainerType type, SkinEditItem* item,
						  int index, bool mustAdd, StringRef description)
	: Action (description),
	  skinData (skinData),
	  type (type),
	  item (item),
	  index (index),
	  mustAdd (mustAdd)
	{
		ASSERT (item)
	}

protected:
	SkinEditData* skinData; // avoid circular reference!
	SkinEditData::ContainerType type;
	SharedPtr<SkinEditItem> item;
	int index;
	bool mustAdd;

	// Action
	bool execute () override
	{
		if(mustAdd)
		{
			item->retain ();
			skinData->addItem (type, item, index);
			mustAdd = false;
		}
		else
		{
			bool removed = skinData->removeItem (type, item, &index);
			ASSERT (removed == true)
			item->release ();
			mustAdd = true;
		}
		return true;
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

// XSTRINGS_OFF - hint for xstring tool to skip this section

BEGIN_XSTRINGS ("SkinEditing")
	XSTRING (ModifyAttribute, "Modify Attribute")
	XSTRING (ModifyAttributes, "Modify Attributes")
	XSTRING (AddAttribute, "Add Attribute")
	XSTRING (AddAttributes, "Add Attributes")
	XSTRING (RemoveAttribute, "Remove Attribute")
	XSTRING (RemoveAttributes, "Remove Attributes")
	XSTRING (ResizeView, "Resize View")
	XSTRING (ResizeViews, "Resize Views")
	XSTRING (DragView, "Drag View")
	XSTRING (DragViews, "Drag Views")
	XSTRING (AddView, "Add View")
	XSTRING (AddViews, "Add Views")
	XSTRING (RemoveView, "Remove View")
	XSTRING (RemoveViews, "Remove Views")
	XSTRING (AddForm, "Add Form")
	XSTRING (RemoveForm, "Remove Form")
	XSTRING (RemoveForms, "Remove Forms")
	XSTRING (AddImage, "Add Image")
	XSTRING (AddImages, "Add Images")
	XSTRING (RemoveImage, "Remove Image")
	XSTRING (RemoveImages, "Remove Images")
	XSTRING (RemoveItem, "Remove Item")
	XSTRING (RemoveItems, "Remove Items")
	XSTRING (AddFont, "Add Font")
	XSTRING (AddStyle, "Add Style")
END_XSTRINGS

//************************************************************************************************
// ActionDescriptions
//************************************************************************************************

DEFINE_ACTION_DESCRIPTION (ModifyAttribute, ModifyAttribute, ModifyAttributes)
DEFINE_ACTION_DESCRIPTION (AddAttribute, AddAttribute, AddAttributes)
DEFINE_ACTION_DESCRIPTION (RemoveAttribute, RemoveAttribute, RemoveAttributes)
DEFINE_ACTION_DESCRIPTION (ResizeView, ResizeView, ResizeViews)
DEFINE_ACTION_DESCRIPTION (DragView, DragView, DragViews)
DEFINE_ACTION_DESCRIPTION (AddView, AddView, AddViews)
DEFINE_ACTION_DESCRIPTION (RemoveView, RemoveView, RemoveViews)
DEFINE_ACTION_DESCRIPTION_SIMPLE (AddForm, AddForm)
DEFINE_ACTION_DESCRIPTION (RemoveForm, RemoveForm, RemoveForms)
DEFINE_ACTION_DESCRIPTION (AddImage, AddImage, AddImages)
DEFINE_ACTION_DESCRIPTION (RemoveImage, RemoveImage, RemoveImages)
DEFINE_ACTION_DESCRIPTION (RemoveItem, RemoveItem, RemoveItems)
DEFINE_ACTION_DESCRIPTION_SIMPLE (AddFont, AddFont)
DEFINE_ACTION_DESCRIPTION_SIMPLE (AddStyle, AddStyle)

//************************************************************************************************
// SkinEditFunctions
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkinEditFunctions, ActionExecuter)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditFunctions::SkinEditFunctions (SkinEditData& skinData)
: ActionExecuter (&skinData),
  skinData (skinData)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditFunctions::modifyAttribute (SkinEditItem* item, const SkinValueChange& valueChange, StringRef _description)
{
	ASSERT (item != nullptr)
	if(!item)
		return false;

	bool exists = item->hasAttribute (valueChange.name);

	String description (_description);
	if(description.isEmpty ())
		description = exists ? ActionDescriptions::ModifyAttribute () : ActionDescriptions::AddAttribute ();

	if(exists)
		return execute (NEW SkinAttributeAction (item, valueChange, description));
	else
		return execute (NEW SkinAttributeInsertAction (item, valueChange, -1, true, description));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditFunctions::removeAttribute (SkinEditItem* item, StringID name)
{
	ASSERT (item != nullptr)
	if(!item)
		return false;

	String description = ActionDescriptions::RemoveAttribute ();
	return execute (NEW SkinAttributeInsertAction (item, SkinValueChange (name), -1, false, description));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditFunctions::resizeView (SkinViewItem* viewItem, RectRef newSize)
{
	auto editSupport = skinData.getEditSupport ();
	ASSERT (editSupport && viewItem)
	if(!editSupport || !viewItem)
		return false;

	SkinValueChange valueChange;
	if(!editSupport->getSizeChange (valueChange, &viewItem->getViewElement (), newSize))
		return false;

	return execute (NEW SkinViewResizeAction (viewItem, valueChange, newSize));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditFunctions::addView (SkinViewItem* parentItem, SkinViewItem* childItem, int index)
{
	ASSERT (parentItem != nullptr && childItem != nullptr)
	if(!parentItem || !childItem)
		return false;

	return execute (NEW SkinViewInsertAction (parentItem, childItem, index, true, ActionDescriptions::AddView ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditFunctions::removeView (SkinViewItem* parentItem, SkinViewItem* childItem)
{
	ASSERT (parentItem != nullptr && childItem != nullptr)
	if(!parentItem || !childItem)
		return false;

	return execute (NEW SkinViewInsertAction (parentItem, childItem, -1, false, ActionDescriptions::RemoveView ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinFormItem* SkinEditFunctions::addForm (StringID formName, StringRef sourceFile, StringRef comment, int index)
{
	ASSERT (!formName.isEmpty () && !sourceFile.isEmpty ())
	if(formName.isEmpty () || sourceFile.isEmpty ())
		return nullptr;

	AutoPtr<SkinFormItem> formItem = ccl_cast<SkinFormItem> (skinData.createItem (ClassID::FormElement));
	ASSERT (formItem)
	if(!formItem)
		return nullptr;

	formItem->getElement ().setName (formName);
	formItem->getElement ().setSourceFile (sourceFile);
	formItem->getElement ().setComment (comment);

	execute (NEW SkinItemInsertAction (&skinData, SkinEditData::kForms, formItem, index, true, ActionDescriptions::AddForm ()));
	return formItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinImageItem* SkinEditFunctions::addImage (StringID imageName, StringRef imagePath, IImage* image, int index)
{
	ASSERT (!imageName.isEmpty () && !imagePath.isEmpty ())
	if(imageName.isEmpty () || imagePath.isEmpty ())
		return nullptr;

	AutoPtr<SkinImageItem> imageItem = ccl_cast<SkinImageItem> (skinData.createItem (ClassID::ImageElement));
	ASSERT (imageItem)
	if(!imageItem)
		return nullptr;

	imageItem->getElement ().setName (imageName);
	imageItem->getImageElement ().setImagePath (imagePath);
	imageItem->getImageElement ().setImage (image);

	execute (NEW SkinItemInsertAction (&skinData, SkinEditData::kImages, imageItem, index, true, ActionDescriptions::AddImage ()));
	return imageItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditItem* SkinEditFunctions::addFont (StringID fontName, int index)
{
	ASSERT (!fontName.isEmpty ())
	if(fontName.isEmpty ())
		return nullptr;

	AutoPtr<SkinEditItem> fontItem = ccl_cast<SkinEditItem> (skinData.createItem (ClassID::FontElement));
	ASSERT (fontItem)
	if(!fontItem)
		return nullptr;

	fontItem->getElement ().setName (fontName);
	execute (NEW SkinItemInsertAction (&skinData, SkinEditData::kFonts, fontItem, index, true, ActionDescriptions::AddFont ()));
	return fontItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditItem* SkinEditFunctions::addStyle (StringID styleName, int index)
{
	ASSERT (!styleName.isEmpty ())
	if(styleName.isEmpty ())
		return nullptr;

	AutoPtr<SkinEditItem> styleItem = ccl_cast<SkinEditItem> (skinData.createItem (ClassID::StyleElement));
	ASSERT (styleItem)
	if(!styleItem)
		return nullptr;

	styleItem->getElement ().setName (styleName);
	execute (NEW SkinItemInsertAction (&skinData, SkinEditData::kStyles, styleItem, index, true, ActionDescriptions::AddStyle ()));
	return styleItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditFunctions::removeItem (SkinEditItem* item, StringRef _description)
{
	auto type = skinData.getTypeForItem (item);
	ASSERT (type != SkinEditData::kUndefined)
	if(type == SkinEditData::kUndefined)
		return false;

	String description (_description);
	if(description.isEmpty ())
		description = XSTR (RemoveItem);

	return execute (NEW SkinItemInsertAction (&skinData, type, item, -1, false, description));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (SkinEditFunctions)
	DEFINE_METHOD_ARGS ("resizeView", "viewItem, newSize")
END_METHOD_NAMES (SkinEditFunctions)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinEditFunctions::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "resizeView")
	{
		auto viewItem = unknown_cast<SkinViewItem> (msg[0].asUnknown ());
		auto newSize = unknown_cast<Boxed::Rect> (msg[1].asUnknown ());
		ASSERT (viewItem && newSize)
		if(viewItem && newSize)
			returnValue = resizeView (viewItem, *newSize);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
