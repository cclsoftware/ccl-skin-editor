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
// Filename    : skindocument.cpp
// Description : Skin Document
//
//************************************************************************************************

#include "skindocument.h"
#include "skindoccontroller.h"
#include "editing/skineditcomponent.h"

#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/jsonarchive.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/cclversion.h"

#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/gui/framework/ifileselector.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (NewSkin, "New Skin")
	XSTRING (SkinEditFile, "Skin Editor File")
END_XSTRINGS

//************************************************************************************************
// SkinDocumentData
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinDocumentData, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinDocumentData::getEditSettings (SkinEditSettings& editSettings) const
{
	editSettings.load (Storage (const_cast<Attributes&> (attributes)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinDocumentData::setEditSettings (const SkinEditSettings& editSettings)
{
	editSettings.save (Storage (attributes));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinDocumentData::getFormat (FileType& format) const
{
	format = FileTypes::Json ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinDocumentData::save (IStream& stream) const
{
	return JsonArchive (stream).saveAttributes (nullptr, attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinDocumentData::load (IStream& stream)
{
	return JsonArchive (stream).loadAttributes (nullptr, attributes);
}

//************************************************************************************************
// SkinDocumentClass
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinDocumentClass, DocumentClass)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinDocumentClass::SkinDocumentClass ()
{
	setFileType (SkinDocument::getFileType ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Document* SkinDocumentClass::createDocument ()
{
	SkinDocument* document = NEW SkinDocument;
	document->setDocumentClass (this);
	return document;
}

//************************************************************************************************
// SkinDocument
//************************************************************************************************

const FileType& SkinDocument::getFileType ()
{
	static FileType fileType (nullptr, "skinedit", CCL_MIME_TYPE "-skinedit");
	return FileTypes::init (fileType, XSTR (SkinEditFile));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (SkinDocument, Document)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinDocument::SkinDocument ()
: controller (nullptr)
{
	skinData = NEW SkinEditData (getActionJournal ());
	controller = NEW SkinDocumentController (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinDocument::~SkinDocument ()
{
	controller->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API SkinDocument::getController () const
{
	return controller->asUnknown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinDocument::prepare (const Attributes* args)
{
	// user needs to select where to save the .skinedit file first
	AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
	fs->addFilter (getFileType ());
	fs->setFileName (LegalFileName (XSTR (NewSkin)));
	if(!fs->run (IFileSelector::kSaveFile, XSTR (NewSkin)))
		return false;

	setPath (*fs->getPath (0));
	Url documentFolder (getPath ());
	documentFolder.ascend ();

	// skin data is saved in a unique subfolder
	Url skinFolder (documentFolder);
	skinFolder.descend (CCLSTR ("skin"), Url::kFolder);
	skinFolder.makeUnique ();
	Url relativeSkinFolder (skinFolder);
	relativeSkinFolder.makeRelative (documentFolder);
	documentData.setRelativeSkinPath (relativeSkinFolder.getPath ());

	// init with default settings
	SkinEditSettings settings;
	settings.setDefaultFormSize (Point (1024, 768));
	documentData.setEditSettings (settings);
	
	// create .skinedit file and skin skeleton
	if(!documentData.saveToFile (getPath ()))
		return false;

	AutoPtr<ISkinLoader> loader = ccl_new<ISkinLoader> (ClassID::CoreSkinLoader);
	ASSERT (loader != nullptr)
	if(!loader->createSkin (skinFolder))
		return false;

	skinData->construct (*loader);	
	documentData.getEditSettings (skinData->getEditSettings ()); // copy edit settings
	controller->getSkinEditor ()->setSkinData (skinData);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinDocument::load ()
{
	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	ProgressNotifyScope progressScope (progress);

	if(!documentData.loadFromFile (getPath ()))
		return false;
	
	Url documentFolder (getPath ());
	documentFolder.ascend ();
	Url skinFolder (documentData.getRelativeSkinPath (), documentFolder, Url::kFolder);

	AutoPtr<ISkinLoader> loader = ccl_new<ISkinLoader> (ClassID::CoreSkinLoader);
	ASSERT (loader != nullptr)
	if(!loader->loadSkin (skinFolder, progress))
		return false;

	skinData->construct (*loader);	
	documentData.getEditSettings (skinData->getEditSettings ()); // copy edit settings
	controller->getSkinEditor ()->setSkinData (skinData);

	return SuperClass::load ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinDocument::save ()
{
	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	ProgressNotifyScope progressScope (progress);
	
	documentData.setEditSettings (skinData->getEditSettings ()); // copy edit settings
	if(!documentData.saveToFile (getPath ()))
		return false;

	if(auto editSupport = skinData->getEditSupport ())
	{
		if(!editSupport->saveModelChanges (progress))
			return false;
	}

	return SuperClass::save ();
}
