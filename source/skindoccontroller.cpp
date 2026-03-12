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
// Filename    : skindoccontroller.cpp
// Description : Skin Document Controller
//
//************************************************************************************************

#include "skindoccontroller.h"
#include "skindocument.h"
#include "skincodeexport.h"
#include "editing/skineditcomponent.h"

#include "ccl/app/controls/listviewmodel.h"
#include "ccl/app/components/colorpicker.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/ifileselector.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum SkinDocumentSetupComponentTags
	{
		kBackColor = 100,
		kFormWidth,
		kFormHeight,
		kSelectBackColor,
		kAddDefault,
		kRemoveDefault,
		kEditDefault
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("SkinSetup")
	XSTRING (AddDefault, "Add Default")
	XSTRING (EditDefault, "Edit Default")
	XSTRING (DefaultName, "Default Name")
	XSTRING (DefaultValue, "Default Value")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (SkinDocumentController)
	DEFINE_COMMAND ("File", "Skin Settings", SkinDocumentController::onSkinSettings)
	DEFINE_COMMAND ("File", "Export Code", SkinDocumentController::onExportCode)
END_COMMANDS (SkinDocumentController)

//************************************************************************************************
// SkinDocumentController
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkinDocumentController, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinDocumentController::SkinDocumentController (SkinDocument& document)
: document (document),
  skinEditor (nullptr)
{
	addComponent (skinEditor = NEW SkinEditComponent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API SkinDocumentController::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "Document")
	{
		IView* view = getTheme ()->createView ("SkinDocumentView", this->asUnknown ());
		if(view && !bounds.isEmpty ())
			ViewBox (view).setSize (bounds);
		return view;
	}
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_COMMANDS (SkinDocumentController, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinDocumentController::onSkinSettings (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		SkinDocumentSettingsComponent (document).runDialog ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinDocumentController::onExportCode (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		AutoPtr<IFolderSelector> fs = ccl_new<IFolderSelector> (ClassID::FolderSelector);
		if(fs->run ())
		{
			String documentName;
			document.getPath ().getName (documentName);
			UrlRef targetFolder = fs->getPath ();
			SkinCodeExporter (document.getSkinData (), documentName, targetFolder).exportAll ();
		}
	}
	return true;
}

//************************************************************************************************
// SkinDocumentSettingsComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkinDocumentSettingsComponent, Component)
DEFINE_STRINGID_MEMBER_ (SkinDocumentSettingsComponent, kValueID, "value")

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinDocumentSettingsComponent::SkinDocumentSettingsComponent (SkinDocument& document)
: document (document),
  defaultsList (nullptr),
  backColorParam (nullptr)
{
	const auto& editSettings = document.getSkinData ().getEditSettings ();

	backColorParam = UnknownPtr<IColorParam> (paramList.addColor ("backColor", Tag::kBackColor));
	backColorParam->setColor (editSettings.getFormEditBackColor ());
	paramList.addParam ("selectBackColor", Tag::kSelectBackColor);

	paramList.addInteger (0, kMaxCoord, "formWidth", Tag::kFormWidth)->setValue (editSettings.getDefaultFormSize ().x);
	paramList.addInteger (0, kMaxCoord, "formHeight", Tag::kFormHeight)->setValue (editSettings.getDefaultFormSize ().y);

	defaultsList = NEW ListViewModel;
	defaultsList->getColumns ().addColumn (200, 0, ListViewModel::kTitleID);
	defaultsList->getColumns ().addColumn (200, 0, kValueID);
	defaultsList->setItemRemovalEnabled (true);
	signalSlots.advise (defaultsList, IItemView::kSelectionChanged, this, &SkinDocumentSettingsComponent::onDefaultsListSelectionChanged);
	signalSlots.advise (defaultsList, ListViewModel::kItemOpened, this, &SkinDocumentSettingsComponent::onDefaultsListOpenItem);
	addObject ("defaultsList", defaultsList);

	ForEachAttribute (editSettings.getDefaults (), name, value)
		ListViewItem* item = NEW ListViewItem;
		item->setTitle (String (name));
		item->getDetails ().set (kValueID, VariantString (value));
		defaultsList->addItem (item);
	EndFor

	paramList.addParam ("addDefault", Tag::kAddDefault);
	paramList.addParam ("removeDefault", Tag::kRemoveDefault)->enable (false);
	paramList.addParam ("editDefault", Tag::kEditDefault)->enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinDocumentSettingsComponent::~SkinDocumentSettingsComponent ()
{
	signalSlots.unadvise (defaultsList);
	defaultsList->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinDocumentSettingsComponent::onDefaultsListSelectionChanged (MessageRef msg)
{
	paramList.byTag (Tag::kRemoveDefault)->enable (defaultsList->canRemoveSelectedItems ());
	paramList.byTag (Tag::kEditDefault)->enable (defaultsList->getFirstSelectedItem () != nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinDocumentSettingsComponent::onDefaultsListOpenItem (MessageRef msg)
{
	if(ListViewItem* item = unknown_cast<ListViewItem> (msg[0].asUnknown ()))
		if(editDefaultsItem (*item))
			defaultsList->invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinDocumentSettingsComponent::editDefaultsItem (ListViewItem& item, StringRef _title)
{
	ParamContainer params;
	IParameter* nameParam = params.addString (XSTR_REF (DefaultName).getKey ());
	IParameter* valueParam = params.addString (XSTR_REF (DefaultValue).getKey ());
	nameParam->fromString (item.getTitle ());
	valueParam->fromString (item.getDetails ().getString (kValueID));

	String title (_title);
	if(title.isEmpty ())
		title = XSTR (EditDefault);

	if(DialogBox ()->runWithParameters ("SkinDefaultsDialog", params, title) == DialogResult::kOkay)
	{
		String newName, newValue;
		nameParam->toString (newName);
		valueParam->toString (newValue);
		newName.trimWhitespace ();
		newValue.trimWhitespace ();
		if(!newName.isEmpty ())
		{
			item.setTitle (newName);
			item.getDetails ().set (kValueID, newValue);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinDocumentSettingsComponent::runDialog ()
{
	IView* view = getTheme ()->createView ("SkinDocumentSettings", this->asUnknown ());
	ASSERT (view != nullptr)
	if(view == nullptr)
		return;

	if(DialogBox ()->runDialog (view) == DialogResult::kOkay)
	{
		auto& editSettings = document.getSkinData ().getEditSettings ();

		Color backColor;
		backColorParam->getColor (backColor);
		editSettings.setFormEditBackColor (backColor);

		Attributes defaults;
		ForEach (*defaultsList, ListViewItem, item)
			MutableCString name (item->getTitle ());
			String value = item->getDetails ().getString (kValueID);
			if(!defaults.contains (name)) // use first occurence
				defaults.set (name, value);
		EndFor
		editSettings.setDefaults (defaults);

		Point formSize (paramList.byTag (Tag::kFormWidth)->getValue ().asInt (), 
						paramList.byTag (Tag::kFormHeight)->getValue ().asInt ());
		editSettings.setDefaultFormSize (formSize);

		document.getSkinData ().editSettingsChanged ();
		document.setDirty ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinDocumentSettingsComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kSelectBackColor :
		{
			Color backColor;
			backColorParam->getColor (backColor);
			if(ColorPickerDialog ().run (backColor))
				backColorParam->setColor (backColor, true);
		}
		break;

	case Tag::kAddDefault :
		{
			AutoPtr<ListViewItem> newItem = NEW ListViewItem;
			if(editDefaultsItem (*newItem, XSTR (AddDefault)))
			{
				defaultsList->addItem (newItem.detach ());
				defaultsList->signal (Message (kChanged));
			}
		}
		break;

	case Tag::kRemoveDefault :
		defaultsList->removeSelectedItems ();
		break;

	case Tag::kEditDefault :
		if(auto item = defaultsList->getFirstSelectedItem ())
			if(editDefaultsItem (*item))
				defaultsList->invalidate ();
		break;
	}
	return true;
}
