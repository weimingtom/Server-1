/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2006  EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "../common/debug.h"
#include "../common/MiscFunctions.h"
#include "../common/features.h"
#include "QuestParserCollection.h"
#include "QuestInterface.h"
#include "zone.h"
#include "questmgr.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

extern Zone* zone;

QuestParserCollection::QuestParserCollection() {
	_player_quest_status = QuestUnloaded;
	_global_player_quest_status = QuestUnloaded;
	_global_npc_quest_status = QuestUnloaded;
}

QuestParserCollection::~QuestParserCollection() {
}

void QuestParserCollection::RegisterQuestInterface(QuestInterface *qi, std::string ext) {
	_interfaces[qi->GetIdentifier()] = qi;
	_extensions[qi->GetIdentifier()] = ext;
	_load_precedence.push_back(qi);
}

void QuestParserCollection::AddVar(std::string name, std::string val) {
	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		(*iter)->AddVar(name, val);
		iter++;
	}
}

void QuestParserCollection::ReloadQuests(bool reset_timers) {
	if(reset_timers) {
		quest_manager.ClearAllTimers();
	}

	_npc_quest_status.clear();
	_player_quest_status = QuestUnloaded;
	_global_player_quest_status = QuestUnloaded;
	_global_npc_quest_status = QuestUnloaded;
	_spell_quest_status.clear();
	_item_quest_status.clear();
	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		(*iter)->ReloadQuests();
		iter++;
	}
}

bool QuestParserCollection::HasQuestSub(uint32 npcid, const char *subname) {
	return HasQuestSubLocal(npcid, subname) || HasQuestSubGlobal(subname);
}

bool QuestParserCollection::HasQuestSubLocal(uint32 npcid, const char *subname) {
	std::map<uint32, uint32>::iterator iter = _npc_quest_status.find(npcid);
	
	if(iter != _npc_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			if(qiter->second->HasQuestSub(npcid, subname)) {
				return true;
			}
		}
	} else {
		std::string filename;
		QuestInterface *qi = GetQIByNPCQuest(npcid, filename);
		if(qi) {
			_npc_quest_status[npcid] = qi->GetIdentifier();

			qi->LoadNPCScript(filename, npcid);
			if(qi->HasQuestSub(npcid, subname)) {
				return true;
			}
		} else {
			_npc_quest_status[npcid] = QuestFailedToLoad;
		}
	}
	return false;
}

bool QuestParserCollection::HasQuestSubGlobal(const char *subname) {
	if(_global_npc_quest_status == QuestUnloaded) {
		std::string filename;
		QuestInterface *qi = GetQIByGlobalNPCQuest(filename);
		if(qi) {
			qi->LoadGlobalNPCScript(filename);
			_global_npc_quest_status = qi->GetIdentifier();
			if(qi->HasGlobalQuestSub(subname)) {
				return true;
			}
		}
	} else {
		if(_global_npc_quest_status != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(_global_npc_quest_status);
			if(qiter->second->HasGlobalQuestSub(subname)) {
				return true;
			}
		}
	}
	return false;
}

bool QuestParserCollection::PlayerHasQuestSub(const char *subname) {
	return PlayerHasQuestSubLocal(subname) || PlayerHasQuestSubGlobal(subname);
}

bool QuestParserCollection::PlayerHasQuestSubLocal(const char *subname) {
	if(_player_quest_status == QuestUnloaded) {
		std::string filename;	
		QuestInterface *qi = GetQIByPlayerQuest(filename);
		if(qi) {
			_player_quest_status = qi->GetIdentifier();
			qi->LoadPlayerScript(filename);
			return qi->PlayerHasQuestSub(subname);
		}
	} else if(_player_quest_status != QuestFailedToLoad) {
		std::map<uint32, QuestInterface*>::iterator iter = _interfaces.find(_player_quest_status);
		return iter->second->PlayerHasQuestSub(subname);
	}
	return false;
}

bool QuestParserCollection::PlayerHasQuestSubGlobal(const char *subname) {
	if(_global_player_quest_status == QuestUnloaded) {
		std::string filename;	
		QuestInterface *qi = GetQIByPlayerQuest(filename);
		if(qi) {
			_global_player_quest_status = qi->GetIdentifier();
			qi->LoadPlayerScript(filename);
			return qi->GlobalPlayerHasQuestSub(subname);
		}
	} else if(_global_player_quest_status != QuestFailedToLoad) {
		std::map<uint32, QuestInterface*>::iterator iter = _interfaces.find(_global_player_quest_status);
		return iter->second->GlobalPlayerHasQuestSub(subname);
	}
	return false;
}

bool QuestParserCollection::SpellHasQuestSub(uint32 spell_id, const char *subname) {
	std::map<uint32, uint32>::iterator iter = _spell_quest_status.find(spell_id);
	if(iter != _spell_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			return qiter->second->SpellHasQuestSub(spell_id, subname);
		}
	} else {
		std::string filename;
		QuestInterface *qi = GetQIBySpellQuest(spell_id, filename);
		if(qi) {
			_spell_quest_status[spell_id] = qi->GetIdentifier();
			qi->LoadSpellScript(filename, spell_id);
			return qi->SpellHasQuestSub(spell_id, subname);
		} else {
			_spell_quest_status[spell_id] = QuestFailedToLoad;
		}
	}
	return false;
}

bool QuestParserCollection::ItemHasQuestSub(ItemInst *itm, const char *subname) {
	std::string item_script;
	if(strcmp("EVENT_SCALE_CALC", subname) == 0 || strcmp("EVENT_ITEM_ENTERZONE", subname) == 0) {
		item_script = itm->GetItem()->CharmFile;
	} else if(strcmp("EVENT_ITEM_CLICK", subname) == 0 || strcmp("EVENT_ITEM_CLICK_CAST", subname) == 0) {
		item_script = "script_";
		item_script += itoa(itm->GetItem()->ScriptFileID);
	} else {
		item_script = itoa(itm->GetItem()->ID);
	}

	std::map<std::string, uint32>::iterator iter = _item_quest_status.find(item_script);
	if(iter != _item_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			return qiter->second->ItemHasQuestSub(itm, subname);
		}
	} else {
		std::string filename;
		QuestInterface *qi = GetQIByItemQuest(item_script, filename);
		if(qi) {
			_item_quest_status[item_script] = qi->GetIdentifier();
			qi->LoadItemScript(filename, item_script);
			return qi->ItemHasQuestSub(itm, subname);
		} else {
			_item_quest_status[item_script] = QuestFailedToLoad;
		}
	}
	return false;
}

void QuestParserCollection::EventNPC(QuestEventID evt, NPC *npc, Mob *init, std::string data, uint32 extra_data) {
	EventNPCLocal(evt, npc, init, data, extra_data);
	EventNPCGlobal(evt, npc, init, data, extra_data);
}

void QuestParserCollection::EventNPCLocal(QuestEventID evt, NPC* npc, Mob *init, std::string data, uint32 extra_data) {
	std::map<uint32, uint32>::iterator iter = _npc_quest_status.find(npc->GetNPCTypeID());
	if(iter != _npc_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			qiter->second->EventNPC(evt, npc, init, data, extra_data);
		}
	} else {
		std::string filename;
		QuestInterface *qi = GetQIByNPCQuest(npc->GetNPCTypeID(), filename);
		if(qi) {
			_npc_quest_status[npc->GetNPCTypeID()] = qi->GetIdentifier();
			qi->LoadNPCScript(filename, npc->GetNPCTypeID());
			qi->EventNPC(evt, npc, init, data, extra_data);
		} else {
			_npc_quest_status[npc->GetNPCTypeID()] = QuestFailedToLoad;
		}
	}
}

void QuestParserCollection::EventNPCGlobal(QuestEventID evt, NPC* npc, Mob *init, std::string data, uint32 extra_data) {
	if(_global_npc_quest_status != QuestUnloaded && _global_npc_quest_status != QuestFailedToLoad) {
		std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(_global_npc_quest_status);
		qiter->second->EventGlobalNPC(evt, npc, init, data, extra_data);
	} else {
		std::string filename;
		QuestInterface *qi = GetQIByGlobalNPCQuest(filename);
		if(qi) {
			_global_npc_quest_status = qi->GetIdentifier();
			qi->LoadGlobalNPCScript(filename);
			qi->EventGlobalNPC(evt, npc, init, data, extra_data);
		} else {
			_global_npc_quest_status = QuestFailedToLoad;
		}
	}
}

void QuestParserCollection::EventPlayer(QuestEventID evt, Client *client, std::string data, uint32 extra_data) {
	EventPlayerLocal(evt, client, data, extra_data);
	EventPlayerGlobal(evt, client, data, extra_data);
}

void QuestParserCollection::EventPlayerLocal(QuestEventID evt, Client *client, std::string data, uint32 extra_data) {
	if(_player_quest_status == QuestUnloaded) {
		std::string filename;
		QuestInterface *qi = GetQIByPlayerQuest(filename);
		if(qi) {
			_player_quest_status = qi->GetIdentifier();
			qi->LoadPlayerScript(filename);
			qi->EventPlayer(evt, client, data, extra_data);
		}
	} else { 
		if(_player_quest_status != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator iter = _interfaces.find(_player_quest_status);
			iter->second->EventPlayer(evt, client, data, extra_data);
		}
	}
}

void QuestParserCollection::EventPlayerGlobal(QuestEventID evt, Client *client, std::string data, uint32 extra_data) {
	if(_global_player_quest_status == QuestUnloaded) {
		std::string filename;
		QuestInterface *qi = GetQIByGlobalPlayerQuest(filename);
		if(qi) {
			_global_player_quest_status = qi->GetIdentifier();
			qi->LoadGlobalPlayerScript(filename);
			qi->EventGlobalPlayer(evt, client, data, extra_data);
		}
	} else { 
		if(_global_player_quest_status != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator iter = _interfaces.find(_global_player_quest_status);
			iter->second->EventGlobalPlayer(evt, client, data, extra_data);
		}
	}
}

void QuestParserCollection::EventItem(QuestEventID evt, Client *client, ItemInst *item, uint32 objid, uint32 extra_data) {
	std::string item_script;
	if(evt == EVENT_SCALE_CALC || evt == EVENT_ITEM_ENTERZONE) {
		item_script = item->GetItem()->CharmFile;
	} else if(evt == EVENT_ITEM_CLICK || evt == EVENT_ITEM_CLICK_CAST) {
		item_script = "script_";
		item_script += itoa(item->GetItem()->ScriptFileID);
	} else {
		item_script = itoa(item->GetItem()->ID);
	}

	std::map<std::string, uint32>::iterator iter = _item_quest_status.find(item_script);
	if(iter != _item_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			qiter->second->EventItem(evt, client, item, objid, extra_data);
		}
	} else {
		std::string filename;
		QuestInterface *qi = GetQIByItemQuest(item_script, filename);
		if(qi) {
			_item_quest_status[item_script] = qi->GetIdentifier();
			qi->LoadItemScript(filename, item_script);
			qi->EventItem(evt, client, item, objid, extra_data);
		} else {
			_item_quest_status[item_script] = QuestFailedToLoad;
		}
	}
}

void QuestParserCollection::EventSpell(QuestEventID evt, NPC* npc, Client *client, uint32 spell_id, uint32 extra_data) {
	std::map<uint32, uint32>::iterator iter = _spell_quest_status.find(spell_id);
	if(iter != _spell_quest_status.end()) {
		//loaded or failed to load
		if(iter->second != QuestFailedToLoad) {
			std::map<uint32, QuestInterface*>::iterator qiter = _interfaces.find(iter->second);
			qiter->second->EventSpell(evt, npc, client, spell_id, extra_data);
		}
	} else {
		std::string filename;
		QuestInterface *qi = GetQIBySpellQuest(spell_id, filename);
		if(qi) {
			_spell_quest_status[spell_id] = qi->GetIdentifier();
			qi->LoadSpellScript(filename, spell_id);
			qi->EventSpell(evt, npc, client, spell_id, extra_data);
		} else {
			_spell_quest_status[spell_id] = QuestFailedToLoad;
		}
	}
}

QuestInterface *QuestParserCollection::GetQIByNPCQuest(uint32 npcid, std::string &filename) {
	//first look for /quests/zone/npcid.ext (precedence)
	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/";
	filename += itoa(npcid);
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//second look for /quests/zone/npcname.ext (precedence)
	const NPCType *npc_type = database.GetNPCType(npcid);
	if(!npc_type) {
		return nullptr;
	}
	std::string npc_name = npc_type->name;
	int sz = static_cast<int>(npc_name.length());
	for(int i = 0; i < sz; ++i) {
		if(npc_name[i] == '`') {
			npc_name[i] = '-';
		}
	}

	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/";
	filename += npc_name;

	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//third look for /quests/templates/npcid.ext (precedence)
	filename = "quests/";
	filename += QUEST_TEMPLATES_DIRECTORY;
	filename += "/";
	filename += itoa(npcid);
	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//fourth look for /quests/templates/npcname.ext (precedence)
	filename = "quests/";
	filename += QUEST_TEMPLATES_DIRECTORY;
	filename += "/";
	filename += npc_name;
	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//fifth look for /quests/zone/default.ext (precedence)
	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/";
	filename += "default";
	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//last look for /quests/templates/default.ext (precedence)
	filename = "quests/";
	filename += QUEST_TEMPLATES_DIRECTORY;
	filename += "/";
	filename += "default";
	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	return nullptr;
}

QuestInterface *QuestParserCollection::GetQIByPlayerQuest(std::string &filename) {

	if(!zone)
	return nullptr;

	//first look for /quests/zone/player_v[instance_version].ext (precedence)
	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/";
	filename += "player_v";
	filename += itoa(zone->GetInstanceVersion());
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}	

	//second look for /quests/zone/player.ext (precedence)
	filename = "quests/";
	filename += zone->GetShortName();
	filename += "/";
	filename += "player";

	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	//third look for /quests/templates/player.ext (precedence)
	filename = "quests/";
	filename += QUEST_TEMPLATES_DIRECTORY;
	filename += "/";
	filename += "player";
	iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	return nullptr;
}

QuestInterface *QuestParserCollection::GetQIByGlobalNPCQuest(std::string &filename) {
	// simply look for templates/global_npc.pl
	filename = "quests/";
	filename += QUEST_TEMPLATES_DIRECTORY;
	filename += "/";
	filename += "global_npc";
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	return nullptr;
}

QuestInterface *QuestParserCollection::GetQIByGlobalPlayerQuest(std::string &filename) {
	//first look for /quests/templates/player.ext (precedence)
	filename = "quests/";
	filename += QUEST_TEMPLATES_DIRECTORY;
	filename += "/";
	filename += "global_player";
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	return nullptr;
}

QuestInterface *QuestParserCollection::GetQIBySpellQuest(uint32 spell_id, std::string &filename) {
	//first look for /quests/spells/spell_id.ext (precedence)
	filename = "quests/spells/";
	filename += itoa(spell_id);
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}	

	return nullptr;
}

QuestInterface *QuestParserCollection::GetQIByItemQuest(std::string item_script, std::string &filename) {
	//first look for /quests/items/item_script.ext (precedence)
	filename = "quests/items/";
	filename += item_script;
	std::string tmp;
	FILE *f = nullptr;

	std::list<QuestInterface*>::iterator iter = _load_precedence.begin();
	while(iter != _load_precedence.end()) {
		tmp = filename;
		std::map<uint32, std::string>::iterator ext = _extensions.find((*iter)->GetIdentifier());
		tmp += ".";
		tmp += ext->second;
		f = fopen(tmp.c_str(), "r");
		if(f) {
			fclose(f);
			filename = tmp;
			return (*iter);
		}

		iter++;
	}

	return nullptr;
}
