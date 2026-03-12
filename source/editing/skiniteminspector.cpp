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
// Filename    : skiniteminspector.cpp
// Description : Skin Item Inspector
//
//************************************************************************************************

#include "skiniteminspector.h"
#include "skineditfunctions.h"
#include "skinediticons.h"

#include "ccl/app/controls/listviewmodel.h"
#include "ccl/app/components/colorpicker.h"
#include "ccl/app/params.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// SkinPropertyListModel
//************************************************************************************************

class SkinPropertyListModel: public ListViewModel
{
public:
	DECLARE_CLASS_ABSTRACT (SkinPropertyListModel, ListViewModel)

	SkinPropertyListModel (SkinItemInspector& component);

	DECLARE_STRINGID_MEMBER (kValueID)
	DECLARE_STRINGID_MEMBER (kTypeID)

	void rebuild ();

	// ListViewModel
	tbool CCL_API canRemoveItem (ItemIndexRef index) override;
	bool removeItems (ItemIndexRef index, const IItemSelection& selection) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) override;
	void onSelectionChanged () override;

protected:
	SkinItemInspector& component;

	static MutableCString getAttributeName (ListViewItem* listItem)
	{
		return MutableCString (listItem->getTitle ());
	}

	static String getValueString (ListViewItem* listItem)
	{
		return listItem->getDetails ().getString (kValueID);
	}

	static SkinAttributeType getAttributeType (ListViewItem* listItem)
	{
		return listItem->getDetails ().getInt (kTypeID);
	}

	void addItem (StringRef name, StringRef value, bool readOnly, 
				  SkinAttributeType type = SkinAttributeTypes::kUnspecified)
	{
		ListViewItem* listItem = NEW ListViewItem (name);
		listItem->getDetails ().set (kValueID, value);
		listItem->getDetails ().set (kTypeID, type);
		listItem->setEnabled (!readOnly);
		items.add (listItem);
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

// XSTRINGS_OFF - hint for xstring tool to skip this section

BEGIN_XSTRINGS ("SkinEditing")
	XSTRING (PropertyName, "Name")
	XSTRING (PropertyValue, "Value")
	XSTRING (ItemName, "Item")
	XSTRING (SourceFile, "Source File")
	XSTRING (ImagePath, "Image Path")
	XSTRING (ClassName, "Class")
	XSTRING (ImageWidth, "Image Width")
	XSTRING (ImageHeight, "Image Height")
	XSTRING (CalculatedSize, "Calculated Size")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum SkinItemInspectorTags
	{
		kValueEdit = 100,
		kAddAttribute,
		kRemoveAttribute
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (SkinItemInspector)
	DEFINE_COMMAND ("Edit", "Add Attribute", SkinItemInspector::onAddAttribute)
	DEFINE_COMMAND ("Edit", "Remove", SkinItemInspector::onEditRemove)
END_COMMANDS (SkinItemInspector)

//************************************************************************************************
// SkinItemInspector
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinItemInspector, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinItemInspector::SkinItemInspector (StringRef name)
: Component (name),
  listModel (nullptr),
  skinItem (nullptr)
{
	listModel = NEW SkinPropertyListModel (*this);
	addObject ("listModel", listModel);

	paramList.addCommand ("Edit", "Add Attribute", "addAttribute", Tag::kAddAttribute)->enable (false);
	paramList.addCommand ("Edit", "Remove", "removeAttribute", Tag::kRemoveAttribute)->enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinItemInspector::~SkinItemInspector ()
{
	paramList.removeAll (); // avoid checking command state
	setSkinItem (nullptr);
	safe_release (listModel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinItemInspector::setSkinItem (SkinEditItem* _skinItem)
{
	share_and_observe (this, skinItem, _skinItem);
	listModel->rebuild ();
	paramList.checkCommandStates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkinItemInspector::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kPropertyChanged && skinItem && skinItem == subject)
	{
		// TODO: update affected property only...
		listModel->rebuild ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinItemInspector::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kValueEdit)
	{
		SkinValueChange valueChange;
		valueChange.name = param->getName ();
		ASSERT (!valueChange.name.isEmpty ())
		valueChange.value = param->getValue (); // preserve data type of parameter
		valueChange.value.share ();

		auto skinData = skinItem ? skinItem->getSkinData () : nullptr;
		auto editSupport = skinData ? skinData->getEditSupport () : nullptr;
		if(editSupport)
		{
			// special handling for size attribute
			bool handled = false;
			if(SkinViewItem* viewItem = ccl_cast<SkinViewItem> (skinItem))
			{
				Rect newSize;
				if(editSupport->detectSizeChange (newSize, &viewItem->getViewElement (), valueChange))
				{
					handled = true;
					SkinEditFunctions (*skinData).resizeView (viewItem, newSize);
				}
			}

			if(handled == false)
				SkinEditFunctions (*skinData).modifyAttribute (skinItem, valueChange);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinItemInspector::onListSelectionChanged ()
{
	paramList.checkCommandStates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_COMMANDS (SkinItemInspector, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinItemInspector::onAddAttribute (CmdArgs args)
{
	if(!skinItem)
		return false;

	if(!args.checkOnly ())
	{
		String nameString;
		if(DialogBox ()->askForString (nameString, XSTR_REF (PropertyName).getKey (), 
									  ActionDescriptions::AddAttribute (), "SkinAttributeAddDialog"))
		{
			nameString.trimWhitespace ();
			MutableCString attributeName (nameString);
			SkinValueChange valueChange (attributeName);
			bool exists = skinItem->hasAttribute (attributeName);
			if(exists == false)
			{
				auto skinData = skinItem->getSkinData ();
				ASSERT (skinData)

				// try to handle known numeric attribute types
				SkinAttributeType type = SkinAttributeTypes::kUnspecified;
				if(auto editSupport = skinData->getEditSupport ())
					type = editSupport->getAttributeType (&skinItem->getElement (), attributeName);
				switch(type)
				{
				case SkinAttributeTypes::kInteger :
				case SkinAttributeTypes::kBool :
					valueChange.value = 0;
					break;
				case SkinAttributeTypes::kFloat :
					valueChange.value = 0.f;
					break;
				default :
					valueChange.value = String::kEmpty;
					break;
				}

				SkinEditFunctions (*skinData).modifyAttribute (skinItem, valueChange);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinItemInspector::onEditRemove (CmdArgs args)
{
	if(!listModel->canRemoveSelectedItems ())
		return false;

	if(!args.checkOnly ())
		listModel->removeSelectedItems ();
	return true;
}

//************************************************************************************************
// SkinPropertyListModel
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkinPropertyListModel, ListViewModel)
DEFINE_STRINGID_MEMBER_ (SkinPropertyListModel, kValueID, "value")
DEFINE_STRINGID_MEMBER_ (SkinPropertyListModel, kTypeID, "type")

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinPropertyListModel::SkinPropertyListModel (SkinItemInspector& component)
: component (component)
{
	auto& columns = getColumns ();
	columns.addColumn (140, XSTR (PropertyName), kTitleID, 100, IColumnHeaderList::kSizable);
	columns.addColumn (140, XSTR (PropertyValue), kValueID, 100, IColumnHeaderList::kSizable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinPropertyListModel::rebuild ()
{
	auto skinItem = component.getSkinItem ();
	items.removeAll ();
	if(skinItem)
	{
		String sourceFile = skinItem->getSourceFile ();
		String itemName (skinItem->getElement ().getName ());

		String className;
		if(auto elementClass = skinItem->getElement ().getElementClass ())
			className = String (elementClass->getClassName ());

		addItem (XSTR (SourceFile), sourceFile, true);
		addItem (XSTR (ItemName), itemName, true);
		addItem (XSTR (ClassName), className, true);

		if(auto viewItem = ccl_cast<SkinViewItem> (skinItem))
		{			
			RectRef r = viewItem->getSize ();
			String sizeString;
			sizeString << r.left << ", " << r.top << " (" << r.getWidth () << " x " << r.getHeight () << ")";
			addItem (XSTR (CalculatedSize), sizeString, true);
		}
		else if(auto imageItem = ccl_cast<SkinImageItem> (skinItem))
		{
			addItem (XSTR (ImagePath), imageItem->getImageElement ().getImagePath (), true);
			if(auto image = imageItem->getImageElement ().getImage ())
			{
				addItem (XSTR (ImageWidth), String () << image->getWidth (), true);
				addItem (XSTR (ImageHeight), String () << image->getHeight (), true);
			}
		}

		auto editSupport = skinItem->getSkinData () ? skinItem->getSkinData ()->getEditSupport () : nullptr;

		Attributes propertyData;
		skinItem->getElement ().getAttributes (propertyData);
		for(int i = 0; i < propertyData.countAttributes (); i++)
		{
			MutableCString name;
			propertyData.getAttributeName (name, i);
			Variant value;
			propertyData.getAttributeValue (value, i);

			SkinAttributeType type = SkinAttributeTypes::kUnspecified;
			if(editSupport)
				type = editSupport->getAttributeType (&skinItem->getElement (), name);

			#if 1 // preserve numeric attribute type
			if(type == SkinAttributeTypes::kUnspecified)
			{
				if(value.isInt ())
					type = SkinAttributeTypes::kInteger;
				else if(value.isFloat ())
					type = SkinAttributeTypes::kFloat;
			}
			#endif

			// TODO: edit objects (inplace styles) in modal inspector instance...
			addItem (String (name), VariantString (value), value.isObject (), type);
		}
	}
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinPropertyListModel::canRemoveItem (ItemIndexRef index)
{
	ListViewItem* listItem = resolve (index);
	return listItem && listItem->isEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinPropertyListModel::removeItems (ItemIndexRef index, const IItemSelection& selection)
{
	if(auto skinItem = component.getSkinItem ())
	{
		auto skinData = skinItem->getSkinData ();
		ASSERT (skinData)
		SkinEditFunctions functions (*skinData);
		functions.beginMultiple (ActionDescriptions::RemoveAttribute (!selection.isMultiple ()));

		ForEachItem (selection, selectedIndex)
			ListViewItem* listItem = resolve (selectedIndex);
			if(listItem && listItem->isEnabled ())
			{
				MutableCString attributeName = getAttributeName (listItem);
				functions.removeAttribute (skinItem, attributeName);
			}
		EndFor

		functions.endMultiple ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinPropertyListModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	auto skinItem = component.getSkinItem ();
	ListViewItem* listItem = resolve (index);
	if(listItem && getColumnID (column) == kValueID)
	{
		SkinAttributeType type = getAttributeType (listItem);
		if(type == SkinAttributeTypes::kColor)
		{
			String valueString (getValueString (listItem));
			Color color;
			Colors::fromString (color, valueString);
			
			Rect rect (info.rect);
			rect.contract (1);
			info.graphics.fillRect (rect, SolidBrush (color));
			
			Color inverseColor;
			inverseColor.red = ~color.red;
			inverseColor.green = ~color.green;
			inverseColor.blue = ~color.blue;
			info.graphics.drawString (rect, valueString, info.style.font, SolidBrush (inverseColor), Alignment::kCenter);			
			
			info.graphics.drawRect (rect, info.style.adaptiveColor);
			return true;
		}
		else if(type == SkinAttributeTypes::kBool)
		{
			bool checked = Variant (getValueString (listItem)).parseBool ();
			drawCheckBox (info, checked, true, Alignment::kLeftCenter);
			return true;
		}
		else if(type == SkinAttributeTypes::kInteger || type == SkinAttributeTypes::kFloat)
		{
			String valueString (getValueString (listItem));
			Color textColor (info.style.adaptiveColor);
			Font font (info.style.font);
			font.isBold (true);
			info.graphics.drawString (info.rect, valueString, font, SolidBrush (textColor));
			return true;
		}
		else
		{
			IImage* attrIcon = nullptr;
			switch(type)
			{
			case SkinAttributeTypes::kStyle : attrIcon = SkinEditIcons::getItemTypeIcon (SkinEditData::kStyles); break;
			case SkinAttributeTypes::kImage : attrIcon = SkinEditIcons::getItemTypeIcon (SkinEditData::kImages); break;
			case SkinAttributeTypes::kForm : attrIcon = SkinEditIcons::getItemTypeIcon (SkinEditData::kForms); break;
			case SkinAttributeTypes::kFont : attrIcon = SkinEditIcons::getItemTypeIcon (SkinEditData::kFonts); break;
			case SkinAttributeTypes::kEnum : attrIcon = SkinEditIcons::getKnownIcon (SkinEditIcons::kEnumeration); break;
			}

			if(attrIcon != nullptr)
			{
				const int kIconWidth = 16;

				Rect modifiedRect (info.rect);
				DrawInfo info2 = {info.view, info.graphics, modifiedRect, info.style, info.state};
				modifiedRect.setWidth (kIconWidth);
				drawIcon (info2, attrIcon);

				modifiedRect = info.rect;
				modifiedRect.left += kIconWidth + 2;
				SuperClass::drawCell (index, column, info2);
				return true;
			}			
		}
	}
	return SuperClass::drawCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinPropertyListModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	auto skinItem = component.getSkinItem ();
	ListViewItem* listItem = resolve (index);
	if(skinItem && listItem && listItem->isEnabled () && getColumnID (column) == kValueID)
	{
		SkinAttributeType type = getAttributeType (listItem);
		MutableCString attributeName = getAttributeName (listItem);
		String valueString = getValueString (listItem);
		auto skinData = skinItem->getSkinData ();
		ASSERT (skinData)

		if(type == SkinAttributeTypes::kColor)
		{
			Color color;
			Colors::fromString (color, valueString);
			if(ColorPickerDialog ().run (color))
			{
				Colors::toString (color, valueString, false);				
				SkinEditFunctions (*skinData).modifyAttribute (skinItem, SkinValueChange (attributeName, valueString));
			}
		}
		else if(type == SkinAttributeTypes::kBool)
		{
			bool checked = Variant (valueString).parseBool ();
			checked = !checked;
			SkinEditFunctions (*skinData).modifyAttribute (skinItem, SkinValueChange (attributeName, checked));
		}
		else
		{
			AutoPtr<IParameter> param;
			switch(type)
			{
			case SkinAttributeTypes::kInteger : param = NEW IntParam (NumericLimits::kMinInt, NumericLimits::kMaxInt); break;
			case SkinAttributeTypes::kFloat : param = NEW FloatParam (NumericLimits::kMinimumFloat, NumericLimits::kMaximumFloat); break;
			case SkinAttributeTypes::kBool : param = NEW Parameter; break;
			default : 
				param = NEW StringParam;
			}
			param->setName (attributeName);
			param->fromString (valueString);
			param->connect (&component, Tag::kValueEdit);

			UIDRef controlClass = param->getType () == IParameter::kString ? CCL::ClassID::EditBox : CCL::ClassID::ValueBox;
			ControlBox control (controlClass, param, info.rect, Styles::kBorder);
			control.setVisualStyle (ViewBox (getItemView ()).getVisualStyle ());
			getItemView ()->setEditControl (control);
		}
		return true;
	}

	return SuperClass::editCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinPropertyListModel::appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection)
{
	menu.addCommandItem (String () << ActionDescriptions::AddAttribute () << IMenu::strFollowIndicator, "Edit", "Add Attribute", &component);
	menu.addCommandItem (String () << ActionDescriptions::RemoveAttribute (), "Edit", "Remove", &component);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinPropertyListModel::onSelectionChanged ()
{
	component.onListSelectionChanged ();
}
