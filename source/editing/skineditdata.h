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
// Filename    : skineditdata.h
// Description : Skin Editing Data Model
//
//************************************************************************************************

#ifndef _ccl_skineditdata_h
#define _ccl_skineditdata_h

#include "ccl/public/gui/framework/iskineditsupport.h"

#include "ccl/app/actions/iactioncontext.h"

#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/graphics/color.h"

namespace CCL {

interface IProgressNotify;

class SkinEditData;
class SkinEditItem;
class SkinViewItem;
class SkinFormItem;
class SkinViewClass;

//************************************************************************************************
// SkinEditSettings
//************************************************************************************************

class SkinEditSettings: public Object
{
public:
	SkinEditSettings ();

	PROPERTY_VARIABLE (Color, formEditBackColor, FormEditBackColor)
	PROPERTY_OBJECT (Point, defaultFormSize, DefaultFormSize)

	const Attributes& getDefaults () const { return defaults; }
	void setDefaults (const Attributes& _defaults) { defaults.copyFrom (_defaults); }

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	Attributes defaults;
};

//************************************************************************************************
// SkinEditData
//************************************************************************************************

class SkinEditData: public Object,
					public IActionContext

{
public:
	DECLARE_CLASS_ABSTRACT (SkinEditData, Object)

	SkinEditData (ActionJournal* actionJournal);
	~SkinEditData ();

	void construct (ISkinLoader& loader);

	ISkinEditSupport* getEditSupport () { return editSupport; }
	SkinFormItem* getReferencedFormItem (SkinViewItem* viewItem) const;

	SkinEditSettings& getEditSettings () { return editSettings; }
	const SkinEditSettings& getEditSettings () const { return editSettings; }

	const Container& getClasses () const { return classes; }

	enum ContainerType
	{
		kFonts = ISkinModel::kFontsElement,
		kStyles = ISkinModel::kStylesElement,
		kImages = ISkinModel::kImagesElement,
		kForms = ISkinModel::kFormsElement,
		kUndefined = 0
	};

	const Container& getContainerForType (ContainerType type) const;
	SkinEditItem* findItemByName (ContainerType type, StringID name) const;
	bool hasItemWithName (ContainerType type, StringID name) const;
	bool hasSourceFile (ContainerType type, StringRef sourceFile) const;

	SkinEditItem* createItem (UIDRef cid) const;
	void addItem (ContainerType type, SkinEditItem* item, int index = -1);
	bool removeItem (ContainerType type, SkinEditItem* item, int* oldIndex = 0);
	ContainerType getTypeForItem (SkinEditItem* item) const;

	// Notifications
	void itemChanged (SkinEditItem* item, StringID attributeName = 0);
	void editSettingsChanged ();

	DECLARE_STRINGID_MEMBER (kItemAdded) // args[0]: container type, args[1]: SkinEditItem, args[2]: index
	DECLARE_STRINGID_MEMBER (kItemRemoved) // args[0]: container type, args[1]: SkinEditItem
	DECLARE_STRINGID_MEMBER (kAssetChanged) // args[0]: item
	DECLARE_STRINGID_MEMBER (kEditSettingsChanged)

	// IActionContext
	ActionJournal* getActionJournal () const override;

	CLASS_INTERFACE (IActionContext, Object)

protected:
	SharedPtr<ISkinLoader> loader;
	SharedPtr<ISkinModel> model;
	UnknownPtr<ISkinEditSupport> editSupport;
	ActionJournal* actionJournal;
	ObjectArray classes;
	ObjectArray images;
	ObjectArray forms;
	ObjectArray styles;
	ObjectArray fonts;
	SkinEditSettings editSettings;

	ObjectArray& getContainer (ContainerType type);

	template <class IFace, class ItemClass>
	void build (Container& items, ISkinModel::ElementType which);
	void buildAll ();
	SkinEditItem* findItemForElement (const ObjectArray& items, const ISkinElement* element) const;
};

//************************************************************************************************
// SkinEditItem
//************************************************************************************************

class SkinEditItem: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (SkinEditItem, Object)

	SkinEditItem (ISkinElement& _element)
	: skinData (0),
	  element (&_element)
	{}

	PROPERTY_POINTER (SkinEditData, skinData, SkinData)
	ISkinElement& getElement () const { return *element; }
	String getSourceFile () const;

	bool hasAttribute (StringID name) const;
	String getStringAttribute (StringID name) const;

	// Object
	Object* clone () const override;

protected:
	SharedPtr<ISkinElement> element;

	SkinEditItem (IUnknown& _element)
	: skinData (0),
	  element (UnknownPtr<ISkinElement> (&_element))
	{
		ASSERT (element.isValid ())
	}

	template <class ItemClass, class Interface>
	ItemClass* cloneElement () const
	{
		AutoPtr<ISkinElement> elementCopy;
		element->clone (elementCopy);
		UnknownPtr<Interface> iface (elementCopy);
		ASSERT (iface != 0)
		return iface ? NEW ItemClass (*iface) : 0;
	}
};

//************************************************************************************************
// SkinImageItem
//************************************************************************************************

class SkinImageItem: public SkinEditItem
{
public:
	DECLARE_CLASS_ABSTRACT (SkinImageItem, SkinEditItem)

	SkinImageItem (ISkinImageElement& _imageElement)
	: SkinEditItem (_imageElement),
	  imageElement (&_imageElement)
	{}

	ISkinImageElement& getImageElement () const { return *imageElement; }

	// SkinEditItem
	Object* clone () const override;

protected:
	SharedPtr<ISkinImageElement> imageElement;
};

//************************************************************************************************
// SkinViewItem
//************************************************************************************************

class SkinViewItem: public SkinEditItem
{
public:
	DECLARE_CLASS_ABSTRACT (SkinViewItem, SkinEditItem)
	DECLARE_METHOD_NAMES (SkinViewItem)

	SkinViewItem (ISkinViewElement& viewElement);

	String getDisplayName () const;
	MutableCString getClassName () const;

	ISkinViewElement& getViewElement () const { return *viewElement; }
	RectRef getSize () const { return viewElement->getSize (); }
	void setSize (RectRef size) { viewElement->setSize (size); }

	PROPERTY_POINTER (SkinViewItem, parentViewItem, ParentViewItem)
	virtual SkinFormItem* getForm () const;

	PROPERTY_FLAG (flags, 1<<0, isHidden)
	PROPERTY_FLAG (flags, 1<<1, isDisabled)
	PROPERTY_FLAG (flags, 1<<2, isVariantOrTabView)
	PROPERTY_FLAG (flags, 1<<3, canHaveChildViews)
	PROPERTY_VARIABLE (int, selectedChildIndex, SelectedChildIndex)
	SkinViewItem* getSelectedChildItem () const;
	bool isSelectable () const;
	void flagsChanged ();

	void buildChildren ();
	void setSkinDataDeep (SkinEditData* skinData);
	void setSourceFileDeep (StringRef sourceFile);

	void addChild (SkinViewItem* viewItem, int index = -1);
	bool removeChild (SkinViewItem* viewItem, int* oldIndex = 0);
	const Container& getChildren () const { return children; }
	int getIndexInParent () const;
	bool hasParent (const SkinViewItem* item) const;
	bool hasChild (const SkinViewItem* item) const;

	void getClientRect (Rect& r) const;
	void clientToForm (Point& p) const;
	void formToClient (Point& p) const;
	void formToClient (Rect& r) const;

	// SkinViewItem
	Object* clone () const override;

protected:
	SharedPtr<ISkinViewElement> viewElement;
	ObjectArray children;
	int flags;

	void cloneDeep (SkinViewItem& other) const;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// SkinFormItem
//************************************************************************************************

class SkinFormItem: public SkinViewItem
{
public:
	DECLARE_CLASS_ABSTRACT (SkinFormItem, SkinViewItem)

	using SkinViewItem::SkinViewItem;

	DECLARE_STRINGID_MEMBER (kViewItemAdded) // args[0]: SkinViewItem, args[1]: index
	DECLARE_STRINGID_MEMBER (kViewItemRemoved) // args[0]: SkinViewItem
	DECLARE_STRINGID_MEMBER (kViewItemChanged) // args[0]: SkinViewItem

	// SkinViewItem
	SkinFormItem* getForm () const override;
	Object* clone () const override;
};

//************************************************************************************************
// SkinItemClass
//************************************************************************************************

class SkinItemClass: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (SkinItemClass, Object)

	SkinItemClass (const ITypeLibrary* library, ITypeInfo& _elementClass);

	PROPERTY_POINTER (SkinEditData, skinData, SkinData)
	PROPERTY_POINTER (const ITypeLibrary, library, Library)
	PROPERTY_MUTABLE_CSTRING (className, ClassName)
	const ITypeInfo& getElementClass () const { return *elementClass; }

protected:
	struct DetailCollector;

	SharedPtr<ITypeInfo> elementClass;
};

//************************************************************************************************
// SkinViewClass
//************************************************************************************************

class SkinViewClass: public SkinItemClass
{
public:
	DECLARE_CLASS_ABSTRACT (SkinViewClass, SkinItemClass)

	SkinViewClass (const ITypeLibrary* library, ITypeInfo& _elementClass);

	SkinViewItem* createViewItem () const;

protected:
	void getDefaultAttributes (Attributes& a) const;
};

} // namespace CCL

#endif // _ccl_skineditdata_h
