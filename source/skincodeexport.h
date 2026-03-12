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
// Filename    : skincodeexport.h
// Description : Skin Code Export
//
//************************************************************************************************

#ifndef _skincodeexport_h
#define _skincodeexport_h

#include "ccl/base/storage/url.h"

namespace CCL {

class SkinEditData;

//************************************************************************************************
// SkinCodeExporter
//************************************************************************************************

class SkinCodeExporter
{
public:
	SkinCodeExporter (SkinEditData& skinData, StringRef documentName, UrlRef targetFolder);

	bool exportAll ();

protected:
	static CStringPtr kFilenameID;
	static CStringPtr kNamespaceID;

	SkinEditData& skinData;
	String documentName;
	Url targetFolder;
};

} // namespace CCL

#endif // _skincodeexport_h
