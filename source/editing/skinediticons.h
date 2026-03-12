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
// Filename    : skinediticons.h
// Description : Skin Editing Icons
//
//************************************************************************************************

#ifndef _ccl_skinediticons_h
#define _ccl_skinediticons_h

#include "skineditdata.h"

namespace CCL {

//************************************************************************************************
// SkinEditIcons
//************************************************************************************************

namespace SkinEditIcons
{
	IImage* getViewItemIcon (const SkinViewItem& item);
	IImage* getViewClassIcon (const SkinViewClass& viewClass);
	IImage* getItemTypeIcon (SkinEditData::ContainerType type);

	enum KnownIcon
	{
		kVisibility,
		kEnablement,
		kChildSelect,
		kEnumeration,
		kNumKnownIcons 
	};

	IImage* getKnownIcon (KnownIcon which);
}

} // namespace CCL

#endif // _ccl_skinediticons_h
