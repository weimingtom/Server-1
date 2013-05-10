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

#ifndef EQEMU_EMBPARSER_H
#define EQMEU_EMBPARSER_H
#ifdef EMBPERL

#include "QuestParserCollection.h"
#include "QuestInterface.h"
#include <string>
#include <queue>
#include <map>
#include "embperl.h"

class ItemInst;
class Mob;
class Client;
class NPC;

typedef enum 
{
	questUnloaded,
	questLoaded,
	questFailedToLoad
} PerlQuestStatus;

class PerlembParser : public QuestInterface {
	typedef struct {
		QuestEventID event;
		uint32 objid;
		std::string data;
		NPC* npcmob;
		ItemInst* iteminst;
		Mob* mob;
		uint32 extradata;
		bool global;
	} EventRecord;

public:
	PerlembParser();
	~PerlembParser();
	
	virtual double EventNPC(QuestEventID evt, NPC* npc, Mob *init, std::string data, uint32 extra_data);
	virtual double EventGlobalNPC(QuestEventID evt, NPC* npc, Mob *init, std::string data, uint32 extra_data);
	virtual double EventPlayer(QuestEventID evt, Client *client, std::string data, uint32 extra_data);
	virtual double EventGlobalPlayer(QuestEventID evt, Client *client, std::string data, uint32 extra_data);
	virtual double EventItem(QuestEventID evt, Client *client, ItemInst *item, uint32 objid, uint32 extra_data);
	virtual double EventSpell(QuestEventID evt, NPC* npc, Client *client, uint32 spell_id, uint32 extra_data);

	virtual bool HasQuestSub(uint32 npcid, const char *subname);
	virtual bool HasGlobalQuestSub(const char *subname);
	virtual bool PlayerHasQuestSub(const char *subname);
	virtual bool GlobalPlayerHasQuestSub(const char *subname);
	virtual bool SpellHasQuestSub(uint32 spell_id, const char *subname);
	virtual bool ItemHasQuestSub(ItemInst *itm, const char *subname);

	virtual void LoadNPCScript(std::string filename, int npc_id);
	virtual void LoadGlobalNPCScript(std::string filename);
	virtual void LoadPlayerScript(std::string filename);
	virtual void LoadGlobalPlayerScript(std::string filename);
	virtual void LoadItemScript(std::string filename, std::string item_script);
	virtual void LoadSpellScript(std::string filename, uint32 spell_id);

	virtual void AddVar(std::string name, std::string val);
	virtual std::string GetVar(std::string name);
	virtual void ReloadQuests();
	virtual uint32 GetIdentifier() { return 0xf8b05c11; }
private:
	Embperl *perl;
	
	void ExportHash(const char *pkgprefix, const char *hashname, std::map<std::string, std::string> &vals);
	void ExportVar(const char *pkgprefix, const char *varname, const char *value) const;
	void ExportVar(const char *pkgprefix, const char *varname, int32 value) const;
	void ExportVar(const char *pkgprefix, const char *varname, uint32 value) const;
	void ExportVar(const char *pkgprefix, const char *varname, float value) const;
	void ExportVarComplex(const char *pkgprefix, const char *varname, const char *value) const;

	void EventCommon(QuestEventID event, uint32 objid, const char * data, NPC* npcmob, ItemInst* iteminst, Mob* mob, 
		uint32 extradata, bool global);
	void SendCommands(const char *pkgprefix, const char *event, uint32 npcid, Mob* other, Mob* mob, ItemInst* iteminst);
	void MapFunctions();

	void HandleQueue();
	void AddQueueEvent(QuestEventID event, uint32 objid, const char * data, NPC* npcmob, ItemInst* iteminst, Mob* mob, 
		uint32 extradata, bool global);

	void GetQuestTypes(bool &isPlayerQuest, bool &isGlobalPlayerQuest, bool &isGlobalNPC, bool &isItemQuest, 
		bool &isSpellQuest, QuestEventID event, NPC* npcmob, ItemInst* iteminst, Mob* mob, bool global);
	void GetQuestPackageName(bool &isPlayerQuest, bool &isGlobalPlayerQuest, bool &isGlobalNPC, bool &isItemQuest, 
		bool &isSpellQuest, std::string &package_name, QuestEventID event, uint32 objid, const char * data, 
		NPC* npcmob, ItemInst* iteminst, bool global);
	void ExportCharID(const std::string &package_name, int &char_id, NPC *npcmob, Mob *mob);
	void ExportQGlobals(bool isPlayerQuest, bool isGlobalPlayerQuest, bool isGlobalNPC, bool isItemQuest, 
		bool isSpellQuest, std::string &package_name, NPC *npcmob, Mob *mob, int char_id);
	void ExportMobVariables(bool isPlayerQuest, bool isGlobalPlayerQuest, bool isGlobalNPC, bool isItemQuest, 
		bool isSpellQuest, std::string &package_name, Mob *mob, NPC *npcmob);
	void ExportZoneVariables(std::string &package_name);
	void ExportItemVariables(std::string &package_name, Mob *mob);
	void ExportEventVariables(std::string &package_name, QuestEventID event, uint32 objid, const char * data, 
		NPC* npcmob, ItemInst* iteminst, Mob* mob, uint32 extradata);
	
	std::map<uint32, PerlQuestStatus> npc_quest_status_;
	PerlQuestStatus global_npc_quest_status_;
	PerlQuestStatus player_quest_status_;
	PerlQuestStatus global_player_quest_status_;
	std::map<std::string, PerlQuestStatus> item_quest_status_;
	std::map<uint32, PerlQuestStatus> spell_quest_status_;

	bool event_queue_in_use_;
	std::queue<EventRecord> event_queue_;

	std::map<std::string, std::string> vars_;
	SV *_empty_sv;
};

#endif
#endif

