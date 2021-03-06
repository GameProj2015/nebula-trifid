//------------------------------------------------------------------------------
//  entitytreewidget.cc
//  (C) 2013-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "entitytreewidget.h"
#include "leveleditor2entitymanager.h"
#include "properties/editorproperty.h"
#include "qevent.h"
#include "qstandarditemmodel.h"
#include "managers/entitymanager.h"
#include "leveleditor2protocol.h"
#include "leveleditor2app.h"
#include "QInputDialog"
#include "editorfeatures/editorblueprintmanager.h"

namespace LevelEditor2
{

//------------------------------------------------------------------------------
/**
*/
EntityTreeItem::~EntityTreeItem()
{
	LevelEditor2App::Instance()->GetWindow()->GetEntityTreeWidget()->Deregister(this->entityGuid);	
}

//------------------------------------------------------------------------------
/**
*/
void
EntityTreeItem::SetIcon(const EntityType& type)
{	
	switch (type)
	{

		case Environment:
		{
			QIcon icon;
			icon.addFile(QString::fromUtf8(":/icons/icons/Environment.png"), QSize(), QIcon::Normal, QIcon::Off);
			this->setIcon(0, icon);
		}
		break;
		case Game:
		{
			QIcon icon;
			icon.addFile(QString::fromUtf8(":/icons/icons/Game.png"), QSize(), QIcon::Normal, QIcon::Off);
			this->setIcon(0, icon);
		}	
		break;
		case Light:
		{
			QIcon icon;
			icon.addFile(QString::fromUtf8(":/icons/icons/Light.png"), QSize(), QIcon::Normal, QIcon::Off);
			this->setIcon(0, icon);
		}
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
EntityTreeWidget::EntityTreeWidget(QWidget* parent) :
	QTreeWidget(parent),
	blockSignal(false)
{
	this->setDragEnabled(true);
	this->setAcceptDrops(true);	
	this->setDragDropMode(QAbstractItemView::DragDrop );
	this->setDefaultDropAction(Qt::MoveAction);
	this->viewport()->setAcceptDrops(true);
	this->setDropIndicatorShown(true);	
}

//------------------------------------------------------------------------------
/**
*/
EntityTreeWidget::~EntityTreeWidget()
{	
	this->editorState = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
EntityTreeWidget::AddEntityTreeItem(EntityTreeItem* item)
{
	n_assert(!this->itemDictionary.Contains(item->GetEntityGuid()));
	this->itemDictionary.Add(item->GetEntityGuid(), item);	

	// find group node
	Ptr<Game::Entity> ent = LevelEditor2EntityManager::Instance()->GetEntityById(item->GetEntityGuid());
	EntityType type = (EntityType) ent->GetInt(Attr::EntityType);
	item->SetIcon(type);

	// add item last
	int index = this->topLevelItemCount();
	this->insertTopLevelItem(index, item);
}

//------------------------------------------------------------------------------
/**
*/
void 
EntityTreeWidget::RemoveEntityTreeItem(EntityGuid id)
{
	if(this->itemDictionary.Contains(id))
	{
		EntityTreeItem* item = this->itemDictionary[id];
		this->blockSignal = true;
		delete item;	
		this->blockSignal = false;
	}		
}

//------------------------------------------------------------------------------
/**
*/
void 
EntityTreeWidget::Deregister(EntityGuid id)
{
	if(this->itemDictionary.Contains(id))
	{
		this->itemDictionary.Erase(id);			
	}	
}

//------------------------------------------------------------------------------
/**
*/
void 
EntityTreeWidget::AppendToSelection(EntityGuid id)
{
	n_assert(this->itemDictionary.Contains(id));

	// doesnt work. weird
	this->blockSignals(true);
	this->blockSignal = true;
	this->itemDictionary[id]->setSelected(true);
	this->blockSignal = false;
	this->blockSignals(false);	
}

//------------------------------------------------------------------------------
/**
*/
void 
EntityTreeWidget::RemoveFromSelection(EntityGuid id)
{
	n_assert(this->itemDictionary.Contains(id));
	this->blockSignals(true);
	this->blockSignal = true;
	this->itemDictionary[id]->setSelected(false);
	this->blockSignal = false;
	this->blockSignals(false);
}

//------------------------------------------------------------------------------
/**
*/
void 
EntityTreeWidget::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{

	QTreeView::selectionChanged(selected, deselected);
		
	if(this->blockSignal)
	{
		return;
	}
	// get selected items from tree
	QList<QTreeWidgetItem *> items = this->selectedItems();

	// add items to a new array
	Util::Array<EntityGuid> entities;
	for(int idx = 0;idx<items.size();idx++)
	{
		entities.Append(dynamic_cast<EntityTreeItem*>(items[idx])->GetEntityGuid());
	}
	// add entities in a big chunk
	if(entities.Size())
	{
		Ptr<SelectAction> action = SelectAction::Create();
		action->SetSelectionMode(SelectAction::SetSelection);
		action->SetEntities(entities);
		ActionManager::Instance()->PerformAction(action.cast<Action>());
	}		
	else
	{
		Ptr<SelectAction> action = SelectAction::Create();
		action->SetSelectionMode(SelectAction::ClearSelection);						
		ActionManager::Instance()->PerformAction(action.cast<Action>());
	}	
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<EntityGuid>
EntityTreeWidget::SelectRecursive(EntityTreeItem* item)
{
	Util::Array<EntityGuid> guids;
	guids.Append(dynamic_cast<EntityTreeItem*>(item)->GetEntityGuid());
	for(int i=0;i<item->childCount();i++)
	{
		guids.AppendArray(SelectRecursive(dynamic_cast<EntityTreeItem*>(item->child(i))));
	}
	return guids;
}

//------------------------------------------------------------------------------
/**
*/
void 
EntityTreeWidget::SelectAllChildren()
{	
	QList<QTreeWidgetItem *> items = this->selectedItems();
	Util::Array<EntityGuid> guids;
	for(int i=0;i<items.size();i++)
	{
		guids.AppendArray(SelectRecursive(dynamic_cast<EntityTreeItem*>(items[i])));
	}
	Ptr<SelectAction> action = SelectAction::Create();
	action->SetSelectionMode(SelectAction::SetSelection);
	action->SetEntities(guids);
	ActionManager::Instance()->PerformAction(action.cast<Action>());
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Ptr<Game::Entity>>  
EntityTreeWidget::GetDirectChildren(const EntityGuid &id)
{
	EntityTreeItem * item = this->GetEntityTreeItem(id);
	Util::Array<Ptr<Game::Entity>> children;
	for(int i=0 ; i < item->childCount();i++)
	{
		children.Append(LevelEditor2EntityManager::Instance()->GetEntityById(dynamic_cast<EntityTreeItem*>(item->child(i))->GetEntityGuid()));
	}
	return children;

}

//------------------------------------------------------------------------------
/**
*/
Qt::DropActions 
EntityTreeWidget::supportedDropActions() const
{	
	return Qt::MoveAction | Qt::CopyAction;
}


//------------------------------------------------------------------------------
/**
*/
void 
EntityTreeWidget::dropEvent ( QDropEvent * event )
{
	QTreeWidget::dropEvent(event);
	SetParentGuids();
}

//------------------------------------------------------------------------------
/**
*/
void
EntityTreeWidget::contextMenuEvent(QContextMenuEvent * event)
{
	EntityTreeItem * myitem = (EntityTreeItem*)this->itemAt(event->pos());
	if (myitem == NULL)
	{
		return;
	}
	Ptr<Game::Entity> entity = LevelEditor2EntityManager::Instance()->GetEntityById(myitem->GetEntityGuid());
	Util::String oldcategory = entity->GetString(Attr::EntityCategory);
	// we dont support morphing light sources
	if (oldcategory == "Light")
	{
		return;
	}

	
	QMenu menu;
	QAction* morphAction = menu.addAction("Morph entity");
	QAction* morphToEnvironment = menu.addAction("Morph to model");
	if (entity->GetInt(Attr::EntityType) != Game)
	{
		morphToEnvironment->setDisabled(true);
	}
	QAction* selected = menu.exec(event->globalPos());
	if (selected == morphAction)
	{			
		Ptr<Toolkit::EditorBlueprintManager> manager = Toolkit::EditorBlueprintManager::Instance();
		QStringList items;
		int selected = 0;
		for (int i = 0; i < manager->GetNumCategories();i++)
		{ 
			Util::String cat = manager->GetCategoryByIndex(i);
			if (cat == oldcategory)
			{
				selected = i;
			}
			items.append(cat.AsCharPtr());
		}
		bool ok;
		QString target = QInputDialog::getItem(this, "Select new blueprint", "Blueprint", items, selected, false, &ok);
		if (ok)
		{			
			LevelEditor2EntityManager::Instance()->MorphEntity(entity, target.toLatin1().constData());
		}
	}
	else if (selected == morphToEnvironment)
	{
		LevelEditor2EntityManager::Instance()->MorphEntity(entity, "_Environment");
	}
	myitem->SetIcon((EntityType)entity->GetInt(Attr::EntityType));
}

//------------------------------------------------------------------------------
/**
*/
QMimeData*
EntityTreeWidget::mimeData(const QList<QTreeWidgetItem*> items) const
{
	// we always use single select, so the amount of items we get can be at most 1
	QTreeWidgetItem* item = items[0];

	// it has to be a base item
	EntityTreeItem* baseItem = dynamic_cast<EntityTreeItem*>(item);

	if (baseItem)
	{
		// create mimedata
		QMimeData* mimeData = QTreeWidget::mimeData(items);
		const Ptr<Game::Entity>& entity = LevelEditor2EntityManager::Instance()->GetEntityById(baseItem->GetEntityGuid());
		if (entity->HasAttr(Attr::Graphics))
		{
			Util::String res = entity->GetString(Attr::Graphics);
			if (res.FindStringIndex("system/") == InvalidIndex)
			{
				mimeData->setData("nebula/resourceid", res.AsCharPtr());				
				return mimeData;
			}
		}				
	}
	// if that fails, return 0
	return NULL;
}

//------------------------------------------------------------------------------
/**
*/
Util::Guid 
EntityTreeWidget::GetParent(const Util::Guid & guid)
{
	Util::Guid parent;
	if(this->itemDictionary.Contains(guid))
	{
		EntityTreeItem * item = this->itemDictionary[guid];
		EntityTreeItem * parentItem = dynamic_cast<EntityTreeItem*>(item->parent());
		if(parentItem != NULL)
		{				
			parent = parentItem->GetEntityGuid();
		}
	}	
	return parent;
}

//------------------------------------------------------------------------------
/**
*/
void 
EntityTreeWidget::SetParentGuids()
{
	Util::Guid invalid;
	for(int i = 0 ; i<this->topLevelItemCount(); i++)
	{
		this->SetParentGuid(invalid, dynamic_cast<EntityTreeItem*>(this->topLevelItem(i)));
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
EntityTreeWidget::SetParentGuid(const Util::Guid & guid, EntityTreeItem * child)
{
	Ptr<Game::Entity> entity = LevelEditor2EntityManager::Instance()->GetEntityById(child->GetEntityGuid());
	entity->SetGuid(Attr::ParentGuid, guid);
	Util::Guid itemGuid = entity->GetGuid(Attr::EntityGuid);
	for(int i=0 ; i < child->childCount();i++)
	{
		this->SetParentGuid(itemGuid, dynamic_cast<EntityTreeItem*>(child->child(i)));		
	}

}

//------------------------------------------------------------------------------
/**
*/
void 
EntityTreeWidget::RebuildTree()
{	
	const Util::Array<Ptr<Game::Entity>> entities = BaseGameFeature::EntityManager::Instance()->GetEntities();	
	for(int i = 0 ; i < entities.Size() ; i++)
	{						
		if(entities[i]->HasAttr(Attr::EntityType) && !BaseGameFeature::EntityManager::Instance()->IsEntityInDelayedJobs(entities[i]))
		{
			Util::Guid parentGuid = entities[i]->GetGuid(Attr::ParentGuid);
			if(parentGuid.IsValid())
			{
				Ptr<Game::Entity> parent = BaseGameFeature::EntityManager::Instance()->GetEntityByAttr(Attr::Attribute(Attr::EntityGuid,parentGuid));
				if(parent.isvalid())
				{
					EntityTreeItem* treeItem = dynamic_cast<EntityTreeItem*>(this->GetEntityTreeItem(entities[i]->GetGuid(Attr::EntityGuid)));				
					EntityTreeItem* parentTreeItem = dynamic_cast<EntityTreeItem*>(this->GetEntityTreeItem(parent->GetGuid(Attr::EntityGuid)));								
					int idx = this->indexOfTopLevelItem (treeItem);
					this->takeTopLevelItem(idx);
					parentTreeItem->addChild(treeItem);
				}
			}		
		}		
	}
}

} // namespace LevelEditor2
//------------------------------------------------------------------------------