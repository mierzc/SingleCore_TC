/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AccountMgr.h"
#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "Battleground.h"
#include "CalendarMgr.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Group.h"
#include "Guild.h"
#include "GuildFinderMgr.h"
#include "GuildMgr.h"
#include "Language.h"
#include "LFGMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Pet.h"
#include "PlayerDump.h"
#include "Player.h"
#include "QueryCallback.h"
#include "ReputationMgr.h"
#include "GitRevision.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "SocialMgr.h"
#include "UpdateMask.h"
#include "Util.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Metric.h"
#include "BattlenetServerManager.h"

class LoginQueryHolder : public SQLQueryHolder
{
    private:
        uint32 m_accountId;
        ObjectGuid m_guid;
    public:
        LoginQueryHolder(uint32 accountId, ObjectGuid guid)
            : m_accountId(accountId), m_guid(guid) { }
        ObjectGuid GetGuid() const { return m_guid; }
        uint32 GetAccountId() const { return m_accountId; }
        bool Initialize();
};

bool LoginQueryHolder::Initialize()
{
    SetSize(MAX_PLAYER_LOGIN_QUERY);

    bool res = true;
    ObjectGuid::LowType lowGuid = m_guid.GetCounter();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_FROM, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GROUP_MEMBER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GROUP, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INSTANCE);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BOUND_INSTANCES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURAS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_DAILY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DAILY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_WEEKLY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_WEEKLY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_MONTHLY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MONTHLY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_SEASONAL);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SEASONAL_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_REPUTATION);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_REPUTATION, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INVENTORY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_VOID_STORAGE);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_VOID_STORAGE, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACTIONS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACTIONS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_MAILCOUNT);
    stmt->setUInt32(0, lowGuid);
    stmt->setUInt64(1, uint64(time(NULL)));
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_COUNT, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_MAILDATE);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_DATE, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SOCIALLIST);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SOCIAL_LIST, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_HOMEBIND);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_HOME_BIND, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELLCOOLDOWNS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELL_COOLDOWNS, stmt);

    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DECLINEDNAMES);
        stmt->setUInt32(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DECLINED_NAMES, stmt);
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_MEMBER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GUILD, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ARENAINFO);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ARENA_INFO, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACHIEVEMENTS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACHIEVEMENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CRITERIAPROGRESS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CRITERIA_PROGRESS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_EQUIPMENTSETS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_EQUIPMENT_SETS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CUF_PROFILES);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CUF_PROFILES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BGDATA);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BG_DATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GLYPHS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GLYPHS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_TALENTS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TALENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_ACCOUNT_DATA);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SKILLS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SKILLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_RANDOMBG);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_RANDOM_BG, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BANNED);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BANNED, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUSREW);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_REW, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_INSTANCELOCKTIMES);
    stmt->setUInt32(0, m_accountId);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INSTANCE_LOCK_TIMES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_CURRENCY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CURRENCY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CORPSE_LOCATION);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CORPSE_LOCATION, stmt);

    return res;
}

void WorldSession::HandleCharEnum(PreparedQueryResult result)
{
    uint32 charCount = 0;
    ByteBuffer bitBuffer;
    ByteBuffer dataBuffer;

    bitBuffer.WriteBits(0, 23);
    bitBuffer.WriteBit(1);
    if (result)
    {
        _legitCharacters.clear();

        charCount = uint32(result->GetRowCount());
        bitBuffer.reserve(24 * charCount / 8);
        dataBuffer.reserve(charCount * 381);

        bitBuffer.WriteBits(charCount, 17);

        do
        {
            ObjectGuid guid(HighGuid::Player, (*result)[0].GetUInt32());

            TC_LOG_INFO("network", "Loading char guid %s from account %u.", guid.ToString().c_str(), GetAccountId());

            Player::BuildEnumData(result, &dataBuffer, &bitBuffer);

            // Do not allow banned characters to log in
            if (!(*result)[23].GetUInt32())
                _legitCharacters.insert(guid);

            if (!sCharacterCache->HasCharacterCacheEntry(guid)) // This can happen if characters are inserted into the database manually. Core hasn't loaded name data yet.
                sCharacterCache->AddCharacterCacheEntry(guid, GetAccountId(), (*result)[1].GetString(), (*result)[4].GetUInt8(), (*result)[2].GetUInt8(), (*result)[3].GetUInt8(), (*result)[10].GetUInt8());

        } while (result->NextRow());
    }
    else
        bitBuffer.WriteBits(0, 17);

    bitBuffer.FlushBits();

    WorldPacket data(SMSG_CHAR_ENUM, 7 + bitBuffer.size() + dataBuffer.size());
    data.append(bitBuffer);
    if (charCount)
        data.append(dataBuffer);

    SendPacket(&data);
}

void WorldSession::HandleCharEnumOpcode(WorldPacket& /*recvData*/)
{
    // remove expired bans
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_EXPIRED_BANS);
    CharacterDatabase.Execute(stmt);

    /// get all the data necessary for loading all characters (along with their pets) on the account

    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM_DECLINED_NAME);
    else
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM);

    stmt->setUInt8(0, PET_SAVE_AS_CURRENT);
    stmt->setUInt32(1, GetAccountId());

    _queryProcessor.AddQuery(CharacterDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&WorldSession::HandleCharEnum, this, std::placeholders::_1)));
}

void WorldSession::HandleCharCreateOpcode(WorldPacket& recvData)
{
    std::shared_ptr<CharacterCreateInfo> createInfo = std::make_shared<CharacterCreateInfo>();

    recvData >> createInfo->Name
             >> createInfo->Race
             >> createInfo->Class
             >> createInfo->Gender
             >> createInfo->Skin
             >> createInfo->Face
             >> createInfo->HairStyle
             >> createInfo->HairColor
             >> createInfo->FacialHair
             >> createInfo->OutfitId;

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_TEAMMASK))
    {
        if (uint32 mask = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED))
        {
            bool disabled = false;

            switch (Player::TeamForRace(createInfo->Race))
            {
                case ALLIANCE:
                    disabled = (mask & (1 << 0)) != 0;
                    break;
                case HORDE:
                    disabled = (mask & (1 << 1)) != 0;
                    break;
            }

            if (disabled)
            {
                SendCharCreate(CHAR_CREATE_DISABLED);
                return;
            }
        }
    }

    ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(createInfo->Class);
    if (!classEntry)
    {
        TC_LOG_ERROR("network", "Class (%u) not found in DBC while creating new char for account (ID: %u): wrong DBC files or cheater?", createInfo->Class, GetAccountId());

        SendCharCreate(CHAR_CREATE_FAILED);
        return;
    }

    ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(createInfo->Race);
    if (!raceEntry)
    {
        TC_LOG_ERROR("network", "Race (%u) not found in DBC while creating new char for account (ID: %u): wrong DBC files or cheater?", createInfo->Race, GetAccountId());
        SendCharCreate(CHAR_CREATE_FAILED);
        return;
    }

    // prevent character creating Expansion race without Expansion account
    if (raceEntry->expansion > Expansion())
    {
        TC_LOG_ERROR("entities.player.cheat", "Expansion %u account:[%d] tried to Create character with expansion %u race (%u)", Expansion(), GetAccountId(), raceEntry->expansion, createInfo->Race);
        SendCharCreate(CHAR_CREATE_EXPANSION);
        return;
    }

    // prevent character creating Expansion class without Expansion account
    if (classEntry->expansion > Expansion())
    {
        TC_LOG_ERROR("entities.player.cheat", "Expansion %u account:[%d] tried to Create character with expansion %u class (%u)", Expansion(), GetAccountId(), classEntry->expansion, createInfo->Class);
        SendCharCreate(CHAR_CREATE_EXPANSION_CLASS);
        return;
    }

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RACEMASK))
    {
        uint32 raceMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK);
        if ((1 << (createInfo->Race - 1)) & raceMaskDisabled)
        {
            SendCharCreate(CHAR_CREATE_DISABLED);
            return;
        }
    }

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_CLASSMASK))
    {
        uint32 classMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_CLASSMASK);
        if ((1 << (createInfo->Class - 1)) & classMaskDisabled)
        {
            SendCharCreate(CHAR_CREATE_DISABLED);
            return;
        }
    }

    // prevent character creating with invalid name
    if (!normalizePlayerName(createInfo->Name))
    {
        TC_LOG_INFO("entities.player.cheat", "Account:[%d] but tried to Create character with empty [name] ", GetAccountId());
        SendCharCreate(CHAR_NAME_NO_NAME);
        return;
    }

    // check name limitations
    ResponseCodes res = ObjectMgr::CheckPlayerName(createInfo->Name, GetSessionDbcLocale(), true);
    if (res != CHAR_NAME_SUCCESS)
    {
        SendCharCreate(res);
        return;
    }

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME) && sObjectMgr->IsReservedName(createInfo->Name))
    {
        SendCharCreate(CHAR_NAME_RESERVED);
        return;
    }

    if (createInfo->Class == CLASS_DEATH_KNIGHT && !HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_DEATH_KNIGHT))
    {
        // speedup check for death knight class disabled case
        if (sWorld->getIntConfig(CONFIG_DEATH_KNIGHTS_PER_REALM) == 0)
        {
            SendCharCreate(CHAR_CREATE_UNIQUE_CLASS_LIMIT);
            return;
        }

        // speedup check for death knight class disabled case
        if (sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_MIN_LEVEL_FOR_DEATH_KNIGHT) > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            SendCharCreate(CHAR_CREATE_LEVEL_REQUIREMENT);
            return;
        }
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHECK_NAME);
    stmt->setString(0, createInfo->Name);

    _queryProcessor.AddQuery(CharacterDatabase.AsyncQuery(stmt)
        .WithChainingPreparedCallback([this](QueryCallback& queryCallback, PreparedQueryResult result)
    {
        if (result)
        {
            SendCharCreate(CHAR_CREATE_NAME_IN_USE);
            return;
        }

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_SUM_REALM_CHARACTERS);
        stmt->setUInt32(0, GetAccountId());
        queryCallback.SetNextQuery(LoginDatabase.AsyncQuery(stmt));
    })
        .WithChainingPreparedCallback([this](QueryCallback& queryCallback, PreparedQueryResult result)
    {
        uint64 acctCharCount = 0;
        if (result)
        {
            Field* fields = result->Fetch();
            acctCharCount = uint64(fields[0].GetDouble());
        }

        if (acctCharCount >= sWorld->getIntConfig(CONFIG_CHARACTERS_PER_ACCOUNT))
        {
            SendCharCreate(CHAR_CREATE_ACCOUNT_LIMIT);
            return;
        }

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SUM_CHARS);
        stmt->setUInt32(0, GetAccountId());
        queryCallback.SetNextQuery(CharacterDatabase.AsyncQuery(stmt));
    })
        .WithChainingPreparedCallback([this, createInfo](QueryCallback& queryCallback, PreparedQueryResult result)
    {
        if (result)
        {
            Field* fields = result->Fetch();
            createInfo->CharCount = uint8(fields[0].GetUInt64()); // SQL's COUNT() returns uint64 but it will always be less than uint8.Max

            if (createInfo->CharCount >= sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM))
            {
                SendCharCreate(CHAR_CREATE_SERVER_LIMIT);
                return;
            }
        }

        bool allowTwoSideAccounts = !sWorld->IsPvPRealm() || HasPermission(rbac::RBAC_PERM_TWO_SIDE_CHARACTER_CREATION);
        uint32 skipCinematics = sWorld->getIntConfig(CONFIG_SKIP_CINEMATICS);

        std::function<void(PreparedQueryResult)> finalizeCharacterCreation = [this, createInfo](PreparedQueryResult result)
        {
            bool haveSameRace = false;
            uint32 deathKnightReqLevel = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_MIN_LEVEL_FOR_DEATH_KNIGHT);
            bool hasDeathKnightReqLevel = (deathKnightReqLevel == 0);
            bool allowTwoSideAccounts = !sWorld->IsPvPRealm() || HasPermission(rbac::RBAC_PERM_TWO_SIDE_CHARACTER_CREATION);
            uint32 skipCinematics = sWorld->getIntConfig(CONFIG_SKIP_CINEMATICS);
            bool checkDeathKnightReqs = createInfo->Class == CLASS_DEATH_KNIGHT && !HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_DEATH_KNIGHT);

            if (result)
            {
                uint32 team = Player::TeamForRace(createInfo->Race);
                uint32 freeDeathKnightSlots = sWorld->getIntConfig(CONFIG_DEATH_KNIGHTS_PER_REALM);

                Field* field = result->Fetch();
                uint8 accRace = field[1].GetUInt8();

                if (checkDeathKnightReqs)
                {
                    uint8 accClass = field[2].GetUInt8();
                    if (accClass == CLASS_DEATH_KNIGHT)
                    {
                        if (freeDeathKnightSlots > 0)
                            --freeDeathKnightSlots;

                        if (freeDeathKnightSlots == 0)
                        {
                            SendCharCreate(CHAR_CREATE_UNIQUE_CLASS_LIMIT);
                            return;
                        }
                    }

                    if (!hasDeathKnightReqLevel)
                    {
                        uint8 accLevel = field[0].GetUInt8();
                        if (accLevel >= deathKnightReqLevel)
                            hasDeathKnightReqLevel = true;
                    }
                }

                // need to check team only for first character
                /// @todo what to if account already has characters of both races?
                if (!allowTwoSideAccounts)
                {
                    uint32 accTeam = 0;
                    if (accRace > 0)
                        accTeam = Player::TeamForRace(accRace);

                    if (accTeam != team)
                    {
                        SendCharCreate(CHAR_CREATE_PVP_TEAMS_VIOLATION);
                        return;
                    }
                }

                // search same race for cinematic or same class if need
                /// @todo check if cinematic already shown? (already logged in?; cinematic field)
                while ((skipCinematics == 1 && !haveSameRace) || createInfo->Class == CLASS_DEATH_KNIGHT)
                {
                    if (!result->NextRow())
                        break;

                    field = result->Fetch();
                    accRace = field[1].GetUInt8();

                    if (!haveSameRace)
                        haveSameRace = createInfo->Race == accRace;

                    if (checkDeathKnightReqs)
                    {
                        uint8 acc_class = field[2].GetUInt8();
                        if (acc_class == CLASS_DEATH_KNIGHT)
                        {
                            if (freeDeathKnightSlots > 0)
                                --freeDeathKnightSlots;

                            if (freeDeathKnightSlots == 0)
                            {
                                SendCharCreate(CHAR_CREATE_UNIQUE_CLASS_LIMIT);
                                return;
                            }
                        }

                        if (!hasDeathKnightReqLevel)
                        {
                            uint8 acc_level = field[0].GetUInt8();
                            if (acc_level >= deathKnightReqLevel)
                                hasDeathKnightReqLevel = true;
                        }
                    }
                }
            }

            if (checkDeathKnightReqs && !hasDeathKnightReqLevel)
            {
                SendCharCreate(CHAR_CREATE_LEVEL_REQUIREMENT);
                return;
            }

            Player newChar(this);
            newChar.GetMotionMaster()->Initialize();
            if (!newChar.Create(sObjectMgr->GetGenerator<HighGuid::Player>().Generate(), createInfo.get()))

            {
                // Player not create (race/class/etc problem?)
                newChar.CleanupsBeforeDelete();

                SendCharCreate(CHAR_CREATE_ERROR);
                return;
            }

            if ((haveSameRace && skipCinematics == 1) || skipCinematics == 2)
                newChar.setCinematic(1);                          // not show intro

            newChar.SetAtLoginFlag(AT_LOGIN_FIRST);               // First login

                                                                  // Player created, save it now
            newChar.SaveToDB(true);
            createInfo->CharCount += 1;

            SQLTransaction trans = LoginDatabase.BeginTransaction();

            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_REALM_CHARACTERS_BY_REALM);
            stmt->setUInt32(0, GetAccountId());
            stmt->setUInt32(1, realm.Id.Realm);
            trans->Append(stmt);

            stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_REALM_CHARACTERS);
            stmt->setUInt32(0, createInfo->CharCount);
            stmt->setUInt32(1, GetAccountId());
            stmt->setUInt32(2, realm.Id.Realm);
            trans->Append(stmt);

            LoginDatabase.CommitTransaction(trans);

            SendCharCreate(CHAR_CREATE_SUCCESS);

            TC_LOG_INFO("entities.player.character", "Account: %d (IP: %s) Create Character:[%s] (GUID: %u)", GetAccountId(), GetRemoteAddress().c_str(), createInfo->Name.c_str(), newChar.GetGUID().GetCounter());
            sScriptMgr->OnPlayerCreate(&newChar);
            sCharacterCache->AddCharacterCacheEntry(newChar.GetGUID(), GetAccountId(), newChar.GetName(), newChar.GetByteValue(PLAYER_BYTES_3, PLAYER_BYTES_3_OFFSET_GENDER), newChar.getRace(), newChar.getClass(), newChar.getLevel());

            newChar.CleanupsBeforeDelete();
        };

        if (allowTwoSideAccounts && !skipCinematics && createInfo->Class != CLASS_DEATH_KNIGHT)
        {
            finalizeCharacterCreation(PreparedQueryResult(nullptr));
            return;
        }

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CREATE_INFO);
        stmt->setUInt32(0, GetAccountId());
        stmt->setUInt32(1, (skipCinematics == 1 || createInfo->Class == CLASS_DEATH_KNIGHT) ? 10 : 1);
        queryCallback.WithPreparedCallback(std::move(finalizeCharacterCreation)).SetNextQuery(CharacterDatabase.AsyncQuery(stmt));
    }));
}

void WorldSession::HandleCharDeleteOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData >> guid;
    // Initiating
    uint32 initAccountId = GetAccountId();

    // can't delete loaded character
    if (ObjectAccessor::FindPlayer(guid))
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        return;
    }

    uint32 accountId = 0;
    uint8 level = 0;
    std::string name;

    // is guild leader
    if (sGuildMgr->GetGuildByLeader(guid))
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        SendCharDelete(CHAR_DELETE_FAILED_GUILD_LEADER);
        return;
    }

    // is arena team captain
    if (sArenaTeamMgr->GetArenaTeamByCaptain(guid))
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        SendCharDelete(CHAR_DELETE_FAILED_ARENA_CAPTAIN);
        return;
    }

    CharacterCacheEntry const* characterInfo = sCharacterCache->GetCharacterCacheByGuid(guid);
    if (!characterInfo)
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        return;
    }

    accountId = characterInfo->AccountId;
    name = characterInfo->Name;
    level = characterInfo->Level;

    // prevent deleting other players' characters using cheating tools
    if (accountId != initAccountId)
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        return;
    }

    TC_LOG_INFO("entities.player.character", "Account: %d, IP: %s deleted character: %s, %s, Level: %u", accountId, GetRemoteAddress().c_str(), name.c_str(), guid.ToString().c_str(), level);

    // To prevent hook failure, place hook before removing reference from DB
    sScriptMgr->OnPlayerDelete(guid, initAccountId); // To prevent race conditioning, but as it also makes sense, we hand the accountId over for successful delete.
    // Shouldn't interfere with character deletion though

    if (sLog->ShouldLog("entities.player.dump", LOG_LEVEL_INFO)) // optimize GetPlayerDump call
    {
        std::string dump;
        if (PlayerDumpWriter().GetDump(guid.GetCounter(), dump))
            sLog->outCharDump(dump.c_str(), accountId, guid.GetRawValue(), name.c_str());
    }

    sGuildFinderMgr->RemoveAllMembershipRequestsFromPlayer(guid);
    sCalendarMgr->RemoveAllPlayerEventsAndInvites(guid);
    Player::DeleteFromDB(guid, accountId);

    SendCharDelete(CHAR_DELETE_SUCCESS);
}

void WorldSession::HandlePlayerLoginOpcode(WorldPacket& recvData)
{
    if (PlayerLoading() || GetPlayer() != NULL)
    {
        TC_LOG_ERROR("network", "Player tries to login again, AccountId = %d", GetAccountId());
        KickPlayer();
        return;
    }

    m_playerLoading = true;
    ObjectGuid playerGuid;

    TC_LOG_DEBUG("network", "WORLD: Recvd Player Logon Message");
    playerGuid[2] = recvData.ReadBit();
    playerGuid[3] = recvData.ReadBit();
    playerGuid[0] = recvData.ReadBit();
    playerGuid[6] = recvData.ReadBit();
    playerGuid[4] = recvData.ReadBit();
    playerGuid[5] = recvData.ReadBit();
    playerGuid[1] = recvData.ReadBit();
    playerGuid[7] = recvData.ReadBit();

    recvData.ReadByteSeq(playerGuid[2]);
    recvData.ReadByteSeq(playerGuid[7]);
    recvData.ReadByteSeq(playerGuid[0]);
    recvData.ReadByteSeq(playerGuid[3]);
    recvData.ReadByteSeq(playerGuid[5]);
    recvData.ReadByteSeq(playerGuid[6]);
    recvData.ReadByteSeq(playerGuid[1]);
    recvData.ReadByteSeq(playerGuid[4]);

    TC_LOG_DEBUG("network", "Character %s logging in", playerGuid.ToString().c_str());

    if (!IsLegitCharacterForAccount(playerGuid))
    {
        TC_LOG_ERROR("network", "Account (%u) can't login with that character (%s).", GetAccountId(), playerGuid.ToString().c_str());
        KickPlayer();
        return;
    }

    LoginQueryHolder *holder = new LoginQueryHolder(GetAccountId(), playerGuid);
    if (!holder->Initialize())
    {
        delete holder;                                      // delete all unprocessed queries
        m_playerLoading = false;
        return;
    }

    _charLoginCallback = CharacterDatabase.DelayQueryHolder(holder);
}

void WorldSession::HandleLoadScreenOpcode(WorldPacket& recvPacket)
{
    TC_LOG_INFO("misc", "WORLD: Recvd CMSG_LOAD_SCREEN");
    uint32 mapID;

    recvPacket >> mapID;
    recvPacket.ReadBit();

    // TODO: Do something with this packet
}

void WorldSession::HandlePlayerLogin(LoginQueryHolder* holder)
{
    ObjectGuid playerGuid = holder->GetGuid();

    Player* pCurrChar = new Player(this);
     // for send server info and strings (config)
    ChatHandler chH = ChatHandler(pCurrChar->GetSession());

    // "GetAccountId() == db stored account id" checked in LoadFromDB (prevent login not own character using cheating tools)
    if (!pCurrChar->LoadFromDB(playerGuid, holder))
    {
        SetPlayer(NULL);
        KickPlayer();                                       // disconnect client, player no set to session and it will not deleted or saved at kick
        delete pCurrChar;                                   // delete it manually
        delete holder;                                      // delete all unprocessed queries
        m_playerLoading = false;
        return;
    }

    pCurrChar->GetMotionMaster()->Initialize();
    pCurrChar->SendDungeonDifficulty(false);

    WorldPacket data(SMSG_LOGIN_VERIFY_WORLD, 20);
    data << pCurrChar->GetMapId();
    data << pCurrChar->GetPositionX();
    data << pCurrChar->GetPositionY();
    data << pCurrChar->GetPositionZ();
    data << pCurrChar->GetOrientation();
    SendPacket(&data);

    // load player specific part before send times
    LoadAccountData(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA), PER_CHARACTER_CACHE_MASK);
    SendAccountDataTimes(PER_CHARACTER_CACHE_MASK);

    bool featureBit4 = true;
    data.Initialize(SMSG_FEATURE_SYSTEM_STATUS, 7);         // checked in 4.2.2
    data << uint8(2);                                       // unknown value
    data << uint32(1);
    data << uint32(1);
    data << uint32(2);
    data << uint32(0);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(featureBit4);
    data.WriteBit(0);
    data.WriteBit(0);
    data.FlushBits();
    if (featureBit4)
    {
        data << uint32(1);
        data << uint32(0);
        data << uint32(10);
        data << uint32(60);
    }

    //if (featureBit5)
    //{
    //    data << uint32(0);
    //    data << uint32(0);
    //    data << uint32(0);
    //}
    SendPacket(&data);

    // Send MOTD
    {
        data.Initialize(SMSG_MOTD, 50);                     // new in 2.0.1
        data << (uint32)0;

        uint32 linecount=0;
        std::string str_motd = sWorld->GetMotd();
        std::string::size_type pos, nextpos;

        pos = 0;
        while ((nextpos= str_motd.find('@', pos)) != std::string::npos)
        {
            if (nextpos != pos)
            {
                data << str_motd.substr(pos, nextpos-pos);
                ++linecount;
            }
            pos = nextpos+1;
        }

        if (pos<str_motd.length())
        {
            data << str_motd.substr(pos);
            ++linecount;
        }

        data.put(0, linecount);

        SendPacket(&data);
        TC_LOG_DEBUG("network", "WORLD: Sent motd (SMSG_MOTD)");

        // send server info
        if (sWorld->getIntConfig(CONFIG_ENABLE_SINFO_LOGIN) == 1)
            chH.PSendSysMessage(GitRevision::GetFullVersion());

        TC_LOG_DEBUG("network", "WORLD: Sent server info");
    }

    //QueryResult* result = CharacterDatabase.PQuery("SELECT guildid, rank FROM guild_member WHERE guid = '%u'", pCurrChar->GetGUID().GetCounter());
    if (PreparedQueryResult resultGuild = holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_GUILD))
    {
        Field* fields = resultGuild->Fetch();
        pCurrChar->SetInGuild(fields[0].GetUInt32());
        pCurrChar->SetRank(fields[1].GetUInt8());
        if (Guild* guild = sGuildMgr->GetGuildById(pCurrChar->GetGuildId()))
            pCurrChar->SetGuildLevel(guild->GetLevel());
    }
    else if (pCurrChar->GetGuildId())                        // clear guild related fields in case wrong data about non existed membership
    {
        pCurrChar->SetInGuild(0);
        pCurrChar->SetRank(0);
        pCurrChar->SetGuildLevel(0);
    }

    data.Initialize(SMSG_LEARNED_DANCE_MOVES, 4+4);
    data << uint64(0);
    SendPacket(&data);

    data.Initialize(SMSG_HOTFIX_INFO);
    HotfixData const& hotfix = sObjectMgr->GetHotfixData();
    data.WriteBits(hotfix.size(), 22);
    data.FlushBits();
    for (uint32 i = 0; i < hotfix.size(); ++i)
    {
        data << uint32(hotfix[i].Type);
        data << uint32(hotfix[i].Timestamp);
        data << uint32(hotfix[i].Entry);
    }
    SendPacket(&data);

    pCurrChar->SendInitialPacketsBeforeAddToMap();

    //Show cinematic at the first time that player login
    if (!pCurrChar->getCinematic())
    {
        pCurrChar->setCinematic(1);

        if (ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(pCurrChar->getClass()))
        {
            if (cEntry->CinematicSequence)
                pCurrChar->SendCinematicStart(cEntry->CinematicSequence);
            else if (ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(pCurrChar->getRace()))
                pCurrChar->SendCinematicStart(rEntry->CinematicSequence);

            // send new char string if not empty
            if (!sWorld->GetNewCharString().empty())
                chH.PSendSysMessage("%s", sWorld->GetNewCharString().c_str());
        }
    }

    if (!pCurrChar->GetMap()->AddPlayerToMap(pCurrChar))
    {
        AreaTriggerStruct const* at = sObjectMgr->GetGoBackTrigger(pCurrChar->GetMapId());
        if (at)
            pCurrChar->TeleportTo(at->target_mapId, at->target_X, at->target_Y, at->target_Z, pCurrChar->GetOrientation());
        else
            pCurrChar->TeleportTo(pCurrChar->m_homebindMapId, pCurrChar->m_homebindX, pCurrChar->m_homebindY, pCurrChar->m_homebindZ, pCurrChar->GetOrientation());
    }

    ObjectAccessor::AddObject(pCurrChar);
    //TC_LOG_DEBUG("Player %s added to Map.", pCurrChar->GetName().c_str());

    if (pCurrChar->GetGuildId() != 0)
    {
        if (Guild* guild = sGuildMgr->GetGuildById(pCurrChar->GetGuildId()))
            guild->SendLoginInfo(this);
        else
        {
            // remove wrong guild data
            TC_LOG_ERROR("misc", "Player %s (GUID: %u) marked as member of not existing guild (id: %u), removing guild membership for player.", pCurrChar->GetName().c_str(), pCurrChar->GetGUID().GetCounter(), pCurrChar->GetGuildId());
            pCurrChar->SetInGuild(0);
        }
    }

    pCurrChar->SendInitialPacketsAfterAddToMap();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_ONLINE);
    stmt->setUInt32(0, pCurrChar->GetGUID().GetCounter());
    CharacterDatabase.Execute(stmt);

    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_ONLINE);
    stmt->setUInt32(0, GetAccountId());
    LoginDatabase.Execute(stmt);

    pCurrChar->SetInGameTime(GameTime::GetGameTimeMS());

    // announce group about member online (must be after add to player list to receive announce to self)
    if (Group* group = pCurrChar->GetGroup())
    {
        //pCurrChar->groupInfo.group->SendInit(this); // useless
        group->SendUpdate();
        group->SendRaidMarkerUpdateToPlayer(playerGuid);
        group->ResetMaxEnchantingLevel();
    }

    // friend status
    sSocialMgr->SendFriendStatus(pCurrChar, FRIEND_ONLINE, pCurrChar->GetGUID(), true);

    // Place character in world (and load zone) before some object loading
    pCurrChar->LoadCorpse(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_CORPSE_LOCATION));

    // setting Ghost+speed if dead
    if (pCurrChar->m_deathState != ALIVE)
    {
        // not blizz like, we must correctly save and load player instead...
        if (pCurrChar->getRace() == RACE_NIGHTELF && !pCurrChar->HasAura(20584))
            pCurrChar->CastSpell(pCurrChar, 20584, true, 0);// auras SPELL_AURA_INCREASE_SPEED(+speed in wisp form), SPELL_AURA_INCREASE_SWIM_SPEED(+swim speed in wisp form), SPELL_AURA_TRANSFORM (to wisp form)

        if (!pCurrChar->HasAura(8326))
            pCurrChar->CastSpell(pCurrChar, 8326, true, 0);     // auras SPELL_AURA_GHOST, SPELL_AURA_INCREASE_SPEED(why?), SPELL_AURA_INCREASE_SWIM_SPEED(why?)

        pCurrChar->SetWaterWalking(true);
    }

    pCurrChar->ContinueTaxiFlight();

    // reset for all pets before pet loading
    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS))
        Pet::resetTalentsForAllPetsOf(pCurrChar);

    // Load pet if any (if player not alive and in taxi flight or another then pet will remember as temporary unsummoned)
    pCurrChar->LoadPet();

    // Set FFA PvP for non GM in non-rest mode
    if (sWorld->IsFFAPvPRealm() && !pCurrChar->IsGameMaster() && !pCurrChar->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING))
        pCurrChar->SetByteFlag(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_PVP_FLAG, UNIT_BYTE2_FLAG_FFA_PVP);

    if (pCurrChar->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP))
        pCurrChar->SetContestedPvP();

    // Apply at_login requests
    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_SPELLS))
    {
        pCurrChar->ResetSpells();
        SendNotification(LANG_RESET_SPELLS);
    }

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_TALENTS))
    {
        pCurrChar->ResetTalents(true);
        pCurrChar->SendTalentsInfoData(false);              // original talents send already in to SendInitialPacketsBeforeAddToMap, resend reset state
        SendNotification(LANG_RESET_TALENTS);
    }

    bool firstLogin = pCurrChar->HasAtLoginFlag(AT_LOGIN_FIRST);
    if (firstLogin)
    {
        pCurrChar->RemoveAtLoginFlag(AT_LOGIN_FIRST);

        PlayerInfo const* info = sObjectMgr->GetPlayerInfo(pCurrChar->getRace(), pCurrChar->getClass());
        for (uint32 spellId : info->castSpells)
            pCurrChar->CastSpell(pCurrChar, spellId, true);

        // Start with all map areas explored if enabled
        if (sWorld->getBoolConfig(CONFIG_START_ALL_EXPLORED))
            for (uint8 i = 0; i < PLAYER_EXPLORED_ZONES_SIZE; ++i)
                pCurrChar->SetFlag(PLAYER_EXPLORED_ZONES_1 + i, 0xFFFFFFFF);

        // Max relevant reputations if "StartAllReputation" is enabled
        if (sWorld->getBoolConfig(CONFIG_START_ALL_REP))
        {
            ReputationMgr& repMgr = pCurrChar->GetReputationMgr();
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 942), 42999, false); // Cenarion Expedition
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 935), 42999, false); // The Sha'tar
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 936), 42999, false); // Shattrath City
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1011), 42999, false); // Lower City
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 970), 42999, false); // Sporeggar
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 967), 42999, false); // The Violet Eye
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 989), 42999, false); // Keepers of Time
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 932), 42999, false); // The Aldor
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 934), 42999, false); // The Scryers
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1038), 42999, false); // Ogri'la
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1077), 42999, false); // Shattered Sun Offensive
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1106), 42999, false); // Argent Crusade
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1104), 42999, false); // Frenzyheart Tribe
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1090), 42999, false); // Kirin Tor
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1098), 42999, false); // Knights of the Ebon Blade
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1156), 42999, false); // The Ashen Verdict
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1073), 42999, false); // The Kalu'ak
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1105), 42999, false); // The Oracles
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1119), 42999, false); // The Sons of Hodir
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1091), 42999, false); // The Wyrmrest Accord
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1204), 42999, false); // Avengers of Hyjal
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1158), 42999, false); // Guardians of Hyjal
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1173), 42999, false); // Ramkahen
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1171), 42999, false); // Therazane
            repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1135), 42999, false); // The Earthen Ring

            // Factions depending on team, like cities and some more stuff
            switch (pCurrChar->GetTeam())
            {
                case ALLIANCE:
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(  72), 42999, false); // Stormwind
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(  47), 42999, false); // Ironforge
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(  69), 42999, false); // Darnassus
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 930), 42999, false); // Exodar
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 730), 42999, false); // Stormpike Guard
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 978), 42999, false); // Kurenai
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(  54), 42999, false); // Gnomeregan Exiles
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 946), 42999, false); // Honor Hold
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1037), 42999, false); // Alliance Vanguard
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1068), 42999, false); // Explorers' League
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1126), 42999, false); // The Frostborn
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1094), 42999, false); // The Silver Covenant
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1050), 42999, false); // Valiance Expedition
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1177), 42999, false); // Baradin's Wardens
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1174), 42999, false); // Wildhammer Clan
                    break;
                case HORDE:
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(  76), 42999, false); // Orgrimmar
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(  68), 42999, false); // Undercity
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(  81), 42999, false); // Thunder Bluff
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 911), 42999, false); // Silvermoon City
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 729), 42999, false); // Frostwolf Clan
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 941), 42999, false); // The Mag'har
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 530), 42999, false); // Darkspear Trolls
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry( 947), 42999, false); // Thrallmar
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1052), 42999, false); // Horde Expedition
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1067), 42999, false); // The Hand of Vengeance
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1124), 42999, false); // The Sunreavers
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1064), 42999, false); // The Taunka
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1085), 42999, false); // Warsong Offensive
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1172), 42999, false); // Dragonmaw Clan
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(1178), 42999, false); // Hellscream's Reach
                    break;
                default:
                    break;
            }
            repMgr.SendState(nullptr);
        }
    }

    // show time before shutdown if shutdown planned.
    if (sWorld->IsShuttingDown())
        sWorld->ShutdownMsg(true, pCurrChar);

    if (sWorld->getBoolConfig(CONFIG_ALL_TAXI_PATHS))
        pCurrChar->SetTaxiCheater(true);

    if (pCurrChar->IsGameMaster())
        SendNotification(LANG_GM_ON);

    std::string IP_str = GetRemoteAddress();
    TC_LOG_INFO("entities.player.character", "Account: %d (IP: %s) Login Character:[%s] (GUID: %u) Level: %d",
        GetAccountId(), IP_str.c_str(), pCurrChar->GetName().c_str(), pCurrChar->GetGUID().GetCounter(), pCurrChar->getLevel());

    if (!pCurrChar->IsStandState() && !pCurrChar->HasUnitState(UNIT_STATE_STUNNED))
        pCurrChar->SetStandState(UNIT_STAND_STATE_STAND);

    m_playerLoading = false;

    // Handle Login-Achievements (should be handled after loading)
    _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN, 1);

    sScriptMgr->OnPlayerLogin(pCurrChar, firstLogin);

    TC_METRIC_EVENT("player_events", "Login", pCurrChar->GetName());

    sBattlenetServer.SendChangeToonOnlineState(GetBattlenetAccountId(), GetAccountId(), _player->GetGUID(), _player->GetName(), true);

    delete holder;
}

void WorldSession::HandleSetFactionAtWar(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_SET_FACTION_ATWAR");

    uint32 repListID;
    uint8  flag;

    recvData >> repListID;
    recvData >> flag;

    GetPlayer()->GetReputationMgr().SetAtWar(repListID, flag != 0);
}

//I think this function is never used :/ I dunno, but i guess this opcode not exists
void WorldSession::HandleSetFactionCheat(WorldPacket& /*recvData*/)
{
    TC_LOG_ERROR("network", "WORLD SESSION: HandleSetFactionCheat, not expected call, please report.");
    GetPlayer()->GetReputationMgr().SendState(nullptr);
}

void WorldSession::HandleTutorialFlag(WorldPacket& recvData)
{
    uint32 data;
    recvData >> data;

    uint8 index = uint8(data / 32);
    if (index >= MAX_ACCOUNT_TUTORIAL_VALUES)
        return;

    uint32 value = (data % 32);

    uint32 flag = GetTutorialInt(index);
    flag |= (1 << value);
    SetTutorialInt(index, flag);
}

void WorldSession::HandleTutorialClear(WorldPacket& /*recvData*/)
{
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        SetTutorialInt(i, 0xFFFFFFFF);
}

void WorldSession::HandleTutorialReset(WorldPacket& /*recvData*/)
{
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        SetTutorialInt(i, 0x00000000);
}

void WorldSession::HandleSetWatchedFactionOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_SET_WATCHED_FACTION");
    uint32 fact;
    recvData >> fact;
    GetPlayer()->SetUInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, fact);
}

void WorldSession::HandleSetFactionInactiveOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_SET_FACTION_INACTIVE");
    uint32 replistid;
    uint8 inactive;
    recvData >> replistid >> inactive;

    _player->GetReputationMgr().SetInactive(replistid, inactive != 0);
}

void WorldSession::HandleShowingHelmOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_SHOWING_HELM for %s", _player->GetName().c_str());
    recvData.read_skip<uint8>(); // unknown, bool?
    _player->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_HELM);
}

void WorldSession::HandleShowingCloakOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_SHOWING_CLOAK for %s", _player->GetName().c_str());
    recvData.read_skip<uint8>(); // unknown, bool?
    _player->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_CLOAK);
}

void WorldSession::HandleCharRenameOpcode(WorldPacket& recvData)
{
    std::shared_ptr<CharacterRenameInfo> renameInfo = std::make_shared<CharacterRenameInfo>();

    recvData >> renameInfo->Guid
             >> renameInfo->Name;

    // prevent character rename to invalid name
    if (!normalizePlayerName(renameInfo->Name))
    {
        SendCharRename(CHAR_NAME_NO_NAME, renameInfo.get());
        return;
    }

    ResponseCodes res = ObjectMgr::CheckPlayerName(renameInfo->Name, GetSessionDbcLocale(), true);
    if (res != CHAR_NAME_SUCCESS)
    {
        SendCharRename(res, renameInfo.get());
        return;
    }

    // check name limitations
    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME) && sObjectMgr->IsReservedName(renameInfo->Name))
    {
        SendCharRename(CHAR_NAME_RESERVED, renameInfo.get());
        return;
    }

    // Ensure that the character belongs to the current account, that rename at login is enabled
    // and that there is no character with the desired new name
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_FREE_NAME);

    stmt->setUInt32(0, renameInfo->Guid.GetCounter());
    stmt->setUInt32(1, GetAccountId());
    stmt->setString(2, renameInfo->Name);

    _queryProcessor.AddQuery(CharacterDatabase.AsyncQuery(stmt)
        .WithPreparedCallback(std::bind(&WorldSession::HandleCharRenameCallback, this, renameInfo, std::placeholders::_1)));
}

void WorldSession::HandleCharRenameCallback(std::shared_ptr<CharacterRenameInfo> renameInfo, PreparedQueryResult result)
{
    if (!result)
    {
        SendCharRename(CHAR_CREATE_ERROR, renameInfo.get());
        return;
    }

    Field* fields = result->Fetch();

    ObjectGuid::LowType guidLow = fields[0].GetUInt32();
    std::string oldName = fields[1].GetString();
    uint16 atLoginFlags = fields[2].GetUInt16();

    if (!(atLoginFlags & AT_LOGIN_RENAME))
    {
        SendCharRename(CHAR_CREATE_ERROR, renameInfo.get());
        return;
    }

    atLoginFlags &= ~AT_LOGIN_RENAME;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    // Update name and at_login flag in the db
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_NAME_AT_LOGIN);

    stmt->setString(0, renameInfo->Name);
    stmt->setUInt16(1, atLoginFlags);
    stmt->setUInt32(2, guidLow);

    CharacterDatabase.Execute(stmt);

    // Removed declined name from db
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_DECLINED_NAME);

    stmt->setUInt32(0, guidLow);

    CharacterDatabase.Execute(stmt);

    TC_LOG_INFO("entities.player.character", "Account: %d (IP: %s) Character:[%s] (%s) Changed name to: %s", GetAccountId(), GetRemoteAddress().c_str(), oldName.c_str(), renameInfo->Guid.ToString().c_str(), renameInfo->Name.c_str());

    SendCharRename(RESPONSE_SUCCESS, renameInfo.get());

    sCharacterCache->UpdateCharacterData(renameInfo->Guid, renameInfo->Name);
}

void WorldSession::HandleSetPlayerDeclinedNames(WorldPacket& recvData)
{
    ObjectGuid guid;

    recvData >> guid;

    // not accept declined names for unsupported languages
    std::string name;
    if (!sCharacterCache->GetCharacterNameByGuid(guid, name))
    {
        SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_ERROR, guid);
        return;
    }

    std::wstring wname;
    if (!Utf8toWStr(name, wname))
    {
        SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_ERROR, guid);
        return;
    }

    if (!isCyrillicCharacter(wname[0]))                      // name already stored as only single alphabet using
    {
        SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_ERROR, guid);
        return;
    }

    std::string name2;
    DeclinedName declinedname;

    recvData >> name2;

    if (name2 != name)                                       // character have different name
    {
        SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_ERROR, guid);
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        recvData >> declinedname.name[i];
        if (!normalizePlayerName(declinedname.name[i]))
        {
            SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_ERROR, guid);
            return;
        }
    }

    if (!ObjectMgr::CheckDeclinedNames(wname, declinedname))
    {
        SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_ERROR, guid);
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        CharacterDatabase.EscapeString(declinedname.name[i]);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_DECLINED_NAME);
    stmt->setUInt32(0, guid.GetCounter());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_DECLINED_NAME);
    stmt->setUInt32(0, guid.GetCounter());

    for (uint8 i = 0; i < 5; i++)
        stmt->setString(i+1, declinedname.name[i]);

    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_SUCCESS, guid);
}

void WorldSession::HandleAlterAppearance(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_ALTER_APPEARANCE");

    uint32 Hair, Color, FacialHair, SkinColor;
    recvData >> Hair >> Color >> FacialHair >> SkinColor;

    BarberShopStyleEntry const* bs_hair = sBarberShopStyleStore.LookupEntry(Hair);

    if (!bs_hair || bs_hair->type != 0 || bs_hair->race != _player->getRace() || bs_hair->gender != _player->GetByteValue(PLAYER_BYTES_3, PLAYER_BYTES_3_OFFSET_GENDER))
        return;

    BarberShopStyleEntry const* bs_facialHair = sBarberShopStyleStore.LookupEntry(FacialHair);

    if (!bs_facialHair || bs_facialHair->type != 2 || bs_facialHair->race != _player->getRace() || bs_facialHair->gender != _player->GetByteValue(PLAYER_BYTES_3, PLAYER_BYTES_3_OFFSET_GENDER))
        return;

    BarberShopStyleEntry const* bs_skinColor = sBarberShopStyleStore.LookupEntry(SkinColor);

    if (bs_skinColor && (bs_skinColor->type != 3 || bs_skinColor->race != _player->getRace() || bs_skinColor->gender != _player->GetByteValue(PLAYER_BYTES_3, PLAYER_BYTES_3_OFFSET_GENDER)))
        return;

    if (!Player::ValidateAppearance(_player->getRace(), _player->getClass(), _player->GetByteValue(PLAYER_BYTES_3, PLAYER_BYTES_3_OFFSET_GENDER),
        bs_hair->hair_id,
        Color,
        _player->GetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_FACE_ID),
        bs_facialHair->hair_id,
        bs_skinColor ? bs_skinColor->hair_id : _player->GetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_SKIN_ID)))

        return;

    GameObject* go = _player->FindNearestGameObjectOfType(GAMEOBJECT_TYPE_BARBER_CHAIR, 5.0f);
    if (!go)
    {
        SendBarberShopResult(BARBER_SHOP_RESULT_NOT_ON_CHAIR);
        return;
    }

    if (_player->GetStandState() != UNIT_STAND_STATE_SIT_LOW_CHAIR + go->GetGOInfo()->barberChair.chairheight)
    {
        SendBarberShopResult(BARBER_SHOP_RESULT_NOT_ON_CHAIR);
        return;
    }

    uint32 cost = _player->GetBarberShopCost(bs_hair->hair_id, Color, bs_facialHair->hair_id, bs_skinColor);

    // 0 - ok
    // 1, 3 - not enough money
    // 2 - you have to sit on barber chair
    if (!_player->HasEnoughMoney((uint64)cost))
    {
        SendBarberShopResult(BARBER_SHOP_RESULT_NO_MONEY);
        return;
    }

    SendBarberShopResult(BARBER_SHOP_RESULT_SUCCESS);

    _player->ModifyMoney(-int64(cost));                     // it isn't free
    _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER, cost);

    _player->SetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_HAIR_STYLE_ID, uint8(bs_hair->hair_id));
    _player->SetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_HAIR_COLOR_ID, uint8(Color));
    _player->SetByteValue(PLAYER_BYTES_2, PLAYER_BYTES_2_OFFSET_FACIAL_STYLE, uint8(bs_facialHair->hair_id));
    if (bs_skinColor)
        _player->SetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_SKIN_ID, uint8(bs_skinColor->hair_id));

    _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP, 1);

    _player->SetStandState(0);                              // stand up
}

void WorldSession::HandleRemoveGlyph(WorldPacket& recvData)
{
    uint32 slot;
    recvData >> slot;

    if (slot >= MAX_GLYPH_SLOT_INDEX)
    {
        TC_LOG_DEBUG("network", "Client sent wrong glyph slot number in opcode CMSG_REMOVE_GLYPH %u", slot);
        return;
    }

    if (uint32 glyph = _player->GetGlyph(_player->GetActiveSpec(), slot))
    {
        if (GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyph))
        {
            _player->RemoveAurasDueToSpell(gp->SpellId);
            _player->SetGlyph(slot, 0);
            _player->SendTalentsInfoData(false);
        }
    }
}

void WorldSession::HandleCharCustomize(WorldPacket& recvData)
{
    std::shared_ptr<CharacterCustomizeInfo> customizeInfo = std::make_shared<CharacterCustomizeInfo>();

    recvData >> customizeInfo->Guid;
    if (!IsLegitCharacterForAccount(customizeInfo->Guid))
    {
        TC_LOG_INFO("entities.player.cheat", "Account %u, IP: %s tried to customise %s, but it does not belong to their account!",
            GetAccountId(), GetRemoteAddress().c_str(), customizeInfo->Guid.ToString().c_str());
        recvData.rfinish();
        KickPlayer();
        return;
    }

    recvData >> customizeInfo->Name
             >> customizeInfo->Gender
             >> customizeInfo->Skin
             >> customizeInfo->HairColor
             >> customizeInfo->HairStyle
             >> customizeInfo->FacialHair
             >> customizeInfo->Face;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CUSTOMIZE_INFO);
    stmt->setUInt32(0, customizeInfo->Guid.GetCounter());

    _queryProcessor.AddQuery(CharacterDatabase.AsyncQuery(stmt)
        .WithPreparedCallback(std::bind(&WorldSession::HandleCharCustomizeCallback, this, customizeInfo, std::placeholders::_1)));
}

void WorldSession::HandleCharCustomizeCallback(std::shared_ptr<CharacterCustomizeInfo> customizeInfo, PreparedQueryResult result)
{
    if (!result)
    {
        SendCharCustomize(CHAR_CREATE_ERROR, customizeInfo.get());
        return;
    }

    Field* fields = result->Fetch();
    std::string oldName = fields[0].GetString();
    uint8 plrRace = fields[1].GetUInt8();
    uint8 plrClass = fields[2].GetUInt8();
    uint8 plrGender = fields[3].GetUInt8();
    uint16 atLoginFlags = fields[4].GetUInt16();

    if (!Player::ValidateAppearance(plrRace, plrClass, plrGender, customizeInfo->HairStyle, customizeInfo->HairColor, customizeInfo->Face, customizeInfo->FacialHair, customizeInfo->Skin, true))
    {
        SendCharCustomize(CHAR_CREATE_ERROR, customizeInfo.get());
        return;
    }

    if (!(atLoginFlags & AT_LOGIN_CUSTOMIZE))
    {
        SendCharCustomize(CHAR_CREATE_ERROR, customizeInfo.get());
        return;
    }

    atLoginFlags &= ~AT_LOGIN_CUSTOMIZE;

    // prevent character rename
    if (sWorld->getBoolConfig(CONFIG_PREVENT_RENAME_CUSTOMIZATION) && (customizeInfo->Name != oldName))
    {
        SendCharCustomize(CHAR_NAME_FAILURE, customizeInfo.get());
        return;
    }

    // prevent character rename to invalid name
    if (!normalizePlayerName(customizeInfo->Name))
    {
        SendCharCustomize(CHAR_NAME_NO_NAME, customizeInfo.get());
        return;
    }

    ResponseCodes res = ObjectMgr::CheckPlayerName(customizeInfo->Name, GetSessionDbcLocale(), true);
    if (res != CHAR_NAME_SUCCESS)
    {
        SendCharCustomize(res, customizeInfo.get());
        return;
    }

    // check name limitations
    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME) && sObjectMgr->IsReservedName(customizeInfo->Name))
    {
        SendCharCustomize(CHAR_NAME_RESERVED, customizeInfo.get());
        return;
    }

    // character with this name already exist
    if (ObjectGuid newGuid = sCharacterCache->GetCharacterGuidByName(customizeInfo->Name))
    {
        if (newGuid != customizeInfo->Guid)
        {
            SendCharCustomize(CHAR_CREATE_NAME_IN_USE, customizeInfo.get());
            return;
        }
    }

    PreparedStatement* stmt = nullptr;
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    ObjectGuid::LowType lowGuid = customizeInfo->Guid.GetCounter();

    /// Customize
    Player::Customize(customizeInfo.get(), trans);

    /// Name Change and update atLogin flags
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_NAME_AT_LOGIN);
        stmt->setString(0, customizeInfo->Name);
        stmt->setUInt16(1, atLoginFlags);
        stmt->setUInt32(2, lowGuid);

        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_DECLINED_NAME);
        stmt->setUInt32(0, lowGuid);

        trans->Append(stmt);
    }

    CharacterDatabase.CommitTransaction(trans);

    sCharacterCache->UpdateCharacterData(customizeInfo->Guid, customizeInfo->Name, &customizeInfo->Gender);

    SendCharCustomize(RESPONSE_SUCCESS, customizeInfo.get());

    TC_LOG_INFO("entities.player.character", "Account: %d (IP: %s), Character[%s] (%s) Customized to: %s",
        GetAccountId(), GetRemoteAddress().c_str(), oldName.c_str(), customizeInfo->Guid.ToString().c_str(), customizeInfo->Name.c_str());
}

void WorldSession::HandleEquipmentSetSave(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_EQUIPMENT_SET_SAVE");

    uint64 setGuid;
    recvData.readPackGUID(setGuid);

    uint32 index;
    recvData >> index;
    if (index >= MAX_EQUIPMENT_SET_INDEX)                    // client set slots amount
        return;

    std::string name;
    recvData >> name;

    std::string iconName;
    recvData >> iconName;

    EquipmentSet eqSet;

    eqSet.Guid      = setGuid;
    eqSet.Name      = name;
    eqSet.IconName  = iconName;
    eqSet.state     = EQUIPMENT_SET_NEW;

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        ObjectGuid itemGuid;
        recvData >> itemGuid.ReadAsPacked();

        // if client sends 0, it means empty slot
        if (itemGuid.IsEmpty())
        {
            eqSet.Items[i] = 0;
            continue;
        }

        // equipment manager sends "1" (as raw GUID) for slots set to "ignore" (don't touch slot at equip set)
        if (itemGuid.GetRawValue() == 1)
        {
            // ignored slots saved as bit mask because we have no free special values for Items[i]
            eqSet.IgnoreMask |= 1 << i;
            continue;
        }

        // some cheating checks
        Item* item = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!item || item->GetGUID() != itemGuid)
        {
            eqSet.Items[i] = 0;
            continue;
        }

        eqSet.Items[i] = itemGuid.GetCounter();
    }

    _player->SetEquipmentSet(index, eqSet);
}

void WorldSession::HandleEquipmentSetDelete(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_EQUIPMENT_SET_DELETE");

    uint64 setGuid;
    recvData.readPackGUID(setGuid);

    _player->DeleteEquipmentSet(setGuid);
}

void WorldSession::HandleEquipmentSetUse(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_EQUIPMENT_SET_USE");

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        ObjectGuid itemGuid;
        recvData >> itemGuid.ReadAsPacked();

        uint8 srcbag, srcslot;
        recvData >> srcbag >> srcslot;

        TC_LOG_DEBUG("entities.player.items", "%s: srcbag %u, srcslot %u", itemGuid.ToString().c_str(), srcbag, srcslot);

        // check if item slot is set to "ignored" (raw value == 1), must not be unequipped then
        if (itemGuid.GetRawValue() == 1)
            continue;

        // Only equip weapons in combat
        if (_player->IsInCombat() && i != EQUIPMENT_SLOT_MAINHAND && i != EQUIPMENT_SLOT_OFFHAND && i != EQUIPMENT_SLOT_RANGED)
            continue;

        Item* item = _player->GetItemByGuid(itemGuid);

        uint16 dstpos = i | (INVENTORY_SLOT_BAG_0 << 8);

        if (!item)
        {
            Item* uItem = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (!uItem)
                continue;

            ItemPosCountVec sDest;
            InventoryResult msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, sDest, uItem, false);
            if (msg == EQUIP_ERR_OK)
            {
                if (_player->CanEquipItem(NULL_SLOT, dstpos, uItem, false) != EQUIP_ERR_OK)
                    continue;

                _player->RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
                _player->StoreItem(sDest, uItem, true);
            }
            else
                _player->SendEquipError(msg, uItem, NULL);

            continue;
        }

        if (item->GetPos() == dstpos)
            continue;

        if (_player->CanUnequipItem(dstpos, true) != EQUIP_ERR_OK)
            continue;

        _player->SwapItem(item->GetPos(), dstpos);
    }

    WorldPacket data(SMSG_EQUIPMENT_SET_USE_RESULT, 1);
    data << uint8(0);                                       // 4 - equipment swap failed - inventory is full
    SendPacket(&data);
}

void WorldSession::HandleCharFactionOrRaceChange(WorldPacket& recvData)
{
    std::shared_ptr<CharacterFactionChangeInfo> factionChangeInfo = std::make_shared<CharacterFactionChangeInfo>();
    recvData >> factionChangeInfo->Guid;

    if (!IsLegitCharacterForAccount(factionChangeInfo->Guid))
    {
        TC_LOG_INFO("entities.player.cheat", "Account %u, IP: %s tried to factionchange character %s, but it does not belong to their account!",
            GetAccountId(), GetRemoteAddress().c_str(), factionChangeInfo->Guid.ToString().c_str());
        recvData.rfinish();
        KickPlayer();
        return;
    }

    recvData >> factionChangeInfo->Name
             >> factionChangeInfo->Gender
             >> factionChangeInfo->Skin
             >> factionChangeInfo->HairColor
             >> factionChangeInfo->HairStyle
             >> factionChangeInfo->FacialHair
             >> factionChangeInfo->Face
             >> factionChangeInfo->Race;

    factionChangeInfo->FactionChange = (recvData.GetOpcode() == CMSG_CHAR_FACTION_CHANGE);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_RACE_OR_FACTION_CHANGE_INFOS);
    stmt->setUInt32(0, factionChangeInfo->Guid.GetCounter());

    _queryProcessor.AddQuery(CharacterDatabase.AsyncQuery(stmt)
        .WithPreparedCallback(std::bind(&WorldSession::HandleCharFactionOrRaceChangeCallback, this, factionChangeInfo, std::placeholders::_1)));
}

void WorldSession::HandleCharFactionOrRaceChangeCallback(std::shared_ptr<CharacterFactionChangeInfo> factionChangeInfo, PreparedQueryResult result)
{
    if (!result)
    {
        SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
        return;
    }

    // get the players old (at this moment current) race
    CharacterCacheEntry const* characterInfo = sCharacterCache->GetCharacterCacheByGuid(factionChangeInfo->Guid);
    if (!characterInfo)
    {
        SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
        return;
    }

    uint8 oldRace     = characterInfo->Race;
    uint8 playerClass = characterInfo->Class;
    uint8 level       = characterInfo->Level;
    std::string oldName = characterInfo->Name;

    if (!sObjectMgr->GetPlayerInfo(factionChangeInfo->Race, playerClass))
    {
        SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
        return;
    }

    Field* fields              = result->Fetch();
    uint32 atLoginFlags        = fields[0].GetUInt16();
    std::string knownTitlesStr = fields[1].GetString();

    uint32 usedLoginFlag = (factionChangeInfo->FactionChange ? AT_LOGIN_CHANGE_FACTION : AT_LOGIN_CHANGE_RACE);
    if (!(atLoginFlags & usedLoginFlag))
    {
        SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
        return;
    }

    uint32 newTeam = Player::TeamForRace(factionChangeInfo->Race);
    if (factionChangeInfo->FactionChange == (Player::TeamForRace(oldRace) == newTeam))
    {
        SendCharFactionChange(factionChangeInfo->FactionChange ? CHAR_CREATE_CHARACTER_SWAP_FACTION : CHAR_CREATE_CHARACTER_RACE_ONLY, factionChangeInfo.get());
        return;
    }

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RACEMASK))
    {
        uint32 raceMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK);
        if ((1 << (factionChangeInfo->Race - 1)) & raceMaskDisabled)
        {
            SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
            return;
        }
    }

    // prevent character rename
    if (sWorld->getBoolConfig(CONFIG_PREVENT_RENAME_CUSTOMIZATION) && (factionChangeInfo->Name != characterInfo->Name))
    {
        SendCharFactionChange(CHAR_NAME_FAILURE, factionChangeInfo.get());
        return;
    }

    // prevent character rename to invalid name
    if (!normalizePlayerName(factionChangeInfo->Name))
    {
        SendCharFactionChange(CHAR_NAME_NO_NAME, factionChangeInfo.get());
        return;
    }

    ResponseCodes res = ObjectMgr::CheckPlayerName(factionChangeInfo->Name, GetSessionDbcLocale(), true);
    if (res != CHAR_NAME_SUCCESS)
    {
        SendCharFactionChange(res, factionChangeInfo.get());
        return;
    }

    // check name limitations
    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME) && sObjectMgr->IsReservedName(factionChangeInfo->Name))
    {
        SendCharFactionChange(CHAR_NAME_RESERVED, factionChangeInfo.get());
        return;
    }

    // character with this name already exist
    ObjectGuid newGuid = sCharacterCache->GetCharacterGuidByName(factionChangeInfo->Name);
    if (!newGuid.IsEmpty())
    {
        if (newGuid != factionChangeInfo->Guid)
        {
            SendCharFactionChange(CHAR_CREATE_NAME_IN_USE, factionChangeInfo.get());
            return;
        }
    }

    if (sArenaTeamMgr->GetArenaTeamByCaptain(factionChangeInfo->Guid))
    {
        SendCharFactionChange(CHAR_CREATE_CHARACTER_ARENA_LEADER, factionChangeInfo.get());
        return;
    }

    // All checks are fine, deal with race change now
    ObjectGuid::LowType lowGuid = factionChangeInfo->Guid.GetCounter();

    PreparedStatement* stmt = nullptr;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    // resurrect the character in case he's dead
    Player::OfflineResurrect(factionChangeInfo->Guid, trans);

    // Name Change and update atLogin flags
    {
        CharacterDatabase.EscapeString(factionChangeInfo->Name);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_NAME_AT_LOGIN);
        stmt->setString(0, factionChangeInfo->Name);
        stmt->setUInt16(1, uint16((atLoginFlags | AT_LOGIN_RESURRECT) & ~usedLoginFlag));
        stmt->setUInt32(2, lowGuid);
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_DECLINED_NAME);
        stmt->setUInt32(0, lowGuid);
        trans->Append(stmt);
    }

    // Customize
    Player::Customize(factionChangeInfo.get(), trans);

    // Race Change
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_RACE);
        stmt->setUInt8(0, factionChangeInfo->Race);
        stmt->setUInt32(1, lowGuid);
        trans->Append(stmt);
    }

    sCharacterCache->UpdateCharacterData(factionChangeInfo->Guid, factionChangeInfo->Name, &factionChangeInfo->Gender, &factionChangeInfo->Race);

    if (oldRace != factionChangeInfo->Race)
    {
        // Switch Languages
        // delete all languages first
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SKILL_LANGUAGES);
        stmt->setUInt32(0, lowGuid);
        trans->Append(stmt);

        // Now add them back
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_SKILL_LANGUAGE);
        stmt->setUInt32(0, lowGuid);

        // Faction specific languages
        if (newTeam == HORDE)
            stmt->setUInt16(1, 109);
        else
            stmt->setUInt16(1, 98);

        trans->Append(stmt);

        // Race specific languages
        if (factionChangeInfo->Race != RACE_ORC && factionChangeInfo->Race != RACE_HUMAN)
        {
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_SKILL_LANGUAGE);
            stmt->setUInt32(0, lowGuid);

            switch (factionChangeInfo->Race)
            {
                case RACE_DWARF:
                    stmt->setUInt16(1, 111);
                    break;
                case RACE_DRAENEI:
                    stmt->setUInt16(1, 759);
                    break;
                case RACE_GNOME:
                    stmt->setUInt16(1, 313);
                    break;
                case RACE_NIGHTELF:
                    stmt->setUInt16(1, 113);
                    break;
                case RACE_WORGEN:
                    stmt->setUInt16(1, 791);
                    break;
                case RACE_UNDEAD_PLAYER:
                    stmt->setUInt16(1, 673);
                    break;
                case RACE_TAUREN:
                    stmt->setUInt16(1, 115);
                    break;
                case RACE_TROLL:
                    stmt->setUInt16(1, 315);
                    break;
                case RACE_BLOODELF:
                    stmt->setUInt16(1, 137);
                    break;
                case RACE_GOBLIN:
                    stmt->setUInt16(1, 792);
                    break;
            }

            trans->Append(stmt);
        }

        if (factionChangeInfo->FactionChange)
        {
            // Delete all Flypaths
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_TAXI_PATH);
            stmt->setUInt32(0, lowGuid);
            trans->Append(stmt);

            if (level > 7)
            {
                // Update Taxi path
                // this doesn't seem to be 100% blizzlike... but it can't really be helped.
                std::ostringstream taximaskstream;
                uint32 numFullTaximasks = level / 7;
                if (numFullTaximasks > 11)
                    numFullTaximasks = 11;

                TaxiMask const& factionMask = newTeam == HORDE ? sHordeTaxiNodesMask : sAllianceTaxiNodesMask;
                for (uint8 i = 0; i < numFullTaximasks; ++i)
                {
                    uint8 deathKnightExtraNode = (playerClass == CLASS_DEATH_KNIGHT) ? sDeathKnightTaxiNodesMask[i] : 0;
                    taximaskstream << uint32(factionMask[i] | deathKnightExtraNode) << ' ';
                }

                uint32 numEmptyTaximasks = 11 - numFullTaximasks;
                for (uint8 i = 0; i < numEmptyTaximasks; ++i)
                    taximaskstream << "0 ";
                taximaskstream << '0';

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_TAXIMASK);
                stmt->setString(0, taximaskstream.str());
                stmt->setUInt32(1, lowGuid);
                trans->Append(stmt);
            }

            if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD))
            {
                // Reset guild
                if (Guild* guild = sGuildMgr->GetGuildById(characterInfo->GuildId))
                    guild->DeleteMember(trans, factionChangeInfo->Guid, false, false, true);

                Player::LeaveAllArenaTeams(factionChangeInfo->Guid);
            }

            if (!HasPermission(rbac::RBAC_PERM_TWO_SIDE_ADD_FRIEND))
            {
                // Delete Friend List
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SOCIAL_BY_GUID);
                stmt->setUInt32(0, lowGuid);
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SOCIAL_BY_FRIEND);
                stmt->setUInt32(0, lowGuid);
                trans->Append(stmt);
            }

            // Reset homebind and position
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PLAYER_HOMEBIND);
            stmt->setUInt32(0, lowGuid);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PLAYER_HOMEBIND);
            stmt->setUInt32(0, lowGuid);

            WorldLocation loc;
            uint16 zoneId = 0;
            if (newTeam == ALLIANCE)
            {
                loc.WorldRelocate(0, -8867.68f, 673.373f, 97.9034f, 0.0f);
                zoneId = 1519;
            }
            else
            {
                loc.WorldRelocate(1, 1633.33f, -4439.11f, 15.7588f, 0.0f);
                zoneId = 1637;
            }

            stmt->setUInt16(1, loc.GetMapId());
            stmt->setUInt16(2, zoneId);
            stmt->setFloat(3, loc.GetPositionX());
            stmt->setFloat(4, loc.GetPositionY());
            stmt->setFloat(5, loc.GetPositionZ());
            trans->Append(stmt);

            Player::SavePositionInDB(loc, zoneId, factionChangeInfo->Guid, trans);

            // Achievement conversion
            for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->FactionChangeAchievements.begin(); it != sObjectMgr->FactionChangeAchievements.end(); ++it)
            {
                uint32 achiev_alliance = it->first;
                uint32 achiev_horde = it->second;

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_BY_ACHIEVEMENT);
                stmt->setUInt16(0, uint16(newTeam == ALLIANCE ? achiev_alliance : achiev_horde));
                stmt->setUInt32(1, lowGuid);
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_ACHIEVEMENT);
                stmt->setUInt16(0, uint16(newTeam == ALLIANCE ? achiev_alliance : achiev_horde));
                stmt->setUInt16(1, uint16(newTeam == ALLIANCE ? achiev_horde : achiev_alliance));
                stmt->setUInt32(2, lowGuid);
                trans->Append(stmt);
            }

            // Item conversion
            for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->FactionChangeItems.begin(); it != sObjectMgr->FactionChangeItems.end(); ++it)
            {
                uint32 item_alliance = it->first;
                uint32 item_horde = it->second;

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_INVENTORY_FACTION_CHANGE);
                stmt->setUInt32(0, (newTeam == ALLIANCE ? item_alliance : item_horde));
                stmt->setUInt32(1, (newTeam == ALLIANCE ? item_horde : item_alliance));
                stmt->setUInt32(2, lowGuid);
                trans->Append(stmt);
            }

            // Delete all current quests
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS);
            stmt->setUInt32(0, lowGuid);
            trans->Append(stmt);

            // Quest conversion
            for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->FactionChangeQuests.begin(); it != sObjectMgr->FactionChangeQuests.end(); ++it)
            {
                uint32 quest_alliance = it->first;
                uint32 quest_horde = it->second;

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS_REWARDED_BY_QUEST);
                stmt->setUInt32(0, lowGuid);
                stmt->setUInt32(1, (newTeam == ALLIANCE ? quest_alliance : quest_horde));
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_QUESTSTATUS_REWARDED_FACTION_CHANGE);
                stmt->setUInt32(0, (newTeam == ALLIANCE ? quest_alliance : quest_horde));
                stmt->setUInt32(1, (newTeam == ALLIANCE ? quest_horde : quest_alliance));
                stmt->setUInt32(2, lowGuid);
                trans->Append(stmt);
            }

            // Mark all rewarded quests as "active" (will count for completed quests achievements)
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_QUESTSTATUS_REWARDED_ACTIVE);
            stmt->setUInt32(0, lowGuid);
            trans->Append(stmt);

            // Disable all old-faction specific quests
            {
                ObjectMgr::QuestMap const& questTemplates = sObjectMgr->GetQuestTemplates();
                for (ObjectMgr::QuestMap::const_iterator iter = questTemplates.begin(); iter != questTemplates.end(); ++iter)
                {
                    Quest const* quest = iter->second;
                    uint32 newRaceMask = (newTeam == ALLIANCE) ? RACEMASK_ALLIANCE : RACEMASK_HORDE;
                    if (!(quest->GetAllowableRaces() & newRaceMask))
                    {
                        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_QUESTSTATUS_REWARDED_ACTIVE_BY_QUEST);
                        stmt->setUInt32(0, lowGuid);
                        stmt->setUInt32(1, quest->GetQuestId());
                        trans->Append(stmt);
                    }
                }
            }

            // Spell conversion
            for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->FactionChangeSpells.begin(); it != sObjectMgr->FactionChangeSpells.end(); ++it)
            {
                uint32 spell_alliance = it->first;
                uint32 spell_horde = it->second;

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SPELL_BY_SPELL);
                stmt->setUInt32(0, (newTeam == ALLIANCE ? spell_alliance : spell_horde));
                stmt->setUInt32(1, lowGuid);
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_SPELL_FACTION_CHANGE);
                stmt->setUInt32(0, (newTeam == ALLIANCE ? spell_alliance : spell_horde));
                stmt->setUInt32(1, (newTeam == ALLIANCE ? spell_horde : spell_alliance));
                stmt->setUInt32(2, lowGuid);
                trans->Append(stmt);
            }

            // Reputation conversion
            for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->FactionChangeReputation.begin(); it != sObjectMgr->FactionChangeReputation.end(); ++it)
            {
                uint32 reputation_alliance = it->first;
                uint32 reputation_horde = it->second;
                uint32 newReputation = (newTeam == ALLIANCE) ? reputation_alliance : reputation_horde;
                uint32 oldReputation = (newTeam == ALLIANCE) ? reputation_horde : reputation_alliance;

                // select old standing set in db
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_REP_BY_FACTION);
                stmt->setUInt32(0, oldReputation);
                stmt->setUInt32(1, lowGuid);

                if (PreparedQueryResult reputationResult = CharacterDatabase.Query(stmt))
                {
                    fields = reputationResult->Fetch();
                    int32 oldDBRep = fields[0].GetInt32();
                    FactionEntry const* factionEntry = sFactionStore.LookupEntry(oldReputation);

                    // old base reputation
                    int32 oldBaseRep = sObjectMgr->GetBaseReputationOf(factionEntry, oldRace, playerClass);

                    // new base reputation
                    int32 newBaseRep = sObjectMgr->GetBaseReputationOf(sFactionStore.LookupEntry(newReputation), factionChangeInfo->Race, playerClass);

                    // final reputation shouldnt change
                    int32 FinalRep = oldDBRep + oldBaseRep;
                    int32 newDBRep = FinalRep - newBaseRep;

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_REP_BY_FACTION);
                    stmt->setUInt32(0, newReputation);
                    stmt->setUInt32(1, lowGuid);
                    trans->Append(stmt);

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_REP_FACTION_CHANGE);
                    stmt->setUInt16(0, uint16(newReputation));
                    stmt->setInt32(1, newDBRep);
                    stmt->setUInt16(2, uint16(oldReputation));
                    stmt->setUInt32(3, lowGuid);
                    trans->Append(stmt);
                }
            }

            // Title conversion
            if (!knownTitlesStr.empty())
            {
                uint32 const ktcount = KNOWN_TITLES_SIZE * 2;
                uint32 knownTitles[ktcount];
                Tokenizer tokens(knownTitlesStr, ' ', ktcount);

                if (tokens.size() != ktcount)
                {
                    SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
                    return;
                }

                for (uint32 index = 0; index < ktcount; ++index)
                    knownTitles[index] = atoul(tokens[index]);

                for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->FactionChangeTitles.begin(); it != sObjectMgr->FactionChangeTitles.end(); ++it)
                {
                    uint32 title_alliance = it->first;
                    uint32 title_horde = it->second;

                    CharTitlesEntry const* atitleInfo = sCharTitlesStore.AssertEntry(title_alliance);
                    CharTitlesEntry const* htitleInfo = sCharTitlesStore.AssertEntry(title_horde);
                    // new team
                    if (newTeam == ALLIANCE)
                    {
                        uint32 bitIndex = htitleInfo->bit_index;
                        uint32 index = bitIndex / 32;
                        uint32 old_flag = 1 << (bitIndex % 32);
                        uint32 new_flag = 1 << (atitleInfo->bit_index % 32);
                        if (knownTitles[index] & old_flag)
                        {
                            knownTitles[index] &= ~old_flag;
                            // use index of the new title
                            knownTitles[atitleInfo->bit_index / 32] |= new_flag;
                        }
                    }
                    else
                    {
                        uint32 bitIndex = atitleInfo->bit_index;
                        uint32 index = bitIndex / 32;
                        uint32 old_flag = 1 << (bitIndex % 32);
                        uint32 new_flag = 1 << (htitleInfo->bit_index % 32);
                        if (knownTitles[index] & old_flag)
                        {
                            knownTitles[index] &= ~old_flag;
                            // use index of the new title
                            knownTitles[htitleInfo->bit_index / 32] |= new_flag;
                        }
                    }

                    std::ostringstream ss;
                    for (uint32 index = 0; index < ktcount; ++index)
                        ss << knownTitles[index] << ' ';

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_TITLES_FACTION_CHANGE);
                    stmt->setString(0, ss.str().c_str());
                    stmt->setUInt32(1, lowGuid);
                    trans->Append(stmt);

                    // unset any currently chosen title
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_RES_CHAR_TITLES_FACTION_CHANGE);
                    stmt->setUInt32(0, lowGuid);
                    trans->Append(stmt);
                }
            }
        }
    }

    CharacterDatabase.CommitTransaction(trans);

    TC_LOG_DEBUG("entities.player", "%s (IP: %s) changed race from %u to %u", GetPlayerInfo().c_str(), GetRemoteAddress().c_str(), oldRace, factionChangeInfo->Race);

    SendCharFactionChange(RESPONSE_SUCCESS, factionChangeInfo.get());
}

void WorldSession::SendCharCreate(ResponseCodes result)
{
    WorldPacket data(SMSG_CHAR_CREATE, 1);
    data << uint8(result);
    SendPacket(&data);
}

void WorldSession::SendCharDelete(ResponseCodes result)
{
    WorldPacket data(SMSG_CHAR_DELETE, 1);
    data << uint8(result);
    SendPacket(&data);
}

void WorldSession::SendCharRename(ResponseCodes result, CharacterRenameInfo const* renameInfo)
{
    WorldPacket data(SMSG_CHAR_RENAME, 1 + 8 + renameInfo->Name.size() + 1);
    data << uint8(result);
    if (result == RESPONSE_SUCCESS)
    {
        data << renameInfo->Guid;
        data << renameInfo->Name;
    }
    SendPacket(&data);
}

void WorldSession::SendCharCustomize(ResponseCodes result, CharacterCustomizeInfo const* customizeInfo)
{
    WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1 + 8 + customizeInfo->Name.size() + 1 + 6);
    data << uint8(result);
    if (result == RESPONSE_SUCCESS)
    {
        data << customizeInfo->Guid;
        data << customizeInfo->Name;
        data << uint8(customizeInfo->Gender);
        data << uint8(customizeInfo->Skin);
        data << uint8(customizeInfo->Face);
        data << uint8(customizeInfo->HairStyle);
        data << uint8(customizeInfo->HairColor);
        data << uint8(customizeInfo->FacialHair);
    }
    SendPacket(&data);
}

void WorldSession::SendCharFactionChange(ResponseCodes result, CharacterFactionChangeInfo const* factionChangeInfo)
{
    WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1 + 8 + factionChangeInfo->Name.size() + 1 + 7);
    data << uint8(result);
    if (result == RESPONSE_SUCCESS)
    {
        data << factionChangeInfo->Guid;
        data << factionChangeInfo->Name;
        data << uint8(factionChangeInfo->Gender);
        data << uint8(factionChangeInfo->Skin);
        data << uint8(factionChangeInfo->Face);
        data << uint8(factionChangeInfo->HairStyle);
        data << uint8(factionChangeInfo->HairColor);
        data << uint8(factionChangeInfo->FacialHair);
        data << uint8(factionChangeInfo->Race);
    }
    SendPacket(&data);
}

void WorldSession::SendSetPlayerDeclinedNamesResult(DeclinedNameResult result, ObjectGuid guid)
{
    WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4 + 8);
    data << uint32(result);
    data << guid;
    SendPacket(&data);
}

void WorldSession::SendBarberShopResult(BarberShopResult result)
{
    WorldPacket data(SMSG_BARBER_SHOP_RESULT, 4);
    data << uint32(result);
    SendPacket(&data);
}

void WorldSession::HandleRandomizeCharNameOpcode(WorldPacket& recvData)
{
    uint8 gender, race;

    recvData >> race;
    recvData >> gender;

    if (!Player::IsValidRace(race))
    {
        TC_LOG_ERROR("misc", "Invalid race (%u) sent by accountId: %u", race, GetAccountId());
        return;
    }

    if (!Player::IsValidGender(gender))
    {
        TC_LOG_ERROR("misc", "Invalid gender (%u) sent by accountId: %u", gender, GetAccountId());
        return;
    }

    std::string const* name = GetRandomCharacterName(race, gender);
    WorldPacket data(SMSG_RANDOMIZE_CHAR_NAME, 10);
    data.WriteBit(0); // unk
    data.WriteBits(name->size(), 7);
    data.WriteString(*name);
    SendPacket(&data);
}

void WorldSession::HandleReorderCharacters(WorldPacket& recvData)
{
    uint32 charactersCount = std::min<uint32>(recvData.ReadBits(10), sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM));

    std::vector<ObjectGuid> guids(charactersCount);
    uint8 position;

    for (uint8 i = 0; i < charactersCount; ++i)
    {
        guids[i][1] = recvData.ReadBit();
        guids[i][4] = recvData.ReadBit();
        guids[i][5] = recvData.ReadBit();
        guids[i][3] = recvData.ReadBit();
        guids[i][0] = recvData.ReadBit();
        guids[i][7] = recvData.ReadBit();
        guids[i][6] = recvData.ReadBit();
        guids[i][2] = recvData.ReadBit();
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    for (uint8 i = 0; i < charactersCount; ++i)
    {
        recvData.ReadByteSeq(guids[i][6]);
        recvData.ReadByteSeq(guids[i][5]);
        recvData.ReadByteSeq(guids[i][1]);
        recvData.ReadByteSeq(guids[i][4]);
        recvData.ReadByteSeq(guids[i][0]);
        recvData.ReadByteSeq(guids[i][3]);

        recvData >> position;

        recvData.ReadByteSeq(guids[i][2]);
        recvData.ReadByteSeq(guids[i][7]);

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_LIST_SLOT);
        stmt->setUInt8(0, position);
        stmt->setUInt32(1, guids[i].GetCounter());
        trans->Append(stmt);
    }

    CharacterDatabase.CommitTransaction(trans);
}

void WorldSession::HandleOpeningCinematic(WorldPacket& /*recvData*/)
{
    // Only players that has not yet gained any experience can use this
    if (_player->GetUInt32Value(PLAYER_XP))
        return;

    if (ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(_player->getClass()))
    {
        if (classEntry->CinematicSequence)
            _player->SendCinematicStart(classEntry->CinematicSequence);
        else if (ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(_player->getRace()))
            _player->SendCinematicStart(raceEntry->CinematicSequence);
    }
}