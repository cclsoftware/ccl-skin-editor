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
// Filename    : skincodeexport.cpp
// Description : Skin Code Export
//
//************************************************************************************************

#include "skincodeexport.h"
#include "editing/skineditdata.h"

#include "ccl/extras/modeling/cplusplus.h"

#include "ccl/base/storage/textfile.h"

#include "ccl/public/base/memorystream.h"

#include "core/gui/coreskinformat.impl.h"

#include "core/extras/tools/bin2c.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Definitions for corelib static views
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CoreCode
{
	const CStringPtr kViewHeader = "core/portable/gui/corestaticview.h";
	const CStringPtr kFontHeader = "core/portable/gui/corefont.h";
	const CStringPtr kSingletonHeader = "core/portable/coresingleton.h";

	const CStringPtr kCoreNamespace = "Core";
	const CStringPtr kPortableNamespace = "Portable";

	const CStringPtr kSingletonTemplate = "Core::Portable::StaticSingleton";
	const CStringPtr kSingletonMacro = "DEFINE_STATIC_SINGLETON";

	String makeSingletonName (StringRef className)
	{
		return String () << CoreCode::kSingletonTemplate << "<" << className << ">";
	}

	String getSingletonInstance (StringRef className)
	{
		return String () << className << "::instance ()";
	}

	const CStringPtr kStyleClass = "Core::Portable::Style";
	const CStringPtr kBitmapClass = "Core::Portable::Bitmap";
	const CStringPtr kBitmapFontClass = "Core::Portable::BitmapInplaceFont";
	const CStringPtr kFontProviderClass = "Core::Portable::FontProvider";

	String makeGetFontMethod (StringRef className = 0)
	{
		String code;
		code << "const Core::Portable::BitmapFont* ";
		if(!className.isEmpty ())
			code << className << "::";
		code << "getFont (Core::CStringPtr name) const";
		return code;
	}

	const CStringPtr kSetExternalFontProviderThis = "FontManager::instance ().setExternalFontProvider (this);";
	String setDefaultColorFont (StringRef fontName)
	{
		return String () << "FontManager::instance ().setDefaultColorFont (&" << fontName << ");";
	}

	const CStringPtr kContainerViewClass = "Core::Portable::StaticContainerView";
	const CStringPtr kLabelClass = "Core::Portable::StaticLabel";
	const CStringPtr kButtonClass = "Core::Portable::StaticButton";
	const CStringPtr kImageViewClass = "Core::Portable::StaticImageView";
	const CStringPtr kTextBoxClass = "Core::Portable::StaticTextBox";
	const CStringPtr kValueBarClass = "Core::Portable::StaticValueBar";
	const CStringPtr kVariantViewClass = "Core::Portable::StaticVariantView";
	const CStringPtr kCustomViewClass = "Core::Portable::StaticCustomView";
	const CStringPtr kListViewClass = "Core::Portable::StaticListView";

	CString toStaticClass (StringID viewClass)
	{
		if(viewClass == Core::Skin::ViewClasses::kLabel)
			return kLabelClass;
		if(viewClass == Core::Skin::ViewClasses::kButton)
			return kButtonClass;
		else if(viewClass == Core::Skin::ViewClasses::kImageView)
			return kImageViewClass;
		else if(viewClass == Core::Skin::ViewClasses::kTextBox)
			return kTextBoxClass;
		else if(viewClass == Core::Skin::ViewClasses::kValueBar)
			return kValueBarClass;
		else if(viewClass == Core::Skin::ViewClasses::kVariantView)
			return kVariantViewClass;
		else if(viewClass == Core::Skin::ViewClasses::kListView)
			return kListViewClass;
		else
			return CString::kEmpty;
	}

	String setName (StringRef member, StringRef name)
	{
		return Model::Cpp::CallMethod (member, "setName", Model::Cpp::Literal (name));
	}

	String setTitle (StringRef member, StringRef title)
	{
		return Model::Cpp::CallMethod (member, "setTitle", Model::Cpp::Literal (title));
	}

	String setSize (StringRef member, RectRef size)
	{
		String args;
		args << "Rect (" << size.left << ", " << size.top << ", " << size.right << ", " << size.bottom << ")";

		return Model::Cpp::CallMethod (member, "setSize", args);
	}

	String callColorMethod (StringRef member, StringRef method, Color c)
	{
		MutableCString arg;
		arg.appendFormat ("Color (0x%02X, 0x%02X, 0x%02X)", static_cast<int> (c.red), static_cast<int> (c.green), static_cast<int> (c.blue));
		return String () << Model::Cpp::CallMethod (member, method, String (arg));
	}

	String setFontName (StringRef member, StringRef fontName)
	{
		return Model::Cpp::CallMethod (member, "setFontName", Model::Cpp::Literal (fontName));
	}

	String setTextAlign (StringRef member, StringRef align)
	{
		String arg;

		if(align == "right")
			arg = "Alignment::kRight";
		else if(align == "center")
			arg = "Alignment::kCenter";
		else
			arg = "Alignment::kLeft";

		return Model::Cpp::CallMethod (member, "setTextAlign", arg);
	}

	String setStyle (StringRef member, StringRef style)
	{
		return Model::Cpp::CallMethod (member, "setStyle", style);
	}

	String addView (StringRef parent, StringRef child)
	{
		return Model::Cpp::CallMethod (parent, "addView", String () << "&" << child);
	}

	String makeOptionCalls (StringRef member, StringRef optionString, const Core::EnumInfo optionEnum[], const Core::EnumInfo optionCalls[], StringRef indent)
	{
		String result;
		int optionMask = Core::EnumInfo::parseMultiple (optionString, optionEnum);
		for(int i = 0, count = Core::EnumInfo::getCount (optionCalls); i < count; i++)
			if(optionMask & optionCalls[i].value)
				result << indent << Model::Cpp::CallMethod (member, optionCalls[i].name, "true");
		return result;
	}
}

} // namespace CCL

using namespace CCL;
using namespace Model::Cpp;

//************************************************************************************************
// SkinCodeExporter
//************************************************************************************************

CStringPtr SkinCodeExporter::kFilenameID = "CodeExport.Filename";
CStringPtr SkinCodeExporter::kNamespaceID = "CodeExport.Namespace";

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinCodeExporter::SkinCodeExporter (SkinEditData& skinData, StringRef documentName, UrlRef targetFolder)
: skinData (skinData),
  documentName (documentName),
  targetFolder (targetFolder)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinCodeExporter::exportAll ()
{
	auto& settings = skinData.getEditSettings ().getDefaults ();
	String fileName = settings.getString (kFilenameID);
	if(fileName.isEmpty ())
	{
		fileName = documentName;
		fileName.toLowercase ();
	}

	Url headerFilePath (targetFolder);
	headerFilePath.descend (fileName);
	headerFilePath.setFileType (HeaderFile (), false);

	Url sourceFilePath (headerFilePath);
	sourceFilePath.setFileType (SourceFile (), true);

	TextFile headerFile (headerFilePath, Text::kASCII);
	TextFile sourceFile (sourceFilePath, Text::kASCII);
	if(!headerFile.isValid () || !sourceFile.isValid ())
		return false;

	String headerFileName;
	headerFilePath.getName (headerFileName);
	CommentLine generatorComment (String () << "Generated from " << documentName);
	String includeGuard = IncludeGuardName (headerFilePath);

	// *** Begin Header File ***
	String header;
	header << generatorComment;
	header << nl;
	header << IncludeGuard (includeGuard, true);
	header << nl;
	header << Include (CoreCode::kViewHeader);
	header << Include (CoreCode::kFontHeader);
	header << Include (CoreCode::kSingletonHeader);
	header << nl;

	String namespaceSetting = settings.getString (kNamespaceID);
	NamespaceBuilder namespaceBuilder (namespaceSetting);
	header << namespaceBuilder.asCode (true);
	header << nl;

	// *** Begin Source File ***
	String source;
	source << generatorComment;
	source << nl;
	source << Include (headerFileName);
	source << nl;
	source << namespaceBuilder.asUsing ();
	source << UsingNamespace (CoreCode::kCoreNamespace);
	source << UsingNamespace (CoreCode::kPortableNamespace);
	source << nl;

	auto separatedSection = [] (StringRef comment)
	{
		return String () << ::bin2c_commentline << CommentLine (comment) << ::bin2c_commentline;
	};

	// *** Resource class declaration ***
	String resourceClassName ("Resources");
	String resourceInstance = CoreCode::getSingletonInstance (resourceClassName);
	auto getResourceAddress = [&] (StringRef resourceName)
	{
		return String () << "&" << resourceInstance << "." << resourceName;
	};

	header << separatedSection (resourceClassName);
	header << nl;
	header << BeginClass (resourceClassName,
						  CoreCode::kFontProviderClass, Access::kPublic,
						  CoreCode::makeSingletonName (resourceClassName), Access::kPublic);
	header << ClassSection (ClassSection::kPublic);
	header << tab << DeclareCtor (resourceClassName);
	header << nl;
	header << tab << CommentLine (CoreCode::kFontProviderClass);
	header << tab << CoreCode::makeGetFontMethod () << ";" << nl;
	header << nl;

	auto binaryToCode = [] (IMemoryStream& binData, CStringPtr arrayName)
	{
		String code;
		MemoryStream dstData;
		CoreStream srcAdapter (binData);
		::bin2c (dstData, srcAdapter, arrayName, binData.getBytesWritten ());
		code.appendASCII (dstData.getBuffer ().as<char> (), dstData.getBytesWritten ());
		return code;
	};

	// *** Styles declarations ***
	auto& styles = skinData.getContainerForType (SkinEditData::kStyles);
	for(auto styleItem : iterate_as<SkinEditItem> (styles))
	{
		// member declaration in resource class
		MutableCString styleName = styleItem->getElement ().getName ();
		header << tab << DeclareMember (CoreCode::kStyleClass, String (styleName));
	}

	// *** Bitmap data ***
	auto& images = skinData.getContainerForType (SkinEditData::kImages);
	for(auto imageItem : iterate_as<SkinImageItem> (images))
	{
		// member declaration in resource class
		MutableCString imageName = imageItem->getElement ().getName ();
		header << tab << DeclareMember (CoreCode::kBitmapClass, String (imageName));
				
		// bitmap data in cpp file
		AutoPtr<IMemoryStream> srcData = skinData.getEditSupport ()->loadBinaryFile (imageItem->getImageElement ().getImagePath ());
		ASSERT (srcData.isValid ())
		if(!srcData)
			continue;

		source << separatedSection (String (imageName));
		source << nl;
		source << binaryToCode (*srcData, imageName);
		source << nl;
	}

	// *** Font Data ***
	const CStringPtr kFontDataSuffix = "_data";
	const CStringPtr kFontImageSuffix = "_image";
	SkinEditItem* defaultColorFont = nullptr;

	auto& fonts = skinData.getContainerForType (SkinEditData::kFonts);
	for(auto fontItem : iterate_as<SkinEditItem> (fonts))
	{
		// member declaration in resource class
		MutableCString fontName = fontItem->getElement ().getName ();
		header << tab << DeclareMember (CoreCode::kBitmapFontClass, String (fontName));

		// check for default font
		Variant defaultVar;
		fontItem->getElement ().getAttributeValue (defaultVar, Core::Skin::ResourceAttributes::kDefault);
		if(defaultVar.asBool ())
			defaultColorFont = fontItem;

		// assume bitmap font with .fnt and .bmp file
		Url fontFilePath;
		fontFilePath.setPath (fontItem->getStringAttribute (Core::Skin::ResourceAttributes::kFile));
		Url fontImagePath (fontFilePath);
		fontImagePath.setExtension ("bmp");

		AutoPtr<IMemoryStream> fontData = skinData.getEditSupport ()->loadBinaryFile (fontFilePath.getPath ());
		AutoPtr<IMemoryStream> imageData = skinData.getEditSupport ()->loadBinaryFile (fontImagePath.getPath ());
		ASSERT (fontData.isValid () && imageData.isValid ())
		if(!fontData || !imageData)
			continue;

		MutableCString fontDataName = fontName;
		fontDataName += kFontDataSuffix;
		MutableCString fontImageName = fontName;
		fontImageName += kFontImageSuffix;

		source << separatedSection (String (fontName));
		source << nl;
		source << binaryToCode (*fontData, fontDataName);
		source << nl;
		source << binaryToCode (*imageData, fontImageName);
		source << nl;		
	}

	// *** End of resource class declaration ***
	header << EndClass ();
	header << nl;

	// *** Resource class ctor ***
	source << separatedSection (resourceClassName);
	source << nl;
	source << CoreCode::kSingletonMacro << " (" << resourceClassName << ")" << nl;
	source << nl;
	source << BeginCtor (resourceClassName) << ":";
	
	bool firstMember = true;
	for(auto imageItem : iterate_as<SkinImageItem> (images))
	{
		MutableCString imageName = imageItem->getElement ().getName ();
		MutableCString code, size;
		::bin2c_code (code, imageName);
		::bin2c_size (size, imageName);
		
		if(firstMember)
			firstMember = false;
		else
			source << "," << nl << " ";

		source << " " << imageName << " (" << code << ", " << size << ")";
	}
	
	for(auto fontItem : iterate_as<SkinEditItem> (fonts))
	{
		MutableCString fontName = fontItem->getElement ().getName ();
		MutableCString fontDataName = fontName;
		fontDataName += kFontDataSuffix;
		MutableCString fontImageName = fontName;
		fontImageName += kFontImageSuffix;

		MutableCString dataCode, dataSize, imageCode, imageSize;
		::bin2c_code (dataCode, fontDataName);
		::bin2c_size (dataSize, fontDataName);
		::bin2c_code (imageCode, fontImageName);
		::bin2c_size (imageSize, fontImageName);
		
		if(firstMember)
			firstMember = false;
		else
			source << "," << nl << " ";

		source << " " << fontName << " (" << Literal (fontName) << ", "
			   << imageCode << ", " << imageSize << ", " << dataCode << ", " << dataSize << ")";
	}

	source << nl;
	source << openBrace << nl;
	source << tab << CoreCode::kSetExternalFontProviderThis << nl;
	if(defaultColorFont)
		source << tab << CoreCode::setDefaultColorFont (String (defaultColorFont->getElement ().getName ())) << nl;
	
	// *** Style definitions ***
	// TODO: handle style inheritance...
	auto applyColor = [] (SkinEditItem* item, StringRef member, CStringPtr method, CStringPtr attrName)
	{
		if(item->hasAttribute (attrName))
		{
			Color c;
			Colors::fromString (c, item->getStringAttribute (attrName));
			return String () << tab << CoreCode::callColorMethod (member, method, c) << nl;
		}
		else
			return String ();
	};

	auto applyInt = [] (SkinEditItem* item, StringRef member, CStringPtr method, CStringPtr attrName)
	{
		if(item->hasAttribute (attrName))
		{
			Variant value;
			item->getElement ().getAttributeValue (value, attrName);
			return String () << tab << CallMethod (member, method, String () << value.parseInt ()) << nl;
		}
		else
			return String ();
	};

	source << nl;
	for(auto styleItem : iterate_as<SkinEditItem> (styles))
	{
		String styleName (styleItem->getElement ().getName ());

		source << applyColor (styleItem, styleName, "setBackColor", Core::Skin::ViewAttributes::kBackColor);
		source << applyColor (styleItem, styleName, "setBackColorDisabled", Core::Skin::ViewAttributes::kBackColorDisabled);
		source << applyColor (styleItem, styleName, "setForeColor", Core::Skin::ViewAttributes::kForeColor);
		source << applyColor (styleItem, styleName, "setForeColorDisabled", Core::Skin::ViewAttributes::kForeColorDisabled);
		source << applyColor (styleItem, styleName, "setTextColor", Core::Skin::ViewAttributes::kTextColor);
		source << applyColor (styleItem, styleName, "setTextColorOn", Core::Skin::ViewAttributes::kTextColorOn);
		source << applyColor (styleItem, styleName, "setTextColorDisabled", Core::Skin::ViewAttributes::kTextColorDisabled);
		source << applyColor (styleItem, styleName, "setHiliteColor", Core::Skin::ViewAttributes::kHiliteColor);

		String fontName = styleItem->getStringAttribute (Core::Skin::ViewAttributes::kFont);
		if(!fontName.isEmpty ())
			source << tab << CoreCode::setFontName (styleName, fontName);

		String textAlign = styleItem->getStringAttribute (Core::Skin::ViewAttributes::kTextAlign);
		if(!textAlign.isEmpty ())
			source << tab << CoreCode::setTextAlign (styleName, textAlign);
	}
	
	source << closeBrace << nl;
	source << nl;

	// *** Font provider method ***
	source << CoreCode::makeGetFontMethod (resourceClassName) << nl;
	source << openBrace << nl;
	for(auto fontItem : iterate_as<SkinEditItem> (fonts))
	{
		MutableCString fontName = fontItem->getElement ().getName ();
		source << tab << "if(" << fontName << ".getName () == name) return &" << fontName << ";" << nl;
	}
	source << tab << "return nullptr;" << nl;
	source << closeBrace << nl;
	source << nl;

	// *** Forms ***
	auto& forms = skinData.getContainerForType (SkinEditData::kForms);
	for(auto formItem : iterate_as<SkinFormItem> (forms))
	{
		String formName = formItem->getDisplayName ();

		header << separatedSection (formName);
		header << nl;
		CString formViewClass = CoreCode::kContainerViewClass;
		if(formItem->getChildren ().isEmpty ())
			formViewClass = CoreCode::kCustomViewClass;

		header << BeginClass (formName, String (formViewClass), ClassSection::kPublic);
		header << ClassSection (ClassSection::kPublic);
		header << tab << DeclareCtor (formName);
		header << nl;
		header << ClassSection (ClassSection::kPrivate);

		struct ViewHelper
		{
			struct Member
			{
				SkinViewItem* viewItem;
				MutableCString className;
				String memberName;
			};

			Vector<Member> members;
			int uniqueMemberCounter = 0;

			String findName (const SkinViewItem* item) const
			{
				for(auto m : members)
					if(m.viewItem == item)
						return m.memberName;
				return String ();
			}

			String declareChildren (SkinViewItem* parentItem)
			{
				String code;
				for(auto viewItem : iterate_as<SkinViewItem> (parentItem->getChildren ()))
				{
					CString className = CoreCode::toStaticClass (viewItem->getClassName ());
					if(className.isEmpty ())
					{
						if(viewItem->getChildren ().isEmpty ())
							className = CoreCode::kCustomViewClass;
						else
							className = CoreCode::kContainerViewClass;
					}

					ValidName name = viewItem->getStringAttribute (Core::Skin::ViewAttributes::kName);
					if(name.isEmpty ())
						name.append ("unnamed");
					name << "_" << uniqueMemberCounter++;

					code << tab << DeclareMember (String (className), name);
					members.add ({viewItem, className, name});

					code << declareChildren (viewItem);
				}
				return code;
			};
		};

		ViewHelper vh;
		header << vh.declareChildren (formItem);
		header << EndClass ();
		header << nl;

		source << separatedSection (formName);
		source << nl;
		source << BeginCtor (formName);
		source << openBrace << nl;
		source << tab << CoreCode::setSize (0, formItem->getSize ());
		//String formStyle = formItem->getStringAttribute (Core::Skin::ViewAttributes::kStyle);
		//if(!formStyle.isEmpty ())
		//	source << tab << CoreCode::setStyle (0, getResourceAddress (formStyle));
		source << nl;		
		
		for(auto m : vh.members)
		{
			source << tab << CoreCode::setSize (m.memberName, m.viewItem->getSize ());
			
			String name = m.viewItem->getStringAttribute (Core::Skin::ViewAttributes::kName);
			if(!name.isEmpty ())
				source << tab << CoreCode::setName (m.memberName, name);

			String styleName = m.viewItem->getStringAttribute (Core::Skin::ViewAttributes::kStyle);
			if(!styleName.isEmpty ())
				source << tab << CoreCode::setStyle (m.memberName, getResourceAddress (styleName));

			if(m.className == CoreCode::kLabelClass || m.className == CoreCode::kButtonClass)
			{
				String title = m.viewItem->getStringAttribute (Core::Skin::ViewAttributes::kTitle);
				source << tab << CoreCode::setTitle (m.memberName, title);
			}
			else if(m.className == CoreCode::kImageViewClass)
			{
				auto imageName = m.viewItem->getStringAttribute (Core::Skin::ViewAttributes::kImage);
				if(!imageName.isEmpty ())
					source << tab << Model::Cpp::CallMethod (m.memberName, "setImage", getResourceAddress (imageName));

				// image view options
				BEGIN_ENUMINFO (imageViewOptionCalls)
					{"isColorize", Core::Skin::kImageViewAppearanceColorize},
				END_ENUMINFO

				String options = m.viewItem->getStringAttribute (Core::Skin::ViewAttributes::kOptions);	
				if(!options.isEmpty ())
					source << CoreCode::makeOptionCalls (m.memberName, options, Core::Skin::Enumerations::imageViewOptions, imageViewOptionCalls, tab);
			}
			else if(m.className == CoreCode::kValueBarClass)
			{
				auto imageName = m.viewItem->getStringAttribute (Core::Skin::ViewAttributes::kImage);
				if(!imageName.isEmpty ())
					source << tab << Model::Cpp::CallMethod (m.memberName, "setImage", getResourceAddress (imageName));

				auto backgroundName = m.viewItem->getStringAttribute (Core::Skin::ViewAttributes::kBackground);
				if(!backgroundName.isEmpty ())
					source << tab << Model::Cpp::CallMethod (m.memberName, "setBackground", getResourceAddress (backgroundName));

				// value bar options
				BEGIN_ENUMINFO (valueBarOptionCalls)
					{"isVertical", Core::Skin::kValueBarAppearanceVertical},
					{"isFilmstrip", Core::Skin::kValueBarAppearanceFilmstrip},
				END_ENUMINFO

				String options = m.viewItem->getStringAttribute (Core::Skin::ViewAttributes::kOptions);	
				if(!options.isEmpty ())
					source << CoreCode::makeOptionCalls (m.memberName, options, Core::Skin::Enumerations::valueBarOptions, valueBarOptionCalls, tab);
			}
			else if(m.className == CoreCode::kListViewClass)
			{
				if(SkinEditItem* styleItem = skinData.findItemByName (SkinEditData::kStyles, MutableCString (styleName)))
				{
					source << applyInt (styleItem, m.memberName, "setRowHeight", Core::Skin::ViewAttributes::kRowHeight);
					source << applyInt (styleItem, m.memberName, "setItemInset", Core::Skin::ViewAttributes::kItemInset);
					source << applyInt (styleItem, m.memberName, "setScrollerSize", Core::Skin::ViewAttributes::kScrollerSize);
					source << applyColor (styleItem, m.memberName, "setSelectColor", Core::Skin::ViewAttributes::kSelectColor);
					source << applyColor (styleItem, m.memberName, "setSeparatorColor", Core::Skin::ViewAttributes::kSeparatorColor);
					// TODO: setFocusBorder()
				}
			}
			
			String parentName = vh.findName (m.viewItem->getParentViewItem ());
			source << tab << CoreCode::addView (parentName, m.memberName);
			source << nl;
		}
		
		source << closeBrace << nl;
	}

	// *** End Header File ***
	header << namespaceBuilder.asCode (false);
	header << nl;
	header << IncludeGuard (includeGuard, false);

	return headerFile->writeString (header) && sourceFile->writeString (source);
}
