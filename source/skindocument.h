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
// Filename    : skindocument.h
// Description : Skin Document
//
//************************************************************************************************

#ifndef _skindocument_h
#define _skindocument_h

#include "ccl/app/documents/document.h"

#include "ccl/base/storage/storableobject.h"

#include "editing/skineditdata.h"

namespace CCL {

class SkinDocumentController;

//************************************************************************************************
// SkinDocumentData
/** Data saved in a .skinedit JSON file. */
//************************************************************************************************

class SkinDocumentData: public StorableObject
{
public:
	DECLARE_CLASS (SkinDocumentData, StorableObject)

	void setRelativeSkinPath (StringRef path) { attributes.set ("skin", path); }
	String getRelativeSkinPath () const { return attributes.getString ("skin"); }

	void getEditSettings (SkinEditSettings& editSettings) const;
	void setEditSettings (const SkinEditSettings& editSettings);

	// StorableObject
	tbool CCL_API getFormat (FileType& format) const override;
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;

protected:
	Attributes attributes;
};

//************************************************************************************************
// SkinDocumentClass
//************************************************************************************************

class SkinDocumentClass: public DocumentClass
{
public:
	DECLARE_CLASS (SkinDocumentClass, DocumentClass)

	SkinDocumentClass ();

	// DocumentClass
	Document* createDocument () override;
};

//************************************************************************************************
// SkinDocument
//************************************************************************************************

class SkinDocument: public Document
{
public:
	DECLARE_CLASS (SkinDocument, Document)

	SkinDocument ();
	~SkinDocument ();

	static const FileType& getFileType ();

	SkinEditData& getSkinData () { return *skinData; }

	// Document
	IUnknown* CCL_API getController () const override;
	bool prepare (const Attributes* args = 0) override;
	bool load () override;
	bool save () override;

protected:
	SkinDocumentData documentData;
	AutoPtr<SkinEditData> skinData;
	SkinDocumentController* controller;
};

} // namespace CCL

#endif // _skindocument_h
