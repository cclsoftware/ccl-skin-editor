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
// Filename    : skinediticons.cpp
// Description : Skin Editing Icons
//
//************************************************************************************************

#include "skinediticons.h"

#include "ccl/app/component.h" // for ITheme
#include "ccl/public/gui/framework/itheme.h"

using namespace CCL;

//************************************************************************************************
// SkinEditIcons
//************************************************************************************************

IImage* SkinEditIcons::getViewItemIcon (const SkinViewItem& item)
{
	MutableCString iconName (item.myClass ().getPersistentName ());
	if(ccl_cast<SkinFormItem> (&item) == nullptr)
	{
		MutableCString className = item.getClassName ();
		if(!className.isEmpty ())
		{
			iconName += ":";
			iconName += className;
		}
	}
	return RootComponent::instance ().getTheme ()->getImage (iconName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* SkinEditIcons::getViewClassIcon (const SkinViewClass& viewClass)
{
	MutableCString iconName (ccl_typeid<SkinViewItem> ().getPersistentName ());
	iconName += ":";
	iconName += viewClass.getClassName ();
	return RootComponent::instance ().getTheme ()->getImage (iconName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* SkinEditIcons::getItemTypeIcon (SkinEditData::ContainerType type)
{
	MutableCString iconName;
	switch(type)
	{
	case SkinEditData::kFonts : iconName = "SkinFontItem"; break;
	case SkinEditData::kStyles : iconName = "SkinStyleItem"; break;
	case SkinEditData::kImages : iconName = "SkinImageItem"; break;
	case SkinEditData::kForms : iconName = "SkinFormItem"; break;
	}
	return RootComponent::instance ().getTheme ()->getImage (iconName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* SkinEditIcons::getKnownIcon (KnownIcon which)
{
	static const CStringPtr iconNames[kNumKnownIcons] =
	{
		"SkinViewItemFlags.Visibility",		// kVisibility
		"SkinViewItemFlags.Enablement",		// kEnablement
		"SkinViewItemFlags.ChildSelect",	// kChildSelect
		"SkinEnumeration",					// kEnumeration
	};

	return RootComponent::instance ().getTheme ()->getImage (iconNames[which]);
}

