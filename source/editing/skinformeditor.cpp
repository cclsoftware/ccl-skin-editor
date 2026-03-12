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
// Filename    : skinformeditor.cpp
// Description : Form Editing Component
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "skinformeditor.h"
#include "skineditfunctions.h"
#include "skiniteminspector.h"
#include "skinformeditview.h"
#include "skinformeditmodel.h"
#include "skinediticons.h"

#include "ccl/app/editing/tools/toolcollection.h"
#include "ccl/app/editing/tools/selecttool.h"
#include "ccl/app/editing/tasks/edittaskcollection.h"
#include "ccl/app/controls/treeviewmodel.h"

#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/graphics/igraphics.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/formatter.h"

namespace CCL {

//************************************************************************************************
// SkinSelectTool
//************************************************************************************************

class SkinSelectTool: public SelectTool
{
public:
	DECLARE_CLASS (SkinSelectTool, SelectTool)

	// SelectTool
	void findActions (ToolActionList& actions, EditView& editView, PointRef where, const GUIEvent& event) override;

protected:
	class ResizeItemAction;
};

//************************************************************************************************
// SkinSelectTool::ResizeItemAction
//************************************************************************************************

class SkinSelectTool::ResizeItemAction: public ToolAction
{
public:
	ResizeItemAction (SkinItemHandle* handle);

	// ToolAction
	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override;

private:
	SharedPtr<SkinItemHandle> handle;
};

//************************************************************************************************
// SkinFormTreeModel
//************************************************************************************************

class SkinFormTreeModel: public TreeViewModel
{
public:
	SkinFormTreeModel (SkinFormEditComponent& component);

	void formChanged ();
	void addTreeNodeForViewItem (TreeViewFolderNode* parentNode, SkinViewItem* viewItem, int index = -1);
	TreeViewNode* findNodeForViewItem (SkinViewItem* viewItem, TreeViewFolderNode** outParentNode = nullptr) const;

	// TreeViewModel
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API canInsertData (CCL::ItemIndexRef index, int column, const CCL::IUnknownList& data, CCL::IDragSession* session, CCL::IView* targetView = nullptr) override;
	tbool CCL_API insertData (CCL::ItemIndexRef index, int column, const CCL::IUnknownList& data, CCL::IDragSession* session) override;

protected:
	SkinFormEditComponent& component;
	AutoPtr<TreeViewFolderNode> rootFolder;
	SharedPtr<IImage> visibilityIcon;
	SharedPtr<IImage> enablementIcon;
	SharedPtr<IImage> childSelectIcon;
};

//************************************************************************************************
// SkinViewItemTreeNode
//************************************************************************************************

class SkinViewItemTreeNode: public TreeViewFolderNode
{
public:
	DECLARE_CLASS_ABSTRACT (SkinViewItemTreeNode, TreeViewFolderNode)

	SkinViewItemTreeNode (SkinViewItem* viewItem);

	PROPERTY_SHARED_AUTO (SkinViewItem, viewItem, ViewItem)

	static SkinViewItem* getViewItem (TreeViewNode* _node)
	{
		auto node = ccl_cast<SkinViewItemTreeNode> (_node);
		return node ? node->getViewItem () : nullptr;
	}

	DECLARE_STRINGID_MEMBER (kHiddenID)
	DECLARE_STRINGID_MEMBER (kDisabledID)
	DECLARE_STRINGID_MEMBER (kChildSelectID)

	// TreeViewFolderNode
	StringID getCustomBackground () const override;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

// XSTRINGS_OFF - hint for xstring tool to skip this section

BEGIN_XSTRINGS ("SkinEditing")
	XSTRING (SelectVariant, "Select Variant")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum SkinFormEditComponentTags
	{
		kShowBoundingBoxes = 100,
		kSnapToGrid,
		kGridSize,
		kZoomFactor
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (SkinFormEditComponent)
	DEFINE_COMMAND ("Select Variant", nullptr, SkinFormEditComponent::onSelectVariant)
END_COMMANDS (SkinFormEditComponent)

//************************************************************************************************
// SkinFormEditComponent
//************************************************************************************************

SkinFormEditComponent::GridSettings::GridSettings ()
: snapActive (false),
  gridSize (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinFormEditComponent::GridSettings::snap (Coord& c) const
{
	if(snapActive && gridSize > 1)
	{
		c /= gridSize;
		c *= gridSize;
	}
}

//************************************************************************************************
// SkinFormEditComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinFormEditComponent, EditorComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinFormEditComponent::SkinFormEditComponent ()
: EditorComponent ("FormEditor"),
  itemInspector (nullptr),
  treeModel (nullptr),
  skinTasks (nullptr),
  formItem (nullptr)
{
	treeModel = NEW SkinFormTreeModel (*this);
	addObject ("treeModel", treeModel);

	getTools ().addTool (NEW SkinSelectTool);

	canPasteOnItem (true); // enable paste in context menu

	// edit tasks
	skinTasks = NEW EditTaskCollection (SkinFormEditModel::kSkinEditCategory);
	skinTasks->collectTasks ();
	addComponent (skinTasks);

	paramList.addParam ("showBoundingBoxes", Tag::kShowBoundingBoxes)->setValue (true);
	paramList.addParam ("snapToGrid", Tag::kSnapToGrid)->setValue (true);
	paramList.addInteger (1, 100, "gridSize", Tag::kGridSize)->setValue (10);
	IParameter* zoomParam = paramList.addInteger (50, 200, "zoomFactor", Tag::kZoomFactor);
	zoomParam->setDefaultValue (100);
	zoomParam->setValue (zoomParam->getDefaultValue ());
	zoomParam->setFormatter (AutoPtr<IFormatter> (NEW Format::PercentInt));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinFormEditComponent::~SkinFormEditComponent ()
{
	setFormItem (nullptr);
	safe_release (treeModel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditModel* SkinFormEditComponent::createModel ()
{
	return NEW SkinFormEditModel (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinFormEditComponent::setFormItem (SkinFormItem* _formItem)
{
	share_and_observe (this, formItem, _formItem);
	treeModel->formChanged ();
	getModel ().getSelection ().unselectAll ();
	updateEditViews ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinFormEditComponent::GridSettings SkinFormEditComponent::getGridSettings () const
{
	GridSettings gs;
	gs.snapActive = paramList.byTag (Tag::kSnapToGrid)->getValue ();
	gs.gridSize = paramList.byTag (Tag::kGridSize)->getValue ();
	return gs;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SkinFormEditComponent::getPaintOptions () const
{
	int options = 0;
	if(paramList.byTag (Tag::kShowBoundingBoxes)->getValue ().asBool ())
		options |= SkinItemPainter::kDrawBoundingBox;
	return options;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float SkinFormEditComponent::getZoomFactor () const
{
	int percent = paramList.byTag (Tag::kZoomFactor)->getValue ();
	return (float)percent / 100.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewItem* SkinFormEditComponent::getSelectedParentItem () const
{
	auto viewItem = SkinViewItemTreeNode::getViewItem (treeModel->getFocusNode ());
	while(viewItem)
	{
		if(viewItem->canHaveChildViews ())
			return viewItem;
		viewItem = viewItem->getParentViewItem (); // on level up should be fine
	}

	return formItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinFormEditComponent::onViewItemFocused (SkinViewItem* viewItem, bool fromEditModel)
{
	static bool insideOnControlFocused = false;
	if(insideOnControlFocused) // avoid reentrance due to TreeView/EditView sync
		return;
	ScopedVar<bool> scope (insideOnControlFocused, true);

	if(itemInspector)
		itemInspector->setSkinItem (viewItem);

	if(viewItem)
	{
		if(fromEditModel == true)
		{
			if(auto node = treeModel->findNodeForViewItem (viewItem))
			{
				// TODO: expand parents as needed...
				treeModel->setFocusNode (node);
			}
		}
		else if(viewItem->isSelectable ())
		{
			auto& selection = getModel ().getSelection ();
			selection.unselectAll ();
			selection.select (viewItem);
			updateEditViews ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinFormEditComponent::onViewItemFlagsEdit (SkinViewItem* changedItem)
{
	// check whole selection, might be a child of changed item
	auto& selection = getModel ().getSelection ();
	IterForEach (getModel ().getSelection ().newIterator<SkinViewItem> (), SkinViewItem, viewItem)
		if(!viewItem->isSelectable ())
			selection.unselect (viewItem);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewItem* SkinFormEditComponent::getFocusViewItem () const
{
	return itemInspector ? ccl_cast<SkinViewItem> (itemInspector->getSkinItem ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API SkinFormEditComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "EditView")
		return *NEW SkinFormEditView (this, bounds);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinFormEditComponent::updateEditViews ()
{
	for(auto editView : iterate_as<EditView> (editViews))
		editView->invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkinFormEditComponent::notify (ISubject* subject, MessageRef msg)
{
	if(msg == SkinFormItem::kViewItemChanged)
	{
		updateEditViews ();

		if(auto treeView = treeModel->getItemView ()) // update custom backgrounds
			ViewBox (treeView).invalidate ();
	}
	else if(msg == SkinFormItem::kViewItemAdded || msg == SkinFormItem::kViewItemRemoved)
	{
		SkinViewItem* viewItem = unknown_cast<SkinViewItem> (msg[0].asUnknown ());
		ASSERT (viewItem)

		if(msg == SkinFormItem::kViewItemRemoved)
		{
			// remove from tree view
			TreeViewFolderNode* parentNode = nullptr;
			if(auto node = treeModel->findNodeForViewItem (viewItem, &parentNode))
			{
				parentNode->remove (node);
				treeModel->refreshNode (parentNode);
				node->release ();
			}

			// make sure to remove from selection
			auto& selection = getModel ().getSelection ();
			if(selection.isSelected (viewItem))
				selection.unselect (viewItem);

			// remove from inspector
			if(itemInspector && itemInspector->getSkinItem () == viewItem)
				itemInspector->setSkinItem (nullptr);
		}
		else // SkinFormItem::kViewItemRemoved
		{
			int index = msg[1].asInt ();

			// add to tree view
			auto parentNode = ccl_cast<TreeViewFolderNode> (treeModel->findNodeForViewItem (viewItem->getParentViewItem ()));
			ASSERT (parentNode)
			if(parentNode)
			{
				treeModel->addTreeNodeForViewItem (parentNode, viewItem, index);
				treeModel->refreshNode (parentNode);
			}
		}

		updateEditViews ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinFormEditComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kShowBoundingBoxes :
		updateEditViews ();
		break;

	case Tag::kZoomFactor :
		// apply zoom to painter
		for(auto editView : iterate_as<EditView> (editViews))
			if(auto formEditView = ccl_cast<SkinFormEditView> (editView))
				formEditView->getPainter ().setZoomFactor (getZoomFactor ());

		updateEditViews ();
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkinFormEditComponent::appendContextMenu (IContextMenu& contextMenu)
{
	auto focuItem = unknown_cast<SkinViewItem> (contextMenu.getFocusItem ());

	// variant selection
	if(focuItem)
	{
		UnknownPtr<IMenu> popupMenu (&contextMenu);
		if(focuItem->isVariantOrTabView () && !focuItem->getChildren ().isEmpty () && popupMenu)
		{
			IMenu* subMenu = popupMenu->createMenu ();
			ASSERT (subMenu)
			subMenu->setMenuAttribute (IMenu::kMenuTitle, XSTR (SelectVariant));

			int index = 0;
			for(auto childItem : iterate_as<SkinViewItem> (focuItem->getChildren ()))
			{
				subMenu->addCommandItem (String () << index+1 << ": " << childItem->getDisplayName (),
										 "Select Variant", MutableCString ().appendInteger (index), this);
				index++;
			}

			popupMenu->addMenu (subMenu);
			contextMenu.addSeparatorItem ();
		}
	}

	// cut/copy/paste
	SuperClass::appendContextMenu (contextMenu);

	// edit tasks
	if(focuItem)
	{
		contextMenu.addSeparatorItem ();
		skinTasks->appendContextMenu (contextMenu);
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_COMMANDS (SkinFormEditComponent, EditorComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinFormEditComponent::onSelectVariant (CmdArgs args)
{
	auto viewItem = getFocusViewItem ();
	if(!viewItem)
		return false;
	if(!viewItem->isVariantOrTabView ())
		return false;

	int64 index = 0;
	args.name.getIntValue (index);

	if(args.checkOnly ())
	{
		if(index == viewItem->getSelectedChildIndex ())
			if(UnknownPtr<IMenuItem> menuItem = args.invoker)
				menuItem->setItemAttribute (IMenuItem::kItemChecked, true);
	}
	else
	{
		viewItem->setSelectedChildIndex ((int)index);
		viewItem->flagsChanged ();
		onViewItemFlagsEdit (viewItem);
	}
	return true;
}

//************************************************************************************************
// SkinSelectTool
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkinSelectTool, SelectTool)

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinSelectTool::findActions (ToolActionList& actions, EditView& editView, PointRef where, const GUIEvent& event)
{
	AutoPtr<Object> item = editView.getModel ().findItemDeep (editView, where);
	if(auto handle = ccl_cast<SkinItemHandle> (item))
	{
		actions.addAction (NEW ResizeItemAction (handle), item, ToolAction::kClick);
		return;
	}
	SuperClass::findActions (actions, editView, where, event);
}

//************************************************************************************************
// SkinSelectTool::ResizeItemAction
//************************************************************************************************

SkinSelectTool::ResizeItemAction::ResizeItemAction (SkinItemHandle* handle)
: handle (handle)
{
	if(handle->getAlignment ().getAlignH () != 0) // left or right
	{
		if(handle->getAlignment ().getAlignV () == 0) // single direction?
			setThemeCursor (ThemeElements::kSizeHorizontalCursor);
		else
			setThemeCursor (ThemeElements::kPointhandCursor);
	}
	else // top or bottom
	{
		if(handle->getAlignment ().getAlignH () == 0) // single direction?
			setThemeCursor (ThemeElements::kSizeVerticalCursor);
		else
			setThemeCursor (ThemeElements::kPointhandCursor);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* SkinSelectTool::ResizeItemAction::perform (EditView& editView, const GUIEvent& event, PointRef where)
{
	if(auto model = ccl_cast<SkinFormEditModel> (&editView.getModel ()))
		return model->resizeItems (editView, handle);
	else
		return nullptr;
}

//************************************************************************************************
// SkinFormTreeModel
//************************************************************************************************

SkinFormTreeModel::SkinFormTreeModel (SkinFormEditComponent& component)
: component (component),
  rootFolder (NEW TreeViewFolderNode)
{
	setRootNode (rootFolder);

	auto& columns = getColumns ();
	columns.addColumn (100, 0, kTitleID); // first column is managed by TreeView
	columns.addColumn (20, 0, SkinViewItemTreeNode::kHiddenID);
	columns.addColumn (20, 0, SkinViewItemTreeNode::kDisabledID);
	columns.addColumn (20, 0, SkinViewItemTreeNode::kChildSelectID);

	visibilityIcon = SkinEditIcons::getKnownIcon (SkinEditIcons::kVisibility);
	enablementIcon = SkinEditIcons::getKnownIcon (SkinEditIcons::kEnablement);
	childSelectIcon = SkinEditIcons::getKnownIcon (SkinEditIcons::kChildSelect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinFormTreeModel::formChanged ()
{
	rootFolder->removeAll ();
	auto formItem = component.getFormItem ();
	if(formItem)
		addTreeNodeForViewItem (rootFolder, formItem);
	refreshNode (rootFolder);

	// expand form
	if(!rootFolder->getContent ().isEmpty ())
		expandNode (static_cast<TreeViewNode*> (rootFolder->getContent ().first ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinFormTreeModel::addTreeNodeForViewItem (TreeViewFolderNode* parentNode, SkinViewItem* viewItem, int index)
{
	struct Helper
	{
		static void addRecursive (TreeViewFolderNode* parentNode, SkinViewItem* viewItem, int index)
		{
			auto subNode = NEW SkinViewItemTreeNode (viewItem);
			if(index != -1)
			{
				if(!parentNode->insertAt (index, subNode))
					parentNode->add (subNode);
			}
			else
				parentNode->add (subNode);

			for(auto subItem : iterate_as<SkinViewItem> (viewItem->getChildren ()))
				addRecursive (subNode, subItem, -1);
		}
	};

	Helper::addRecursive (parentNode, viewItem, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeViewNode* SkinFormTreeModel::findNodeForViewItem (SkinViewItem* viewItem, TreeViewFolderNode** outParentNode) const
{
	struct Helper
	{
		static TreeViewNode* find (TreeViewFolderNode& parentNode, SkinViewItem* viewItem, TreeViewFolderNode** outParentNode)
		{
			for(auto node : iterate_as<TreeViewNode> (parentNode.getContent ()))
			{
				if(SkinViewItemTreeNode::getViewItem (node) == viewItem)
				{
					if(outParentNode)
						*outParentNode = &parentNode;
					return node;
				}

				if(auto folderNode = ccl_cast<TreeViewFolderNode> (node))
					if(auto result = find (*folderNode, viewItem, outParentNode))
						return result;
			}
			return nullptr;
		}
	};

	ASSERT (viewItem != nullptr)
	if(viewItem == nullptr)
		return nullptr;
	return Helper::find (*rootFolder, viewItem, outParentNode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinFormTreeModel::onItemFocused (ItemIndexRef index)
{
	auto focusNode = resolveNode (index);
	auto viewItem = focusNode ? SkinViewItemTreeNode::getViewItem (focusNode) : nullptr;
	component.onViewItemFocused (viewItem, false);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinFormTreeModel::appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection)
{
	if(auto node = resolveNode (item))
		if(auto viewItem = SkinViewItemTreeNode::getViewItem (node))
		{
			menu.setFocusItem (viewItem->asUnknown ());
			component.appendContextMenu (menu);
			return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinFormTreeModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	StringID id = getColumnID (column);
	if(id == SkinViewItemTreeNode::kHiddenID || id == SkinViewItemTreeNode::kDisabledID || id == SkinViewItemTreeNode::kChildSelectID)
	{
		auto viewItem = SkinViewItemTreeNode::getViewItem (resolveNode (index));
		if(viewItem && !viewItem->canCast (ccl_typeid<SkinFormItem> ()))
		{
			// toggle state
			if(id == SkinViewItemTreeNode::kHiddenID)
				viewItem->isHidden (!viewItem->isHidden ());
			else if(id == SkinViewItemTreeNode::kDisabledID)
				viewItem->isDisabled (!viewItem->isDisabled ());
			// select child
			else if(id == SkinViewItemTreeNode::kChildSelectID)
			{
				if(auto parentItem = viewItem->getParentViewItem ())
					if(parentItem->isVariantOrTabView ())
						parentItem->setSelectedChildIndex (viewItem->getIndexInParent ());
			}

			viewItem->flagsChanged ();
			component.onViewItemFlagsEdit (viewItem);
		}
		return true;
	}
	return SuperClass::editCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinFormTreeModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	StringID id = getColumnID (column);
	if(id == SkinViewItemTreeNode::kHiddenID || id == SkinViewItemTreeNode::kDisabledID || id == SkinViewItemTreeNode::kChildSelectID)
	{
		auto viewItem = SkinViewItemTreeNode::getViewItem (resolveNode (index));
		if(viewItem && !viewItem->canCast (ccl_typeid<SkinFormItem> ()))
		{
			if(id == SkinViewItemTreeNode::kChildSelectID)
			{
				bool selected = false;
				if(auto parentItem = viewItem->getParentViewItem ())
					if(parentItem->isVariantOrTabView ())
						selected = parentItem->getSelectedChildItem () == viewItem;

				if(selected == true && childSelectIcon)
					drawIcon (info, childSelectIcon);
			}
			else
			{
				bool state = id == SkinViewItemTreeNode::kHiddenID ? viewItem->isHidden () : viewItem->isDisabled ();
				IImage* icon = id == SkinViewItemTreeNode::kHiddenID ? visibilityIcon : enablementIcon;
				if(icon)
				{
					#if 1
					drawIcon (info, icon, !state);
					#else
					drawIcon (info, icon);
					if(state == true) // strikethrough
					{
						Rect r (0, 0, 12, 12);
						r.center (info.rect);
						info.graphics.drawLine (r.getLeftTop (), r.getRightBottom (), Pen (info.style.adaptiveColor, 2.f));
					}
					#endif
				}
			}
		}
		return true;
	}
	else
		return SuperClass::drawCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinFormTreeModel::canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	if(!unknown_cast<SkinViewItemTreeNode> (data.getFirst ()))
		return false;

	if(session)
	{
		UnknownPtr<IItemView> itemView (targetView);
		if(itemView)
		{
			int flags = IItemView::kCanDragOnItem|IItemView::kCanDragBetweenItems|IItemView::kDropInsertsData;
			AutoPtr<IDragHandler> handler = itemView->createDragHandler (flags);
			session->setDragHandler (handler);
			session->setResult (IDragSession::kDropMove);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinFormTreeModel::insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session)
{
	auto targetNode = resolveNode (index);
	auto targetItem = SkinViewItemTreeNode::getViewItem (targetNode);
	int indexInTarget = targetItem ? targetItem->getIndexInParent () : -1;
	while(targetItem)
	{
		if(targetItem->canHaveChildViews ())
			break;
		targetItem = targetItem->getParentViewItem ();
	}
	if(!targetItem)
		return false;

	ASSERT (targetItem->getSkinData ())
	SkinEditFunctions functions (*targetItem->getSkinData ());
	functions.beginMultiple (ActionDescriptions::DragView (IterCountData (data.createIterator ()) == 1));

	ForEachUnknown (data, unk)
		auto viewItem = SkinViewItemTreeNode::getViewItem (unknown_cast<TreeViewNode> (unk));
		if(!viewItem)
			continue;
		if(viewItem->hasChild (targetItem)) // sanity check: can't move into child
			continue;

		auto oldParentItem = viewItem->getParentViewItem ();
		if(!oldParentItem) // looks like it's the form?
			continue;

		// Note: parent could be same as before but index could have changed.
		functions.removeView (oldParentItem, viewItem);
		functions.addView (targetItem, viewItem, indexInTarget);
	EndFor

	functions.endMultiple ();
	return true;
}

//************************************************************************************************
// SkinViewItemTreeNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkinViewItemTreeNode, TreeViewFolderNode)
DEFINE_STRINGID_MEMBER_ (SkinViewItemTreeNode, kHiddenID, "hidden")
DEFINE_STRINGID_MEMBER_ (SkinViewItemTreeNode, kDisabledID, "disabled")
DEFINE_STRINGID_MEMBER_ (SkinViewItemTreeNode, kChildSelectID, "childselect")

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinViewItemTreeNode::SkinViewItemTreeNode (SkinViewItem* viewItem)
: TreeViewFolderNode (viewItem->getDisplayName ())
{
	setViewItem (viewItem);
	setIcon (SkinEditIcons::getViewItemIcon (*viewItem));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID SkinViewItemTreeNode::getCustomBackground () const
{
	if(viewItem->isDisabled () || viewItem->isHidden ())
		return CSTR ("SkinViewItem.Disabled");
	if(!viewItem->isSelectable ())
		return CSTR ("SkinViewItem.ParentDisabled");
	return SuperClass::getCustomBackground ();
}
