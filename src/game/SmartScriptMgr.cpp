/*
* This file is part of the OregonCore Project. See AUTHORS file for Copyright information
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Database/DatabaseEnv.h"
#include "ObjectMgr.h"
#include "GridDefines.h"
#include "GridNotifiers.h"
#include "SpellMgr.h"
#include "Cell.h"
#include "GameEventMgr.h"
#include "CreatureTextMgr.h"

#include "SmartScriptMgr.h"

void SmartWaypointMgr::LoadFromDB()
{
    uint32 oldMSTime = getMSTime();

    for (UNORDERED_MAP<uint32, WPPath*>::iterator itr = waypoint_map.begin(); itr != waypoint_map.end(); ++itr)
    {
        for (WPPath::iterator pathItr = itr->second->begin(); pathItr != itr->second->end(); ++pathItr)
            delete pathItr->second;

        delete itr->second;
    }

    waypoint_map.clear();

    QueryResult_AutoPtr result = WorldDatabase.Query("SELECT entry, pointid, position_x, position_y, position_z FROM waypoints ORDER BY entry, pointid");

    if (!result)
    {
        sLog.outString(">> Loaded 0 SmartAI Waypoint Paths. DB table `waypoints` is empty.");
        return;
    }

    uint32 count = 0;
    uint32 total = 0;
    uint32 last_entry = 0;
    uint32 last_id = 1;

    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].GetUInt32();
        uint32 id = fields[1].GetUInt32();
        float x, y, z;
        x = fields[2].GetFloat();
        y = fields[3].GetFloat();
        z = fields[4].GetFloat();

        if (last_entry != entry)
        {
            waypoint_map[entry] = new WPPath();
            last_id = 1;
            count++;
        }

        if (last_id != id)
            sLog.outError("SmartWaypointMgr::LoadFromDB: Path entry %u, unexpected point id %u, expected %u.", entry, id, last_id);

        last_id++;
        (*waypoint_map[entry])[id] = new WayPoint(id, x, y, z);

        last_entry = entry;
        total++;
    }
    while (result->NextRow());

    sLog.outString(">> Loaded %u SmartAI waypoint paths (total %u waypoints) in %u ms", count, total, GetMSTimeDiffToNow(oldMSTime));
}

SmartWaypointMgr::~SmartWaypointMgr()
{
    for (UNORDERED_MAP<uint32, WPPath*>::iterator itr = waypoint_map.begin(); itr != waypoint_map.end(); ++itr)
    {
        for (WPPath::iterator pathItr = itr->second->begin(); pathItr != itr->second->end(); ++pathItr)
            delete pathItr->second;

        delete itr->second;
    }
}

void SmartAIMgr::LoadSmartAIFromDB()
{
    LoadHelperStores();

    uint32 oldMSTime = getMSTime();

    for (uint8 i = 0; i < SMART_SCRIPT_TYPE_MAX; i++)
        mEventMap[i].clear();  //Drop Existing SmartAI List

    QueryResult_AutoPtr result = WorldDatabase.Query("SELECT entryorguid, source_type, id, link, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, target_type, target_param1, target_param2, target_param3, target_x, target_y, target_z, target_o FROM smart_scripts ORDER BY entryorguid, source_type, id, link");

    if (!result)
    {
        sLog.outString(">> Loaded 0 SmartAI scripts. DB table `smartai_scripts` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        SmartScriptHolder temp;

        temp.entryOrGuid = fields[0].GetInt32();
        SmartScriptType source_type = (SmartScriptType)fields[1].GetUInt8();
        if (source_type >= SMART_SCRIPT_TYPE_MAX)
        {
            sLog.outError("SmartAIMgr::LoadSmartAIFromDB: invalid source_type (%u), skipped loading.", uint32(source_type));
            continue;
        }
        if (temp.entryOrGuid >= 0)
        {
            switch (source_type)
            {
                case SMART_SCRIPT_TYPE_CREATURE:
                {
                    CreatureInfo const* creatureInfo = sObjectMgr.GetCreatureTemplate((uint32)temp.entryOrGuid);
                    if (!creatureInfo)
                    {
                        sLog.outError("SmartAIMgr::LoadSmartAIFromDB: Creature entry (%u) does not exist, skipped loading.", uint32(temp.entryOrGuid));
                        continue;
                    }

                    if (creatureInfo->GetAIName() != "SmartAI")
                    {
                        sLog.outError("SmartAIMgr::LoadSmartAIFromDB: Creature entry (%u) is not using SmartAI, skipped loading.", uint32(temp.entryOrGuid));
                        continue;
                    }
                    break;
                }
                case SMART_SCRIPT_TYPE_GAMEOBJECT:
                {
                    GameObjectInfo const* gameObjectInfo = sObjectMgr.GetGameObjectInfo((uint32)temp.entryOrGuid);
                    if (!gameObjectInfo)
                    {
                        sLog.outError("SmartAIMgr::LoadSmartAIFromDB: GameObject entry (%u) does not exist, skipped loading.", uint32(temp.entryOrGuid));
                        continue;
                    }

                    if (gameObjectInfo->GetAIName() != "SmartGameObjectAI")
                    {
                        sLog.outError("SmartAIMgr::LoadSmartAIFromDB: GameObject entry (%u) is not using SmartGameObjectAI, skipped loading.", uint32(temp.entryOrGuid));
                        continue;
                    }
                    break;
                }
                case SMART_SCRIPT_TYPE_AREATRIGGER:
                {
                    if (!sAreaTriggerStore.LookupEntry((uint32)temp.entryOrGuid))
                    {
                        sLog.outError("SmartAIMgr::LoadSmartAIFromDB: AreaTrigger entry (%u) does not exist, skipped loading.", uint32(temp.entryOrGuid));
                        continue;
                    }
                    break;
                }
                case SMART_SCRIPT_TYPE_TIMED_ACTIONLIST:
                    break;//nothing to check, really
                default:
                    sLog.outError("SmartAIMgr::LoadSmartAIFromDB: not yet implemented source_type %u", (uint32)source_type);
                    continue;
            }
        }
        else
        {
            CreatureData const* creature = sObjectMgr.GetCreatureData(uint32(std::abs(temp.entryOrGuid)));
            if (!creature)
            {
                sLog.outError("SmartAIMgr::LoadSmartAIFromDB: Creature guid (%u) does not exist, skipped loading.", uint32(std::abs(temp.entryOrGuid)));
                continue;
            }

            CreatureInfo const* creatureInfo = sObjectMgr.GetCreatureTemplate(creature->id);
            if (!creatureInfo)
            {
                sLog.outError("SmartAIMgr::LoadSmartAIFromDB: Creature entry (%u) guid (%u) does not exist, skipped loading.", creature->id, uint32(std::abs(temp.entryOrGuid)));
                continue;
            }

            if (creatureInfo->GetAIName() != "SmartAI")
            {
                sLog.outError("SmartAIMgr::LoadSmartAIFromDB: Creature entry (%u) guid (%u) is not using SmartAI, skipped loading.", creature->id, uint32(std::abs(temp.entryOrGuid)));
                continue;
            }
        }

        temp.source_type = source_type;
        temp.event_id = fields[2].GetUInt16();
        temp.link = fields[3].GetUInt16();
        temp.event.type = (SMART_EVENT)fields[4].GetUInt8();
        temp.event.event_phase_mask = fields[5].GetUInt8();
        temp.event.event_chance = fields[6].GetUInt8();
        temp.event.event_flags = fields[7].GetUInt8();

        temp.event.raw.param1 = fields[8].GetUInt32();
        temp.event.raw.param2 = fields[9].GetUInt32();
        temp.event.raw.param3 = fields[10].GetUInt32();
        temp.event.raw.param4 = fields[11].GetUInt32();

        temp.event.raw.param5 = fields[12].GetUInt32();

        temp.action.type = (SMART_ACTION)fields[13].GetUInt8();
        temp.action.raw.param1 = fields[14].GetUInt32();
        temp.action.raw.param2 = fields[15].GetUInt32();
        temp.action.raw.param3 = fields[16].GetUInt32();
        temp.action.raw.param4 = fields[17].GetUInt32();
        temp.action.raw.param5 = fields[18].GetUInt32();
        temp.action.raw.param6 = fields[19].GetUInt32();

        temp.target.type = (SMARTAI_TARGETS)fields[20].GetUInt8();
        temp.target.raw.param1 = fields[21].GetUInt32();
        temp.target.raw.param2 = fields[22].GetUInt32();
        temp.target.raw.param3 = fields[23].GetUInt32();
        temp.target.x = fields[24].GetFloat();
        temp.target.y = fields[25].GetFloat();
        temp.target.z = fields[26].GetFloat();
        temp.target.o = fields[27].GetFloat();

        //check target
        if (!IsTargetValid(temp))
            continue;

        // check all event and action params
        if (!IsEventValid(temp))
            continue;

        // creature entry / guid not found in storage, create empty event list for it and increase counters
        if (mEventMap[source_type].find(temp.entryOrGuid) == mEventMap[source_type].end())
        {
            ++count;
            SmartAIEventList eventList;
            mEventMap[source_type][temp.entryOrGuid] = eventList;
        }
        // store the new event
        mEventMap[source_type][temp.entryOrGuid].push_back(temp);
    }
    while (result->NextRow());

    // Post Loading Validation
    for (uint8 i = 0; i < SMART_SCRIPT_TYPE_MAX; ++i)
    {
        for (SmartAIEventMap::iterator itr = mEventMap[i].begin(); itr != mEventMap[i].end(); ++itr)
        {
            for (SmartScriptHolder const& e : itr->second)
            {
                if (e.link)
                {
                    if (!FindLinkedEvent(itr->second, e.link))
                    {
                        sLog.outError("SmartAIMgr::LoadSmartAIFromDB: Entry %d SourceType %u, Event %u, Link Event %u not found or invalid.",
                            e.entryOrGuid, e.GetScriptType(), e.event_id, e.link);
                    }
                }

                if (e.GetEventType() == SMART_EVENT_LINK)
                {
                    if (!FindLinkedSourceEvent(itr->second, e.event_id))
                    {
                        sLog.outError("SmartAIMgr::LoadSmartAIFromDB: Entry %d SourceType %u, Event %u, Link Source Event not found or invalid. Event will never trigger.",
                            e.entryOrGuid, e.GetScriptType(), e.event_id);
                    }
                }
            }
        }
    }

    sLog.outString(">> Loaded %u SmartAI scripts in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

    UnLoadHelperStores();
}

bool SmartAIMgr::IsTargetValid(SmartScriptHolder const& e)
{
    if (std::abs(e.target.o) > 2 * float(M_PI))
        sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u has abs(`target.o` = %f) > 2*PI (orientation is expressed in radians)",
            e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.target.o);

    if (e.GetActionType() == SMART_ACTION_INSTALL_AI_TEMPLATE)
        return true; // AI template has special handling

    switch (e.GetTargetType())
    {
        case SMART_TARGET_CREATURE_DISTANCE:
        case SMART_TARGET_CREATURE_RANGE:
        {
            if (e.target.unitDistance.creature && !sObjectMgr.GetCreatureTemplate(e.target.unitDistance.creature))
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Creature entry %u as target_param1, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.target.unitDistance.creature);
                return false;
            }
            break;
        }
        case SMART_TARGET_GAMEOBJECT_DISTANCE:
        case SMART_TARGET_GAMEOBJECT_RANGE:
        {
            if (e.target.goDistance.entry && !sObjectMgr.GetGameObjectInfo(e.target.goDistance.entry))
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent GameObject entry %u as target_param1, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.target.goDistance.entry);
                return false;
            }
            break;
        }
        case SMART_TARGET_CREATURE_GUID:
        {
            if (e.target.unitGUID.entry && !IsCreatureValid(e, e.target.unitGUID.entry))
                return false;
            break;
        }
        case SMART_TARGET_GAMEOBJECT_GUID:
        {
            if (e.target.goGUID.entry && !IsGameObjectValid(e, e.target.goGUID.entry))
                return false;
            break;
        }
        case SMART_TARGET_PLAYER_DISTANCE:
        case SMART_TARGET_CLOSEST_PLAYER:
        {
            if (e.target.playerDistance.dist == 0)
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u has maxDist 0 as target_param1, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                return false;
            }
            break;
        }
        case SMART_TARGET_PLAYER_RANGE:
        case SMART_TARGET_SELF:
        case SMART_TARGET_VICTIM:
        case SMART_TARGET_HOSTILE_SECOND_AGGRO:
        case SMART_TARGET_HOSTILE_LAST_AGGRO:
        case SMART_TARGET_HOSTILE_RANDOM:
        case SMART_TARGET_HOSTILE_RANDOM_NOT_TOP:
        case SMART_TARGET_ACTION_INVOKER:
        case SMART_TARGET_INVOKER_PARTY:
        case SMART_TARGET_POSITION:
        case SMART_TARGET_NONE:
        case SMART_TARGET_ACTION_INVOKER_VEHICLE:
        case SMART_TARGET_OWNER_OR_SUMMONER:
        case SMART_TARGET_THREAT_LIST:
        case SMART_TARGET_CLOSEST_GAMEOBJECT:
        case SMART_TARGET_CLOSEST_CREATURE:
        case SMART_TARGET_CLOSEST_ENEMY:
        case SMART_TARGET_CLOSEST_FRIENDLY:
        case SMART_TARGET_STORED:
            break;
        default:
            sLog.outError("SmartAIMgr: Not handled target_type(%u), Entry %d SourceType %u Event %u Action %u, skipped.", e.GetTargetType(), e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
            return false;
    }
    return true;
}

bool SmartAIMgr::IsEventValid(SmartScriptHolder& e)
{
    if (e.event.type >= SMART_EVENT_END)
    {
        sLog.outError("SmartAIMgr: EntryOrGuid %d using event(%u) has invalid event type (%u), skipped.", e.entryOrGuid, e.event_id, e.GetEventType());
        return false;
    }

    // in SMART_SCRIPT_TYPE_TIMED_ACTIONLIST all event types are overriden by core
    if (e.GetScriptType() != SMART_SCRIPT_TYPE_TIMED_ACTIONLIST && !(SmartAIEventMask[e.event.type][1] & SmartAITypeMask[e.GetScriptType()][1]))
    {
        sLog.outError("SmartAIMgr: EntryOrGuid %d, event type %u can not be used for Script type %u", e.entryOrGuid, e.GetEventType(), e.GetScriptType());
        return false;
    }

    if (e.action.type <= 0 || e.action.type >= SMART_ACTION_END)
    {
        sLog.outError("SmartAIMgr: EntryOrGuid %d using event(%u) has invalid action type (%u), skipped.", e.entryOrGuid, e.event_id, e.GetActionType());
        return false;
    }

    if (e.event.event_phase_mask > SMART_EVENT_PHASE_ALL)
    {
        sLog.outError("SmartAIMgr: EntryOrGuid %d using event(%u) has invalid phase mask (%u), skipped.", e.entryOrGuid, e.event_id, e.event.event_phase_mask);
        return false;
    }

    if (e.event.event_flags > SMART_EVENT_FLAGS_ALL)
    {
        sLog.outError("SmartAIMgr: EntryOrGuid %d using event(%u) has invalid event flags (%u), skipped.", e.entryOrGuid, e.event_id, e.event.event_flags);
        return false;
    }

    if (e.link && e.link == e.event_id)
    {
        sLog.outError("SmartAIMgr: EntryOrGuid %d SourceType %u, Event %u, Event is linking self (infinite loop), skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id);
        return false;
    }

    if (e.GetScriptType() == SMART_SCRIPT_TYPE_TIMED_ACTIONLIST)
    {
        e.event.type = SMART_EVENT_UPDATE_OOC;//force default OOC, can change when calling the script!
        if (!IsMinMaxValid(e, e.event.minMaxRepeat.min, e.event.minMaxRepeat.max))
            return false;

        if (!IsMinMaxValid(e, e.event.minMaxRepeat.repeatMin, e.event.minMaxRepeat.repeatMax))
            return false;
    }
    else
    {
        switch (e.GetEventType())
        {
            case SMART_EVENT_UPDATE:
            case SMART_EVENT_UPDATE_IC:
            case SMART_EVENT_UPDATE_OOC:
            case SMART_EVENT_HEALT_PCT:
            case SMART_EVENT_MANA_PCT:
            case SMART_EVENT_TARGET_HEALTH_PCT:
            case SMART_EVENT_TARGET_MANA_PCT:
            case SMART_EVENT_RANGE:
            case SMART_EVENT_DAMAGED:
            case SMART_EVENT_DAMAGED_TARGET:
            case SMART_EVENT_RECEIVE_HEAL:
                if (!IsMinMaxValid(e, e.event.minMaxRepeat.min, e.event.minMaxRepeat.max))
                    return false;

                if (!IsMinMaxValid(e, e.event.minMaxRepeat.repeatMin, e.event.minMaxRepeat.repeatMax))
                    return false;
                break;
            case SMART_EVENT_SPELLHIT:
            case SMART_EVENT_SPELLHIT_TARGET:
                if (e.event.spellHit.spell)
                {
                    SpellEntry const* spellInfo = sSpellStore.LookupEntry(e.event.spellHit.spell);
                    if (!spellInfo)
                    {
                        sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Spell entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.spellHit.spell);
                        return false;
                    }
                    if (e.event.spellHit.school && (e.event.spellHit.school & spellInfo->SchoolMask) != spellInfo->SchoolMask)
                    {
                        sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses Spell entry %u with invalid school mask, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.spellHit.spell);
                        return false;
                    }
                }
                if (!IsMinMaxValid(e, e.event.spellHit.cooldownMin, e.event.spellHit.cooldownMax))
                    return false;
                break;
            case SMART_EVENT_OOC_LOS:
            case SMART_EVENT_IC_LOS:
                if (!IsMinMaxValid(e, e.event.los.cooldownMin, e.event.los.cooldownMax))
                    return false;
                break;
            case SMART_EVENT_RESPAWN:
                if (e.event.respawn.type == SMART_SCRIPT_RESPAWN_CONDITION_MAP && !sMapStore.LookupEntry(e.event.respawn.map))
                {
                    sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Map entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.respawn.map);
                    return false;
                }
                if (e.event.respawn.type == SMART_SCRIPT_RESPAWN_CONDITION_AREA && !GetAreaEntryByAreaID(e.event.respawn.area))
                {
                    sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Area entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.respawn.area);
                    return false;
                }
                break;
            case SMART_EVENT_FRIENDLY_HEALTH:
                if (!NotNULL(e, e.event.friendlyHealth.radius))
                    return false;

                if (!IsMinMaxValid(e, e.event.friendlyHealth.repeatMin, e.event.friendlyHealth.repeatMax))
                    return false;
                break;
            case SMART_EVENT_FRIENDLY_IS_CC:
                if (!IsMinMaxValid(e, e.event.friendlyCC.repeatMin, e.event.friendlyCC.repeatMax))
                    return false;
                break;
            case SMART_EVENT_FRIENDLY_MISSING_BUFF:
            {
                if (!IsSpellValid(e, e.event.missingBuff.spell))
                    return false;

                if (!NotNULL(e, e.event.missingBuff.radius))
                    return false;

                if (!IsMinMaxValid(e, e.event.missingBuff.repeatMin, e.event.missingBuff.repeatMax))
                    return false;
                break;
            }
            case SMART_EVENT_KILL:
                if (!IsMinMaxValid(e, e.event.kill.cooldownMin, e.event.kill.cooldownMax))
                    return false;

                if (e.event.kill.creature && !IsCreatureValid(e, e.event.kill.creature))
                    return false;
                break;
            case SMART_EVENT_VICTIM_CASTING:
                if (e.event.targetCasting.spellId > 0 && !sSpellStore.LookupEntry(e.event.targetCasting.spellId))
                {
                    sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Spell entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.spellHit.spell);
                    return false;
                }

                if (!IsMinMaxValid(e, e.event.targetCasting.repeatMin, e.event.targetCasting.repeatMax))
                    return false;
                break;
            case SMART_EVENT_PASSENGER_BOARDED:
            case SMART_EVENT_PASSENGER_REMOVED:
                if (!IsMinMaxValid(e, e.event.minMax.repeatMin, e.event.minMax.repeatMax))
                    return false;
                break;
            case SMART_EVENT_SUMMON_DESPAWNED:
            case SMART_EVENT_SUMMONED_UNIT:
                if (e.event.summoned.creature && !IsCreatureValid(e, e.event.summoned.creature))
                    return false;

                if (!IsMinMaxValid(e, e.event.summoned.cooldownMin, e.event.summoned.cooldownMax))
                    return false;
                break;
            case SMART_EVENT_ACCEPTED_QUEST:
            case SMART_EVENT_REWARD_QUEST:
                if (e.event.quest.quest && !IsQuestValid(e, e.event.quest.quest))
                    return false;
                break;
            case SMART_EVENT_RECEIVE_EMOTE:
            {
                if (e.event.emote.emote && !IsTextEmoteValid(e, e.event.emote.emote))
                    return false;

                if (!IsMinMaxValid(e, e.event.emote.cooldownMin, e.event.emote.cooldownMax))
                    return false;
                break;
            }
            case SMART_EVENT_HAS_AURA:
            case SMART_EVENT_TARGET_BUFFED:
            {
                if (!IsSpellValid(e, e.event.aura.spell))
                    return false;

                if (!IsMinMaxValid(e, e.event.aura.repeatMin, e.event.aura.repeatMax))
                    return false;
                break;
            }
            case SMART_EVENT_TRANSPORT_ADDCREATURE:
            {
                if (e.event.transportAddCreature.creature && !IsCreatureValid(e, e.event.transportAddCreature.creature))
                    return false;
                break;
            }
            case SMART_EVENT_MOVEMENTINFORM:
            {
                if (e.event.movementInform.type > NULL_MOTION_TYPE)
                {
                    sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses invalid Motion type %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.movementInform.type);
                    return false;
                }
                break;
            }
            case SMART_EVENT_DATA_SET:
            {
                if (!IsMinMaxValid(e, e.event.dataSet.cooldownMin, e.event.dataSet.cooldownMax))
                    return false;
                break;
            }
            case SMART_EVENT_AREATRIGGER_ONTRIGGER:
            {
                if (e.event.areatrigger.id && !IsAreaTriggerValid(e, e.event.areatrigger.id))
                    return false;
                break;
            }
            case SMART_EVENT_TEXT_OVER:
                if (!IsTextValid(e, e.event.textOver.textGroupID))
                    return false;
                break;
            case SMART_EVENT_DUMMY_EFFECT:
            {
                if (!IsSpellValid(e, e.event.dummy.spell))
                    return false;

                if (e.event.dummy.effIndex > EFFECT_2)
                    return false;
                break;
            }
            case SMART_EVENT_IS_BEHIND_TARGET:
            {
                if (!IsMinMaxValid(e, e.event.behindTarget.cooldownMin, e.event.behindTarget.cooldownMax))
                    return false;
                break;
            }
            case SMART_EVENT_GAME_EVENT_START:
            case SMART_EVENT_GAME_EVENT_END:
            {
                GameEventMgr::GameEventDataMap const& events = sGameEventMgr.GetEventMap();
                if (e.event.gameEvent.gameEventId >= events.size() || !events[e.event.gameEvent.gameEventId].isValid())
                    return false;
                break;
            }
            case SMART_EVENT_ACTION_DONE:
            {
                if (e.event.doAction.eventId > EVENT_CHARGE)
                {
                    sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses invalid event id %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.doAction.eventId);
                    return false;
                }
                break;
            }
            case SMART_EVENT_FRIENDLY_HEALTH_PCT:
                if (!IsMinMaxValid(e, e.event.friendlyHealthPct.repeatMin, e.event.friendlyHealthPct.repeatMax))
                    return false;

                if (e.event.friendlyHealthPct.maxHpPct > 100 || e.event.friendlyHealthPct.minHpPct > 100)
                {
                    sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u has pct value above 100, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                    return false;
                }

                switch (e.GetTargetType())
                {
                    case SMART_TARGET_CREATURE_RANGE:
                    case SMART_TARGET_CREATURE_GUID:
                    case SMART_TARGET_CREATURE_DISTANCE:
                    case SMART_TARGET_CLOSEST_CREATURE:
                    case SMART_TARGET_CLOSEST_PLAYER:
                    case SMART_TARGET_PLAYER_RANGE:
                    case SMART_TARGET_PLAYER_DISTANCE:
                        break;
                    default:
                        sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses invalid target_type %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.GetTargetType());
                        return false;
                }
                break;
            case SMART_EVENT_DISTANCE_CREATURE:
                if (e.event.distance.guid == 0 && e.event.distance.entry == 0)
                {
                    sLog.outError("SmartAIMgr: Event SMART_EVENT_DISTANCE_CREATURE did not provide creature guid or entry, skipped.");
                    return false;
                }

                if (e.event.distance.guid != 0 && e.event.distance.entry != 0)
                {
                    sLog.outError("SmartAIMgr: Event SMART_EVENT_DISTANCE_CREATURE provided both an entry and guid, skipped.");
                    return false;
                }

                if (e.event.distance.guid != 0 && !sObjectMgr.GetCreatureData(e.event.distance.guid))
                {
                    sLog.outError("SmartAIMgr: Event SMART_EVENT_DISTANCE_CREATURE using invalid creature guid %u, skipped.", e.event.distance.guid);
                    return false;
                }

                if (e.event.distance.entry != 0 && !sObjectMgr.GetCreatureTemplate(e.event.distance.entry))
                {
                    sLog.outError("SmartAIMgr: Event SMART_EVENT_DISTANCE_CREATURE using invalid creature entry %u, skipped.", e.event.distance.entry);
                    return false;
                }
                break;
            case SMART_EVENT_DISTANCE_GAMEOBJECT:
                if (e.event.distance.guid == 0 && e.event.distance.entry == 0)
                {
                    sLog.outError("SmartAIMgr: Event SMART_EVENT_DISTANCE_GAMEOBJECT did not provide gameobject guid or entry, skipped.");
                    return false;
                }

                if (e.event.distance.guid != 0 && e.event.distance.entry != 0)
                {
                    sLog.outError("SmartAIMgr: Event SMART_EVENT_DISTANCE_GAMEOBJECT provided both an entry and guid, skipped.");
                    return false;
                }

                if (e.event.distance.guid != 0 && !sObjectMgr.GetGOData(e.event.distance.guid))
                {
                    sLog.outError("SmartAIMgr: Event SMART_EVENT_DISTANCE_GAMEOBJECT using invalid gameobject guid %u, skipped.", e.event.distance.guid);
                    return false;
                }

                if (e.event.distance.entry != 0 && !sObjectMgr.GetGameObjectInfo(e.event.distance.entry))
                {
                    sLog.outError("SmartAIMgr: Event SMART_EVENT_DISTANCE_GAMEOBJECT using invalid gameobject entry %u, skipped.", e.event.distance.entry);
                    return false;
                }
                break;
            case SMART_EVENT_COUNTER_SET:
                if (!IsMinMaxValid(e, e.event.counter.cooldownMin, e.event.counter.cooldownMax))
                    return false;

                if (e.event.counter.id == 0)
                {
                    sLog.outError("SmartAIMgr: Event SMART_EVENT_COUNTER_SET using invalid counter id %u, skipped.", e.event.counter.id);
                    return false;
                }

                if (e.event.counter.value == 0)
                {
                    sLog.outError("SmartAIMgr: Event SMART_EVENT_COUNTER_SET using invalid value %u, skipped.", e.event.counter.value);
                    return false;
                }
                break;
            case SMART_EVENT_LINK:
            case SMART_EVENT_GO_STATE_CHANGED:
            case SMART_EVENT_GO_EVENT_INFORM:
            case SMART_EVENT_TIMED_EVENT_TRIGGERED:
            case SMART_EVENT_INSTANCE_PLAYER_ENTER:
            case SMART_EVENT_TRANSPORT_RELOCATE:
            case SMART_EVENT_CHARMED:
            case SMART_EVENT_CHARMED_TARGET:
            case SMART_EVENT_CORPSE_REMOVED:
            case SMART_EVENT_AI_INIT:
            case SMART_EVENT_TRANSPORT_ADDPLAYER:
            case SMART_EVENT_TRANSPORT_REMOVE_PLAYER:
            case SMART_EVENT_AGGRO:
            case SMART_EVENT_DEATH:
            case SMART_EVENT_EVADE:
            case SMART_EVENT_REACHED_HOME:
            case SMART_EVENT_RESET:
            case SMART_EVENT_QUEST_ACCEPTED:
            case SMART_EVENT_QUEST_OBJ_COPLETETION:
            case SMART_EVENT_QUEST_COMPLETION:
            case SMART_EVENT_QUEST_REWARDED:
            case SMART_EVENT_QUEST_FAIL:
            case SMART_EVENT_JUST_SUMMONED:
            case SMART_EVENT_WAYPOINT_START:
            case SMART_EVENT_WAYPOINT_REACHED:
            case SMART_EVENT_WAYPOINT_PAUSED:
            case SMART_EVENT_WAYPOINT_RESUMED:
            case SMART_EVENT_WAYPOINT_STOPPED:
            case SMART_EVENT_WAYPOINT_ENDED:
            case SMART_EVENT_GOSSIP_SELECT:
            case SMART_EVENT_GOSSIP_HELLO:
            case SMART_EVENT_JUST_CREATED:
            case SMART_EVENT_FOLLOW_COMPLETED:
            case SMART_EVENT_ON_SPELLCLICK:
                break;
            default:
                sLog.outError("SmartAIMgr: Not handled event_type(%u), Entry %d SourceType %u Event %u Action %u, skipped.", e.GetEventType(), e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                return false;
        }
    }

    switch (e.GetActionType())
    {
        case SMART_ACTION_TALK:
        case SMART_ACTION_SIMPLE_TALK:
            if (!IsTextValid(e, e.action.talk.textGroupID))
                return false;
            break;
        case SMART_ACTION_SET_FACTION:
            if (e.action.faction.factionID && !sFactionTemplateStore.LookupEntry(e.action.faction.factionID))
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Faction %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.faction.factionID);
                return false;
            }
            break;
        case SMART_ACTION_MORPH_TO_ENTRY_OR_MODEL:
        case SMART_ACTION_MOUNT_TO_ENTRY_OR_MODEL:
            if (e.action.morphOrMount.creature || e.action.morphOrMount.model)
            {
                if (e.action.morphOrMount.creature > 0 && !sObjectMgr.GetCreatureTemplate(e.action.morphOrMount.creature))
                {
                    sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Creature entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.morphOrMount.creature);
                    return false;
                }

                if (e.action.morphOrMount.model)
                {
                    if (e.action.morphOrMount.creature)
                    {
                        sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u has ModelID set with also set CreatureId, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                        return false;
                    }
                    else if (!sCreatureDisplayInfoStore.LookupEntry(e.action.morphOrMount.model))
                    {
                        sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Model id %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.morphOrMount.model);
                        return false;
                    }
                }
            }
            break;
        case SMART_ACTION_SOUND:
            if (!IsSoundValid(e, e.action.sound.sound))
                return false;
            break;
        case SMART_ACTION_SET_EMOTE_STATE:
        case SMART_ACTION_PLAY_EMOTE:
            if (!IsEmoteValid(e, e.action.emote.emote))
                return false;
            break;
        case SMART_ACTION_FAIL_QUEST:
        case SMART_ACTION_ADD_QUEST:
            if (!e.action.quest.quest || !IsQuestValid(e, e.action.quest.quest))
                return false;
            break;
        case SMART_ACTION_ACTIVATE_TAXI:
            {
                if (!sTaxiPathStore.LookupEntry(e.action.taxi.id))
                {
                    sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses invalid Taxi path ID %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.taxi.id);
                    return false;
                }
                break;
            }
        case SMART_ACTION_RANDOM_EMOTE:
            if (e.action.randomEmote.emote1 && !IsEmoteValid(e, e.action.randomEmote.emote1))
                return false;

            if (e.action.randomEmote.emote2 && !IsEmoteValid(e, e.action.randomEmote.emote2))
                return false;

            if (e.action.randomEmote.emote3 && !IsEmoteValid(e, e.action.randomEmote.emote3))
                return false;

            if (e.action.randomEmote.emote4 && !IsEmoteValid(e, e.action.randomEmote.emote4))
                return false;

            if (e.action.randomEmote.emote5 && !IsEmoteValid(e, e.action.randomEmote.emote5))
                return false;

            if (e.action.randomEmote.emote6 && !IsEmoteValid(e, e.action.randomEmote.emote6))
                return false;
            break;
        case SMART_ACTION_RANDOM_SOUND:
            for (uint8 i = 0; i < SMART_ACTION_PARAM_COUNT - 1; i++)
            {
                if (e.action.randomSound.sound[i] && !IsSoundValid(e, e.action.randomSound.sound[i]))
                    return false;
            }
            break;
        case SMART_ACTION_CAST:
        {
            if (!IsSpellValid(e, e.action.cast.spell))
                return false;

            SpellEntry const* spellInfo = sSpellStore.LookupEntry(e.action.cast.spell);
            for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
            {
                if (spellInfo->HasEffect(j, SPELL_EFFECT_KILL_CREDIT))
                {
                    if (spellInfo->EffectImplicitTargetA[j] == TARGET_UNIT_CASTER)
                        sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u Effect: SPELL_EFFECT_KILL_CREDIT: (SpellId: %u targetA: %u - targetB: %u) has invalid target for this Action",
                        e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.cast.spell, spellInfo->EffectImplicitTargetA[j], spellInfo->EffectImplicitTargetB);
                }
            }
            break;
        }
        case SMART_ACTION_ADD_AURA:
        case SMART_ACTION_INVOKER_CAST:
            if (!IsSpellValid(e, e.action.cast.spell))
                return false;
            break;
        case SMART_ACTION_CALL_AREAEXPLOREDOREVENTHAPPENS:
        case SMART_ACTION_CALL_GROUPEVENTHAPPENS:
            if (Quest const* qid = sObjectMgr.GetQuestTemplate(e.action.quest.quest))
            {
                if (!qid->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
                {
                    sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u SpecialFlags for Quest entry %u does not include FLAGS_EXPLORATION_OR_EVENT(2), skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.quest.quest);
                    return false;
                }
            }
            else
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Quest entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.quest.quest);
                return false;
            }
            break;
        case SMART_ACTION_SET_EVENT_PHASE:
            if (e.action.setEventPhase.phase >= SMART_EVENT_PHASE_MAX)
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u attempts to set phase %u. Phase mask cannot be used past phase %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.setEventPhase.phase, SMART_EVENT_PHASE_MAX - 1);
                return false;
            }
            break;
        case SMART_ACTION_INC_EVENT_PHASE:
            if (!e.action.incEventPhase.inc && !e.action.incEventPhase.dec)
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u is incrementing phase by 0, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                return false;
            }
            else if (e.action.incEventPhase.inc > SMART_EVENT_PHASE_MAX || e.action.incEventPhase.dec > SMART_EVENT_PHASE_MAX)
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u attempts to increment phase by too large value, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                return false;
            }
            break;
        case SMART_ACTION_REMOVEAURASFROMSPELL:
            if (e.action.removeAura.spell != 0 && !IsSpellValid(e, e.action.removeAura.spell))
                return false;
            break;
        case SMART_ACTION_RANDOM_PHASE:
            {
                if (e.action.randomPhase.phase1 >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhase.phase2 >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhase.phase3 >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhase.phase4 >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhase.phase5 >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhase.phase6 >= SMART_EVENT_PHASE_MAX)
                {
                    sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u attempts to set invalid phase, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                    return false;
                }
            }
            break;
        case SMART_ACTION_RANDOM_PHASE_RANGE:       //PhaseMin, PhaseMax
            {
                if (e.action.randomPhaseRange.phaseMin >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhaseRange.phaseMax >= SMART_EVENT_PHASE_MAX)
                {
                    sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u attempts to set invalid phase, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                    return false;
                }

                if (!IsMinMaxValid(e, e.action.randomPhaseRange.phaseMin, e.action.randomPhaseRange.phaseMax))
                    return false;
                break;
            }
        case SMART_ACTION_SUMMON_CREATURE:
        {
            if (!IsCreatureValid(e, e.action.summonCreature.creature))
                return false;

            CacheSpellContainerBounds sBounds = GetSummonCreatureSpellContainerBounds(e.action.summonCreature.creature);
            for (CacheSpellContainer::const_iterator itr = sBounds.first; itr != sBounds.second; ++itr)
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u creature summon: There is a summon spell for creature entry %u (SpellId: %u, effect: %u)",
                                e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.summonCreature.creature, itr->second.first, itr->second.second);

            if (e.action.summonCreature.type < TEMPSUMMON_TIMED_OR_DEAD_DESPAWN || e.action.summonCreature.type > TEMPSUMMON_MANUAL_DESPAWN)
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses incorrect TempSummonType %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.summonCreature.type);
                return false;
            }
            break;
        }
        case SMART_ACTION_CALL_KILLEDMONSTER:
        {
            if (!IsCreatureValid(e, e.action.killedMonster.creature))
                return false;

            CacheSpellContainerBounds sBounds = GetKillCreditSpellContainerBounds(e.action.killedMonster.creature);
            for (CacheSpellContainer::const_iterator itr = sBounds.first; itr != sBounds.second; ++itr)
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u Kill Credit: There is a killcredit spell for creatureEntry %u (SpellId: %u effect: %u)",
                                e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.killedMonster.creature, itr->second.first, itr->second.second);

            if (e.GetTargetType() == SMART_TARGET_POSITION)
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses incorrect TargetType %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.GetTargetType());
                return false;
            }
            break;
        }
        case SMART_ACTION_UPDATE_TEMPLATE:
            if (!IsCreatureValid(e, e.action.updateTemplate.creature))
                return false;
            break;
        case SMART_ACTION_SET_SHEATH:
            if (e.action.setSheath.sheath && e.action.setSheath.sheath >= MAX_SHEATH_STATE)
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses incorrect Sheath state %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.setSheath.sheath);
                return false;
            }
            break;
        case SMART_ACTION_SET_REACT_STATE:
            {
                if (e.action.react.state > REACT_AGGRESSIVE)
                {
                    sLog.outError("SmartAIMgr: Creature %d Event %u Action %u uses invalid React State %u, skipped.", e.entryOrGuid, e.event_id, e.GetActionType(), e.action.react.state);
                    return false;
                }
                break;
            }
        case SMART_ACTION_SUMMON_GO:
        {
            if (!IsGameObjectValid(e, e.action.summonGO.entry))
                return false;

            CacheSpellContainerBounds sBounds = GetSummonGameObjectSpellContainerBounds(e.action.summonGO.entry);
            for (CacheSpellContainer::const_iterator itr = sBounds.first; itr != sBounds.second; ++itr)
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u gameobject summon: There is a summon spell for gameobject entry %u (SpellId: %u, effect: %u)",
                    e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.summonGO.entry, itr->second.first, itr->second.second);
            break;
        }
        case SMART_ACTION_REMOVE_ITEM:
            if (!IsItemValid(e, e.action.item.entry))
                return false;

            if (!NotNULL(e, e.action.item.count))
                return false;
            break;
        case SMART_ACTION_ADD_ITEM:
        {
            if (!IsItemValid(e, e.action.item.entry))
                return false;

            if (!NotNULL(e, e.action.item.count))
                return false;

            CacheSpellContainerBounds sBounds = GetCreateItemSpellContainerBounds(e.action.item.entry);
            for (CacheSpellContainer::const_iterator itr = sBounds.first; itr != sBounds.second; ++itr)
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u Create Item: There is a create item spell for item %u (SpellId: %u effect: %u)",
                    e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.item.entry, itr->second.first, itr->second.second);
            break;
        }
        case SMART_ACTION_TELEPORT:
            if (!sMapStore.LookupEntry(e.action.teleport.mapID))
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Map entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.teleport.mapID);
                return false;
            }
            break;
        case SMART_ACTION_SET_COUNTER:
            if (e.action.setCounter.counterId == 0)
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses wrong counterId %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.setCounter.counterId);
                return false;
            }

            if (e.action.setCounter.value == 0)
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses wrong value %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.setCounter.value);
                return false;
            }

            break;
        case SMART_ACTION_INSTALL_AI_TEMPLATE:
            if (e.action.installTtemplate.id >= SMARTAI_TEMPLATE_END)
            {
                sLog.outError("SmartAIMgr: Creature %d Event %u Action %u uses non-existent AI template id %u, skipped.", e.entryOrGuid, e.event_id, e.GetActionType(), e.action.installTtemplate.id);
                return false;
            }
            break;
        case SMART_ACTION_WP_STOP:
            if (e.action.wpStop.quest && !IsQuestValid(e, e.action.wpStop.quest))
                return false;
            break;
        case SMART_ACTION_WP_START:
            {
                if (!sSmartWaypointMgr->GetPath(e.action.wpStart.pathID))
                {
                    sLog.outError("SmartAIMgr: Creature %d Event %u Action %u uses non-existent WaypointPath id %u, skipped.", e.entryOrGuid, e.event_id, e.GetActionType(), e.action.wpStart.pathID);
                    return false;
                }
                if (e.action.wpStart.quest && !IsQuestValid(e, e.action.wpStart.quest))
                    return false;
                if (e.action.wpStart.reactState > REACT_AGGRESSIVE)
                {
                    sLog.outError("SmartAIMgr: Creature %d Event %u Action %u uses invalid React State %u, skipped.", e.entryOrGuid, e.event_id, e.GetActionType(), e.action.wpStart.reactState);
                    return false;
                }
                break;
            }
        case SMART_ACTION_CREATE_TIMED_EVENT:
        {
            if (!IsMinMaxValid(e, e.action.timeEvent.min, e.action.timeEvent.max))
                return false;

            if (!IsMinMaxValid(e, e.action.timeEvent.repeatMin, e.action.timeEvent.repeatMax))
                return false;
            break;
        }
        case SMART_ACTION_CALL_RANDOM_RANGE_TIMED_ACTIONLIST:
        {
            if (!IsMinMaxValid(e, e.action.randTimedActionList.entry1, e.action.randTimedActionList.entry2))
                return false;
            break;
        }
        case SMART_ACTION_SET_POWER:
        case SMART_ACTION_ADD_POWER:
        case SMART_ACTION_REMOVE_POWER:
            if (e.action.power.powerType > MAX_POWERS)
            {
                sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Power %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.power.powerType);
                return false;
            }
            break;
        case SMART_ACTION_GAME_EVENT_STOP:
        {
            uint32 eventId = e.action.gameEventStop.id;

            GameEventMgr::GameEventDataMap const& events = sGameEventMgr.GetEventMap();
            if (eventId < 1 || eventId >= events.size())
            {
                sLog.outError("SmartAIMgr: Entry %u SourceType %u Event %u Action %u uses non-existent event, eventId %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.gameEventStop.id);
                return false;
            }

            GameEventData const& eventData = events[eventId];
            if (!eventData.isValid())
            {
                sLog.outError("SmartAIMgr: Entry %u SourceType %u Event %u Action %u uses non-existent event, eventId %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.gameEventStop.id);
                return false;
            }
            break;
        }
        case SMART_ACTION_GAME_EVENT_START:
        {
            uint32 eventId = e.action.gameEventStart.id;

            GameEventMgr::GameEventDataMap const& events = sGameEventMgr.GetEventMap();
            if (eventId < 1 || eventId >= events.size())
            {
                sLog.outError("SmartAIMgr: Entry %u SourceType %u Event %u Action %u uses non-existent event, eventId %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.gameEventStart.id);
                return false;
            }

            GameEventData const& eventData = events[eventId];
            if (!eventData.isValid())
            {
                sLog.outError("SmartAIMgr: Entry %u SourceType %u Event %u Action %u uses non-existent event, eventId %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.gameEventStart.id);
                return false;
            }
            break;
        }
        case SMART_ACTION_EQUIP:
        {
            if (e.GetScriptType() == SMART_SCRIPT_TYPE_CREATURE)
            {
                uint32 equipId = e.action.equip.entry;

                if (equipId)
                {
                    EquipmentInfo const* einfo = sObjectMgr.GetEquipmentInfo(equipId);
                    if (!einfo)
                    {
                        sLog.outError("SmartScript: SMART_ACTION_EQUIP uses non-existent equipment info id %u for creature %u, skipped.", equipId, e.entryOrGuid);
                        return false;
                    }
                }
            }
            break;
        }
        case SMART_ACTION_START_CLOSEST_WAYPOINT:
        case SMART_ACTION_FOLLOW:
        case SMART_ACTION_SET_ORIENTATION:
        case SMART_ACTION_STORE_TARGET_LIST:
        case SMART_ACTION_EVADE:
        case SMART_ACTION_FLEE_FOR_ASSIST:
        case SMART_ACTION_COMBAT_STOP:
        case SMART_ACTION_DIE:
        case SMART_ACTION_SET_IN_COMBAT_WITH_ZONE:
        case SMART_ACTION_SET_ACTIVE:
        case SMART_ACTION_WP_RESUME:
        case SMART_ACTION_KILL_UNIT:
        case SMART_ACTION_SET_INVINCIBILITY_HP_LEVEL:
        case SMART_ACTION_RESET_GOBJECT:
        case SMART_ACTION_ATTACK_START:
        case SMART_ACTION_THREAT_ALL_PCT:
        case SMART_ACTION_THREAT_SINGLE_PCT:
        case SMART_ACTION_SET_INST_DATA:
        case SMART_ACTION_SET_INST_DATA64:
        case SMART_ACTION_AUTO_ATTACK:
        case SMART_ACTION_ALLOW_COMBAT_MOVEMENT:
        case SMART_ACTION_CALL_FOR_HELP:
        case SMART_ACTION_SET_DATA:
        case SMART_ACTION_MOVE_FORWARD:
        case SMART_ACTION_SET_VISIBILITY:
        case SMART_ACTION_WP_PAUSE:
        case SMART_ACTION_SET_FLY:
        case SMART_ACTION_SET_RUN:
        case SMART_ACTION_SET_SWIM:
        case SMART_ACTION_FORCE_DESPAWN:
        case SMART_ACTION_SET_INGAME_PHASE_MASK:
        case SMART_ACTION_SET_UNIT_FLAG:
        case SMART_ACTION_REMOVE_UNIT_FLAG:
        case SMART_ACTION_PLAYMOVIE:
        case SMART_ACTION_MOVE_TO_POS:
        case SMART_ACTION_RESPAWN_TARGET:
        case SMART_ACTION_CLOSE_GOSSIP:
        case SMART_ACTION_TRIGGER_TIMED_EVENT:
        case SMART_ACTION_REMOVE_TIMED_EVENT:
        case SMART_ACTION_OVERRIDE_SCRIPT_BASE_OBJECT:
        case SMART_ACTION_RESET_SCRIPT_BASE_OBJECT:
        case SMART_ACTION_ACTIVATE_GOBJECT:
        case SMART_ACTION_CALL_SCRIPT_RESET:
        case SMART_ACTION_SET_RANGED_MOVEMENT:
        case SMART_ACTION_CALL_TIMED_ACTIONLIST:
        case SMART_ACTION_SET_NPC_FLAG:
        case SMART_ACTION_ADD_NPC_FLAG:
        case SMART_ACTION_REMOVE_NPC_FLAG:
        case SMART_ACTION_CROSS_CAST:
        case SMART_ACTION_CALL_RANDOM_TIMED_ACTIONLIST:
        case SMART_ACTION_RANDOM_MOVE:
        case SMART_ACTION_SET_UNIT_FIELD_BYTES_1:
        case SMART_ACTION_REMOVE_UNIT_FIELD_BYTES_1:
        case SMART_ACTION_INTERRUPT_SPELL:
        case SMART_ACTION_SEND_GO_CUSTOM_ANIM:
        case SMART_ACTION_SET_DYNAMIC_FLAG:
        case SMART_ACTION_ADD_DYNAMIC_FLAG:
        case SMART_ACTION_REMOVE_DYNAMIC_FLAG:
        case SMART_ACTION_JUMP_TO_POS:
        case SMART_ACTION_SEND_GOSSIP_MENU:
        case SMART_ACTION_GO_SET_LOOT_STATE:
        case SMART_ACTION_SEND_TARGET_TO_TARGET:
        case SMART_ACTION_SET_HOME_POS:
        case SMART_ACTION_SET_HEALTH_REGEN:
        case SMART_ACTION_SET_ROOT:
        case SMART_ACTION_SET_GO_FLAG:
        case SMART_ACTION_ADD_GO_FLAG:
        case SMART_ACTION_REMOVE_GO_FLAG:
        case SMART_ACTION_SUMMON_CREATURE_GROUP:
            break;
        default:
            sLog.outError("SmartAIMgr: Not handled action_type(%u), event_type(%u), Entry %d SourceType %u Event %u, skipped.", e.GetActionType(), e.GetEventType(), e.entryOrGuid, e.GetScriptType(), e.event_id);
            return false;
    }

    return true;
}

bool SmartAIMgr::IsTextValid(SmartScriptHolder const& e, uint32 id)
{
    if (e.GetScriptType() != SMART_SCRIPT_TYPE_CREATURE)
        return true;

    uint32 entry = 0;

    if (e.GetEventType() == SMART_EVENT_TEXT_OVER)
    {
        entry = e.event.textOver.creatureEntry;
    }
    else
    {
        switch (e.GetTargetType())
        {
            case SMART_TARGET_CREATURE_DISTANCE:
            case SMART_TARGET_CREATURE_RANGE:
            case SMART_TARGET_CLOSEST_CREATURE:
                return true; // ignore
            default:
                if (e.entryOrGuid < 0)
                {
                    int32 guid = -e.entryOrGuid;
                    CreatureData const* data = sObjectMgr.GetCreatureData((uint32)guid);
                    if (!data)
                    {
                        sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u using non-existent Creature guid %d, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), guid);
                        return false;
                    }
                    else
                        entry = data->id;
                }
                else
                    entry = uint32(e.entryOrGuid);
                break;
        }
    }

    if (!entry || !sCreatureTextMgr->TextExist(entry, uint8(id)))
    {
        sLog.outError("SmartAIMgr: Entry %d SourceType %u Event %u Action %u using non-existent Text id %d, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), id);
        return false;
    }

    return true;
}

void SmartAIMgr::LoadHelperStores()
{
    uint32 oldMSTime = getMSTime();

    SpellEntry const* spellInfo = NULL;
    for (uint32 i = 1; i < sSpellStore.GetNumRows(); ++i)
    {
        SpellEntry const* spellInfo = sSpellStore.LookupEntry(i);
        if (!spellInfo)
            continue;

        for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            if (spellInfo->HasEffect(j, SPELL_EFFECT_SUMMON))
                SummonCreatureSpellStore.insert(std::make_pair(uint32(spellInfo->EffectMiscValue[j]), std::make_pair(i, SpellEffIndex(j))));

            else if (spellInfo->HasEffect(j, SPELL_EFFECT_SUMMON_OBJECT_WILD))
                SummonGameObjectSpellStore.insert(std::make_pair(uint32(spellInfo->EffectMiscValue[j]), std::make_pair(i, SpellEffIndex(j))));

            else if (spellInfo->HasEffect(j, SPELL_EFFECT_KILL_CREDIT))
                KillCreditSpellStore.insert(std::make_pair(uint32(spellInfo->EffectMiscValue[j]), std::make_pair(i, SpellEffIndex(j))));

            else if (spellInfo->HasEffect(j, SPELL_EFFECT_CREATE_ITEM))
                CreateItemSpellStore.insert(std::make_pair(uint32(spellInfo->EffectItemType[j]), std::make_pair(i, SpellEffIndex(j))));
        }
    }

    sLog.outString(">> Loaded SmartAIMgr Helpers in %u ms", GetMSTimeDiffToNow(oldMSTime));
}

void SmartAIMgr::UnLoadHelperStores()
{
    SummonCreatureSpellStore.clear();
    SummonGameObjectSpellStore.clear();
    KillCreditSpellStore.clear();
    CreateItemSpellStore.clear();
}

CacheSpellContainerBounds SmartAIMgr::GetSummonCreatureSpellContainerBounds(uint32 creatureEntry) const
{
    return SummonCreatureSpellStore.equal_range(creatureEntry);
}

CacheSpellContainerBounds SmartAIMgr::GetSummonGameObjectSpellContainerBounds(uint32 gameObjectEntry) const
{
    return SummonGameObjectSpellStore.equal_range(gameObjectEntry);
}

CacheSpellContainerBounds SmartAIMgr::GetKillCreditSpellContainerBounds(uint32 killCredit) const
{
    return KillCreditSpellStore.equal_range(killCredit);
}

CacheSpellContainerBounds SmartAIMgr::GetCreateItemSpellContainerBounds(uint32 itemId) const
{
    return CreateItemSpellStore.equal_range(itemId);
}

