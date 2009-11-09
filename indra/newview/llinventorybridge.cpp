/**
 * @file llinventorybridge.cpp
 * @brief Implementation of the Inventory-Folder-View-Bridge classes.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include <utility> // for std::pair<>

#include "llfloaterinventory.h"
#include "llinventorybridge.h"

#include "message.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llcallingcard.h"
#include "llcheckboxctrl.h"		// for radio buttons
#include "llfloaterreg.h"
#include "llradiogroup.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "llui.h"

#include "llviewercontrol.h"
#include "llfirstuse.h"
#include "llfoldertype.h"
#include "llfloaterchat.h"
#include "llfloatercustomize.h"
#include "llfloaterproperties.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "llfolderview.h"
#include "llfriendcard.h"
#include "llavataractions.h"
#include "llgesturemgr.h"
#include "lliconctrl.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "llinventoryclipboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "llscrollcontainer.h"
#include "llimview.h"
#include "lltooldraganddrop.h"
#include "llviewerfoldertype.h"
#include "llviewertexturelist.h"
#include "llviewerinventory.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llwearable.h"
#include "llwearablelist.h"
#include "llviewerassettype.h"
#include "llviewermessage.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "lltabcontainer.h"
#include "lluictrlfactory.h"
#include "llselectmgr.h"
#include "llsidetray.h"
#include "llfloateropenobject.h"
#include "lltrans.h"
#include "llappearancemgr.h"
#include "llimfloater.h"

using namespace LLOldEvents;

// Helpers
// bug in busy count inc/dec right now, logic is complex... do we really need it?
void inc_busy_count()
{
// 	gViewerWindow->getWindow()->incBusyCount();
//  check balance of these calls if this code is changed to ever actually
//  *do* something!
}
void dec_busy_count()
{
// 	gViewerWindow->getWindow()->decBusyCount();
//  check balance of these calls if this code is changed to ever actually
//  *do* something!
}

// Function declarations
void wear_add_inventory_item_on_avatar(LLInventoryItem* item);
void remove_inventory_category_from_avatar(LLInventoryCategory* category);
void remove_inventory_category_from_avatar_step2( BOOL proceed, LLUUID category_id);
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, LLMoveInv*);
bool confirm_replace_attachment_rez(const LLSD& notification, const LLSD& response);

std::string ICON_NAME[ICON_NAME_COUNT] =
{
	"Inv_Texture",
	"Inv_Sound",
	"Inv_CallingCard",
	"Inv_CallingCard",
	"Inv_Landmark",
	"Inv_Landmark",
	"Inv_Script",
	"Inv_Clothing",
	"Inv_Object",
	"Inv_Object",
	"Inv_Notecard",
	"Inv_Skin",
	"Inv_Snapshot",

	"Inv_BodyShape",
	"Inv_Skin",
	"Inv_Hair",
	"Inv_Eye",
	"Inv_Shirt",
	"Inv_Pants",
	"Inv_Shoe",
	"Inv_Socks",
	"Inv_Jacket",
	"Inv_Gloves",
	"Inv_Undershirt",
	"Inv_Underpants",
	"Inv_Skirt",
	"Inv_Alpha",
	"Inv_Tattoo",

	"Inv_Animation",
	"Inv_Gesture",

	"inv_item_linkitem.tga",
	"inv_item_linkfolder.tga"
};


// +=================================================+
// |        LLInventoryPanelObserver                 |
// +=================================================+
void LLInventoryPanelObserver::changed(U32 mask)
{
	mIP->modelChanged(mask);
}


// +=================================================+
// |        LLInvFVBridge                            |
// +=================================================+

LLInvFVBridge::LLInvFVBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
mUUID(uuid), mInvType(LLInventoryType::IT_NONE)
{
	mInventoryPanel = inventory->getHandle();
}

const std::string& LLInvFVBridge::getName() const
{
	LLInventoryObject* obj = getInventoryObject();
	if(obj)
	{
		return obj->getName();
	}
	return LLStringUtil::null;
}

const std::string& LLInvFVBridge::getDisplayName() const
{
	return getName();
}

// Folders have full perms
PermissionMask LLInvFVBridge::getPermissionMask() const
{

	return PERM_ALL;
}

// virtual
LLFolderType::EType LLInvFVBridge::getPreferredType() const
{
	return LLFolderType::FT_NONE;
}


// Folders don't have creation dates.
time_t LLInvFVBridge::getCreationDate() const
{
	return 0;
}

// Can be destoryed (or moved to trash)
BOOL LLInvFVBridge::isItemRemovable()
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	if(model->isObjectDescendentOf(mUUID, gInventory.getRootFolderID()))
	{
		return TRUE;
	}
	return FALSE;
}

// Sends an update to all link items that point to the base item.
void LLInvFVBridge::renameLinkedItems(const LLUUID &item_id, const std::string& new_name)
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;

	LLInventoryItem* itemp = model->getItem(mUUID);
	if (!itemp) return;

	if (itemp->getIsLinkType())
	{
		return;
	}

	LLInventoryModel::item_array_t item_array = model->collectLinkedItems(item_id);
	for (LLInventoryModel::item_array_t::iterator iter = item_array.begin();
		 iter != item_array.end();
		 iter++)
	{
		LLViewerInventoryItem *linked_item = (*iter);
		if (linked_item->getUUID() == item_id) continue;

		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(linked_item);
		new_item->rename(new_name);
		new_item->updateServer(FALSE);
		model->updateItem(new_item);
		// model->addChangedMask(LLInventoryObserver::LABEL, linked_item->getUUID());
	}
	model->notifyObservers();
}

// Can be moved to another folder
BOOL LLInvFVBridge::isItemMovable() const
{
	return TRUE;
}

/*virtual*/
/**
 * @brief Adds this item into clipboard storage
 */
void LLInvFVBridge::cutToClipboard()
{
	if(isItemMovable())
	{
		LLInventoryClipboard::instance().cut(mUUID);
	}
}
// *TODO: make sure this does the right thing
void LLInvFVBridge::showProperties()
{
	LLSD key;
	key["id"] = mUUID;
	LLSideTray::getInstance()->showPanel("sidepanel_inventory", key);

	// Disable old properties floater; this is replaced by the sidepanel.
	/*
	LLFloaterReg::showInstance("properties", mUUID);
	*/
}

void LLInvFVBridge::removeBatch(LLDynamicArray<LLFolderViewEventListener*>& batch)
{
	// Deactivate gestures when moving them into Trash
	LLInvFVBridge* bridge;
	LLInventoryModel* model = getInventoryModel();
	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	S32 count = batch.count();
	S32 i,j;
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if(!bridge || !bridge->isItemRemovable()) continue;
		item = (LLViewerInventoryItem*)model->getItem(bridge->getUUID());
		if (item)
		{
			if(LLAssetType::AT_GESTURE == item->getType())
			{
				LLGestureManager::instance().deactivateGesture(item->getUUID());
			}
		}
	}
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if(!bridge || !bridge->isItemRemovable()) continue;
		cat = (LLViewerInventoryCategory*)model->getCategory(bridge->getUUID());
		if (cat)
		{
			gInventory.collectDescendents( cat->getUUID(), descendent_categories, descendent_items, FALSE );
			for (j=0; j<descendent_items.count(); j++)
			{
				if(LLAssetType::AT_GESTURE == descendent_items[j]->getType())
				{
					LLGestureManager::instance().deactivateGesture(descendent_items[j]->getUUID());
				}
			}
		}
	}
	removeBatchNoCheck(batch);
}

void LLInvFVBridge::removeBatchNoCheck(LLDynamicArray<LLFolderViewEventListener*>& batch)
{
	// this method moves a bunch of items and folders to the trash. As
	// per design guidelines for the inventory model, the message is
	// built and the accounting is performed first. After all of that,
	// we call LLInventoryModel::moveObject() to move everything
	// around.
	LLInvFVBridge* bridge;
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;
	LLMessageSystem* msg = gMessageSystem;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	std::vector<LLUUID> move_ids;
	LLInventoryModel::update_map_t update;
	bool start_new_message = true;
	S32 count = batch.count();
	S32 i;
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if(!bridge || !bridge->isItemRemovable()) continue;
		item = (LLViewerInventoryItem*)model->getItem(bridge->getUUID());
		if(item)
		{
			if(item->getParentUUID() == trash_id) continue;
			move_ids.push_back(item->getUUID());
			LLPreview::hide(item->getUUID());
			--update[item->getParentUUID()];
			++update[trash_id];
			if(start_new_message)
			{
				start_new_message = false;
				msg->newMessageFast(_PREHASH_MoveInventoryItem);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addBOOLFast(_PREHASH_Stamp, TRUE);
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_ItemID, item->getUUID());
			msg->addUUIDFast(_PREHASH_FolderID, trash_id);
			msg->addString("NewName", NULL);
			if(msg->isSendFullFast(_PREHASH_InventoryData))
			{
				start_new_message = true;
				gAgent.sendReliableMessage();
				gInventory.accountForUpdate(update);
				update.clear();
			}
		}
	}
	if(!start_new_message)
	{
		start_new_message = true;
		gAgent.sendReliableMessage();
		gInventory.accountForUpdate(update);
		update.clear();
	}
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if(!bridge || !bridge->isItemRemovable()) continue;
		cat = (LLViewerInventoryCategory*)model->getCategory(bridge->getUUID());
		if(cat)
		{
			if(cat->getParentUUID() == trash_id) continue;
			move_ids.push_back(cat->getUUID());
			--update[cat->getParentUUID()];
			++update[trash_id];
			if(start_new_message)
			{
				start_new_message = false;
				msg->newMessageFast(_PREHASH_MoveInventoryFolder);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addBOOL("Stamp", TRUE);
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_FolderID, cat->getUUID());
			msg->addUUIDFast(_PREHASH_ParentID, trash_id);
			if(msg->isSendFullFast(_PREHASH_InventoryData))
			{
				start_new_message = true;
				gAgent.sendReliableMessage();
				gInventory.accountForUpdate(update);
				update.clear();
			}
		}
	}
	if(!start_new_message)
	{
		gAgent.sendReliableMessage();
		gInventory.accountForUpdate(update);
	}

	// move everything.
	std::vector<LLUUID>::iterator it = move_ids.begin();
	std::vector<LLUUID>::iterator end = move_ids.end();
	for(; it != end; ++it)
	{
		gInventory.moveObject((*it), trash_id);
	}

	// notify inventory observers.
	model->notifyObservers();
}

BOOL LLInvFVBridge::isClipboardPasteable() const
{
	if (!LLInventoryClipboard::instance().hasContents() || !isAgentInventory())
	{
		return FALSE;
	}
	LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return FALSE;
	}

	const LLUUID &agent_id = gAgent.getID();

	LLDynamicArray<LLUUID> objects;
	LLInventoryClipboard::instance().retrieve(objects);
	S32 count = objects.count();
	for(S32 i = 0; i < count; i++)
	{
		const LLUUID &item_id = objects.get(i);

		// Can't paste folders
		const LLInventoryCategory *cat = model->getCategory(item_id);
		if (cat)
		{
			return FALSE;
		}

		const LLInventoryItem *item = model->getItem(item_id);
		if (item)
		{
			if (!item->getPermissions().allowCopyBy(agent_id))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

BOOL LLInvFVBridge::isClipboardPasteableAsLink() const
{
	if (!LLInventoryClipboard::instance().hasContents() || !isAgentInventory())
	{
		return FALSE;
	}
	const LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return FALSE;
	}

	LLDynamicArray<LLUUID> objects;
	LLInventoryClipboard::instance().retrieve(objects);
	S32 count = objects.count();
	for(S32 i = 0; i < count; i++)
	{
		const LLInventoryItem *item = model->getItem(objects.get(i));
		if (item)
		{
			if (!LLAssetType::lookupCanLink(item->getActualType()))
			{
				return FALSE;
			}
		}
		const LLViewerInventoryCategory *cat = model->getCategory(objects.get(i));
		if (cat && !LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
		{
			return FALSE;
		}
	}
	return TRUE;
}

void hide_context_entries(LLMenuGL& menu, 
						const std::vector<std::string> &entries_to_show,
						const std::vector<std::string> &disabled_entries)
{
	const LLView::child_list_t *list = menu.getChildList();

	LLView::child_list_t::const_iterator itor;
	for (itor = list->begin(); itor != list->end(); ++itor)
	{
		std::string name = (*itor)->getName();

		// descend into split menus:
		LLMenuItemBranchGL* branchp = dynamic_cast<LLMenuItemBranchGL*>(*itor);
		if ((name == "More") && branchp)
		{
			hide_context_entries(*branchp->getBranch(), entries_to_show, disabled_entries);
		}


		bool found = false;
		std::vector<std::string>::const_iterator itor2;
		for (itor2 = entries_to_show.begin(); itor2 != entries_to_show.end(); ++itor2)
		{
			if (*itor2 == name)
			{
				found = true;
			}
		}
		if (!found)
		{
			(*itor)->setVisible(FALSE);
		}
		else
		{
			for (itor2 = disabled_entries.begin(); itor2 != disabled_entries.end(); ++itor2)
			{
				if (*itor2 == name)
				{
					(*itor)->setEnabled(FALSE);
				}
			}
		}
	}
}

// Helper for commonly-used entries
void LLInvFVBridge::getClipboardEntries(bool show_asset_id,
										std::vector<std::string> &items,
										std::vector<std::string> &disabled_items, U32 flags)
{
	items.push_back(std::string("Rename"));
	if (!isItemRenameable() || (flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Rename"));
	}

	if (show_asset_id)
	{
		items.push_back(std::string("Copy Asset UUID"));
		if ( (! ( isItemPermissive() || gAgent.isGodlike() ) )
			  || (flags & FIRST_SELECTED_ITEM) == 0)
		{
			disabled_items.push_back(std::string("Copy Asset UUID"));
		}
	}

	items.push_back(std::string("Copy Separator"));

	items.push_back(std::string("Copy"));
	if (!isItemCopyable())
	{
		disabled_items.push_back(std::string("Copy"));
	}

	items.push_back(std::string("Paste"));
	if (!isClipboardPasteable() || (flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Paste"));
	}

	items.push_back(std::string("Paste As Link"));
	if (!isClipboardPasteableAsLink() || (flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Paste As Link"));
	}
	items.push_back(std::string("Paste Separator"));

	items.push_back(std::string("Delete"));
	if (!isItemRemovable())
	{
		disabled_items.push_back(std::string("Delete"));
	}

	// If multiple items are selected, disable properties (if it exists).
	if ((flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Properties"));
	}
}

void LLInvFVBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLInvFVBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if(isInTrash())
	{
		items.push_back(std::string("PurgeItem"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("PurgeItem"));
		}
		items.push_back(std::string("RestoreItem"));
	}
	else
	{
		items.push_back(std::string("Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}
	hide_context_entries(menu, items, disabled_items);
}

// *TODO: remove this
BOOL LLInvFVBridge::startDrag(EDragAndDropType* type, LLUUID* id) const
{
	BOOL rv = FALSE;

	const LLInventoryObject* obj = getInventoryObject();

	if(obj)
	{
		*type = LLViewerAssetType::lookupDragAndDropType(obj->getActualType());
		if(*type == DAD_NONE)
		{
			return FALSE;
		}

		*id = obj->getUUID();
		//object_ids.put(obj->getUUID());

		if (*type == DAD_CATEGORY)
		{
			gInventory.startBackgroundFetch(obj->getUUID());
		}

		rv = TRUE;
	}

	return rv;
}

LLInventoryObject* LLInvFVBridge::getInventoryObject() const
{
	LLInventoryObject* obj = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		obj = (LLInventoryObject*)model->getObject(mUUID);
	}
	return obj;
}

LLInventoryModel* LLInvFVBridge::getInventoryModel() const
{
	LLInventoryPanel* panel = dynamic_cast<LLInventoryPanel*>(mInventoryPanel.get());
	return panel ? panel->getModel() : NULL;
}

BOOL LLInvFVBridge::isInTrash() const
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	return model->isObjectDescendentOf(mUUID, trash_id);
}

BOOL LLInvFVBridge::isLinkedObjectInTrash() const
{
	if (isInTrash()) return TRUE;

	const LLInventoryObject *obj = getInventoryObject();
	if (obj && obj->getIsLinkType())
	{
		LLInventoryModel* model = getInventoryModel();
		if(!model) return FALSE;
		const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
		return model->isObjectDescendentOf(obj->getLinkedUUID(), trash_id);
	}
	return FALSE;
}

BOOL LLInvFVBridge::isAgentInventory() const
{
	const LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	if(gInventory.getRootFolderID() == mUUID) return TRUE;
	return model->isObjectDescendentOf(mUUID, gInventory.getRootFolderID());
}

BOOL LLInvFVBridge::isCOFFolder() const
{
	const LLInventoryModel* model = getInventoryModel();
	if(!model) return TRUE;
	const LLUUID cof_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
	if (mUUID == cof_id || model->isObjectDescendentOf(mUUID, cof_id))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL LLInvFVBridge::isItemPermissive() const
{
	return FALSE;
}

// static
void LLInvFVBridge::changeItemParent(LLInventoryModel* model,
									 LLViewerInventoryItem* item,
									 const LLUUID& new_parent,
									 BOOL restamp)
{
	if(item->getParentUUID() != new_parent)
	{
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(item->getParentUUID(),-1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(new_parent, 1);
		update.push_back(new_folder);
		gInventory.accountForUpdate(update);

		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setParent(new_parent);
		new_item->updateParentOnServer(restamp);
		model->updateItem(new_item);
		model->notifyObservers();
	}
}

// static
void LLInvFVBridge::changeCategoryParent(LLInventoryModel* model,
										 LLViewerInventoryCategory* cat,
										 const LLUUID& new_parent,
										 BOOL restamp)
{
	if(cat->getParentUUID() != new_parent)
	{
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(cat->getParentUUID(), -1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(new_parent, 1);
		update.push_back(new_folder);
		gInventory.accountForUpdate(update);

		LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat);
		new_cat->setParent(new_parent);
		new_cat->updateParentOnServer(restamp);
		model->updateCategory(new_cat);
		model->notifyObservers();
	}
}


const std::string safe_inv_type_lookup(LLInventoryType::EType inv_type)
{
	const std::string rv= LLInventoryType::lookup(inv_type);
	if(rv.empty())
	{
		return std::string("<invalid>");
	}
	return rv;
}

LLInvFVBridge* LLInvFVBridge::createBridge(LLAssetType::EType asset_type,
										   LLAssetType::EType actual_asset_type,
										   LLInventoryType::EType inv_type,
										   LLInventoryPanel* inventory,
										   const LLUUID& uuid,
										   U32 flags)
{
	LLInvFVBridge* new_listener = NULL;
	switch(asset_type)
	{
		case LLAssetType::AT_TEXTURE:
			if(!(inv_type == LLInventoryType::IT_TEXTURE || inv_type == LLInventoryType::IT_SNAPSHOT))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLTextureBridge(inventory, uuid, inv_type);
			break;

		case LLAssetType::AT_SOUND:
			if(!(inv_type == LLInventoryType::IT_SOUND))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLSoundBridge(inventory, uuid);
			break;

		case LLAssetType::AT_LANDMARK:
			if(!(inv_type == LLInventoryType::IT_LANDMARK))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLLandmarkBridge(inventory, uuid, flags);
			break;

		case LLAssetType::AT_CALLINGCARD:
			if(!(inv_type == LLInventoryType::IT_CALLINGCARD))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLCallingCardBridge(inventory, uuid);
			break;

		case LLAssetType::AT_SCRIPT:
			if(!(inv_type == LLInventoryType::IT_LSL))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLScriptBridge(inventory, uuid);
			break;

		case LLAssetType::AT_OBJECT:
			if(!(inv_type == LLInventoryType::IT_OBJECT || inv_type == LLInventoryType::IT_ATTACHMENT))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLObjectBridge(inventory, uuid, inv_type, flags);
			break;

		case LLAssetType::AT_NOTECARD:
			if(!(inv_type == LLInventoryType::IT_NOTECARD))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLNotecardBridge(inventory, uuid);
			break;

		case LLAssetType::AT_ANIMATION:
			if(!(inv_type == LLInventoryType::IT_ANIMATION))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLAnimationBridge(inventory, uuid);
			break;

		case LLAssetType::AT_GESTURE:
			if(!(inv_type == LLInventoryType::IT_GESTURE))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLGestureBridge(inventory, uuid);
			break;

		case LLAssetType::AT_LSL_TEXT:
			if(!(inv_type == LLInventoryType::IT_LSL))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLLSLTextBridge(inventory, uuid);
			break;

		case LLAssetType::AT_CLOTHING:
		case LLAssetType::AT_BODYPART:
			if(!(inv_type == LLInventoryType::IT_WEARABLE))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLWearableBridge(inventory, uuid, asset_type, inv_type, (EWearableType)flags);
			break;
		case LLAssetType::AT_CATEGORY:
		case LLAssetType::AT_ROOT_CATEGORY:
			if (actual_asset_type == LLAssetType::AT_LINK_FOLDER)
			{
				// Create a link folder handler instead.
				new_listener = new LLLinkFolderBridge(inventory, uuid);
				break;
			}
			new_listener = new LLFolderBridge(inventory, uuid);
			break;
		case LLAssetType::AT_LINK:
			// Only should happen for broken links.
			new_listener = new LLLinkItemBridge(inventory, uuid);
			break;
		case LLAssetType::AT_LINK_FOLDER:
			// Only should happen for broken links.
			new_listener = new LLLinkItemBridge(inventory, uuid);
			break;
		default:
			llinfos << "Unhandled asset type (llassetstorage.h): "
					<< (S32)asset_type << llendl;
			break;
	}

	if (new_listener)
	{
		new_listener->mInvType = inv_type;
	}

	return new_listener;
}

void LLInvFVBridge::purgeItem(LLInventoryModel *model, const LLUUID &uuid)
{
	LLInventoryCategory* cat = model->getCategory(uuid);
	if (cat)
	{
		model->purgeDescendentsOf(uuid);
		model->notifyObservers();
	}
	LLInventoryObject* obj = model->getObject(uuid);
	if (obj)
	{
		model->purgeObject(uuid);
		model->notifyObservers();
	}
}

// +=================================================+
// |        InventoryFVBridgeBuilder                 |
// +=================================================+
LLInvFVBridge* LLInventoryFVBridgeBuilder::createBridge(LLAssetType::EType asset_type,
														LLAssetType::EType actual_asset_type,
														LLInventoryType::EType inv_type,
														LLInventoryPanel* inventory,
														const LLUUID& uuid,
														U32 flags /* = 0x00 */) const
{
	return LLInvFVBridge::createBridge(asset_type,
		actual_asset_type,
		inv_type,
		inventory,
		uuid,
		flags);
}

// +=================================================+
// |        LLItemBridge                             |
// +=================================================+

void LLItemBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("goto" == action)
	{
		gotoItem(folder);
	}
	if ("open" == action)
	{
		openItem();
		return;
	}
	else if ("properties" == action)
	{
		showProperties();
		return;
	}
	else if ("purge" == action)
	{
		purgeItem(model, mUUID);
		return;
	}
	else if ("restoreToWorld" == action)
	{
		restoreToWorld();
		return;
	}
	else if ("restore" == action)
	{
		restoreItem();
		return;
	}
	else if ("copy_uuid" == action)
	{
		// Single item only
		LLInventoryItem* item = model->getItem(mUUID);
		if(!item) return;
		LLUUID asset_id = item->getAssetUUID();
		std::string buffer;
		asset_id.toString(buffer);

		gViewerWindow->mWindow->copyTextToClipboard(utf8str_to_wstring(buffer));
		return;
	}
	else if ("copy" == action)
	{
		copyToClipboard();
		return;
	}
	else if ("paste" == action)
	{
		// Single item only
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;

		LLFolderViewItem* folder_view_itemp = folder->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;

		folder_view_itemp->getListener()->pasteFromClipboard();
		return;
	}
	else if ("paste_link" == action)
	{
		// Single item only
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;

		LLFolderViewItem* folder_view_itemp = folder->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;

		folder_view_itemp->getListener()->pasteLinkFromClipboard();
		return;
	}
}

void LLItemBridge::selectItem()
{
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)getItem();
	if(item && !item->isComplete())
	{
		item->fetchFromServer();
	}
}

void LLItemBridge::restoreItem()
{
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)getItem();
	if(item)
	{
		LLInventoryModel* model = getInventoryModel();
		const LLUUID new_parent = model->findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(item->getType()));
		// do not restamp on restore.
		LLInvFVBridge::changeItemParent(model, item, new_parent, FALSE);
	}
}

void LLItemBridge::restoreToWorld()
{
	LLViewerInventoryItem* itemp = (LLViewerInventoryItem*)getItem();
	if (itemp)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("RezRestoreToWorld");
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

		msg->nextBlockFast(_PREHASH_InventoryData);
		itemp->packMessage(msg);
		msg->sendReliable(gAgent.getRegion()->getHost());
	}

	//Similar functionality to the drag and drop rez logic
	BOOL remove_from_inventory = FALSE;

	//remove local inventory copy, sim will deal with permissions and removing the item
	//from the actual inventory if its a no-copy etc
	if(!itemp->getPermissions().allowCopyBy(gAgent.getID()))
	{
		remove_from_inventory = TRUE;
	}

	// Check if it's in the trash. (again similar to the normal rez logic)
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if(gInventory.isObjectDescendentOf(itemp->getUUID(), trash_id))
	{
		remove_from_inventory = TRUE;
	}

	if(remove_from_inventory)
	{
		gInventory.deleteObject(itemp->getUUID());
		gInventory.notifyObservers();
	}
}

void LLItemBridge::gotoItem(LLFolderView *folder)
{
	LLInventoryObject *obj = getInventoryObject();
	if (obj && obj->getIsLinkType())
	{
		LLInventoryPanel* active_panel = LLFloaterInventory::getActiveInventory()->getPanel();
		if (active_panel)
		{
			active_panel->setSelection(obj->getLinkedUUID(), TAKE_FOCUS_NO);
		}
	}
}

LLUIImagePtr LLItemBridge::getIcon() const
{
	return LLUI::getUIImage(ICON_NAME[OBJECT_ICON_NAME]);
}

PermissionMask LLItemBridge::getPermissionMask() const
{
	LLViewerInventoryItem* item = getItem();
	PermissionMask perm_mask = 0;
	if(item)
	{
		BOOL copy = item->getPermissions().allowCopyBy(gAgent.getID());
		BOOL mod = item->getPermissions().allowModifyBy(gAgent.getID());
		BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
															gAgent.getID());

		if (copy) perm_mask |= PERM_COPY;
		if (mod)  perm_mask |= PERM_MODIFY;
		if (xfer) perm_mask |= PERM_TRANSFER;

	}
	return perm_mask;
}

const std::string& LLItemBridge::getDisplayName() const
{
	if(mDisplayName.empty())
	{
		buildDisplayName(getItem(), mDisplayName);
	}
	return mDisplayName;
}

void LLItemBridge::buildDisplayName(LLInventoryItem* item, std::string& name)
{
	if(item)
	{
		name.assign(item->getName());
	}
	else
	{
		name.assign(LLStringUtil::null);
	}
}

LLFontGL::StyleFlags LLItemBridge::getLabelStyle() const
{
	U8 font = LLFontGL::NORMAL;

	if( gAgentWearables.isWearingItem( mUUID ) )
	{
		// llinfos << "BOLD" << llendl;
		font |= LLFontGL::BOLD;
	}

	const LLViewerInventoryItem* item = getItem();
	if (item && item->getIsLinkType())
	{
		font |= LLFontGL::ITALIC;
	}
	return (LLFontGL::StyleFlags)font;
}

std::string LLItemBridge::getLabelSuffix() const
{
	// String table is loaded before login screen and inventory items are
	// loaded after login, so LLTrans should be ready.
	static std::string NO_COPY =LLTrans::getString("no_copy");
	static std::string NO_MOD = LLTrans::getString("no_modify");
	static std::string NO_XFER = LLTrans::getString("no_transfer");
	static std::string LINK = LLTrans::getString("link");
	static std::string BROKEN_LINK = LLTrans::getString("broken_link");
	std::string suffix;
	LLInventoryItem* item = getItem();
	if(item)
	{
		// it's a bit confusing to put nocopy/nomod/etc on calling cards.
		if(LLAssetType::AT_CALLINGCARD != item->getType()
		   && item->getPermissions().getOwner() == gAgent.getID())
		{
			BOOL broken_link = LLAssetType::lookupIsLinkType(item->getType());
			if (broken_link) return BROKEN_LINK;

			BOOL link = item->getIsLinkType();
			if (link) return LINK;

			BOOL copy = item->getPermissions().allowCopyBy(gAgent.getID());
			if (!copy)
			{
				suffix += NO_COPY;
			}
			BOOL mod = item->getPermissions().allowModifyBy(gAgent.getID());
			if (!mod)
			{
				suffix += NO_MOD;
			}
			BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
																gAgent.getID());
			if (!xfer)
			{
				suffix += NO_XFER;
			}
		}
	}
	return suffix;
}

time_t LLItemBridge::getCreationDate() const
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		return item->getCreationDate();
	}
	return 0;
}


BOOL LLItemBridge::isItemRenameable() const
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		// (For now) Don't allow calling card rename since that may confuse users as to
		// what the calling card points to.
		if (item->getInventoryType() == LLInventoryType::IT_CALLINGCARD)
		{
			return FALSE;
		}
		return (item->getPermissions().allowModifyBy(gAgent.getID()));
	}
	return FALSE;
}

BOOL LLItemBridge::renameItem(const std::string& new_name)
{
	if(!isItemRenameable())
		return FALSE;
	LLPreview::dirty(mUUID);
	LLInventoryModel* model = getInventoryModel();
	if(!model)
		return FALSE;
	LLViewerInventoryItem* item = getItem();
	if(item && (item->getName() != new_name))
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->rename(new_name);
		buildDisplayName(new_item, mDisplayName);
		new_item->updateServer(FALSE);
		model->updateItem(new_item);

		model->notifyObservers();
	}
	// return FALSE because we either notified observers (& therefore
	// rebuilt) or we didn't update.
	return FALSE;
}


BOOL LLItemBridge::removeItem()
{
	if(!isItemRemovable())
	{
		return FALSE;
	}
	// move it to the trash
	LLPreview::hide(mUUID, TRUE);
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	LLViewerInventoryItem* item = getItem();

	// if item is not already in trash
	if(item && !model->isObjectDescendentOf(mUUID, trash_id))
	{
		// move to trash, and restamp
		LLInvFVBridge::changeItemParent(model, item, trash_id, TRUE);
		// delete was successful
		return TRUE;
	}
	else
	{
		// tried to delete already item in trash (should purge?)
		return FALSE;
	}
}

BOOL LLItemBridge::isItemCopyable() const
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		// can't copy worn objects. DEV-15183
		LLVOAvatarSelf *avatarp = gAgent.getAvatarObject();
		if( !avatarp )
		{
			return FALSE;
		}

		if(avatarp->isWearingAttachment(mUUID))
		{
			return FALSE;
		}

		// All items can be copied, not all can be pasted.
		// The only time an item can't be copied is if it's a link
		// return (item->getPermissions().allowCopyBy(gAgent.getID()));
		if (item->getIsLinkType())
		{
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
BOOL LLItemBridge::copyToClipboard() const
{
	if(isItemCopyable())
	{
		LLInventoryClipboard::instance().add(mUUID);
		return TRUE;
	}
	return FALSE;
}

LLViewerInventoryItem* LLItemBridge::getItem() const
{
	LLViewerInventoryItem* item = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		item = (LLViewerInventoryItem*)model->getItem(mUUID);
	}
	return item;
}

BOOL LLItemBridge::isItemPermissive() const
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		U32 mask = item->getPermissions().getMaskBase();
		if((mask & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED)
		{
			return TRUE;
		}
	}
	return FALSE;
}

// +=================================================+
// |        LLFolderBridge                           |
// +=================================================+

LLFolderBridge* LLFolderBridge::sSelf=NULL;

// Can be moved to another folder
BOOL LLFolderBridge::isItemMovable() const
{
	LLInventoryObject* obj = getInventoryObject();
	if(obj)
	{
		return (!LLFolderType::lookupIsProtectedType(((LLInventoryCategory*)obj)->getPreferredType()));
	}
	return FALSE;
}

void LLFolderBridge::selectItem()
{
}


// Can be destroyed (or moved to trash)
BOOL LLFolderBridge::isItemRemovable()
{
	LLInventoryModel* model = getInventoryModel();
	if(!model)
	{
		return FALSE;
	}

	if(!model->isObjectDescendentOf(mUUID, gInventory.getRootFolderID()))
	{
		return FALSE;
	}

	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	if( !avatar )
	{
		return FALSE;
	}

	LLInventoryCategory* category = model->getCategory(mUUID);
	if( !category )
	{
		return FALSE;
	}

	if(LLFolderType::lookupIsProtectedType(category->getPreferredType()))
	{
		return FALSE;
	}

	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	gInventory.collectDescendents( mUUID, descendent_categories, descendent_items, FALSE );

	S32 i;
	for( i = 0; i < descendent_categories.count(); i++ )
	{
		LLInventoryCategory* category = descendent_categories[i];
		if(LLFolderType::lookupIsProtectedType(category->getPreferredType()))
		{
			return FALSE;
		}
	}

	for( i = 0; i < descendent_items.count(); i++ )
	{
		LLInventoryItem* item = descendent_items[i];
		if( (item->getType() == LLAssetType::AT_CLOTHING) ||
			(item->getType() == LLAssetType::AT_BODYPART) )
		{
			if(gAgentWearables.isWearingItem(item->getUUID()))
			{
				return FALSE;
			}
		}
		else
		if( item->getType() == LLAssetType::AT_OBJECT )
		{
			if(avatar->isWearingAttachment(item->getUUID()))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

BOOL LLFolderBridge::isUpToDate() const
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	LLViewerInventoryCategory* category = (LLViewerInventoryCategory*)model->getCategory(mUUID);
	if( !category )
	{
		return FALSE;
	}

	return category->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN;
}

BOOL LLFolderBridge::isItemCopyable() const
{
	return TRUE;
}

BOOL LLFolderBridge::copyToClipboard() const
{
	if(isItemCopyable())
	{
		LLInventoryClipboard::instance().add(mUUID);
		return TRUE;
	}
	return FALSE;
}

BOOL LLFolderBridge::isClipboardPasteable() const
{
	if ( ! LLInvFVBridge::isClipboardPasteable() )
		return FALSE;

	// Don't allow pasting duplicates to the Calling Card/Friends subfolders, see bug EXT-1599
	if ( LLFriendCardsManager::instance().isCategoryInFriendFolder( getCategory() ) )
	{
		LLInventoryModel* model = getInventoryModel();
		if ( !model )
		{
			return FALSE;
		}

		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);
		const LLViewerInventoryCategory *current_cat = getCategory();

		// Search for the direct descendent of current Friends subfolder among all pasted items,
		// and return false if is found.
		for(S32 i = objects.count() - 1; i >= 0; --i)
		{
			const LLUUID &obj_id = objects.get(i);
			if ( LLFriendCardsManager::instance().isObjDirectDescendentOfCategory(model->getObject(obj_id), current_cat) )
			{
				return FALSE;
			}
		}

	}
	return TRUE;
}

BOOL LLFolderBridge::isClipboardPasteableAsLink() const
{
	// Check normal paste-as-link permissions
	if (!LLInvFVBridge::isClipboardPasteableAsLink())
	{
		return FALSE;
	}

	const LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return FALSE;
	}

	const LLViewerInventoryCategory *current_cat = getCategory();
	if (current_cat)
	{
		const BOOL is_in_friend_folder = LLFriendCardsManager::instance().isCategoryInFriendFolder( current_cat );
		const LLUUID &current_cat_id = current_cat->getUUID();
		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);
		S32 count = objects.count();
		for(S32 i = 0; i < count; i++)
		{
			const LLUUID &obj_id = objects.get(i);
			const LLInventoryCategory *cat = model->getCategory(obj_id);
			if (cat)
			{
				const LLUUID &cat_id = cat->getUUID();
				// Don't allow recursive pasting
				if ((cat_id == current_cat_id) ||
					model->isObjectDescendentOf(current_cat_id, cat_id))
				{
					return FALSE;
				}
			}
			// Don't allow pasting duplicates to the Calling Card/Friends subfolders, see bug EXT-1599
			if ( is_in_friend_folder )
			{
				// If object is direct descendent of current Friends subfolder than return false.
				// Note: We can't use 'const LLInventoryCategory *cat', because it may be null
				// in case type of obj_id is LLInventoryItem.
				if ( LLFriendCardsManager::instance().isObjDirectDescendentOfCategory(model->getObject(obj_id), current_cat) )
				{
					return FALSE;
				}
			}
		}
	}
	return TRUE;

}

BOOL LLFolderBridge::dragCategoryIntoFolder(LLInventoryCategory* inv_cat,
											BOOL drop)
{
	// This should never happen, but if an inventory item is incorrectly parented,
	// the UI will get confused and pass in a NULL.
	if(!inv_cat) return FALSE;

	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;

	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	if(!avatar) return FALSE;

	// cannot drag categories into library
	if(!isAgentInventory())
	{
		return FALSE;
	}

	// check to make sure source is agent inventory, and is represented there.
	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	BOOL is_agent_inventory = (model->getCategory(inv_cat->getUUID()) != NULL)
		&& (LLToolDragAndDrop::SOURCE_AGENT == source);

	BOOL accept = FALSE;
	S32 i;
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	if(is_agent_inventory)
	{
		const LLUUID& cat_id = inv_cat->getUUID();

		// Is the destination the trash?
		const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
		BOOL move_is_into_trash = (mUUID == trash_id)
				|| model->isObjectDescendentOf(mUUID, trash_id);
		BOOL is_movable = (!LLFolderType::lookupIsProtectedType(inv_cat->getPreferredType()));
		const LLUUID current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
		BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
		BOOL move_is_into_outfit = (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);
		if (move_is_into_current_outfit || move_is_into_outfit)
		{
			// BAP - restrictions?
			is_movable = true;
		}

		if (mUUID == gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE))
		{
			is_movable = FALSE; // It's generally movable but not into Favorites folder. EXT-1604
		}

		if( is_movable )
		{
			gInventory.collectDescendents( cat_id, descendent_categories, descendent_items, FALSE );

			for( i = 0; i < descendent_categories.count(); i++ )
			{
				LLInventoryCategory* category = descendent_categories[i];
				if(LLFolderType::lookupIsProtectedType(category->getPreferredType()))
				{
					// ...can't move "special folders" like Textures
					is_movable = FALSE;
					break;
				}
			}

			if( is_movable )
			{
				if( move_is_into_trash )
				{
					for( i = 0; i < descendent_items.count(); i++ )
					{
						LLInventoryItem* item = descendent_items[i];
						if( (item->getType() == LLAssetType::AT_CLOTHING) ||
							(item->getType() == LLAssetType::AT_BODYPART) )
						{
							if( gAgentWearables.isWearingItem( item->getUUID() ) )
							{
								is_movable = FALSE;  // It's generally movable, but not into the trash!
								break;
							}
						}
						else
						if( item->getType() == LLAssetType::AT_OBJECT )
						{
							if( avatar->isWearingAttachment( item->getUUID() ) )
							{
								is_movable = FALSE;  // It's generally movable, but not into the trash!
								break;
							}
						}
					}
				}
			}
		}


		accept =	is_movable
					&& (mUUID != cat_id)								// Can't move a folder into itself
					&& (mUUID != inv_cat->getParentUUID())				// Avoid moves that would change nothing
					&& !(model->isObjectDescendentOf(mUUID, cat_id));	// Avoid circularity
		if(accept && drop)
		{
			// Look for any gestures and deactivate them
			if (move_is_into_trash)
			{
				for (i = 0; i < descendent_items.count(); i++)
				{
					LLInventoryItem* item = descendent_items[i];
					if (item->getType() == LLAssetType::AT_GESTURE
						&& LLGestureManager::instance().isGestureActive(item->getUUID()))
					{
						LLGestureManager::instance().deactivateGesture(item->getUUID());
					}
				}
			}
			// if target is an outfit or current outfit folder we use link
			if (move_is_into_current_outfit || move_is_into_outfit)
			{
#if SUPPORT_ENSEMBLES
				// BAP - should skip if dup.
				if (move_is_into_current_outfit)
				{
					LLAppearanceManager::wearEnsemble(inv_cat);
				}
				else
				{
					LLPointer<LLInventoryCallback> cb = NULL;
					link_inventory_item(
						gAgent.getID(),
						inv_cat->getUUID(),
						mUUID,
						inv_cat->getName(),
						LLAssetType::AT_LINK_FOLDER,
						cb);
				}
#endif
			}
			else
			{

				// Reparent the folder and restamp children if it's moving
				// into trash.
				LLInvFVBridge::changeCategoryParent(
					model,
					(LLViewerInventoryCategory*)inv_cat,
					mUUID,
					move_is_into_trash);
			}
		}
	}
	else if(LLToolDragAndDrop::SOURCE_WORLD == source)
	{
		// content category has same ID as object itself
		LLUUID object_id = inv_cat->getUUID();
		LLUUID category_id = mUUID;
		accept = move_inv_category_world_to_agent(object_id, category_id, drop);
	}
	return accept;
}

void warn_move_inventory(LLViewerObject* object, LLMoveInv* move_inv)
{
	const char* dialog = NULL;
	if (object->flagScripted())
	{
		dialog = "MoveInventoryFromScriptedObject";
	}
	else
	{
		dialog = "MoveInventoryFromObject";
	}
	LLNotifications::instance().add(dialog, LLSD(), LLSD(), boost::bind(move_task_inventory_callback, _1, _2, move_inv));
}

// Move/copy all inventory items from the Contents folder of an in-world
// object to the agent's inventory, inside a given category.
BOOL move_inv_category_world_to_agent(const LLUUID& object_id,
									  const LLUUID& category_id,
									  BOOL drop,
									  void (*callback)(S32, void*),
									  void* user_data)
{
	// Make sure the object exists. If we allowed dragging from
	// anonymous objects, it would be possible to bypass
	// permissions.
	// content category has same ID as object itself
	LLViewerObject* object = gObjectList.findObject(object_id);
	if(!object)
	{
		llinfos << "Object not found for drop." << llendl;
		return FALSE;
	}

	// this folder is coming from an object, as there is only one folder in an object, the root,
	// we need to collect the entire contents and handle them as a group
	InventoryObjectList inventory_objects;
	object->getInventoryContents(inventory_objects);

	if (inventory_objects.empty())
	{
		llinfos << "Object contents not found for drop." << llendl;
		return FALSE;
	}

	BOOL accept = TRUE;
	BOOL is_move = FALSE;

	// coming from a task. Need to figure out if the person can
	// move/copy this item.
	InventoryObjectList::iterator it = inventory_objects.begin();
	InventoryObjectList::iterator end = inventory_objects.end();
	for ( ; it != end; ++it)
	{
		// coming from a task. Need to figure out if the person can
		// move/copy this item.
		LLPermissions perm(((LLInventoryItem*)((LLInventoryObject*)(*it)))->getPermissions());
		if((perm.allowCopyBy(gAgent.getID(), gAgent.getGroupID())
			&& perm.allowTransferTo(gAgent.getID())))
//			|| gAgent.isGodlike())
		{
			accept = TRUE;
		}
		else if(object->permYouOwner())
		{
			// If the object cannot be copied, but the object the
			// inventory is owned by the agent, then the item can be
			// moved from the task to agent inventory.
			is_move = TRUE;
			accept = TRUE;
		}
		else
		{
			accept = FALSE;
			break;
		}
	}

	if(drop && accept)
	{
		it = inventory_objects.begin();
		InventoryObjectList::iterator first_it = inventory_objects.begin();
		LLMoveInv* move_inv = new LLMoveInv;
		move_inv->mObjectID = object_id;
		move_inv->mCategoryID = category_id;
		move_inv->mCallback = callback;
		move_inv->mUserData = user_data;

		for ( ; it != end; ++it)
		{
			two_uuids_t two(category_id, (*it)->getUUID());
			move_inv->mMoveList.push_back(two);
		}

		if(is_move)
		{
			// Callback called from within here.
			warn_move_inventory(object, move_inv);
		}
		else
		{
			LLNotification::Params params("MoveInventoryFromObject");
			params.functor.function(boost::bind(move_task_inventory_callback, _1, _2, move_inv));
			LLNotifications::instance().forceResponse(params, 0);
		}
	}
	return accept;
}

bool LLFindCOFValidItems::operator()(LLInventoryCategory* cat,
									 LLInventoryItem* item)
{
	// Valid COF items are:
	// - links to wearables (body parts or clothing)
	// - links to attachments
	// - links to gestures
	// - links to ensemble folders
	LLViewerInventoryItem *linked_item = ((LLViewerInventoryItem*)item)->getLinkedItem(); // BAP - safe?
	if (linked_item)
	{
		LLAssetType::EType type = linked_item->getType();
		return (type == LLAssetType::AT_CLOTHING ||
				type == LLAssetType::AT_BODYPART ||
				type == LLAssetType::AT_GESTURE ||
				type == LLAssetType::AT_OBJECT);
	}
	else
	{
		LLViewerInventoryCategory *linked_category = ((LLViewerInventoryItem*)item)->getLinkedCategory(); // BAP - safe?
		// BAP remove AT_NONE support after ensembles are fully working?
		return (linked_category &&
				((linked_category->getPreferredType() == LLFolderType::FT_NONE) ||
				 (LLFolderType::lookupIsEnsembleType(linked_category->getPreferredType()))));
	}
}


bool LLFindWearables::operator()(LLInventoryCategory* cat,
								 LLInventoryItem* item)
{
	if(item)
	{
		if((item->getType() == LLAssetType::AT_CLOTHING)
		   || (item->getType() == LLAssetType::AT_BODYPART))
		{
			return TRUE;
		}
	}
	return FALSE;
}



//Used by LLFolderBridge as callback for directory recursion.
class LLRightClickInventoryFetchObserver : public LLInventoryFetchObserver
{
public:
	LLRightClickInventoryFetchObserver() :
		mCopyItems(false)
	{ };
	LLRightClickInventoryFetchObserver(const LLUUID& cat_id, bool copy_items) :
		mCatID(cat_id),
		mCopyItems(copy_items)
		{ };
	virtual void done()
	{
		// we've downloaded all the items, so repaint the dialog
		LLFolderBridge::staticFolderOptionsMenu();

		gInventory.removeObserver(this);
		delete this;
	}


protected:
	LLUUID mCatID;
	bool mCopyItems;

};

//Used by LLFolderBridge as callback for directory recursion.
class LLRightClickInventoryFetchDescendentsObserver : public LLInventoryFetchDescendentsObserver
{
public:
	LLRightClickInventoryFetchDescendentsObserver(bool copy_items) : mCopyItems(copy_items) {}
	~LLRightClickInventoryFetchDescendentsObserver() {}
	virtual void done();
protected:
	bool mCopyItems;
};

void LLRightClickInventoryFetchDescendentsObserver::done()
{
	// Avoid passing a NULL-ref as mCompleteFolders.front() down to
	// gInventory.collectDescendents()
	if( mCompleteFolders.empty() )
	{
		llwarns << "LLRightClickInventoryFetchDescendentsObserver::done with empty mCompleteFolders" << llendl;
		dec_busy_count();
		gInventory.removeObserver(this);
		delete this;
		return;
	}

	// What we do here is get the complete information on the items in
	// the library, and set up an observer that will wait for that to
	// happen.
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(mCompleteFolders.front(),
								  cat_array,
								  item_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	S32 count = item_array.count();
#if 0 // HACK/TODO: Why?
	// This early causes a giant menu to get produced, and doesn't seem to be needed.
	if(!count)
	{
		llwarns << "Nothing fetched in category " << mCompleteFolders.front()
				<< llendl;
		dec_busy_count();
		gInventory.removeObserver(this);
		delete this;
		return;
	}
#endif

	LLRightClickInventoryFetchObserver* outfit;
	outfit = new LLRightClickInventoryFetchObserver(mCompleteFolders.front(), mCopyItems);
	LLInventoryFetchObserver::item_ref_t ids;
	for(S32 i = 0; i < count; ++i)
	{
		ids.push_back(item_array.get(i)->getUUID());
	}

	// clean up, and remove this as an observer since the call to the
	// outfit could notify observers and throw us into an infinite
	// loop.
	dec_busy_count();
	gInventory.removeObserver(this);
	delete this;

	// increment busy count and either tell the inventory to check &
	// call done, or add this object to the inventory for observation.
	inc_busy_count();

	// do the fetch
	outfit->fetchItems(ids);
	outfit->done();				//Not interested in waiting and this will be right 99% of the time.
//Uncomment the following code for laggy Inventory UI.
/*	if(outfit->isEverythingComplete())
	{
		// everything is already here - call done.
		outfit->done();
	}
	else
	{
		// it's all on it's way - add an observer, and the inventory
		// will call done for us when everything is here.
		gInventory.addObserver(outfit);
	}*/
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryWearObserver
//
// Observer for "copy and wear" operation to support knowing
// when the all of the contents have been added to inventory.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryCopyAndWearObserver : public LLInventoryObserver
{
public:
	LLInventoryCopyAndWearObserver(const LLUUID& cat_id, int count) :mCatID(cat_id), mContentsCount(count), mFolderAdded(FALSE) {}
	virtual ~LLInventoryCopyAndWearObserver() {}
	virtual void changed(U32 mask);

protected:
	LLUUID mCatID;
	int    mContentsCount;
	BOOL   mFolderAdded;
};



void LLInventoryCopyAndWearObserver::changed(U32 mask)
{
	if((mask & (LLInventoryObserver::ADD)) != 0)
	{
		if (!mFolderAdded)
		{
			const std::set<LLUUID>& changed_items = gInventory.getChangedIDs();

			std::set<LLUUID>::const_iterator id_it = changed_items.begin();
			std::set<LLUUID>::const_iterator id_end = changed_items.end();
			for (;id_it != id_end; ++id_it)
			{
				if ((*id_it) == mCatID)
				{
					mFolderAdded = TRUE;
					break;
				}
			}
		}

		if (mFolderAdded)
		{
			LLViewerInventoryCategory* category = gInventory.getCategory(mCatID);

			if (NULL == category)
			{
				llwarns << "gInventory.getCategory(" << mCatID
					<< ") was NULL" << llendl;
			}
			else
			{
				if (category->getDescendentCount() ==
				    mContentsCount)
				{
					gInventory.removeObserver(this);
					LLAppearanceManager::wearInventoryCategory(category, FALSE, TRUE);
					delete this;
				}
			}
		}

	}
}



void LLFolderBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("open" == action)
	{
		openItem();
		return;
	}
	else if ("paste" == action)
	{
		pasteFromClipboard();
		return;
	}
	else if ("paste_link" == action)
	{
		pasteLinkFromClipboard();
		return;
	}
	else if ("properties" == action)
	{
		showProperties();
		return;
	}
	else if ("replaceoutfit" == action)
	{
		modifyOutfit(FALSE);
		return;
	}
#if SUPPORT_ENSEMBLES
	else if ("wearasensemble" == action)
	{
		LLInventoryModel* model = getInventoryModel();
		if(!model) return;
		LLViewerInventoryCategory* cat = getCategory();
		if(!cat) return;
		LLAppearanceManager::wearEnsemble(cat,true);
		return;
	}
#endif
	else if ("addtooutfit" == action)
	{
		modifyOutfit(TRUE);
		return;
	}
	else if ("copy" == action)
	{
		copyToClipboard();
		return;
	}
	else if ("removefromoutfit" == action)
	{
		LLInventoryModel* model = getInventoryModel();
		if(!model) return;
		LLViewerInventoryCategory* cat = getCategory();
		if(!cat) return;

		remove_inventory_category_from_avatar ( cat );
		return;
	}
	else if ("purge" == action)
	{
		purgeItem(model, mUUID);
		return;
	}
	else if ("restore" == action)
	{
		restoreItem();
		return;
	}
}

void LLFolderBridge::openItem()
{
	lldebugs << "LLFolderBridge::openItem()" << llendl;
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;
	bool fetching_inventory = model->fetchDescendentsOf(mUUID);
	// Only change folder type if we have the folder contents.
	if (!fetching_inventory)
	{
		// Disabling this for now, it's causing crash when new items are added to folders
		// since folder type may change before new item item has finished processing.
		// determineFolderType();
	}
}

void LLFolderBridge::closeItem()
{
	determineFolderType();
}

void LLFolderBridge::determineFolderType()
{
	if (isUpToDate())
	{
		LLInventoryModel* model = getInventoryModel();
		LLViewerInventoryCategory* category = model->getCategory(mUUID);
		category->determineFolderType();
	}
}

BOOL LLFolderBridge::isItemRenameable() const
{
	LLViewerInventoryCategory* cat = (LLViewerInventoryCategory*)getCategory();
	if(cat && !LLFolderType::lookupIsProtectedType(cat->getPreferredType())
	   && (cat->getOwnerID() == gAgent.getID()))
	{
		return TRUE;
	}
	return FALSE;
}

void LLFolderBridge::restoreItem()
{
	LLViewerInventoryCategory* cat;
	cat = (LLViewerInventoryCategory*)getCategory();
	if(cat)
	{
		LLInventoryModel* model = getInventoryModel();
		const LLUUID new_parent = model->findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(cat->getType()));
		// do not restamp children on restore
		LLInvFVBridge::changeCategoryParent(model, cat, new_parent, FALSE);
	}
}

LLFolderType::EType LLFolderBridge::getPreferredType() const
{
	LLFolderType::EType preferred_type = LLFolderType::FT_NONE;
	LLViewerInventoryCategory* cat = getCategory();
	if(cat)
	{
		preferred_type = cat->getPreferredType();
	}

	return preferred_type;
}

// Icons for folders are based on the preferred type
LLUIImagePtr LLFolderBridge::getIcon() const
{
	LLFolderType::EType preferred_type = LLFolderType::FT_NONE;
	LLViewerInventoryCategory* cat = getCategory();
	if(cat)
	{
		preferred_type = cat->getPreferredType();
	}
	return getIcon(preferred_type);
}

LLUIImagePtr LLFolderBridge::getIcon(LLFolderType::EType preferred_type)
{
	// we only have one folder image now
	return LLUI::getUIImage("Inv_FolderClosed");
}

BOOL LLFolderBridge::renameItem(const std::string& new_name)
{
	if(!isItemRenameable())
		return FALSE;
	LLInventoryModel* model = getInventoryModel();
	if(!model)
		return FALSE;
	LLViewerInventoryCategory* cat = getCategory();
	if(cat && (cat->getName() != new_name))
	{
		LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat);
		new_cat->rename(new_name);
		new_cat->updateServer(FALSE);
		model->updateCategory(new_cat);

		model->notifyObservers();
	}
	// return FALSE because we either notified observers (& therefore
	// rebuilt) or we didn't update.
	return FALSE;
}

BOOL LLFolderBridge::removeItem()
{
	if(!isItemRemovable())
	{
		return FALSE;
	}
	// move it to the trash
	LLPreview::hide(mUUID);
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;

	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);

	// Look for any gestures and deactivate them
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	gInventory.collectDescendents( mUUID, descendent_categories, descendent_items, FALSE );

	S32 i;
	for (i = 0; i < descendent_items.count(); i++)
	{
		LLInventoryItem* item = descendent_items[i];
		if (item->getType() == LLAssetType::AT_GESTURE
			&& LLGestureManager::instance().isGestureActive(item->getUUID()))
		{
			LLGestureManager::instance().deactivateGesture(item->getUUID());
		}
	}

	// go ahead and do the normal remove if no 'last calling
	// cards' are being removed.
	LLViewerInventoryCategory* cat = getCategory();
	if(cat)
	{
		LLInvFVBridge::changeCategoryParent(model, cat, trash_id, TRUE);
	}

	return TRUE;
}

void LLFolderBridge::pasteFromClipboard()
{
	LLInventoryModel* model = getInventoryModel();
	if(model && isClipboardPasteable())
	{
		LLInventoryItem* item = NULL;
		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);
		S32 count = objects.count();
		const LLUUID parent_id(mUUID);
		for(S32 i = 0; i < count; i++)
		{
			item = model->getItem(objects.get(i));
			if (item)
			{
				if(LLInventoryClipboard::instance().isCutMode())
				{
					// move_inventory_item() is not enough,
					//we have to update inventory locally too
					changeItemParent(model, dynamic_cast<LLViewerInventoryItem*>(item), parent_id, FALSE);
				}
				else
				{
					copy_inventory_item(
						gAgent.getID(),
						item->getPermissions().getOwner(),
						item->getUUID(),
						parent_id,
						std::string(),
						LLPointer<LLInventoryCallback>(NULL));
				}
			}
		}
	}
}

void LLFolderBridge::pasteLinkFromClipboard()
{
	const LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);
		S32 count = objects.count();
		LLUUID parent_id(mUUID);
		for(S32 i = 0; i < count; i++)
		{
			const LLUUID &object_id = objects.get(i);
#if SUPPORT_ENSEMBLES
			if (LLInventoryCategory *cat = model->getCategory(object_id))
			{
				link_inventory_item(
					gAgent.getID(),
					cat->getUUID(),
					parent_id,
					cat->getName(),
					LLAssetType::AT_LINK_FOLDER,
					LLPointer<LLInventoryCallback>(NULL));
			}
			else
#endif
			if (LLInventoryItem *item = model->getItem(object_id))
			{
				link_inventory_item(
					gAgent.getID(),
					item->getUUID(),
					parent_id,
					item->getName(),
					LLAssetType::AT_LINK,
					LLPointer<LLInventoryCallback>(NULL));
			}
		}
	}
}

void LLFolderBridge::staticFolderOptionsMenu()
{
	if (!sSelf) return;
	sSelf->folderOptionsMenu();
}

void LLFolderBridge::folderOptionsMenu()
{
	std::vector<std::string> disabled_items;

	LLInventoryModel* model = getInventoryModel();
	if(!model) return;

	const LLInventoryCategory* category = model->getCategory(mUUID);
	LLFolderType::EType type = category->getPreferredType();
	const bool is_default_folder = category && LLFolderType::lookupIsProtectedType(type);
	// BAP change once we're no longer treating regular categories as ensembles.
	const bool is_ensemble = category && (type == LLFolderType::FT_NONE ||
										  LLFolderType::lookupIsEnsembleType(type));

	// calling card related functionality for folders.

	// Only enable calling-card related options for non-default folders.
	if (!is_default_folder)
	{
		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		if (mCallingCards || checkFolderForContentsOfType(model, is_callingcard))
		{
			mItems.push_back(std::string("Calling Card Separator"));
			mItems.push_back(std::string("Conference Chat Folder"));
			mItems.push_back(std::string("IM All Contacts In Folder"));
		}
	}

	// wearables related functionality for folders.
	//is_wearable
	LLFindWearables is_wearable;
	LLIsType is_object( LLAssetType::AT_OBJECT );
	LLIsType is_gesture( LLAssetType::AT_GESTURE );

	if (mWearables ||
		checkFolderForContentsOfType(model, is_wearable)  ||
		checkFolderForContentsOfType(model, is_object) ||
		checkFolderForContentsOfType(model, is_gesture) )
	{
		mItems.push_back(std::string("Folder Wearables Separator"));

		// Only enable add/replace outfit for non-default folders.
		if (!is_default_folder)
		{
			mItems.push_back(std::string("Add To Outfit"));
			mItems.push_back(std::string("Replace Outfit"));
		}
		if (is_ensemble)
		{
			mItems.push_back(std::string("Wear As Ensemble"));
		}
		mItems.push_back(std::string("Take Off Items"));
	}
	hide_context_entries(*mMenu, mItems, disabled_items);
}

BOOL LLFolderBridge::checkFolderForContentsOfType(LLInventoryModel* model, LLInventoryCollectFunctor& is_type)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	model->collectDescendentsIf(mUUID,
								cat_array,
								item_array,
								LLInventoryModel::EXCLUDE_TRASH,
								is_type);
	return ((item_array.count() > 0) ? TRUE : FALSE );
}

// Flags unused
void LLFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	mItems.clear();
	mDisabledItems.clear();

	lldebugs << "LLFolderBridge::buildContextMenu()" << llendl;
//	std::vector<std::string> disabled_items;
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	const LLUUID lost_and_found_id = model->findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);

	mItems.clear(); //adding code to clear out member Items (which means Items should not have other data here at this point)
	mDisabledItems.clear(); //adding code to clear out disabled members from previous
	if (lost_and_found_id == mUUID)
	  {
		// This is the lost+found folder.
		  mItems.push_back(std::string("Empty Lost And Found"));
	  }

	if(trash_id == mUUID)
	{
		// This is the trash.
		mItems.push_back(std::string("Empty Trash"));
	}
	else if(model->isObjectDescendentOf(mUUID, trash_id))
	{
		// This is a folder in the trash.
		mItems.clear(); // clear any items that used to exist
		mItems.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			mDisabledItems.push_back(std::string("Purge Item"));
		}

		mItems.push_back(std::string("Restore Item"));
	}
	else if(isAgentInventory()) // do not allow creating in library
	{
		LLViewerInventoryCategory *cat =  getCategory();
		// BAP removed protected check to re-enable standard ops in untyped folders.
		// Not sure what the right thing is to do here.
		if (!isCOFFolder() && cat /*&&
			LLAssetType::lookupIsProtectedCategoryType(cat->getPreferredType())*/)
		{
			// Do not allow to create 2-level subfolder in the Calling Card/Friends folder. EXT-694.
			if (!LLFriendCardsManager::instance().isCategoryInFriendFolder(cat))
				mItems.push_back(std::string("New Folder"));
			mItems.push_back(std::string("New Script"));
			mItems.push_back(std::string("New Note"));
			mItems.push_back(std::string("New Gesture"));
			mItems.push_back(std::string("New Clothes"));
			mItems.push_back(std::string("New Body Parts"));
			mItems.push_back(std::string("Change Type"));

			LLViewerInventoryCategory *cat = getCategory();
			if (cat && LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
			{
				mDisabledItems.push_back(std::string("Change Type"));
			}

			getClipboardEntries(false, mItems, mDisabledItems, flags);
		}
		else
		{
			// Want some but not all of the items from getClipboardEntries for outfits.
			if (cat && cat->getPreferredType()==LLFolderType::FT_OUTFIT)
			{
				mItems.push_back(std::string("Rename"));
				mItems.push_back(std::string("Delete"));
			}
		}

		//Added by spatters to force inventory pull on right-click to display folder options correctly. 07-17-06
		mCallingCards = mWearables = FALSE;

		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		if (checkFolderForContentsOfType(model, is_callingcard))
		{
			mCallingCards=TRUE;
		}

		LLFindWearables is_wearable;
		LLIsType is_object( LLAssetType::AT_OBJECT );
		LLIsType is_gesture( LLAssetType::AT_GESTURE );

		if (checkFolderForContentsOfType(model, is_wearable)  ||
			checkFolderForContentsOfType(model, is_object) ||
			checkFolderForContentsOfType(model, is_gesture) )
		{
			mWearables=TRUE;
		}

		mMenu = &menu;
		sSelf = this;
		LLRightClickInventoryFetchDescendentsObserver* fetch = new LLRightClickInventoryFetchDescendentsObserver(FALSE);

		LLInventoryFetchDescendentsObserver::folder_ref_t folders;
		LLViewerInventoryCategory* category = (LLViewerInventoryCategory*)model->getCategory(mUUID);
		if (category)
		{
			folders.push_back(category->getUUID());
		}
		fetch->fetchDescendents(folders);
		inc_busy_count();
		if(fetch->isEverythingComplete())
		{
			// everything is already here - call done.
			fetch->done();
		}
		else
		{
			// it's all on it's way - add an observer, and the inventory
			// will call done for us when everything is here.
			gInventory.addObserver(fetch);
		}
	}
	else
	{
		mItems.push_back(std::string("--no options--"));
		mDisabledItems.push_back(std::string("--no options--"));
	}
	hide_context_entries(menu, mItems, mDisabledItems);
}

BOOL LLFolderBridge::hasChildren() const
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	LLInventoryModel::EHasChildren has_children;
	has_children = gInventory.categoryHasChildren(mUUID);
	return has_children != LLInventoryModel::CHILDREN_NO;
}

BOOL LLFolderBridge::dragOrDrop(MASK mask, BOOL drop,
								EDragAndDropType cargo_type,
								void* cargo_data)
{
	//llinfos << "LLFolderBridge::dragOrDrop()" << llendl;
	BOOL accept = FALSE;
	switch(cargo_type)
	{
		case DAD_TEXTURE:
		case DAD_SOUND:
		case DAD_CALLINGCARD:
		case DAD_LANDMARK:
		case DAD_SCRIPT:
		case DAD_OBJECT:
		case DAD_NOTECARD:
		case DAD_CLOTHING:
		case DAD_BODYPART:
		case DAD_ANIMATION:
		case DAD_GESTURE:
		case DAD_LINK:
			accept = dragItemIntoFolder((LLInventoryItem*)cargo_data,
										drop);
			break;
		case DAD_CATEGORY:
			if (LLFriendCardsManager::instance().isAnyFriendCategory(mUUID))
			{
				accept = FALSE;
			}
			else
			{
				accept = dragCategoryIntoFolder((LLInventoryCategory*)cargo_data, drop);
			}
			break;
		default:
			break;
	}
	return accept;
}

LLViewerInventoryCategory* LLFolderBridge::getCategory() const
{
	LLViewerInventoryCategory* cat = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		cat = (LLViewerInventoryCategory*)model->getCategory(mUUID);
	}
	return cat;
}


// static
void LLFolderBridge::pasteClipboard(void* user_data)
{
	LLFolderBridge* self = (LLFolderBridge*)user_data;
	if(self) self->pasteFromClipboard();
}

void LLFolderBridge::createNewCategory(void* user_data)
{
	LLFolderBridge* bridge = (LLFolderBridge*)user_data;
	if(!bridge) return;
	LLInventoryPanel* panel = dynamic_cast<LLInventoryPanel*>(bridge->mInventoryPanel.get());
	if (!panel) return;
	LLInventoryModel* model = panel->getModel();
	if(!model) return;
	LLUUID id;
	id = model->createNewCategory(bridge->getUUID(),
								  LLFolderType::FT_NONE,
								  LLStringUtil::null);
	model->notifyObservers();

	// At this point, the bridge has probably been deleted, but the
	// view is still there.
	panel->setSelection(id, TAKE_FOCUS_YES);
}

void LLFolderBridge::createNewShirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SHIRT);
}

void LLFolderBridge::createNewPants(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_PANTS);
}

void LLFolderBridge::createNewShoes(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SHOES);
}

void LLFolderBridge::createNewSocks(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SOCKS);
}

void LLFolderBridge::createNewJacket(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_JACKET);
}

void LLFolderBridge::createNewSkirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SKIRT);
}

void LLFolderBridge::createNewGloves(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_GLOVES);
}

void LLFolderBridge::createNewUndershirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_UNDERSHIRT);
}

void LLFolderBridge::createNewUnderpants(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_UNDERPANTS);
}

void LLFolderBridge::createNewShape(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SHAPE);
}

void LLFolderBridge::createNewSkin(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SKIN);
}

void LLFolderBridge::createNewHair(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_HAIR);
}

void LLFolderBridge::createNewEyes(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_EYES);
}

// static
void LLFolderBridge::createWearable(LLFolderBridge* bridge, EWearableType type)
{
	if(!bridge) return;
	LLUUID parent_id = bridge->getUUID();
	createWearable(parent_id, type);
}

// Separate function so can be called by global menu as well as right-click
// menu.
// static
void LLFolderBridge::createWearable(const LLUUID &parent_id, EWearableType type)
{
	LLWearable* wearable = LLWearableList::instance().createNewWearable(type);
	LLAssetType::EType asset_type = wearable->getAssetType();
	LLInventoryType::EType inv_type = LLInventoryType::IT_WEARABLE;
	create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
		parent_id, wearable->getTransactionID(), wearable->getName(),
		wearable->getDescription(), asset_type, inv_type, wearable->getType(),
		wearable->getPermissions().getMaskNextOwner(),
		LLPointer<LLInventoryCallback>(NULL));
}

void LLFolderBridge::modifyOutfit(BOOL append)
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;
	LLViewerInventoryCategory* cat = getCategory();
	if(!cat) return;

	// BAP - was:
	// wear_inventory_category_on_avatar( cat, append );
	LLAppearanceManager::wearInventoryCategory( cat, FALSE, append );
}

// helper stuff
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, LLMoveInv* move_inv)
{
	LLFloaterOpenObject::LLCatAndWear* cat_and_wear = (LLFloaterOpenObject::LLCatAndWear* )move_inv->mUserData;
	LLViewerObject* object = gObjectList.findObject(move_inv->mObjectID);
	S32 option = LLNotification::getSelectedOption(notification, response);

	if(option == 0 && object)
	{
		if (cat_and_wear && cat_and_wear->mWear)
		{
			InventoryObjectList inventory_objects;
			object->getInventoryContents(inventory_objects);
			int contents_count = inventory_objects.size()-1; //subtract one for containing folder

			LLInventoryCopyAndWearObserver* inventoryObserver = new LLInventoryCopyAndWearObserver(cat_and_wear->mCatID, contents_count);
			gInventory.addObserver(inventoryObserver);
		}

		two_uuids_list_t::iterator move_it;
		for (move_it = move_inv->mMoveList.begin();
			move_it != move_inv->mMoveList.end();
			++move_it)
		{
			object->moveInventory(move_it->first, move_it->second);
		}

		// update the UI.
		dialog_refresh_all();
	}

	if (move_inv->mCallback)
	{
		move_inv->mCallback(option, move_inv->mUserData);
	}

	delete move_inv;
	return false;
}

/*
Next functions intended to reorder items in the inventory folder and save order on server
Is now used for Favorites folder.

*TODO: refactoring is needed with Favorites Bar functionality. Probably should be moved in LLInventoryModel
*/
void saveItemsOrder(LLInventoryModel::item_array_t& items)
{
	int sortField = 0;

	// current order is saved by setting incremental values (1, 2, 3, ...) for the sort field
	for (LLInventoryModel::item_array_t::iterator i = items.begin(); i != items.end(); ++i)
	{
		LLViewerInventoryItem* item = *i;

		item->setSortField(++sortField);
		item->setComplete(TRUE);
		item->updateServer(FALSE);

		gInventory.updateItem(item);
	}

	gInventory.notifyObservers();
}

LLInventoryModel::item_array_t::iterator findItemByUUID(LLInventoryModel::item_array_t& items, const LLUUID& id)
{
	LLInventoryModel::item_array_t::iterator result = items.end();

	for (LLInventoryModel::item_array_t::iterator i = items.begin(); i != items.end(); ++i)
	{
		if ((*i)->getUUID() == id)
		{
			result = i;
			break;
		}
	}

	return result;
}

void updateItemsOrder(LLInventoryModel::item_array_t& items, const LLUUID& srcItemId, const LLUUID& destItemId)
{
	LLViewerInventoryItem* srcItem = gInventory.getItem(srcItemId);
	LLViewerInventoryItem* destItem = gInventory.getItem(destItemId);

	items.erase(findItemByUUID(items, srcItem->getUUID()));
	items.insert(findItemByUUID(items, destItem->getUUID()), srcItem);
}

BOOL LLFolderBridge::dragItemIntoFolder(LLInventoryItem* inv_item,
										BOOL drop)
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;

	// cannot drag into library
	if(!isAgentInventory())
	{
		return FALSE;
	}

	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	if(!avatar) return FALSE;

	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	BOOL accept = FALSE;
	LLViewerObject* object = NULL;
	if(LLToolDragAndDrop::SOURCE_AGENT == source)
	{

		BOOL is_movable = TRUE;
		switch( inv_item->getActualType() )
		{
		case LLAssetType::AT_ROOT_CATEGORY:
			is_movable = FALSE;
			break;

		case LLAssetType::AT_CATEGORY:
			is_movable = !LLFolderType::lookupIsProtectedType(((LLInventoryCategory*)inv_item)->getPreferredType());
			break;
		default:
			break;
		}

		const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
		BOOL move_is_into_trash = (mUUID == trash_id) || model->isObjectDescendentOf(mUUID, trash_id);
		const LLUUID current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
		BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
		BOOL move_is_into_outfit = (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);

		if(is_movable && move_is_into_trash)
		{
			switch(inv_item->getType())
			{
			case LLAssetType::AT_CLOTHING:
			case LLAssetType::AT_BODYPART:
				is_movable = !gAgentWearables.isWearingItem(inv_item->getUUID());
				break;

			case LLAssetType::AT_OBJECT:
				is_movable = !avatar->isWearingAttachment(inv_item->getUUID());
				break;
			default:
				break;
			}
		}

		if ( is_movable )
		{
			// Don't allow creating duplicates in the Calling Card/Friends
			// subfolders, see bug EXT-1599. Check is item direct descendent
			// of target folder and forbid item's movement if it so.
			// Note: isItemDirectDescendentOfCategory checks if
			// passed category is in the Calling Card/Friends folder
			is_movable = ! LLFriendCardsManager::instance()
				.isObjDirectDescendentOfCategory (inv_item, getCategory());
		}

		const LLUUID& favorites_id = model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE);

		// we can move item inside a folder only if this folder is Favorites. See EXT-719
		accept = is_movable && ((mUUID != inv_item->getParentUUID()) || (mUUID == favorites_id));
		if(accept && drop)
		{
			if (inv_item->getType() == LLAssetType::AT_GESTURE
				&& LLGestureManager::instance().isGestureActive(inv_item->getUUID()) && move_is_into_trash)
			{
				LLGestureManager::instance().deactivateGesture(inv_item->getUUID());
			}
			// If an item is being dragged between windows, unselect
			// everything in the active window so that we don't follow
			// the selection to its new location (which is very
			// annoying).
			if (LLFloaterInventory::getActiveInventory())
			{
				LLInventoryPanel* active_panel = LLFloaterInventory::getActiveInventory()->getPanel();
				LLInventoryPanel* panel = dynamic_cast<LLInventoryPanel*>(mInventoryPanel.get());
				if (active_panel && (panel != active_panel))
				{
					active_panel->unSelectAll();
				}
			}

			// if dragging from/into favorites folder only reorder items
			if ((mUUID == inv_item->getParentUUID()) && (favorites_id == mUUID))
			{
				LLInventoryModel::cat_array_t cats;
				LLInventoryModel::item_array_t items;
				LLIsType is_type(LLAssetType::AT_LANDMARK);
				model->collectDescendentsIf(favorites_id, cats, items, LLInventoryModel::EXCLUDE_TRASH, is_type);

				LLInventoryPanel* panel = dynamic_cast<LLInventoryPanel*>(mInventoryPanel.get());
				LLFolderViewItem* itemp = panel ? panel->getRootFolder()->getDraggingOverItem() : NULL;
				if (itemp)
				{
					LLUUID srcItemId = inv_item->getUUID();
					LLUUID destItemId = itemp->getListener()->getUUID();

					// update order
					updateItemsOrder(items, srcItemId, destItemId);

					saveItemsOrder(items);
				}
			}
			else if (favorites_id == mUUID) // if target is the favorites folder we use copy
			{
				copy_inventory_item(
					gAgent.getID(),
					inv_item->getPermissions().getOwner(),
					inv_item->getUUID(),
					mUUID,
					std::string(),
					LLPointer<LLInventoryCallback>(NULL));
			}
			else if (move_is_into_current_outfit || move_is_into_outfit)
			{
				// BAP - should skip if dup.
				if (move_is_into_current_outfit)
				{
					LLAppearanceManager::wearItem(inv_item);
				}
				else
				{
					LLPointer<LLInventoryCallback> cb = NULL;
					link_inventory_item(
						gAgent.getID(),
						inv_item->getUUID(),
						mUUID,
						std::string(),
						LLAssetType::AT_LINK,
						cb);
				}
			}
			else
			{
				// restamp if the move is into the trash.
				LLInvFVBridge::changeItemParent(
					model,
					(LLViewerInventoryItem*)inv_item,
					mUUID,
					move_is_into_trash);
			}
		}
	}
	else if(LLToolDragAndDrop::SOURCE_WORLD == source)
	{
		// Make sure the object exists. If we allowed dragging from
		// anonymous objects, it would be possible to bypass
		// permissions.
		object = gObjectList.findObject(inv_item->getParentUUID());
		if(!object)
		{
			llinfos << "Object not found for drop." << llendl;
			return FALSE;
		}

		// coming from a task. Need to figure out if the person can
		// move/copy this item.
		LLPermissions perm(inv_item->getPermissions());
		BOOL is_move = FALSE;
		if((perm.allowCopyBy(gAgent.getID(), gAgent.getGroupID())
			&& perm.allowTransferTo(gAgent.getID())))
//		   || gAgent.isGodlike())

		{
			accept = TRUE;
		}
		else if(object->permYouOwner())
		{
			// If the object cannot be copied, but the object the
			// inventory is owned by the agent, then the item can be
			// moved from the task to agent inventory.
			is_move = TRUE;
			accept = TRUE;
		}
		if(drop && accept)
		{
			LLMoveInv* move_inv = new LLMoveInv;
			move_inv->mObjectID = inv_item->getParentUUID();
			two_uuids_t item_pair(mUUID, inv_item->getUUID());
			move_inv->mMoveList.push_back(item_pair);
			move_inv->mCallback = NULL;
			move_inv->mUserData = NULL;
			if(is_move)
			{
				warn_move_inventory(object, move_inv);
			}
			else
			{
				LLNotification::Params params("MoveInventoryFromObject");
				params.functor.function(boost::bind(move_task_inventory_callback, _1, _2, move_inv));
				LLNotifications::instance().forceResponse(params, 0);
			}
		}

	}
	else if(LLToolDragAndDrop::SOURCE_NOTECARD == source)
	{
		accept = TRUE;
		if(drop)
		{
			copy_inventory_from_notecard(LLToolDragAndDrop::getInstance()->getObjectID(),
				LLToolDragAndDrop::getInstance()->getSourceID(), inv_item);
		}
	}
	else if(LLToolDragAndDrop::SOURCE_LIBRARY == source)
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)inv_item;
		if(item && item->isComplete())
		{
			accept = TRUE;
			if(drop)
			{
				copy_inventory_item(
					gAgent.getID(),
					inv_item->getPermissions().getOwner(),
					inv_item->getUUID(),
					mUUID,
					std::string(),
					LLPointer<LLInventoryCallback>(NULL));
			}
		}
	}
	else
	{
		llwarns << "unhandled drag source" << llendl;
	}
	return accept;
}

// +=================================================+
// |        LLScriptBridge (DEPRECTED)               |
// +=================================================+

LLUIImagePtr LLScriptBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_SCRIPT, LLInventoryType::IT_LSL, 0, FALSE);
}

// +=================================================+
// |        LLTextureBridge                          |
// +=================================================+

LLUIImagePtr LLTextureBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_TEXTURE, mInvType, 0, FALSE);
}

void LLTextureBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}

void LLTextureBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLTextureBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		items.push_back(std::string("Texture Separator"));
		items.push_back(std::string("Save As"));
	}
	hide_context_entries(menu, items, disabled_items);	
}

// virtual
void LLTextureBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("save_as" == action)
	{
		LLFloaterReg::showInstance("preview_texture", LLSD(mUUID), TAKE_FOCUS_YES);
		LLPreviewTexture* preview_texture = LLFloaterReg::findTypedInstance<LLPreviewTexture>("preview_texture", mUUID);
		if (preview_texture)
		{
			preview_texture->openToSave();
		}
	}
	else LLItemBridge::performAction(folder, model, action);
}

// +=================================================+
// |        LLSoundBridge                            |
// +=================================================+

LLUIImagePtr LLSoundBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_SOUND, LLInventoryType::IT_SOUND, 0, FALSE);
}

void LLSoundBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
/*
// Changed this back to the way it USED to work:
// only open the preview dialog through the contextual right-click menu
// double-click just plays the sound

	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		openSoundPreview((void*)this);
		//send_uuid_sound_trigger(item->getAssetUUID(), 1.0);
	}
*/
}

void LLSoundBridge::previewItem()
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		send_sound_trigger(item->getAssetUUID(), 1.0);
	}
}

void LLSoundBridge::openSoundPreview(void* which)
{
	LLSoundBridge *me = (LLSoundBridge *)which;
	LLFloaterReg::showInstance("preview_sound", LLSD(me->mUUID), TAKE_FOCUS_YES);
}

void LLSoundBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLSoundBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Sound Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	items.push_back(std::string("Sound Separator"));
	items.push_back(std::string("Sound Play"));

	hide_context_entries(menu, items, disabled_items);
}

// +=================================================+
// |        LLLandmarkBridge                         |
// +=================================================+

LLLandmarkBridge::LLLandmarkBridge(LLInventoryPanel* inventory, const LLUUID& uuid, U32 flags/* = 0x00*/) :
LLItemBridge(inventory, uuid)
{
	mVisited = FALSE;
	if (flags & LLInventoryItem::II_FLAGS_LANDMARK_VISITED)
	{
		mVisited = TRUE;
	}
}

LLUIImagePtr LLLandmarkBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_LANDMARK, LLInventoryType::IT_LANDMARK, mVisited, FALSE);
}

void LLLandmarkBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	lldebugs << "LLLandmarkBridge::buildContextMenu()" << llendl;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Landmark Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	items.push_back(std::string("Landmark Separator"));
	items.push_back(std::string("About Landmark"));

	// Disable "About Landmark" menu item for
	// multiple landmarks selected. Only one landmark
	// info panel can be shown at a time.
	if ((flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("About Landmark"));
	}

	hide_context_entries(menu, items, disabled_items);
}

// Convenience function for the two functions below.
void teleport_via_landmark(const LLUUID& asset_id)
{
	gAgent.teleportViaLandmark( asset_id );

	// we now automatically track the landmark you're teleporting to
	// because you'll probably arrive at a telehub instead
	LLFloaterWorldMap* floater_world_map = LLFloaterWorldMap::getInstance();
	if( floater_world_map )
	{
		floater_world_map->trackLandmark( asset_id );
	}
}

// virtual
void LLLandmarkBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("teleport" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if(item)
		{
			teleport_via_landmark(item->getAssetUUID());
		}
	}
	else if ("about" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if(item)
		{
			LLSD key;
			key["type"] = "landmark";
			key["id"] = item->getUUID();

			LLSideTray::getInstance()->showPanel("panel_places", key);
		}
	}
	else
	{
		LLItemBridge::performAction(folder, model, action);
	}
}

static bool open_landmark_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	LLUUID asset_id = notification["payload"]["asset_id"].asUUID();
	if (option == 0)
	{
		teleport_via_landmark(asset_id);
	}

	return false;
}
static LLNotificationFunctorRegistration open_landmark_callback_reg("TeleportFromLandmark", open_landmark_callback);


void LLLandmarkBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
/*
	LLViewerInventoryItem* item = getItem();
	if( item )
	{
		// Opening (double-clicking) a landmark immediately teleports,
		// but warns you the first time.
		// open_landmark(item);
		LLSD payload;
		payload["asset_id"] = item->getAssetUUID();
		LLNotifications::instance().add("TeleportFromLandmark", LLSD(), payload);
	}
*/
}


// +=================================================+
// |        LLCallingCardObserver                    |
// +=================================================+
void LLCallingCardObserver::changed(U32 mask)
{
	mBridgep->refreshFolderViewItem();
}

// +=================================================+
// |        LLCallingCardBridge                      |
// +=================================================+

LLCallingCardBridge::LLCallingCardBridge( LLInventoryPanel* inventory, const LLUUID& uuid ) :
	LLItemBridge(inventory, uuid)
{
	mObserver = new LLCallingCardObserver(this);
	LLAvatarTracker::instance().addObserver(mObserver);
}

LLCallingCardBridge::~LLCallingCardBridge()
{
	LLAvatarTracker::instance().removeObserver(mObserver);
	delete mObserver;
}

void LLCallingCardBridge::refreshFolderViewItem()
{
	LLInventoryPanel* panel = dynamic_cast<LLInventoryPanel*>(mInventoryPanel.get());
	LLFolderViewItem* itemp = panel ? panel->getRootFolder()->getItemByID(mUUID) : NULL;
	if (itemp)
	{
		itemp->refresh();
	}
}

// virtual
void LLCallingCardBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("begin_im" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgent.getID()) &&
			(!item->getCreatorUUID().isNull()))
		{
			std::string callingcard_name;
			gCacheName->getFullName(item->getCreatorUUID(), callingcard_name);
			LLUUID session_id = gIMMgr->addSession(callingcard_name, IM_NOTHING_SPECIAL, item->getCreatorUUID());
			if (session_id != LLUUID::null)
			{
				LLIMFloater::show(session_id);
			}
		}
	}
	else if ("lure" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgent.getID()) &&
			(!item->getCreatorUUID().isNull()))
		{
			LLAvatarActions::offerTeleport(item->getCreatorUUID());
		}
	}
	else LLItemBridge::performAction(folder, model, action);
}

LLUIImagePtr LLCallingCardBridge::getIcon() const
{
	BOOL online = FALSE;
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		online = LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID());
	}
	return get_item_icon(LLAssetType::AT_CALLINGCARD, LLInventoryType::IT_CALLINGCARD, online, FALSE);
}

std::string LLCallingCardBridge::getLabelSuffix() const
{
	LLViewerInventoryItem* item = getItem();
	if( item && LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID()) )
	{
		return LLItemBridge::getLabelSuffix() + " (online)";
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

void LLCallingCardBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
/*
	LLViewerInventoryItem* item = getItem();
	if(item && !item->getCreatorUUID().isNull())
	{
		LLAvatarActions::showProfile(item->getCreatorUUID());
	}
*/
}

void LLCallingCardBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLCallingCardBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		LLInventoryItem* item = getItem();
		BOOL good_card = (item
						  && (LLUUID::null != item->getCreatorUUID())
						  && (item->getCreatorUUID() != gAgent.getID()));
		BOOL user_online = (LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID()));
		items.push_back(std::string("Send Instant Message Separator"));
		items.push_back(std::string("Send Instant Message"));
		items.push_back(std::string("Offer Teleport..."));
		items.push_back(std::string("Conference Chat"));

		if (!good_card)
		{
			disabled_items.push_back(std::string("Send Instant Message"));
		}
		if (!good_card || !user_online)
		{
			disabled_items.push_back(std::string("Offer Teleport..."));
			disabled_items.push_back(std::string("Conference Chat"));
		}
	}
	hide_context_entries(menu, items, disabled_items);
}

BOOL LLCallingCardBridge::dragOrDrop(MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data)
{
	LLViewerInventoryItem* item = getItem();
	BOOL rv = FALSE;
	if(item)
	{
		// check the type
		switch(cargo_type)
		{
		case DAD_TEXTURE:
		case DAD_SOUND:
		case DAD_LANDMARK:
		case DAD_SCRIPT:
		case DAD_CLOTHING:
		case DAD_OBJECT:
		case DAD_NOTECARD:
		case DAD_BODYPART:
		case DAD_ANIMATION:
		case DAD_GESTURE:
			{
				LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;
				const LLPermissions& perm = inv_item->getPermissions();
				if(gInventory.getItem(inv_item->getUUID())
				   && perm.allowOperationBy(PERM_TRANSFER, gAgent.getID()))
				{
					rv = TRUE;
					if(drop)
					{
						LLToolDragAndDrop::giveInventory(item->getCreatorUUID(),
														 (LLInventoryItem*)cargo_data);
					}
				}
				else
				{
					// It's not in the user's inventory (it's probably in
					// an object's contents), so disallow dragging it here.
					// You can't give something you don't yet have.
					rv = FALSE;
				}
				break;
			}
		case DAD_CATEGORY:
			{
				LLInventoryCategory* inv_cat = (LLInventoryCategory*)cargo_data;
				if( gInventory.getCategory( inv_cat->getUUID() ) )
				{
					rv = TRUE;
					if(drop)
					{
						LLToolDragAndDrop::giveInventoryCategory(
							item->getCreatorUUID(),
							inv_cat);
					}
				}
				else
				{
					// It's not in the user's inventory (it's probably in
					// an object's contents), so disallow dragging it here.
					// You can't give something you don't yet have.
					rv = FALSE;
				}
				break;
			}
		default:
			break;
		}
	}
	return rv;
}

BOOL LLCallingCardBridge::removeItem()
{
	if (LLFriendCardsManager::instance().isItemInAnyFriendsList(getItem()))
	{
		LLAvatarActions::removeFriendDialog(getItem()->getCreatorUUID());
		return FALSE;
	}
	else
	{
		return LLItemBridge::removeItem();
	}
}
// +=================================================+
// |        LLNotecardBridge                         |
// +=================================================+

LLUIImagePtr LLNotecardBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_NOTECARD, LLInventoryType::IT_NOTECARD, 0, FALSE);
}

void LLNotecardBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}

/*
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLFloaterReg::showInstance("preview_notecard", LLSD(item->getUUID()), TAKE_FOCUS_YES);
	}
*/
}


// +=================================================+
// |        LLGestureBridge                          |
// +=================================================+

LLUIImagePtr LLGestureBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_GESTURE, LLInventoryType::IT_GESTURE, 0, FALSE);
}

LLFontGL::StyleFlags LLGestureBridge::getLabelStyle() const
{
	if( LLGestureManager::instance().isGestureActive(mUUID) )
	{
		return LLFontGL::BOLD;
	}
	else
	{
		return LLFontGL::NORMAL;
	}
}

std::string LLGestureBridge::getLabelSuffix() const
{
	if( LLGestureManager::instance().isGestureActive(mUUID) )
	{
		return LLItemBridge::getLabelSuffix() + " (active)";
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

// virtual
void LLGestureBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("activate" == action)
	{
		LLGestureManager::instance().activateGesture(mUUID);

		LLViewerInventoryItem* item = gInventory.getItem(mUUID);
		if (!item) return;

		// Since we just changed the suffix to indicate (active)
		// the server doesn't need to know, just the viewer.
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}
	else if ("deactivate" == action)
	{
		LLGestureManager::instance().deactivateGesture(mUUID);

		LLViewerInventoryItem* item = gInventory.getItem(mUUID);
		if (!item) return;

		// Since we just changed the suffix to indicate (active)
		// the server doesn't need to know, just the viewer.
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}
	else LLItemBridge::performAction(folder, model, action);
}

void LLGestureBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
/*
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLPreviewGesture* preview = LLPreviewGesture::show(mUUID, LLUUID::null);
		preview->setFocus(TRUE);
	}
*/
}

BOOL LLGestureBridge::removeItem()
{
	// Force close the preview window, if it exists
	LLGestureManager::instance().deactivateGesture(mUUID);
	return LLItemBridge::removeItem();
}

void LLGestureBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLGestureBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		items.push_back(std::string("Gesture Separator"));
		items.push_back(std::string("Activate"));
		items.push_back(std::string("Deactivate"));
	}
	hide_context_entries(menu, items, disabled_items);
}

// +=================================================+
// |        LLAnimationBridge                        |
// +=================================================+

LLUIImagePtr LLAnimationBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_ANIMATION, LLInventoryType::IT_ANIMATION, 0, FALSE);
}

void LLAnimationBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	lldebugs << "LLAnimationBridge::buildContextMenu()" << llendl;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Animation Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	items.push_back(std::string("Animation Separator"));
	items.push_back(std::string("Animation Play"));
	items.push_back(std::string("Animation Audition"));

	hide_context_entries(menu, items, disabled_items);

}

// virtual
void LLAnimationBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ((action == "playworld") || (action == "playlocal"))
	{
		if (getItem())
		{
			LLPreviewAnim::e_activation_type activate = LLPreviewAnim::NONE;
			if ("playworld" == action) activate = LLPreviewAnim::PLAY;
			if ("playlocal" == action) activate = LLPreviewAnim::AUDITION;

			LLPreviewAnim* preview = LLFloaterReg::showTypedInstance<LLPreviewAnim>("preview_anim", LLSD(mUUID));
			if (preview)
			{
				preview->activate(activate);
			}
		}
	}
	else
	{
		LLItemBridge::performAction(folder, model, action);
	}
}

void LLAnimationBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
/*
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLFloaterReg::showInstance("preview_anim", LLSD(mUUID), TAKE_FOCUS_YES);
	}
*/
}

// +=================================================+
// |        LLObjectBridge                           |
// +=================================================+

// static
LLUUID LLObjectBridge::sContextMenuItemID;

LLObjectBridge::LLObjectBridge(LLInventoryPanel* inventory, const LLUUID& uuid, LLInventoryType::EType type, U32 flags) :
LLItemBridge(inventory, uuid), mInvType(type)
{
	mAttachPt = (flags & 0xff); // low bye of inventory flags

	mIsMultiObject = ( flags & LLInventoryItem::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS ) ?  TRUE: FALSE;
}

BOOL LLObjectBridge::isItemRemovable()
{
	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	if(!avatar) return FALSE;
	if(avatar->isWearingAttachment(mUUID)) return FALSE;
	return LLInvFVBridge::isItemRemovable();
}

LLUIImagePtr LLObjectBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_OBJECT, mInvType, mAttachPt, mIsMultiObject );
}

LLInventoryObject* LLObjectBridge::getObject() const
{
	LLInventoryObject* object = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		object = (LLInventoryObject*)model->getObject(mUUID);
	}
	return object;
}

// virtual
void LLObjectBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("attach" == action)
	{
		LLUUID object_id = mUUID;
		LLViewerInventoryItem* item;
		item = (LLViewerInventoryItem*)gInventory.getItem(object_id);
		if(item && gInventory.isObjectDescendentOf(object_id, gInventory.getRootFolderID()))
		{
			rez_attachment(item, NULL);
		}
		else if(item && item->isComplete())
		{
			// must be in library. copy it to our inventory and put it on.
			LLPointer<LLInventoryCallback> cb = new RezAttachmentCallback(0);
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
		gFocusMgr.setKeyboardFocus(NULL);
	}
	else if ("detach" == action)
	{
		LLInventoryItem* item = gInventory.getItem(mUUID);
		if(item)
		{
			gMessageSystem->newMessageFast(_PREHASH_DetachAttachmentIntoInv);
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			gMessageSystem->addUUIDFast(_PREHASH_ItemID, item->getLinkedUUID());
			gMessageSystem->sendReliable( gAgent.getRegion()->getHost());
		}
		// this object might have been selected, so let the selection manager know it's gone now
		LLViewerObject *found_obj =
			gObjectList.findObject(item->getUUID());
		if (found_obj)
		{
			LLSelectMgr::getInstance()->remove(found_obj);
		}
		else
		{
			llwarns << "object not found - ignoring" << llendl;
		}
	}
	else LLItemBridge::performAction(folder, model, action);
}

void LLObjectBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}

	LLSD key;
	key["id"] = mUUID;
	LLSideTray::getInstance()->showPanel("sidepanel_inventory", key);

	// Disable old properties floater; this is replaced by the sidepanel.
	/*
	LLFloaterReg::showInstance("properties", mUUID);
	*/
}

LLFontGL::StyleFlags LLObjectBridge::getLabelStyle() const
{
	U8 font = LLFontGL::NORMAL;

	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	if( avatar && avatar->isWearingAttachment( mUUID ) )
	{
		font |= LLFontGL::BOLD;
	}

	LLInventoryItem* item = getItem();
	if (item && item->getIsLinkType())
	{
		font |= LLFontGL::ITALIC;
	}

	return (LLFontGL::StyleFlags)font;
}

std::string LLObjectBridge::getLabelSuffix() const
{
	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	if( avatar && avatar->isWearingAttachment( mUUID ) )
	{
		std::string attachment_point_name = avatar->getAttachedPointName(mUUID);
		LLStringUtil::toLower(attachment_point_name);

		LLStringUtil::format_map_t args;
		args["[ATTACHMENT_POINT]"] =  attachment_point_name.c_str();
		return LLItemBridge::getLabelSuffix() + LLTrans::getString("WornOnAttachmentPoint", args);
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

void rez_attachment(LLViewerInventoryItem* item, LLViewerJointAttachment* attachment)
{
	LLSD payload;
	payload["item_id"] = item->getLinkedUUID(); // Wear the base object in case this is a link.

	S32 attach_pt = 0;
	if (gAgent.getAvatarObject() && attachment)
	{
		for (LLVOAvatar::attachment_map_t::iterator iter = gAgent.getAvatarObject()->mAttachmentPoints.begin();
			 iter != gAgent.getAvatarObject()->mAttachmentPoints.end(); ++iter)
		{
			if (iter->second == attachment)
			{
				attach_pt = iter->first;
				break;
			}
		}
	}

	payload["attachment_point"] = attach_pt;

#if !ENABLE_MULTIATTACHMENTS
	if (attachment && attachment->getNumObjects() > 0)
	{
		LLNotifications::instance().add("ReplaceAttachment", LLSD(), payload, confirm_replace_attachment_rez);
	}
	else
#endif
	{
		LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
	}
}

bool confirm_replace_attachment_rez(const LLSD& notification, const LLSD& response)
{
	LLVOAvatar *avatarp = gAgent.getAvatarObject();

	if (!avatarp->canAttachMoreObjects())
	{
		LLSD args;
		args["MAX_ATTACHMENTS"] = llformat("%d", MAX_AGENT_ATTACHMENTS);
		LLNotifications::instance().add("MaxAttachmentsOnOutfit", args);
		return false;
	}

	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0/*YES*/)
	{
		LLViewerInventoryItem* itemp = gInventory.getItem(notification["payload"]["item_id"].asUUID());

		if (itemp)
		{
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_RezSingleAttachmentFromInv);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addUUIDFast(_PREHASH_ItemID, itemp->getUUID());
			msg->addUUIDFast(_PREHASH_OwnerID, itemp->getPermissions().getOwner());
			U8 attachment_pt = notification["payload"]["attachment_point"].asInteger();
#if ENABLE_MULTIATTACHMENTS
			attachment_pt |= ATTACHMENT_ADD;
#endif
			msg->addU8Fast(_PREHASH_AttachmentPt, attachment_pt);
			pack_permissions_slam(msg, itemp->getFlags(), itemp->getPermissions());
			msg->addStringFast(_PREHASH_Name, itemp->getName());
			msg->addStringFast(_PREHASH_Description, itemp->getDescription());
			msg->sendReliable(gAgent.getRegion()->getHost());
		}
	}
	return false;
}
static LLNotificationFunctorRegistration confirm_replace_attachment_rez_reg("ReplaceAttachment", confirm_replace_attachment_rez);

void LLObjectBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		LLInventoryItem* item = getItem();
		if (item && item->getIsLinkType())
		{
			items.push_back(std::string("Goto Link"));
		}

		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		LLObjectBridge::sContextMenuItemID = mUUID;

		if(item)
		{
			LLVOAvatarSelf* avatarp = gAgent.getAvatarObject();
			if( !avatarp )
			{
				return;
			}

			if( avatarp->isWearingAttachment( mUUID ) )
			{
				items.push_back(std::string("Detach From Yourself"));
			}
			else
			if( !isInTrash() && !isLinkedObjectInTrash() )
			{
				items.push_back(std::string("Attach Separator"));
				items.push_back(std::string("Object Wear"));
				items.push_back(std::string("Attach To"));
				items.push_back(std::string("Attach To HUD"));
				// commented out for DEV-32347
				//items.push_back(std::string("Restore to Last Position"));

				if (!avatarp->canAttachMoreObjects())
				{
					disabled_items.push_back(std::string("Object Wear"));
					disabled_items.push_back(std::string("Attach To"));
					disabled_items.push_back(std::string("Attach To HUD"));
				}
				LLMenuGL* attach_menu = menu.findChildMenuByName("Attach To", TRUE);
				LLMenuGL* attach_hud_menu = menu.findChildMenuByName("Attach To HUD", TRUE);
				LLVOAvatar *avatarp = gAgent.getAvatarObject();
				if (attach_menu
					&& (attach_menu->getChildCount() == 0)
					&& attach_hud_menu
					&& (attach_hud_menu->getChildCount() == 0)
					&& avatarp)
				{
					for (LLVOAvatar::attachment_map_t::iterator iter = avatarp->mAttachmentPoints.begin();
						 iter != avatarp->mAttachmentPoints.end(); )
					{
						LLVOAvatar::attachment_map_t::iterator curiter = iter++;
						LLViewerJointAttachment* attachment = curiter->second;
						LLMenuItemCallGL::Params p;
						std::string submenu_name = attachment->getName();
						if (LLTrans::getString(submenu_name) != "")
						{
						    p.name = (" ")+LLTrans::getString(submenu_name)+" ";
						}
						else
						{
							p.name = submenu_name;
						}
						LLSD cbparams;
						cbparams["index"] = curiter->first;
						cbparams["label"] = attachment->getName();
						p.on_click.function_name = "Inventory.AttachObject";
						p.on_click.parameter = LLSD(attachment->getName());
						p.on_enable.function_name = "Attachment.Label";
						p.on_enable.parameter = cbparams;
						LLView* parent = attachment->getIsHUDAttachment() ? attach_hud_menu : attach_menu;
						LLUICtrlFactory::create<LLMenuItemCallGL>(p, parent);
					}
				}
			}
		}
	}
	hide_context_entries(menu, items, disabled_items);
}

BOOL LLObjectBridge::renameItem(const std::string& new_name)
{
	if(!isItemRenameable())
		return FALSE;
	LLPreview::dirty(mUUID);
	LLInventoryModel* model = getInventoryModel();
	if(!model)
		return FALSE;
	LLViewerInventoryItem* item = getItem();
	if(item && (item->getName() != new_name))
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->rename(new_name);
		buildDisplayName(new_item, mDisplayName);
		new_item->updateServer(FALSE);
		model->updateItem(new_item);

		model->notifyObservers();

		LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
		if( avatar )
		{
			LLViewerObject* obj = avatar->getWornAttachment( item->getUUID() );
			if( obj )
			{
				LLSelectMgr::getInstance()->deselectAll();
				LLSelectMgr::getInstance()->addAsIndividual( obj, SELECT_ALL_TES, FALSE );
				LLSelectMgr::getInstance()->selectionSetObjectName( new_name );
				LLSelectMgr::getInstance()->deselectAll();
			}
		}
	}
	// return FALSE because we either notified observers (& therefore
	// rebuilt) or we didn't update.
	return FALSE;
}

// +=================================================+
// |        LLLSLTextBridge                          |
// +=================================================+

LLUIImagePtr LLLSLTextBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_SCRIPT, LLInventoryType::IT_LSL, 0, FALSE);
}

void LLLSLTextBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
	/*
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLFloaterReg::showInstance("preview_script", LLSD(mUUID), TAKE_FOCUS_YES);
	}
	*/
}

// +=================================================+
// |        LLWearableBridge                         |
// +=================================================+

// *NOTE: hack to get from avatar inventory to avatar
void wear_inventory_item_on_avatar( LLInventoryItem* item )
{
	if(item)
	{
		lldebugs << "wear_inventory_item_on_avatar( " << item->getName()
				 << " )" << llendl;

		LLAppearanceManager::wearItem(item);
	}
}

void wear_add_inventory_item_on_avatar( LLInventoryItem* item )
{
	if(item)
	{
		lldebugs << "wear_add_inventory_item_on_avatar( " << item->getName()
				 << " )" << llendl;

		LLWearableList::instance().getAsset(item->getAssetUUID(),
							   item->getName(),
							   item->getType(),
							   LLWearableBridge::onWearAddOnAvatarArrived,
							   new LLUUID(item->getUUID()));
	}
}

void remove_inventory_category_from_avatar( LLInventoryCategory* category )
{
	if(!category) return;
	lldebugs << "remove_inventory_category_from_avatar( " << category->getName()
			 << " )" << llendl;


	if( gFloaterCustomize )
	{
		gFloaterCustomize->askToSaveIfDirty(
			boost::bind(remove_inventory_category_from_avatar_step2, _1, category->getUUID()));
	}
	else
	{
		remove_inventory_category_from_avatar_step2(TRUE, category->getUUID() );
	}
}

struct OnRemoveStruct
{
	LLUUID mUUID;
	OnRemoveStruct(const LLUUID& uuid):
		mUUID(uuid)
	{
	}
};

void remove_inventory_category_from_avatar_step2( BOOL proceed, LLUUID category_id)
{

	// Find all the wearables that are in the category's subtree.
	lldebugs << "remove_inventory_category_from_avatar_step2()" << llendl;
	if(proceed)
	{
		LLInventoryModel::cat_array_t cat_array;
		LLInventoryModel::item_array_t item_array;
		LLFindWearables is_wearable;
		gInventory.collectDescendentsIf(category_id,
										cat_array,
										item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_wearable);
		S32 i;
		S32 wearable_count = item_array.count();

		LLInventoryModel::cat_array_t	obj_cat_array;
		LLInventoryModel::item_array_t	obj_item_array;
		LLIsType is_object( LLAssetType::AT_OBJECT );
		gInventory.collectDescendentsIf(category_id,
										obj_cat_array,
										obj_item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_object);
		S32 obj_count = obj_item_array.count();

		// Find all gestures in this folder
		LLInventoryModel::cat_array_t	gest_cat_array;
		LLInventoryModel::item_array_t	gest_item_array;
		LLIsType is_gesture( LLAssetType::AT_GESTURE );
		gInventory.collectDescendentsIf(category_id,
										gest_cat_array,
										gest_item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_gesture);
		S32 gest_count = gest_item_array.count();

		if (wearable_count > 0)	//Loop through wearables.  If worn, remove.
		{
			for(i = 0; i  < wearable_count; ++i)
			{
				if( gAgentWearables.isWearingItem (item_array.get(i)->getUUID()) )
				{
					LLWearableList::instance().getAsset(item_array.get(i)->getAssetUUID(),
														item_array.get(i)->getName(),
														item_array.get(i)->getType(),
														LLWearableBridge::onRemoveFromAvatarArrived,
														new OnRemoveStruct(item_array.get(i)->getUUID()));

				}
			}
		}


		if (obj_count > 0)
		{
			for(i = 0; i  < obj_count; ++i)
			{
				gMessageSystem->newMessageFast(_PREHASH_DetachAttachmentIntoInv);
				gMessageSystem->nextBlockFast(_PREHASH_ObjectData );
				gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
				gMessageSystem->addUUIDFast(_PREHASH_ItemID, obj_item_array.get(i)->getUUID() );

				gMessageSystem->sendReliable( gAgent.getRegion()->getHost() );

				// this object might have been selected, so let the selection manager know it's gone now
				LLViewerObject *found_obj = gObjectList.findObject( obj_item_array.get(i)->getUUID());
				if (found_obj)
				{
					LLSelectMgr::getInstance()->remove(found_obj);
				}
				else
				{
					llwarns << "object not found, ignoring" << llendl;
				}
			}
		}

		if (gest_count > 0)
		{
			for(i = 0; i  < gest_count; ++i)
			{
				if ( LLGestureManager::instance().isGestureActive( gest_item_array.get(i)->getUUID()) )
				{
					LLGestureManager::instance().deactivateGesture( gest_item_array.get(i)->getUUID() );
					gInventory.updateItem( gest_item_array.get(i) );
					gInventory.notifyObservers();
				}

			}
		}
	}
}

BOOL LLWearableBridge::renameItem(const std::string& new_name)
{
	if( gAgentWearables.isWearingItem( mUUID ) )
	{
		gAgentWearables.setWearableName( mUUID, new_name );
	}
	return LLItemBridge::renameItem(new_name);
}

BOOL LLWearableBridge::isItemRemovable()
{
	if (gAgentWearables.isWearingItem(mUUID)) return FALSE;
	return LLInvFVBridge::isItemRemovable();
}

std::string LLWearableBridge::getLabelSuffix() const
{
	if( gAgentWearables.isWearingItem( mUUID ) )
	{
		return LLItemBridge::getLabelSuffix() + LLTrans::getString("worn");
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

LLUIImagePtr LLWearableBridge::getIcon() const
{
	return get_item_icon(mAssetType, mInvType, mWearableType, FALSE);
}

// virtual
void LLWearableBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("wear" == action)
	{
		wearOnAvatar();
	}
	else if ("wear_add" == action)
	{
		wearAddOnAvatar();
	}
	else if ("edit" == action)
	{
		editOnAvatar();
		return;
	}
	else if ("take_off" == action)
	{
		if(gAgentWearables.isWearingItem(mUUID))
		{
			LLViewerInventoryItem* item = getItem();
			if (item)
			{
				LLWearableList::instance().getAsset(item->getAssetUUID(),
													item->getName(),
													item->getType(),
													LLWearableBridge::onRemoveFromAvatarArrived,
													new OnRemoveStruct(mUUID));
			}
		}
	}
	else LLItemBridge::performAction(folder, model, action);
}

void LLWearableBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
	/*
	if( isInTrash() )
	{
		LLNotifications::instance().add("CannotWearTrash");
	}
	else if(isAgentInventory())
	{
		if( !gAgentWearables.isWearingItem( mUUID ) )
		{
			wearOnAvatar();
		}
	}
	else
	{
		// must be in the inventory library. copy it to our inventory
		// and put it on right away.
		LLViewerInventoryItem* item = getItem();
		if(item && item->isComplete())
		{
			LLPointer<LLInventoryCallback> cb = new WearOnAvatarCallback();
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
		else if(item)
		{
			// *TODO: We should fetch the item details, and then do
			// the operation above.
			LLNotifications::instance().add("CannotWearInfoNotComplete");
		}
	}
	*/
}

void LLWearableBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLWearableBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{	// FWIW, it looks like SUPPRESS_OPEN_ITEM is not set anywhere
		BOOL no_open = ((flags & SUPPRESS_OPEN_ITEM) == SUPPRESS_OPEN_ITEM);

		// If we have clothing, don't add "Open" as it's the same action as "Wear"   SL-18976
		LLViewerInventoryItem* item = getItem();
		if( !no_open && item )
		{
			no_open = (item->getType() == LLAssetType::AT_CLOTHING) ||
					  (item->getType() == LLAssetType::AT_BODYPART);
		}
		if (!no_open)
		{
			items.push_back(std::string("Open"));
		}

		if (item && item->getIsLinkType())
		{
			items.push_back(std::string("Goto Link"));
		}

		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		items.push_back(std::string("Wearable Separator"));

		items.push_back(std::string("Wearable Wear"));
		items.push_back(std::string("Wearable Add"));
		items.push_back(std::string("Wearable Edit"));

		if ((flags & FIRST_SELECTED_ITEM) == 0)
		{
			disabled_items.push_back(std::string("Wearable Edit"));
		}
		// Don't allow items to be worn if their baseobj is in the trash.
		if (isLinkedObjectInTrash())
		{
			disabled_items.push_back(std::string("Wearable Wear"));
			disabled_items.push_back(std::string("Wearable Add"));
			disabled_items.push_back(std::string("Wearable Edit"));
		}

		// Disable wear and take off based on whether the item is worn.
		if(item)
		{
			switch (item->getType())
			{
				case LLAssetType::AT_CLOTHING:
					items.push_back(std::string("Take Off"));
				case LLAssetType::AT_BODYPART:
					if (gAgentWearables.isWearingItem(item->getUUID()))
					{
						disabled_items.push_back(std::string("Wearable Wear"));
						disabled_items.push_back(std::string("Wearable Add"));
					}
					else
					{
						disabled_items.push_back(std::string("Take Off"));
					}
					break;
				default:
					break;
			}
		}
	}
	hide_context_entries(menu, items, disabled_items);
}

// Called from menus
// static
BOOL LLWearableBridge::canWearOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return FALSE;
	if(!self->isAgentInventory())
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)self->getItem();
		if(!item || !item->isComplete()) return FALSE;
	}
	return (!gAgentWearables.isWearingItem(self->mUUID));
}

// Called from menus
// static
void LLWearableBridge::onWearOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return;
	self->wearOnAvatar();
}

void LLWearableBridge::wearOnAvatar()
{
	// Don't wear anything until initial wearables are loaded, can
	// destroy clothing items.
	if (!gAgentWearables.areWearablesLoaded())
	{
		LLNotifications::instance().add("CanNotChangeAppearanceUntilLoaded");
		return;
	}

	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		if(!isAgentInventory())
		{
			LLPointer<LLInventoryCallback> cb = new WearOnAvatarCallback();
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
		else
		{
			wear_inventory_item_on_avatar(item);
		}
	}
}

void LLWearableBridge::wearAddOnAvatar()
{
	// Don't wear anything until initial wearables are loaded, can
	// destroy clothing items.
	if (!gAgentWearables.areWearablesLoaded())
	{
		LLNotifications::instance().add("CanNotChangeAppearanceUntilLoaded");
		return;
	}

	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		if(!isAgentInventory())
		{
			LLPointer<LLInventoryCallback> cb = new WearOnAvatarCallback();
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
		else
		{
			wear_add_inventory_item_on_avatar(item);
		}
	}
}

// static
void LLWearableBridge::onWearOnAvatarArrived( LLWearable* wearable, void* userdata )
{
	LLUUID* item_id = (LLUUID*) userdata;
	if(wearable)
	{
		LLViewerInventoryItem* item = NULL;
		item = (LLViewerInventoryItem*)gInventory.getItem(*item_id);
		if(item)
		{
			if(item->getAssetUUID() == wearable->getAssetID())
			{
				gAgentWearables.setWearableItem(item, wearable);
				gInventory.notifyObservers();
				//self->getFolderItem()->refreshFromRoot();
			}
			else
			{
				llinfos << "By the time wearable asset arrived, its inv item already pointed to a different asset." << llendl;
			}
		}
	}
	delete item_id;
}

// static
// BAP remove the "add" code path once everything is fully COF-ified.
void LLWearableBridge::onWearAddOnAvatarArrived( LLWearable* wearable, void* userdata )
{
	LLUUID* item_id = (LLUUID*) userdata;
	if(wearable)
	{
		LLViewerInventoryItem* item = NULL;
		item = (LLViewerInventoryItem*)gInventory.getItem(*item_id);
		if(item)
		{
			if(item->getAssetUUID() == wearable->getAssetID())
			{
				bool do_append = true;
				gAgentWearables.setWearableItem(item, wearable, do_append);
				gInventory.notifyObservers();
				//self->getFolderItem()->refreshFromRoot();
			}
			else
			{
				llinfos << "By the time wearable asset arrived, its inv item already pointed to a different asset." << llendl;
			}
		}
	}
	delete item_id;
}

// static
BOOL LLWearableBridge::canEditOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return FALSE;

	return (gAgentWearables.isWearingItem(self->mUUID));
}

// static
void LLWearableBridge::onEditOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(self)
	{
		self->editOnAvatar();
	}
}

void LLWearableBridge::editOnAvatar()
{
	const LLWearable* wearable = gAgentWearables.getWearableFromItemID(mUUID);
	if( wearable )
	{
		// Set the tab to the right wearable.
		if (gFloaterCustomize)
			gFloaterCustomize->setCurrentWearableType( wearable->getType() );

		if( CAMERA_MODE_CUSTOMIZE_AVATAR != gAgent.getCameraMode() )
		{
			// Start Avatar Customization
			gAgent.changeCameraToCustomizeAvatar();
		}
	}
}

// static
BOOL LLWearableBridge::canRemoveFromAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if( self && (LLAssetType::AT_BODYPART != self->mAssetType) )
	{
		return gAgentWearables.isWearingItem( self->mUUID );
	}
	return FALSE;
}

// static
void LLWearableBridge::onRemoveFromAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return;
	if(gAgentWearables.isWearingItem(self->mUUID))
	{
		LLViewerInventoryItem* item = self->getItem();
		if (item)
		{
			LLUUID parent_id = item->getParentUUID();
			LLWearableList::instance().getAsset(item->getAssetUUID(),
												item->getName(),
												item->getType(),
												onRemoveFromAvatarArrived,
												new OnRemoveStruct(LLUUID(self->mUUID)));
		}
	}
}

// static
void LLWearableBridge::onRemoveFromAvatarArrived(LLWearable* wearable,
												 void* userdata)
{
	OnRemoveStruct *on_remove_struct = (OnRemoveStruct*) userdata;
	const LLUUID &item_id = gInventory.getLinkedItemID(on_remove_struct->mUUID);
	if(wearable)
	{
		if( gAgentWearables.isWearingItem( item_id ) )
		{
			EWearableType type = wearable->getType();

			if( !(type==WT_SHAPE || type==WT_SKIN || type==WT_HAIR || type==WT_EYES ) ) //&&
				//!((!gAgent.isTeen()) && ( type==WT_UNDERPANTS || type==WT_UNDERSHIRT )) )
			{
				// MULTI_WEARABLE: FIXME HACK - always remove all
				bool do_remove_all = false;
				gAgentWearables.removeWearable( type, do_remove_all, 0 );
			}
		}
	}

	// Find and remove this item from the COF.
	LLInventoryModel::item_array_t items = gInventory.collectLinkedItems(item_id, LLAppearanceManager::getCOF());
	llassert(items.size() == 1); // Should always have one and only one item linked to this in the COF.
	for (LLInventoryModel::item_array_t::const_iterator iter = items.begin();
		 iter != items.end();
		 ++iter)
	{
		const LLViewerInventoryItem *linked_item = (*iter);
		const LLUUID &item_id = linked_item->getUUID();
		gInventory.purgeObject(item_id);
	}
	gInventory.notifyObservers();

	delete on_remove_struct;
}

LLInvFVBridgeAction* LLInvFVBridgeAction::createAction(LLAssetType::EType asset_type,
													   const LLUUID& uuid,LLInventoryModel* model)
{
	LLInvFVBridgeAction* action = NULL;
	switch(asset_type)
	{
	case LLAssetType::AT_TEXTURE:
		action = new LLTextureBridgeAction(uuid,model);
		break;

	case LLAssetType::AT_SOUND:
		action = new LLSoundBridgeAction(uuid,model);
		break;

	case LLAssetType::AT_LANDMARK:
		action = new LLLandmarkBridgeAction(uuid,model);
		break;

	case LLAssetType::AT_CALLINGCARD:
		action = new LLCallingCardBridgeAction(uuid,model);
		break;

	case LLAssetType::AT_OBJECT:
		action = new LLObjectBridgeAction(uuid,model);
		break;

	case LLAssetType::AT_NOTECARD:
		action = new LLNotecardBridgeAction(uuid,model);
		break;

	case LLAssetType::AT_ANIMATION:
		action = new LLAnimationBridgeAction(uuid,model);
		break;

	case LLAssetType::AT_GESTURE:
		action = new LLGestureBridgeAction(uuid,model);
		break;

	case LLAssetType::AT_LSL_TEXT:
		action = new LLLSLTextBridgeAction(uuid,model);
		break;

	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_BODYPART:
		action = new LLWearableBridgeAction(uuid,model);

		break;

	default:
		break;
	}
	return action;
}

//static
void LLInvFVBridgeAction::doAction(LLAssetType::EType asset_type,
								   const LLUUID& uuid,LLInventoryModel* model)
{
	LLInvFVBridgeAction* action = createAction(asset_type,uuid,model);
	if(action)
	{
		action->doIt();
		delete action;
	}
}

//static
void LLInvFVBridgeAction::doAction(const LLUUID& uuid, LLInventoryModel* model)
{
	LLAssetType::EType asset_type = model->getItem(uuid)->getType();
	LLInvFVBridgeAction* action = createAction(asset_type,uuid,model);
	if(action)
	{
		action->doIt();
		delete action;
	}
}

LLViewerInventoryItem* LLInvFVBridgeAction::getItem() const
{
	if(mModel)
		return (LLViewerInventoryItem*)mModel->getItem(mUUID);
	return NULL;
}

//virtual
void	LLTextureBridgeAction::doIt()
{
	if (getItem())
	{
		LLFloaterReg::showInstance("preview_texture", LLSD(mUUID), TAKE_FOCUS_YES);
	}

	LLInvFVBridgeAction::doIt();
}

//virtual
void	LLSoundBridgeAction::doIt()
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		LLFloaterReg::showInstance("preview_sound", LLSD(mUUID), TAKE_FOCUS_YES);
	}

	LLInvFVBridgeAction::doIt();
}


//virtual
void	LLLandmarkBridgeAction::doIt()
{
	LLViewerInventoryItem* item = getItem();
	if( item )
	{
		// Opening (double-clicking) a landmark immediately teleports,
		// but warns you the first time.
		LLSD payload;
		payload["asset_id"] = item->getAssetUUID();
		LLNotifications::instance().add("TeleportFromLandmark", LLSD(), payload);
	}

	LLInvFVBridgeAction::doIt();
}


//virtual
void	LLCallingCardBridgeAction::doIt()
{
	LLViewerInventoryItem* item = getItem();
	if(item && item->getCreatorUUID().notNull())
	{
		LLAvatarActions::showProfile(item->getCreatorUUID());
	}

	LLInvFVBridgeAction::doIt();
}

//virtual
void
LLNotecardBridgeAction::doIt()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLFloaterReg::showInstance("preview_notecard", LLSD(item->getUUID()), TAKE_FOCUS_YES);
	}

	LLInvFVBridgeAction::doIt();
}

//virtual
void	LLGestureBridgeAction::doIt()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLPreviewGesture* preview = LLPreviewGesture::show(mUUID, LLUUID::null);
		preview->setFocus(TRUE);
	}

	LLInvFVBridgeAction::doIt();
}

//virtual
void	LLAnimationBridgeAction::doIt()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLFloaterReg::showInstance("preview_anim", LLSD(mUUID), TAKE_FOCUS_YES);
	}

	LLInvFVBridgeAction::doIt();
}


//virtual
void	LLObjectBridgeAction::doIt()
{
	LLFloaterReg::showInstance("properties", mUUID);

	LLInvFVBridgeAction::doIt();
}


//virtual
void	LLLSLTextBridgeAction::doIt()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLFloaterReg::showInstance("preview_script", LLSD(mUUID), TAKE_FOCUS_YES);
	}

	LLInvFVBridgeAction::doIt();
}


BOOL LLWearableBridgeAction::isInTrash() const
{
	if(!mModel) return FALSE;
	const LLUUID trash_id = mModel->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	return mModel->isObjectDescendentOf(mUUID, trash_id);
}

BOOL LLWearableBridgeAction::isAgentInventory() const
{
	if(!mModel) return FALSE;
	if(gInventory.getRootFolderID() == mUUID) return TRUE;
	return mModel->isObjectDescendentOf(mUUID, gInventory.getRootFolderID());
}

void LLWearableBridgeAction::wearOnAvatar()
{
	// Don't wear anything until initial wearables are loaded, can
	// destroy clothing items.
	if (!gAgentWearables.areWearablesLoaded())
	{
		LLNotifications::instance().add("CanNotChangeAppearanceUntilLoaded");
		return;
	}

	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		if(!isAgentInventory())
		{
			LLPointer<LLInventoryCallback> cb = new WearOnAvatarCallback();
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
		else
		{
			wear_inventory_item_on_avatar(item);
		}
	}
}

//virtual
void LLWearableBridgeAction::doIt()
{
	if(isInTrash())
	{
		LLNotifications::instance().add("CannotWearTrash");
	}
	else if(isAgentInventory())
	{
		if(!gAgentWearables.isWearingItem(mUUID))
		{
			wearOnAvatar();
		}
	}
	else
	{
		// must be in the inventory library. copy it to our inventory
		// and put it on right away.
		LLViewerInventoryItem* item = getItem();
		if(item && item->isComplete())
		{
			LLPointer<LLInventoryCallback> cb = new WearOnAvatarCallback();
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
		else if(item)
		{
			// *TODO: We should fetch the item details, and then do
			// the operation above.
			LLNotifications::instance().add("CannotWearInfoNotComplete");
		}
	}

	LLInvFVBridgeAction::doIt();
}

// +=================================================+
// |        LLLinkItemBridge                         |
// +=================================================+
// For broken links

std::string LLLinkItemBridge::sPrefix("Link: ");


LLUIImagePtr LLLinkItemBridge::getIcon() const
{
	if (LLViewerInventoryItem *item = getItem())
	{
		return get_item_icon(item->getActualType(), LLInventoryType::IT_NONE, 0, FALSE);
	}
	return get_item_icon(LLAssetType::AT_LINK, LLInventoryType::IT_NONE, 0, FALSE);
}

void LLLinkItemBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	lldebugs << "LLLink::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Delete"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Delete"));
		}
	}
	hide_context_entries(menu, items, disabled_items);
}


// +=================================================+
// |        LLLinkBridge                             |
// +=================================================+
// For broken links.

std::string LLLinkFolderBridge::sPrefix("Link: ");


LLUIImagePtr LLLinkFolderBridge::getIcon() const
{
	LLFolderType::EType preferred_type = LLFolderType::FT_NONE;
	if (LLViewerInventoryItem *item = getItem())
	{
		if (const LLViewerInventoryCategory* cat = item->getLinkedCategory())
		{
			preferred_type = cat->getPreferredType();
		}
	}
	return LLFolderBridge::getIcon(preferred_type);
}

void LLLinkFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	lldebugs << "LLLink::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Goto Link"));
		items.push_back(std::string("Delete"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Delete"));
		}
	}
	hide_context_entries(menu, items, disabled_items);
}

void LLLinkFolderBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("goto" == action)
	{
		gotoItem(folder);
		return;
	}
	LLItemBridge::performAction(folder,model,action);
}

void LLLinkFolderBridge::gotoItem(LLFolderView *folder)
{
	const LLUUID &cat_uuid = getFolderID();
	if (!cat_uuid.isNull())
	{
		if (LLFolderViewItem *base_folder = folder->getItemByID(cat_uuid))
		{
			if (LLInventoryModel* model = getInventoryModel())
			{
				model->fetchDescendentsOf(cat_uuid);
			}
			base_folder->setOpen(TRUE);
			folder->setSelectionFromRoot(base_folder,TRUE);
			folder->scrollToShowSelection();
		}
	}
}

const LLUUID &LLLinkFolderBridge::getFolderID() const
{
	if (LLViewerInventoryItem *link_item = getItem())
	{
		if (const LLViewerInventoryCategory *cat = link_item->getLinkedCategory())
		{
			const LLUUID& cat_uuid = cat->getUUID();
			return cat_uuid;
		}
	}
	return LLUUID::null;
}
