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
// Filename    : skineditdata.cpp
// Description : Skin Editing Data Model
//
//************************************************************************************************

#define DEBUG_LOG 1
#define CCL_BOX_STATIC // use static "boxing"

#include "skineditdata.h"

#include "ccl/app/actions/actionjournal.h"
#include "ccl/app/utilities/boxedguitypes.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/base/itypelib.h"

namespace CCL {

//************************************************************************************************
// SkinItemInserter
//************************************************************************************************

struct SkinItemInserter
{
	ObjectArray& itemContainer;
	ISkinElementChildren* elementContainer;

	SkinItemInserter (ObjectArray& itemContainer, ISkinElementChildren* elementContainer)
	: itemContainer (itemContainer),
	  elementContainer (elementContainer)
	{}

	void addItem (SkinEditItem* item, int index)
	{
		if(index != -1)
		{
			if(!itemContainer.insertAt (index, item))
				itemContainer.add (item);
		}
		else
			itemContainer.add (item);

		if(elementContainer)
		{
			if(elementContainer->addChildElement (&item->getElement (), index))
				item->getElement ().retain ();
		}
	}

	bool removeItem (SkinEditItem* item, int* oldIndex)
	{
		if(oldIndex)
			*oldIndex = itemContainer.index (item);
		if(!itemContainer.remove (item))
			return false;

		if(elementContainer)
		{
			if(elementContainer->removeChildElement (&item->getElement ()))
				item->getElement ().release ();
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
	XSTRING (Unnamed, "<unnamed>")
END_XSTRINGS

//************************************************************************************************
// SkinEditSettings
//************************************************************************************************

SkinEditSettings::SkinEditSettings ()
: formEditBackColor (Colors::kWhite)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditSettings::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	Colors::fromString (formEditBackColor, a.getString ("backcolor"));
	defaultFormSize.x = a.getInt ("formwidth");
	defaultFormSize.y = a.getInt ("formheight");
	if(auto savedDefaults = a.getAttributes ("defaults"))
		defaults.copyFrom (*savedDefaults);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditSettings::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	String colorString;
	Colors::toString (formEditBackColor, colorString, false);
	a.set ("backcolor", colorString);
	a.set ("formwidth", defaultFormSize.x);
	a.set ("formheight", defaultFormSize.y);
	a.set ("defaults", defaults, true);
	return true;
}

//************************************************************************************************
// SkinEditData
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinEditData, Object)
DEFINE_STRINGID_MEMBER_ (SkinEditData, kItemAdded, "itemAdded")
DEFINE_STRINGID_MEMBER_ (SkinEditData, kItemRemoved, "itemRemoved")
DEFINE_STRINGID_MEMBER_ (SkinEditData, kAssetChanged, "assetChanged")
DEFINE_STRINGID_MEMBER_ (SkinEditData, kEditSettingsChanged, "editSettingsChanged")

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditData::SkinEditData (ActionJournal* actionJournal)
: actionJournal (actionJournal)
{
	ASSERT (actionJournal)
	actionJournal->retain ();

	classes.objectCleanup (true);
	images.objectCleanup (true);
	forms.objectCleanup (true);
	styles.objectCleanup (true);
	fonts.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditData::~SkinEditData ()
{
	actionJournal->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinEditData::construct (ISkinLoader& _loader)
{
	ISkinModel* _model = _loader.getISkinModel ();
	ASSERT (_model != nullptr && model == nullptr)
	loader = &_loader;
	model = _model;
	editSupport = _model;

	buildAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectArray& SkinEditData::getContainer (ContainerType type)
{
	switch(type)
	{
	case kFonts : return fonts;
	case kStyles : return styles;
	case kImages : return images;
	default : ASSERT (type == kForms)
	case kForms : return forms;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& SkinEditData::getContainerForType (ContainerType type) const
{
	return const_cast<SkinEditData*> (this)->getContainer (type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditItem* SkinEditData::findItemByName (ContainerType type, StringID name) const
{
	for(auto item : iterate_as<SkinEditItem> (getContainerForType (type)))
		if(item->getElement ().getName ().compare (name, false) == Text::kEqual)
			return item;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditData::hasItemWithName (ContainerType type, StringID name) const
{
	return findItemByName (type, name) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditData::hasSourceFile (ContainerType type, StringRef sourceFile) const
{
	for(auto item : iterate_as<SkinEditItem> (getContainerForType (type)))
	{
		if(sourceFile.compare (item->getSourceFile (), false) == Text::kEqual)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditItem* SkinEditData::createItem (UIDRef cid) const
{
	AutoPtr<ISkinElement> newElement;
	if(editSupport)
		editSupport->createInstance (cid, ccl_iid<ISkinElement> (), newElement.as_ppv ());
	if(!newElement.isValid ())
		return nullptr;

	if(cid == ClassID::FormElement)
	{
		UnknownPtr<ISkinViewElement> viewElement (newElement);
		ASSERT (viewElement.isValid ())
		return viewElement ? NEW SkinFormItem (*viewElement) : nullptr;
	}
	else if(cid == ClassID::ImageElement)
	{
		UnknownPtr<ISkinImageElement> imageElement (newElement);
		ASSERT (imageElement.isValid ())
		return imageElement ? NEW SkinImageItem (*imageElement) : nullptr;
	}
	else
	{
		return NEW SkinEditItem (*newElement);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinEditData::addItem (ContainerType type, SkinEditItem* item, int index)
{
	ASSERT (item != nullptr)
	ObjectArray& itemContainer = getContainer (type);
	UnknownPtr<ISkinElementChildren> elementContainer (model->getContainerForType (type));
	SkinItemInserter (itemContainer, elementContainer).addItem (item, index);
	item->setSkinData (this);

	if(index == -1)
		index = itemContainer.count ()-1;
	signal (Message (kItemAdded, type, item->asUnknown (), index));

	if(editSupport)
		editSupport->setModelDirty (type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditData::removeItem (ContainerType type, SkinEditItem* item, int* oldIndex)
{
	ASSERT (item != nullptr)
	ObjectArray& itemContainer = getContainer (type);
	UnknownPtr<ISkinElementChildren> elementContainer (model->getContainerForType (type));
	if(!SkinItemInserter (itemContainer, elementContainer).removeItem (item, oldIndex))
		return false;

	signal (Message (kItemRemoved, type, item->asUnknown ()));
	item->setSkinData (nullptr);

	if(editSupport)
		editSupport->setModelDirty (type);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditData::ContainerType SkinEditData::getTypeForItem (SkinEditItem* item) const
{
	if(ccl_cast<SkinFormItem> (item) != nullptr)
		return kForms;
	if(ccl_cast<SkinImageItem> (item) != nullptr)
		return kImages;
	if(styles.contains (item))
		return kStyles;
	if(fonts.contains (item))
		return kFonts;

	// should not happen!
	CCL_DEBUGGER ("Ambiguous container type!")
	return kUndefined;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class IFace, class ItemClass>
void SkinEditData::build (Container& items, ISkinModel::ElementType which)
{
	if(auto c = model->getContainerForType (which))
		ForEachUnknown (*c, unk)
			if(UnknownPtr<IFace> e = unk)
			{
				ItemClass* item = NEW ItemClass (*e);
				item->setSkinData (this);
				items.add (item);
			}
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinEditData::buildAll ()
{
	build<ISkinElement, SkinEditItem> (fonts, ISkinModel::kFontsElement);
	build<ISkinElement, SkinEditItem> (styles, ISkinModel::kStylesElement);
	build<ISkinImageElement, SkinImageItem> (images, ISkinModel::kImagesElement);
	build<ISkinViewElement, SkinFormItem> (forms, ISkinModel::kFormsElement);

	// form children
	for(auto formItem : iterate_as<SkinFormItem> (forms))
	{
		formItem->buildChildren ();
		formItem->setSkinDataDeep (this); // update cached flags, etc.
	}

	// classes
	auto typeLibrary = editSupport ? editSupport->getTypeLibrary () : nullptr;
	if(typeLibrary)
	{
		const ITypeInfo* viewBaseClass = editSupport->getViewBaseClass ();
		const ITypeInfo* formClass = editSupport->getFormClass ();
		ASSERT (viewBaseClass != nullptr)
		IterForEachUnknown (typeLibrary->newTypeIterator (), unk)
			if(UnknownPtr<ITypeInfo> typeInfo = unk)
			{
				SkinItemClass* itemClass = nullptr;
				if(ccl_is_base_of (viewBaseClass, typeInfo))
				{
					if(ccl_is_base_of (formClass, typeInfo)) // ignore form class
						continue;

					itemClass = NEW SkinViewClass (typeLibrary, *typeInfo);
				}
				else
					itemClass = NEW SkinItemClass (typeLibrary, *typeInfo);

				itemClass->setSkinData (this);
				classes.add (itemClass);
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinEditItem* SkinEditData::findItemForElement (const ObjectArray& items, const ISkinElement* element) const
{
	for(auto item : iterate_as<SkinEditItem> (items))
		if(&item->getElement () == element)
			return item;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinFormItem* SkinEditData::getReferencedFormItem (SkinViewItem* viewItem) const
{
	ASSERT (viewItem != nullptr)
	if(editSupport && viewItem)
		if(UnknownPtr<ISkinElement> otherElement = editSupport->getReferencedForm (&viewItem->getViewElement ()))
			return static_cast<SkinFormItem*> (findItemForElement (forms, otherElement));
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionJournal* SkinEditData::getActionJournal () const
{
	return actionJournal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinEditData::itemChanged (SkinEditItem* item, StringID attributeName)
{
	ASSERT (item)
	if(!attributeName.isEmpty ())
		item->signal (Message (kPropertyChanged, String (attributeName)));
	else
		item->signal (Message (kPropertyChanged));

	auto viewItem = ccl_cast<SkinViewItem> (item);
	auto formItem = viewItem ? viewItem->getForm () : nullptr;
	if(formItem)
		formItem->signal (Message (SkinFormItem::kViewItemChanged, viewItem->asUnknown ()));
	else
		signal (Message (kAssetChanged, item->asUnknown ()));

	if(editSupport)
	{
		if(formItem)
			editSupport->setModelDirty (ISkinModel::kFormsElement, &formItem->getElement ());
		else
			editSupport->setModelDirty (getTypeForItem (item));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinEditData::editSettingsChanged ()
{
	signal (Message (kEditSettingsChanged));
}

//************************************************************************************************
// SkinEditItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinEditItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

String SkinEditItem::getSourceFile () const
{
	String sourceFile;
	int32 lineNumber = 0;
	element->getSourceInfo (sourceFile, lineNumber);
	return sourceFile;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinEditItem::hasAttribute (StringID name) const
{
	Variant value;
	return element->getAttributeValue (value, name) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String SkinEditItem::getStringAttribute (StringID name) const
{
	Variant value;
	element->getAttributeValue (value, name);
	return value.asString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* SkinEditItem::clone () const
{
	return cloneElement<SkinEditItem, ISkinElement> ();
}

//************************************************************************************************
// SkinImageItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinImageItem, SkinEditItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* SkinImageItem::clone () const
{
	return cloneElement<SkinImageItem, ISkinImageElement> ();
}

//************************************************************************************************
// SkinViewItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinViewItem, SkinEditItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewItem::SkinViewItem (ISkinViewElement& _viewElement)
: SkinEditItem (_viewElement),
  parentViewItem (nullptr),
  selectedChildIndex (0),
  viewElement (&_viewElement),
  flags (0)
{
	children.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* SkinViewItem::clone () const
{
	auto other = cloneElement<SkinViewItem, ISkinViewElement> ();
	if(other)
		cloneDeep (*other);
	return other;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItem::cloneDeep (SkinViewItem& other) const
{
	other.flags = flags;
	other.buildChildren ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String SkinViewItem::getDisplayName () const
{
	String name (getElement ().getName ());
	if(name.isEmpty ())
		name = XSTR (Unnamed);
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString SkinViewItem::getClassName () const
{
	MutableCString className;
	if(auto elementClass = getElement ().getElementClass ())
		className = elementClass->getClassName ();
	else
		className = getElement ().getName ();
	return className;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinFormItem* SkinViewItem::getForm () const
{
	return parentViewItem ? parentViewItem->getForm () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItem::buildChildren ()
{
	UnknownPtr<IContainer> c (element);
	if(c)
		ForEachUnknown (*c, unk)
			if(UnknownPtr<ISkinViewElement> e = unk)
			{
				SkinViewItem* childItem = NEW SkinViewItem (*e);
				childItem->setParentViewItem (this);
				children.add (childItem);
				childItem->buildChildren ();
			}
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItem::setSkinDataDeep (SkinEditData* skinData)
{
	setSkinData  (skinData);

	// update cached flags when attached
	if(skinData)
	{
		auto editSupport = skinData->getEditSupport ();
		isVariantOrTabView (editSupport && editSupport->isVariantOrTabView (viewElement));
		canHaveChildViews (editSupport && editSupport->canHaveChildViews (viewElement));
	}

	for(auto childItem : iterate_as<SkinViewItem> (children))
		childItem->setSkinDataDeep (skinData);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItem::setSourceFileDeep (StringRef sourceFile)
{
	element->setSourceFile (sourceFile);

	for(auto childItem : iterate_as<SkinViewItem> (children))
		childItem->setSourceFileDeep (sourceFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItem::addChild (SkinViewItem* viewItem, int index)
{
	ASSERT (viewItem && !viewItem->getParentViewItem ())
	viewItem->setParentViewItem (this);
	viewItem->setSkinDataDeep (skinData);

	UnknownPtr<ISkinElementChildren> elementContainer (element);
	SkinItemInserter (children, elementContainer).addItem (viewItem, index);

	if(SkinFormItem* formItem = getForm ())
	{
		if(index == -1)
			index = children.count ()-1;
		formItem->signal (Message (SkinFormItem::kViewItemAdded, viewItem->asUnknown (), index));
	}

	if(skinData)
		skinData->itemChanged (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinViewItem::removeChild (SkinViewItem* viewItem, int* oldIndex)
{
	ASSERT (viewItem)
	UnknownPtr<ISkinElementChildren> elementContainer (element);
	if(!SkinItemInserter (children, elementContainer).removeItem (viewItem, oldIndex))
		return false;

	if(SkinFormItem* formItem = getForm ())
		formItem->signal (Message (SkinFormItem::kViewItemRemoved, viewItem->asUnknown ()));

	ASSERT (viewItem->getParentViewItem () == this)
	viewItem->setParentViewItem (nullptr);
	viewItem->setSkinDataDeep (nullptr);

	if(skinData)
		skinData->itemChanged (this);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SkinViewItem::getIndexInParent () const
{
	if(parentViewItem)
	{
		int index = parentViewItem->getChildren ().index (this);
		CCL_PRINTF ("Index of %p in parent %p = %d (count = %d)\n",
					this, parentViewItem, index, parentViewItem->getChildren ().count ())
		return index;
	}
	else
		return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinViewItem::hasParent (const SkinViewItem* item) const
{
	ASSERT (item)
	if(parentViewItem == item)
		return true;
	if(parentViewItem)
		return parentViewItem->hasParent (item);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinViewItem::hasChild (const SkinViewItem* item) const
{
	ASSERT (item)
	if(children.contains (item))
		return true;
	for(auto child : iterate_as<SkinViewItem> (children))
		if(child->hasChild (item))
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewItem* SkinViewItem::getSelectedChildItem () const
{
	if(isVariantOrTabView ())
		return static_cast<SkinViewItem*> (children.at (selectedChildIndex));
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinViewItem::isSelectable () const
{
	if(isHidden () || isDisabled ())
		return false;

	if(parentViewItem)
	{
		if(parentViewItem->isVariantOrTabView ())
		{
			if(parentViewItem->getSelectedChildItem () != this)
				return false;
		}

		return parentViewItem->isSelectable ();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItem::flagsChanged ()
{
	if(skinData)
		skinData->itemChanged (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItem::getClientRect (Rect& r) const
{
	RectRef size = getSize ();
	r (0, 0, size.getWidth (), size.getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItem::clientToForm (Point& p) const
{
	RectRef size = getSize ();
	p.x += size.left;
	p.y += size.top;

	if(parentViewItem)
		parentViewItem->clientToForm (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItem::formToClient (Point& p) const
{
	Point offset;
	clientToForm (offset);

	p.x -= offset.x;
	p.y -= offset.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewItem::formToClient (Rect& r) const
{
	Point p (r.getLeftTop ());
	formToClient (p);
	r.moveTo (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (SkinViewItem)
	DEFINE_METHOD_NAME ("getDisplayName")
	DEFINE_METHOD_NAME ("getClassName")
	DEFINE_METHOD_NAME ("getSize")
	DEFINE_METHOD_NAME ("getParentViewItem")
END_METHOD_NAMES (SkinViewItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinViewItem::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getDisplayName")
	{
		String string (getDisplayName ());
		returnValue = string;
		returnValue.share ();
		return true;
	}
	else if(msg == "getClassName")
	{
		String string (getClassName ());
		returnValue = string;
		returnValue.share ();
		return true;
	}
	else if(msg == "getSize")
	{
		RectRef size = getSize ();
		CCL_BOX (Boxed::Rect, boxedSize, size)
		returnValue.takeShared (boxedSize->asUnknown ());
		return true;
	}
	else if(msg == "getParentViewItem")
	{
		returnValue.takeShared (ccl_as_unknown (getParentViewItem ()));
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// SkinFormItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinFormItem, SkinViewItem)
DEFINE_STRINGID_MEMBER_ (SkinFormItem, kViewItemAdded, "viewItemAdded")
DEFINE_STRINGID_MEMBER_ (SkinFormItem, kViewItemRemoved, "viewItemRemoved")
DEFINE_STRINGID_MEMBER_ (SkinFormItem, kViewItemChanged, "viewItemChanged")

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* SkinFormItem::clone () const
{
	auto other = cloneElement<SkinFormItem, ISkinViewElement> ();
	if(other)
		cloneDeep (*other);
	return other;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinFormItem* SkinFormItem::getForm () const
{
	return const_cast<SkinFormItem*> (this);
}

//************************************************************************************************
// SkinItemClass::DetailCollector
//************************************************************************************************

struct SkinItemClass::DetailCollector: public Object, public ITypeInfoDetails
{
	Vector<Model::MemberDescription> members;

	// ITypeInfoDetails
	void CCL_API addMember (const Model::MemberDescription& member) override
	{
		members.add (member);
	}

	void CCL_API setAttribute (StringID id, VariantRef value) override
	{
	}

	CLASS_INTERFACE (ITypeInfoDetails, Object)
};

//************************************************************************************************
// SkinItemClass
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinItemClass, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinItemClass::SkinItemClass (const ITypeLibrary* library, ITypeInfo& _elementClass)
: skinData (nullptr),
  library (library),
  elementClass (&_elementClass)
{
	setClassName (_elementClass.getClassName ());
}

//************************************************************************************************
// SkinViewClass
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinViewClass, SkinItemClass)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewClass::SkinViewClass (const ITypeLibrary* library, ITypeInfo& elementClass)
: SkinItemClass (library, elementClass)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewItem* SkinViewClass::createViewItem () const
{
	AutoPtr<IUnknown> unk = elementClass->createInstance ();
	UnknownPtr<ISkinViewElement> viewElement (unk);
	if(!viewElement)
		return nullptr;

	SkinViewItem* item = NEW SkinViewItem (*viewElement);
	item->getElement ().setName (elementClass->getClassName ()); // hmm, is it always the class name?

	Attributes defaultAttr;
	getDefaultAttributes (defaultAttr);
	item->getElement ().setAttributes (defaultAttr);

	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinViewClass::getDefaultAttributes (Attributes& a) const
{
	Vector<const ITypeInfo*> allTypes;
	for(const ITypeInfo* current = elementClass; current != nullptr; current = current->getParentType ())
		allTypes.add (current);

	DetailCollector collector;
	VectorForEachReverse (allTypes, const ITypeInfo*, current) // start with base class
		library->getTypeDetails (collector, *current);
	EndFor

	VectorForEachFast (collector.members, Model::MemberDescription, member)
		if(member.type == ITypeInfo::kString)
		{
			// init with default value (if available)
			ASSERT (skinData != nullptr)
			MutableCString defaultsId (getClassName ());
			defaultsId += ".";
			defaultsId += member.name;
			String defaultString = skinData->getEditSettings ().getDefaults ().getString (defaultsId);

			a.set (member.name, defaultString);
		}
	EndFor
}
