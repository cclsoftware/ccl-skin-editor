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
// Filename    : skineditcomponent.cpp
// Description : Skin Editing Component
//
//************************************************************************************************

#include "skineditcomponent.h"
#include "skinformeditor.h"
#include "skiniteminspector.h"
#include "skinediticons.h"
#include "skineditfunctions.h"

#include "ccl/app/controls/treeviewmodel.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/ifileselector.h"

namespace CCL {

//************************************************************************************************
// SkinDataTreeModel
//************************************************************************************************

class SkinDataTreeModel: public TreeViewModel
{
public:
	enum Options
	{
		kKeepNodesTopLevel = 1<<0,
		kCommentColumn = 1<<1
	};

	SkinDataTreeModel (SkinEditComponent& component, int options = 0);

	DECLARE_STRINGID_MEMBER (kCommentID)

	void removeAll ();
	void addModelItems (SkinEditData::ContainerType type);
	void addModelItem (SkinEditData::ContainerType type, SkinEditItem* skinItem, int index);
	void removeModelItem (SkinEditData::ContainerType type, SkinEditItem* skinItem);
	
	void focusModelItem (SkinEditData::ContainerType type, SkinEditItem* skinItem);

	// TreeViewModel
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection) override;

protected:
	SkinEditComponent& component;
	AutoPtr<TreeViewFolderNode> rootFolder;
	bool keepNodesTopLevel;

	TreeViewFolderNode* findFolderNode (SkinEditData::ContainerType type) const;

	static String getContainerTitle (SkinEditData::ContainerType type);
	static TreeViewNode* createNodeForItem (SkinEditItem* skinItem, IImage* icon);
};

//************************************************************************************************
// SkinClassListModel
//************************************************************************************************

class SkinClassListModel: public ListViewModel
{};

//************************************************************************************************
// SkinClassListItem
//************************************************************************************************

class SkinClassListItem: public ListViewItem
{
public:
	SkinClassListItem (SkinViewClass& _viewClass)
	: viewClass (&_viewClass)
	{
		setTitle (String (_viewClass.getClassName ()));
		setIcon (SkinEditIcons::getViewClassIcon (_viewClass));
	}

	// ListViewItem
	IUnknown* createDragObject () override
	{
		return return_shared<IObject> (viewClass);
	}

protected:
	SharedPtr<SkinViewClass> viewClass;
};

//************************************************************************************************
// SkinDataTreeNode
//************************************************************************************************

namespace SkinDataTreeNode
{
	SkinEditItem* getSkinItem (TreeViewNode* treeNode);
	void setSkinItem (TreeViewNode* treeNode, SkinEditItem* skinItem);
}

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum SkinEditComponentTags
	{
		kImageProvider = 100,
		kSourceCode
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

// XSTRINGS_OFF - hint for xstring tool to skip this section

BEGIN_XSTRINGS ("SkinEditing")
	XSTRING (Forms, "Forms")
	XSTRING (Images, "Images")
	XSTRING (Styles, "Styles")
	XSTRING (Fonts, "Fonts")
	XSTRING (Comment, "Comment")
	XSTRING (FormName, "Form Name")
	XSTRING (SourceFile, "Source File")
	XSTRING (FontName, "Font Name")
	XSTRING (StyleName, "Style Name")
	XSTRING (FormNameAlreadyExists, "A form with this name already exists.")
	XSTRING (ImageNameAlreadyExists, "An image with this name already exists.")
	XSTRING (FontNameAlreadyExists, "A font with this name already exists.")
	XSTRING (StyleNameAlreadyExists, "A style with this name already exists.")
	XSTRING (SourceFileAlreadyExists, "A source file with this name already exists.")
	XSTRING (ImageImportFailed, "Some images failed to import.")
	XSTRING (FileNotInsidePackage, "The file is not inside the skin package.")
	XSTRING (UnsupportedImageFormat, "Unsupported image format.")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (SkinEditComponent)
	DEFINE_COMMAND ("Skin", "Add Form", SkinEditComponent::onAddForm)
	DEFINE_COMMAND ("Skin", "Add Image", SkinEditComponent::onAddImage)
	DEFINE_COMMAND ("Skin", "Add Font", SkinEditComponent::onAddFont)
	DEFINE_COMMAND ("Skin", "Add Style", SkinEditComponent::onAddStyle)
END_COMMANDS (SkinEditComponent)

//************************************************************************************************
// SkinEditComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinEditComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditComponent::SkinEditComponent ()
: Component ("SkinEditor"),
  skinData (nullptr),
  assetTreeModel (nullptr),
  assetInspector (nullptr),
  formTreeModel (nullptr),
  formEditor (nullptr),
  formInspector (nullptr),
  classListModel (nullptr),
  imageProvider (nullptr),
  currentSourceItem (nullptr),
  sourceCodeParam (nullptr)
{
	assetTreeModel = NEW SkinDataTreeModel (*this);
	addObject ("assetTreeModel", assetTreeModel);
	addComponent (assetInspector = NEW SkinItemInspector ("AssetInspector"));	

	formTreeModel = NEW SkinDataTreeModel (*this, SkinDataTreeModel::kKeepNodesTopLevel|SkinDataTreeModel::kCommentColumn);
	addObject ("formTreeModel", formTreeModel);
	addComponent (formEditor = NEW SkinFormEditComponent);
	addComponent (formInspector = NEW SkinItemInspector ("FormInspector"));	
	formEditor->setItemInspector (formInspector);

	classListModel = NEW SkinClassListModel;
	addObject ("classListModel", classListModel);
	imageProvider = paramList.addImage ("imageProvider", Tag::kImageProvider);
	sourceCodeParam = paramList.addString ("sourceCode", Tag::kSourceCode);
	sourceCodeParam->setReadOnly (true); // editing not yet implemented
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditComponent::~SkinEditComponent ()
{
	assetInspector->setSkinItem (nullptr);
	formInspector->setSkinItem (nullptr);

	setSkinData (nullptr);

	safe_release (assetTreeModel);
	safe_release (formTreeModel);
	safe_release (classListModel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinEditComponent::setSkinData (SkinEditData* _skinData)
{
	share_and_observe (this, skinData, _skinData);
	
	assetTreeModel->removeAll ();
	formTreeModel->removeAll ();
	if(skinData != nullptr)
	{
		assetTreeModel->addModelItems (SkinEditData::kFonts);
		assetTreeModel->addModelItems (SkinEditData::kImages);
		assetTreeModel->addModelItems (SkinEditData::kStyles);
		
		formTreeModel->addModelItems (SkinEditData::kForms);
	}
	
	setCurrentSourceItem (nullptr);

	// view classes
	classListModel->removeAll ();
	if(skinData && !skinData->getClasses ().isEmpty ())
	{
		for(auto itemClass : iterate_as<SkinItemClass> (skinData->getClasses ()))
			if(auto viewClass = ccl_cast<SkinViewClass> (itemClass))
				classListModel->addItem (NEW SkinClassListItem (*viewClass));
		classListModel->signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinEditComponent::onItemFocused (SkinEditItem* focusItem)
{
	if(auto formItem = ccl_cast<SkinFormItem> (focusItem))
	{
		formEditor->setFormItem (formItem);
		formInspector->setSkinItem (formItem);

		// show form source code
		setCurrentSourceItem (formItem);
	}
	else
	{
		IImage* image = nullptr;
		if(auto imageItem = ccl_cast<SkinImageItem> (focusItem))
		{
			image = imageItem->getImageElement ().getImage ();
			if(image)
				image = image->getOriginal (); // resolve filmstrip to source image	
		}

		imageProvider->setImage (image);
		assetInspector->setSkinItem (focusItem);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinEditComponent::performRemoval (Container& items)
{
	ASSERT (skinData != nullptr)
	SkinEditFunctions functions (*skinData);
	bool first = true;
	for(auto item : iterate_as<SkinEditItem> (items))
	{
		if(first)
		{			
			String description;
			if(auto formItem = ccl_cast<SkinFormItem> (item))
				description = ActionDescriptions::RemoveForm (items.count () == 1);
			else if(auto imageItem = ccl_cast<SkinImageItem> (item))
				description = ActionDescriptions::RemoveImage (items.count () == 1);
			else
				description = ActionDescriptions::RemoveItem (items.count () == 1);

			functions.beginMultiple (description);
			first = false;
		}

		functions.removeItem (item);
	}

	if(!first)
		functions.endMultiple ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkinEditComponent::notify (ISubject* subject, MessageRef msg)
{
	if(msg == SkinFormItem::kViewItemChanged)
	{
		auto viewItem = unknown_cast<SkinViewItem> (msg[0].asUnknown ());
		if(currentSourceItem && viewItem && viewItem->getForm () == currentSourceItem)
			updateSourceCode ();
	}
	else if(msg == SkinEditData::kItemAdded || msg == SkinEditData::kItemRemoved)
	{
		auto type = static_cast<SkinEditData::ContainerType> (msg[0].asInt ());
		SkinEditItem* item = unknown_cast<SkinEditItem> (msg[1].asUnknown ());
		ASSERT (item != nullptr)

		if(msg == SkinEditData::kItemAdded)
		{
			int index = msg[2].asInt ();

			if(type == SkinEditData::kForms)
				formTreeModel->addModelItem (type, item, index);
			else
				assetTreeModel->addModelItem (type, item, index);
		}
		else // SkinEditData::kItemRemoved
		{
			if(type == SkinEditData::kForms)
			{
				formTreeModel->removeModelItem (type, item);

				if(formEditor->getFormItem () == item)
				{
					formEditor->setFormItem (nullptr);
					setCurrentSourceItem (nullptr);
				}
				if(formInspector->getSkinItem () == item)
					formInspector->setSkinItem (nullptr);
			}
			else
			{
				assetTreeModel->removeModelItem (type, item);

				if(assetInspector->getSkinItem () == item)
				{
					assetInspector->setSkinItem (nullptr);					
					if(auto imageItem = ccl_cast<SkinImageItem> (item))
						imageProvider->setImage (nullptr);
				}
			}
		}
	}
	else if(msg == SkinEditData::kEditSettingsChanged || msg == SkinEditData::kAssetChanged)
	{
		formEditor->updateEditViews ();

		#if 0 // TODO: do we need this???
		if(SkinEditData::kAssetChanged)
		{
			if(auto imageItem = unknown_cast<SkinImageItem> (msg[0].asUnknown ()))
				if(assetInspector->getSkinItem () == imageItem)
				{
					// image preview might have changed
					IImage* image = imageItem->getImageElement ().getImage ();
					image = image ? image->getOriginal () : 0;
					imageProvider->setImage (image);
				}
		}
		#endif
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinEditComponent::setCurrentSourceItem (SkinEditItem* item)
{
	share_and_observe (this, currentSourceItem, item);
	updateSourceCode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinEditComponent::updateSourceCode ()
{
	String sourceCode;
	if(skinData && skinData->getEditSupport () && currentSourceItem)
		skinData->getEditSupport ()->getSourceCodeForElement (sourceCode, &currentSourceItem->getElement ());

	sourceCodeParam->fromString (sourceCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_COMMANDS (SkinEditComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditComponent::onAddForm (CmdArgs args)
{
	if(!args.checkOnly () && skinData)
	{
		ParamContainer params;
		IParameter* formNameParam = params.addString (XSTR_REF (FormName).getKey ());
		IParameter* sourceFileParam = params.addString (XSTR_REF (SourceFile).getKey ());
		IParameter* commentParam = params.addString (XSTR_REF (Comment).getKey ());

		String formName ("NewForm");
		String sourceFile;
		if(auto editSupport = skinData->getEditSupport ())
			editSupport->suggestSourceFile (sourceFile, ClassID::FormElement, formName);

		formNameParam->fromString (formName);
		sourceFileParam->fromString (sourceFile);

		if(DialogBox ()->runWithParameters ("SkinAddFormDialog", params, ActionDescriptions::AddForm ()) == DialogResult::kOkay)
		{
			formNameParam->toString (formName);
			sourceFileParam->toString (sourceFile);
			formName.trimWhitespace ();
			formName.substitute ();
			sourceFile.trimWhitespace ();
			sourceFile.substitute ();
			if(!formName.isEmpty () && !sourceFile.isEmpty ())
			{
				MutableCString asciiFormName (formName);

				if(skinData->hasItemWithName (SkinEditData::kForms, asciiFormName))
					Alert::warn (String () << formName << "\n\n" << XSTR (FormNameAlreadyExists));
				else if(skinData->hasSourceFile (SkinEditData::kForms, sourceFile))
					Alert::warn (String () << sourceFile << "\n\n" << XSTR (SourceFileAlreadyExists));
				else
				{
					String comment;
					commentParam->toString (comment);

					// insert after currently focused form
					int insertIndex = -1;
					if(auto focusFormItem = formEditor->getFormItem ())
					{
						insertIndex = skinData->getContainerForType (SkinEditData::kForms).index (focusFormItem);
						insertIndex++;
					}

					SkinEditFunctions functions (*skinData);
					functions.beginMultiple (ActionDescriptions::AddForm ());
					SkinFormItem* formItem = functions.addForm (asciiFormName, sourceFile, comment, insertIndex);
					Point defaultSize = skinData->getEditSettings ().getDefaultFormSize ();
					if(formItem && defaultSize != Point ())
						functions.resizeView (formItem, Rect (defaultSize));
					functions.endMultiple ();

					// make visible in tree/editor
					if(formItem)
						formTreeModel->focusModelItem (SkinEditData::kForms, formItem);
				}
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditComponent::onAddImage (CmdArgs args)
{
	if(!args.checkOnly () && skinData)
	{
		auto editSupport = skinData->getEditSupport ();
		if(!editSupport)
			return false;

		Url topLevelFolder;
		editSupport->suggestAssetFolder (topLevelFolder);
		Url imageFolder;
		editSupport->suggestAssetFolder (imageFolder, ClassID::ImageElement);
		FileTypeFilter fileTypes;
		editSupport->getSupportedFileTypes (fileTypes, ClassID::ImageElement);

		AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
		fs->setFolder (imageFolder);
		for(auto ft : fileTypes.getContent ())
			fs->addFilter (ft);

		if(fs->run (IFileSelector::kOpenMultipleFiles, ActionDescriptions::AddImage ()))
		{
			SkinEditFunctions functions (*skinData);
			functions.beginMultiple (ActionDescriptions::AddImage (fs->countPaths () == 1));
		
			int insertIndex = -1;
			if(auto focusAssetItem = assetInspector->getSkinItem ())
				if(skinData->getTypeForItem (focusAssetItem) == SkinEditData::kImages)
				{
					insertIndex = skinData->getContainerForType (SkinEditData::kImages).index (focusAssetItem);
					insertIndex++;
				}

			SkinImageItem* lastImageItem = nullptr;

			ErrorContextGuard errorScope;
			for(int i = 0; i < fs->countPaths (); i++)
				if(const IUrl* path = fs->getPath (i))
				{
					String name;
					path->getName (name, false);
					MutableCString asciiImageName (name);

					Url relativePath (*path);
					relativePath.makeRelative (topLevelFolder);
					String fileName = relativePath.getPath ();
					if(fileName.startsWith (CCLSTR ("./")))
					   fileName.remove (0, 2);

					auto raiseError = [] (StringRef name, StringRef message)
					{
						ccl_raise (String () << "\"" << name << "\": " << message);
					};

					// TODO: check if file already used???
					if(!topLevelFolder.contains (*path))
						raiseError (name, XSTR (FileNotInsidePackage));
					else if(skinData->hasItemWithName (SkinEditData::kImages, asciiImageName))
						raiseError (name, XSTR (ImageNameAlreadyExists));
					else
					{
						AutoPtr<IImage> image = editSupport->loadImage (fileName);
						if(!image)
							raiseError (name, XSTR (UnsupportedImageFormat));
						else
							lastImageItem = functions.addImage (asciiImageName, fileName, image, insertIndex);
					}
				}

			functions.endMultiple ();

			if(errorScope.hasErrors ())
				Alert::errorWithContext (XSTR (ImageImportFailed), true);

			// make visible in tree/asset inspector
			if(lastImageItem)
				assetTreeModel->focusModelItem (SkinEditData::kImages, lastImageItem);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditComponent::onAddFont (CmdArgs args)
{
	if(!args.checkOnly () && skinData)
	{
		String nameString ("NewFont");
		if(DialogBox ()->askForString (nameString, XSTR_REF (FontName).getKey (), 
									  ActionDescriptions::AddFont (), "SkinAddFontDialog"))
		{
			nameString.trimWhitespace ();
			nameString.substitute ();
			if(!nameString.isEmpty ())
			{
				MutableCString asciiName (nameString);

				int insertIndex = -1;
				if(auto focusAssetItem = assetInspector->getSkinItem ())
					if(skinData->getTypeForItem (focusAssetItem) == SkinEditData::kFonts)
					{
						insertIndex = skinData->getContainerForType (SkinEditData::kFonts).index (focusAssetItem);
						insertIndex++;
					}

				SkinEditItem* newItem = nullptr;
				if(skinData->hasItemWithName (SkinEditData::kFonts, asciiName))
					Alert::warn (String () << nameString << "\n\n" << XSTR (FontNameAlreadyExists));
				else
					newItem = SkinEditFunctions (*skinData).addFont (asciiName, insertIndex);

				// make visible in tree/asset inspector
				if(newItem)
					assetTreeModel->focusModelItem (SkinEditData::kFonts, newItem);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditComponent::onAddStyle (CmdArgs args)
{
	if(!args.checkOnly () && skinData)
	{
		String nameString ("NewStyle");
		if(DialogBox ()->askForString (nameString, XSTR_REF (StyleName).getKey (), 
									  ActionDescriptions::AddStyle (), "SkinAddStyleDialog"))
		{
			nameString.trimWhitespace ();
			nameString.substitute ();
			if(!nameString.isEmpty ())
			{
				MutableCString asciiName (nameString);

				int insertIndex = -1;
				if(auto focusAssetItem = assetInspector->getSkinItem ())
					if(skinData->getTypeForItem (focusAssetItem) == SkinEditData::kStyles)
					{
						insertIndex = skinData->getContainerForType (SkinEditData::kStyles).index (focusAssetItem);
						insertIndex++;
					}

				SkinEditItem* newItem = nullptr;
				if(skinData->hasItemWithName (SkinEditData::kStyles, asciiName))
					Alert::warn (String () << nameString << "\n\n" << XSTR (StyleNameAlreadyExists));
				else
					newItem = SkinEditFunctions (*skinData).addStyle (asciiName, insertIndex);

				// make visible in tree/asset inspector
				if(newItem)
					assetTreeModel->focusModelItem (SkinEditData::kStyles, newItem);
			}
		}
	}
	return true;
}

//************************************************************************************************
// SkinDataTreeModel
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (SkinDataTreeModel, kCommentID, "value")

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinDataTreeModel::SkinDataTreeModel (SkinEditComponent& component, int options)
: component (component),
  rootFolder (NEW TreeViewFolderNode),
  keepNodesTopLevel ((options & kKeepNodesTopLevel) != 0)
{
	setRootNode (rootFolder);

	auto& columns = getColumns ();
	columns.addColumn (150, 0, kTitleID); // first column is managed by TreeView
	if(options & kCommentColumn)
		columns.addColumn (150, XSTR (Comment), kCommentID, 100, IColumnHeaderList::kSizable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinDataTreeModel::removeAll ()
{
	rootFolder->removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ String SkinDataTreeModel::getContainerTitle (SkinEditData::ContainerType type)
{
	switch(type)
	{
	case SkinEditData::kForms : return XSTR (Forms);
	case SkinEditData::kImages : return XSTR (Images);
	case SkinEditData::kStyles : return XSTR (Styles);
	case SkinEditData::kFonts : return XSTR (Fonts);
	}
	return String ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ TreeViewNode* SkinDataTreeModel::createNodeForItem (SkinEditItem* skinItem, IImage* icon)
{
	TreeViewNode* treeNode = NEW TreeViewNode (String (skinItem->getElement ().getName ()));
	treeNode->setIcon (icon);
	SkinDataTreeNode::setSkinItem (treeNode, skinItem);
	String comment;
	skinItem->getElement ().getComment (comment);
	if(!comment.isEmpty ())
		treeNode->getDetails ().set (kCommentID, comment);
	return treeNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinDataTreeModel::addModelItems (SkinEditData::ContainerType type)
{
	auto skinData = component.getSkinData ();
	ASSERT (skinData != nullptr)
	auto& items = skinData->getContainerForType (type);

	TreeViewFolderNode* folderNode = rootFolder;
	if(keepNodesTopLevel == false)
	{
		String title = getContainerTitle (type);
		folderNode = NEW TreeViewFolderNode (title);
		rootFolder->add (folderNode);
	}

	IImage* icon = SkinEditIcons::getItemTypeIcon (type);

	for(auto skinItem : iterate_as<SkinEditItem> (items))
	{
		auto treeNode = createNodeForItem (skinItem, icon);
		folderNode->add (treeNode);		
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeViewFolderNode* SkinDataTreeModel::findFolderNode (SkinEditData::ContainerType type) const
{
	TreeViewFolderNode* folderNode = rootFolder;
	if(keepNodesTopLevel == false)
	{
		String title = getContainerTitle (type);
		for(auto treeNode : iterate_as<TreeViewNode> (rootFolder->getContent ()))
			if(treeNode->getTitle () == title)
			{
				folderNode = ccl_cast<TreeViewFolderNode> (treeNode);
				break;
			}
	}
	return folderNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinDataTreeModel::addModelItem (SkinEditData::ContainerType type, SkinEditItem* skinItem, int index)
{
	auto folderNode = findFolderNode (type);
	ASSERT (folderNode != nullptr)
	if(folderNode == nullptr) // should not happen!
		return;

	IImage* icon = SkinEditIcons::getItemTypeIcon (type);
	auto treeNode = createNodeForItem (skinItem, icon);

	if(index != -1)
	{
		if(!folderNode->insertAt (index, treeNode))
			folderNode->add (treeNode);
	}
	else
		folderNode->add (treeNode);

	refreshNode (folderNode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinDataTreeModel::removeModelItem (SkinEditData::ContainerType type, SkinEditItem* skinItem)
{
	auto folderNode = findFolderNode (type);
	ASSERT (folderNode != nullptr)
	if(folderNode == nullptr) // should not happen!
		return;

	for(auto treeNode : iterate_as<TreeViewNode> (folderNode->getContent ()))
		if(SkinDataTreeNode::getSkinItem (treeNode) == skinItem)		
		{
			folderNode->remove (treeNode);
			treeNode->release ();
			refreshNode (folderNode);
			break;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinDataTreeModel::focusModelItem (SkinEditData::ContainerType type, SkinEditItem* skinItem)
{
	if(auto folderNode = findFolderNode (type))
	for(auto treeNode : iterate_as<TreeViewNode> (folderNode->getContent ()))
		if(SkinDataTreeNode::getSkinItem (treeNode) == skinItem)
		{
			setFocusNode (treeNode);
			makeNodeVisible (treeNode);
			break;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinDataTreeModel::onItemFocused (ItemIndexRef index)
{
	auto focusNode = resolveNode (index);
	auto item = focusNode ? SkinDataTreeNode::getSkinItem (focusNode) : nullptr;
	component.onItemFocused (item);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinDataTreeModel::interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection)
{
	if(msg.category == "Edit" && msg.name == "Delete")
	{
		if(!msg.checkOnly ())
		{
			ObjectArray itemsToDelete;
			ForEachItem (selection, selectedIndex)
				if(auto treeNode = resolveNode (selectedIndex))
					if(auto skinItem = SkinDataTreeNode::getSkinItem (treeNode))
						itemsToDelete.add (skinItem);
			EndFor

			component.performRemoval (itemsToDelete);
		}
		return true;
	}
	return SuperClass::interpretCommand (msg, item, selection);
}

//************************************************************************************************
// SkinDataTreeNode
//************************************************************************************************

SkinEditItem* SkinDataTreeNode::getSkinItem (TreeViewNode* treeNode)
{
	return treeNode->getDetails ().getObject<SkinEditItem> ("skinItem"); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinDataTreeNode::setSkinItem (TreeViewNode* treeNode, SkinEditItem* skinItem)
{
	treeNode->getDetails ().set ("skinItem", skinItem, Attributes::kShare); 
}
