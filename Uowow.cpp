#include "Common.h"
#include "Uowow.h" 
#include "WorldPacket.h"
#include "Player.h"
#include "Policies/SingletonImp.h"
#include "Chat.h"
#include "Guild.h"
#include "World.h"
#include "Vehicle.h"
//version 330a


INSTANTIATE_SINGLETON_1(uowToolSet);

uowToolSet::~uowToolSet()
{
}

void uowToolSet::MakeMark(Player* player)
{
	AreaTableEntry const* current_zone = GetAreaEntryByAreaID(player->GetAreaId());
	if (!current_zone || current_zone->ID==616 || player->InBattleGround() || player->InArena() || uowTools.IsSiegeCity(player->GetAreaId()) || player->FindNearestGameObject(77814,1024) || player->FindNearestGameObject(NOMARK_MARKER,1024))
	{
		ChatHandler(player).PSendSysMessage("You may not mark in this area.");
		player->SendPlaySound(42,true);
		return;
	}
	std::string areaname(current_zone->area_name[0]); 
	CharacterDatabase.escape_string(areaname);
	CharacterDatabase.DirectPExecute("INSERT INTO uow_recallrunes (characterguid,name,mapid,positionx,positiony,positionz,orientation) VALUES (%u,'%s:%s',%u,%f,%f,%f,%f)",player->GetGUIDLow(),player->GetBaseMap()->GetMapName(), areaname.c_str(),player->GetBaseMap()->GetId(),player->GetPositionX(),player->GetPositionY(),player->GetPositionZ(),player->GetOrientation());
	LoadPlayerMarks(player);
	ChatHandler(player).PSendSysMessage("A mark has been created for this area.");
	player->SendPlaySound(1435,false);
}

void uowToolSet::LoadPlayerMarks(Player * player)
{
	player->m_marks.clear();
	Mark * blank = new Mark();
	blank->id=0; blank->mapid=player->GetMapId(); blank->name="blank"; blank->x=player->GetPositionX(); blank->y=player->GetPositionY(); blank->z=player->GetPositionZ(); blank->o=player->GetOrientation();
	player->m_currentmark = blank;
    QueryResult *result = CharacterDatabase.PQuery("SELECT * from uow_recallrunes where characterguid = %u",player->GetGUIDLow());
    if(!result)
        return;
    do
    {
        Field *fields = result->Fetch();
		Mark *  newmark = new Mark();
		newmark->id = fields[0].GetUInt32();
		newmark->name = fields[2].GetString();
		newmark->mapid = fields[3].GetUInt32();
		newmark->x = fields[4].GetFloat();
		newmark->y = fields[5].GetFloat();
		newmark->z = fields[6].GetFloat();
		newmark->o = fields[7].GetFloat();
		player->m_marks.insert(MarkCollection::value_type(newmark->id,newmark));
		player->m_currentmark = newmark;
    } while (result->NextRow());
    delete result;
}
void uowToolSet::SelectMark(uint32 id, Player * player)
{
    bool found = false;
	MarkCollection::const_iterator itr;
    for(itr = player->m_marks.begin(); itr != player->m_marks.end(); ++itr)
    {
      if(itr->second->id == id)
	  {
        found = true;
        break;
	  }
    }
    // no match, return
    if(!found)
        return;
	player->m_currentmark = itr->second;
}
void uowToolSet::LoadCityData()
{
	WorldGames.SiegeCities.clear();
    QueryResult *result = CharacterDatabase.PQuery("SELECT * from uow_SiegeCities");
    if(!result)
        return;
    do
    {
        Field *fields = result->Fetch();
		CityInfo newcity;
		newcity.areaid = fields[0].GetUInt32();
		newcity.guildid = fields[1].GetUInt32();
		newcity.rank = fields[2].GetUInt32();
		newcity.contestedby = 0;
		WorldGames.SiegeCities.insert(CityCollection::value_type(newcity.areaid,newcity));
    } while (result->NextRow());
    delete result;

	result = CharacterDatabase.PQuery("SELECT LastCitySieged from uow_variables");
    if(!result)
		return;
    Field *fields = result->Fetch();
	WorldGames.CurrentSiegeCity = fields[0].GetUInt32();
}
void uowToolSet::BroadcastSiegeStatus(uint32 areaid,Player * player)
{
	CityCollection::const_iterator itr;
    for(itr = WorldGames.SiegeCities.begin(); itr != WorldGames.SiegeCities.end(); ++itr)
    {
		if (areaid==0 || areaid == itr->second.areaid)
		{
			std::ostringstream msg;
			std::ostringstream occupied;
			std::ostringstream attackable;
			AreaTableEntry const* current_zone = GetAreaEntryByAreaID(itr->second.areaid);
			if(!current_zone)
			{
				return;
			}
			if(itr->second.guildid==0)
			{
				occupied << MSG_COLOR_LIGHTRED << "[No Guild]";
			}
			else
			{
				Guild* g = objmgr.GetGuildById(itr->second.guildid);
				if(g)
					occupied << MSG_COLOR_RED << "[" << g->GetName() << "]";
				else
					occupied << MSG_COLOR_LIGHTRED << "[No Guild]";

			}
			if(itr->second.areaid==WorldGames.CurrentSiegeCity)
			  attackable << MSG_COLOR_GREEN << "[can be attacked]";
			else
			  attackable << MSG_COLOR_RED << "[can not be attacked]";		  
			msg << MSG_COLOR_YELLOW << "[" << current_zone->area_name[0] << "]" << MSG_COLOR_LIGHTBLUE << " is currently occupied by " <<  occupied.str( ).c_str( )  << MSG_COLOR_LIGHTBLUE << " and " << attackable.str( ).c_str( );
			if(!player)
				sWorld.SendWorldText(LANG_AUTO_BROADCAST, msg.str().c_str());
			else
				ChatHandler(player).PSendSysMessage(msg.str().c_str());
		}
    }
}

bool uowToolSet::IsSiegeCity(uint32 areaid)
{
	CityCollection::const_iterator itr;
	for(itr = WorldGames.SiegeCities.begin(); itr != WorldGames.SiegeCities.end(); ++itr)
	{
	  if(itr->second.areaid == areaid)
		return true;
	}
	return false;
}
void uowToolSet::BroadcastChatHelp(Player * player)
{
	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "[@?]" << MSG_COLOR_YELLOW << " This help menu.\n"
		<< MSG_COLOR_GREEN << "[@h]" << MSG_COLOR_YELLOW << " Display current Hot Zone status.\n"		
		<< MSG_COLOR_GREEN << "[@p]" << MSG_COLOR_YELLOW << " Toggle revive spot.\n"	
		<< MSG_COLOR_GREEN << "[@r]" << MSG_COLOR_YELLOW << " Rename rune. Usage: @r newname\n"
		<< MSG_COLOR_GREEN << "[@g0]" << MSG_COLOR_YELLOW << " [Games] Display every games Queue Status.\n"
		<< MSG_COLOR_GREEN << "[@g1]" << MSG_COLOR_YELLOW << " [Games] Display City Siege Status.\n"
		<< MSG_COLOR_GREEN << "[@g2]" << MSG_COLOR_YELLOW << " [Games] Display Crater Capture Status.\n"
		<< MSG_COLOR_GREEN << "[@g3]" << MSG_COLOR_YELLOW << " [Games] Display Bloodsail Booty Capture Status.\n"
		<< MSG_COLOR_GREEN << "[@g4]" << MSG_COLOR_YELLOW << " [Games] Guild City Bane Stauts.\n"
		<< MSG_COLOR_GREEN << "[@s1]" << MSG_COLOR_YELLOW << " [Server Rankigs] Top UO-WOW Players.\n"
		<< MSG_COLOR_GREEN << "[@s2]" << MSG_COLOR_YELLOW << " [Server Rankigs] Top World Killers.\n"
		<< MSG_COLOR_GREEN << "[@s3]" << MSG_COLOR_YELLOW << " [Server Rankigs] Top World Deaths.\n"
		<< MSG_COLOR_GREEN << "[@s4]" << MSG_COLOR_YELLOW << " [Server Rankigs] Top Mini-Game Killers.\n"
		<< MSG_COLOR_GREEN << "[@s5]" << MSG_COLOR_YELLOW << " [Server Rankigs] Top Mini-Game Deaths.\n"
		<< MSG_COLOR_GREEN << "[@s6]" << MSG_COLOR_YELLOW << " [Server Rankigs] Top Mini-Game Captures.\n"
		<< MSG_COLOR_GREEN << "[@s7]" << MSG_COLOR_YELLOW << " [Server Rankigs] Most Mini-Games Played.\n"
		<< MSG_COLOR_GREEN << "[@s8]" << MSG_COLOR_YELLOW << " [Server Rankigs] Most Hot Zone Creature Kills.\n"
		<< MSG_COLOR_GREEN << "[@s9]" << MSG_COLOR_YELLOW << " [Server Rankigs] Most Shrines Completed.\n"
		<< MSG_COLOR_GREEN << "[@bot]" << MSG_COLOR_YELLOW << " [Housing] Select nearest OBJECT.\n"
		<< MSG_COLOR_GREEN << "[@bod]" << MSG_COLOR_YELLOW << " [Housing] Delete selected OBJECT.\n"
		<< MSG_COLOR_GREEN << "[@boo]" << MSG_COLOR_YELLOW << " [Housing] Re-Orient selected OBJECT.\n"
		<< MSG_COLOR_GREEN << "[@bnd]" << MSG_COLOR_YELLOW << " [Housing] Delete selected NPC.\n"
		<< MSG_COLOR_GREEN << "[@bno]" << MSG_COLOR_YELLOW << " [Housing] Re-Orient selected NPC.\n"
		<< MSG_COLOR_GREEN << "[@cs]" << MSG_COLOR_YELLOW << " [Crafting] Search craftable items.\n"
		<< MSG_COLOR_GREEN << "[@ci]" << MSG_COLOR_YELLOW << " [Crafting] Craft item.";
		

	ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
}
void uowToolSet::BroadcastHotZone(Player * player)
{
	AreaTableEntry const* current_zone = GetAreaEntryByAreaID(WorldGames.CurrentHotZone);
	std::stringstream extratext;
	extratext << MSG_COLOR_ORANGE << "[" << current_zone->area_name[0] << "]" << MSG_COLOR_YELLOW << " is a HOT ZONE for the next 55 minutes. Kill as many monsters and people as fast as you can! Get Arena points for monsters and PVP kills!";
	if(!player)
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,extratext.str().c_str());
	else
		ChatHandler(player).PSendSysMessage(extratext.str().c_str());
}
void uowToolSet::BroadcastServerInfo()
{
	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "[UOWOW v2.0 by Hawthorne] - For world chat type /join world. visit us @ [www.uowow.com]. For more help type /say @?";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));

	//std::ostringstream msg2;
	//msg2 << MSG_COLOR_ORANGE << "[World Game: Redneck Rampage] - Starts every 10 minutes, check out the World Games option using your Panda!";
	//sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg2.str( ).c_str( ));
}
void uowToolSet::BroadcastCraterStatus(Player * player)
{
	//If the game is not active display activation time left etc
	if(WorldGames.Game_CC.gamestage == 0)
	{
		std::ostringstream msg;
		msg << MSG_COLOR_RED << "Crater Capture is currently not active. Time until next game " << MSG_COLOR_WHITE << "[" << (int)(10 - (WorldGames.Game_CC.elapsedseconds / 60)) << " minutes]."  << MSG_COLOR_RED " If another world game is active this game will not start until that one is complete.";
		if(!player)
			sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
		else
			ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
		return;
	}
	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "Crater Capture is currently active. Time left in this game" << "[" << MSG_COLOR_WHITE << (int)(20 - (WorldGames.Game_CC.elapsedseconds / 60)) << " minutes]";
	if(!player)
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
	else
		ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
	BroadcastCraterScore(player);
}
void uowToolSet::GiveMaxRep(Player* player)
{
	//WOTLK
	player->SetReputation(1104,42999);
	player->SetReputation(1105,42999);
	player->SetReputation(1106,42999);
	player->SetReputation(1090,42999);
	player->SetReputation(1098,42999);
	player->SetReputation(1073,42999);
	player->SetReputation(1119,42999);
	player->SetReputation(1091,42999);

	//TBC
	player->SetReputation(1012,42999);
	player->SetReputation(942,42999);
	player->SetReputation(989,42999);
	player->SetReputation(1015,42999);
	player->SetReputation(1038,42999);
	player->SetReputation(970,42999);
	player->SetReputation(933,42999);
	player->SetReputation(990,42999);
	player->SetReputation(967,42999);
	player->SetReputation(1011,42999);
	player->SetReputation(1031,42999);
	player->SetReputation(1077,42999);
	player->SetReputation(932,42999);
	player->SetReputation(934,42999);
	player->SetReputation(935,42999);

	//ORIGINAL
	player->SetReputation(942,42999);
	player->SetReputation(935,42999);
    player->SetReputation(936,42999);
    player->SetReputation(1011,42999);
    player->SetReputation(970,42999);
    player->SetReputation(967,42999);
    player->SetReputation(989,42999);
    player->SetReputation(932,42999);
    player->SetReputation(934,42999);
    player->SetReputation(1038,42999);
    player->SetReputation(1077,42999);
    // Factions depending on team, like cities and some more stuff
    switch(player->GetTeam())
    {
    case ALLIANCE:
		player->SetReputation(1037,42999);
		player->SetReputation(1068,42999);
		player->SetReputation(1126,42999);
		player->SetReputation(1094,42999);
		player->SetReputation(1050,42999);

		player->SetReputation(946,42999);
		player->SetReputation(978,42999);

        player->SetReputation(72,42999);
        player->SetReputation(47,42999);
        player->SetReputation(69,42999);
        player->SetReputation(930,42999);
        player->SetReputation(730,42999);
        player->SetReputation(978,42999);
        player->SetReputation(54,42999);
        player->SetReputation(946,42999);
        break;
    case HORDE:
		player->SetReputation(1052,42999);
		player->SetReputation(1067,42999);
		player->SetReputation(1124,42999);
		player->SetReputation(1064,42999);
		player->SetReputation(1085,42999);

		player->SetReputation(941,42999);
		player->SetReputation(947,42999);
		player->SetReputation(911,42999);
		player->SetReputation(922,42999);
 

        player->SetReputation(76,42999);
        player->SetReputation(68,42999);
        player->SetReputation(81,42999);
        player->SetReputation(911,42999);
        player->SetReputation(729,42999);
        player->SetReputation(941,42999);
        player->SetReputation(530,42999);
        player->SetReputation(947,42999);
        break;
    default:
        break;
    }

}
void uowToolSet::GiveFreeStuff(Player* player)
{
//Free Starting Mount
	uint32 mount=0;
	switch (player->getRace())
	{
	case RACE_GNOME:
		mount=10873;
		break;
	case RACE_NIGHTELF:
		mount=10787;
		break;
	case RACE_ORC:
		mount=459;
		break;
	case RACE_TAUREN:
		mount=18989;
		break;
	case RACE_TROLL:
		mount=8395;
		break;
	case RACE_UNDEAD_PLAYER:
		mount=17462;
		break;
	case RACE_BLOODELF:
		mount=34795;
		break;
	case RACE_DWARF:
		mount=6777;
		break;
	case RACE_HUMAN:
		mount=458;
		break;
	case RACE_DRAENEI:
		mount=34406;
		break;
	default:
		mount=6777;
		break;
	}
	//uowow buddy (Panda Collar)
	if (!player->HasSpell(17707) && !uowTools.HasMats(player,13583,1))
	{
		uowTools.GiveItems(player,13583,1);
		ChatHandler(player).PSendSysMessage("You have been awarded a Uowow Buddy Panda Collar! Use this item now and talk to this pet to access the UOWOW features.");
	}

	//Mount & Bags
	mount = 17481; //mount override baron's
	if (!player->HasSpell(mount))
	{	//Free stuff!
		ChatHandler(player).PSendSysMessage("You have been awarded a free mount! Check your pet tab!");
		//player->addSpell(mount,false,false,false,false)
		player->learnSpell(mount, false);

		//4 12 slot bags (if not dk)
		if(player->getClass()!=CLASS_DEATH_KNIGHT)
		{
			ChatHandler(player).PSendSysMessage("You have been awarded 4 starting bags.");
			GiveItems(player,38145,4);
		}
	}
}

void uowToolSet::AnnounceKill(Player * killer, Player * killed)
{
	std::ostringstream msg;
	std::ostringstream KillerName;
	std::ostringstream KilledName;
	std::ostringstream areaname;
	if (killer->InBattleGround() || killer->InArena())
		return;
	if (killer->GetName() == killed->GetName())
		return;
	if(killer->GetGuildId()==0)
		KillerName << killer->GetName();
	else
		KillerName << killer->GetName() << " of " << killer->GetGuildName();
	if(killed->GetGuildId()==0)
		KilledName << killed->GetName();
	else
		KilledName << killed->GetName() << " of " << killed->GetGuildName();
	AreaTableEntry const* current_zone = GetAreaEntryByAreaID(killer->GetAreaId());
	if(!current_zone)
		areaname << "Unknown";
	else
		areaname << current_zone->area_name[0];
	if(killer->getLevel() > killed->getLevel())
	{
		if(killer->getLevel() - killed->getLevel() > 3)
		{
			msg << MSG_COLOR_LIGHTRED << "[" << KillerName.str( ).c_str( ) << "]" << MSG_COLOR_RED << " just newbie squashed " << MSG_COLOR_LIGHTRED << "[" << KilledName.str( ).c_str( ) << "] " << MSG_COLOR_RED << "in " << MSG_COLOR_GREEN << "[" << killer->GetBaseMap()->GetMapName() << ":" << areaname.str( ).c_str( ) << "]. "<< MSG_COLOR_RED << "How noble of him.";
			ChatHandler(killer).SendGlobalSysMessage(msg.str( ).c_str( ));
			return;
		}
	}
	msg << MSG_COLOR_LIGHTRED << "[" << KillerName.str( ).c_str( ) << "]" << MSG_COLOR_RED << " just made a corpse out of " << MSG_COLOR_LIGHTRED << "[" << KilledName.str( ).c_str( ) << "] " << MSG_COLOR_RED << "in " << MSG_COLOR_GREEN << "[" << killer->GetBaseMap()->GetMapName() << ":" << areaname.str( ).c_str( ) << "]";
	ChatHandler(killer).SendGlobalSysMessage(msg.str( ).c_str( ));

}

void uowToolSet::UpdateEvents(uint32 diff)
{
	EventCollection::const_iterator itr;
    for(itr = Events.begin(); itr != Events.end(); ++itr)
    {
		//Update all timers
		if (itr->second->GetCurrent()>=0)
			itr->second->Update(diff);
		else 
			itr->second->SetCurrent(0);
		
		//Update WorldGames
		if(itr->first == UPDATE_WORLDGAMES && itr->second->Passed())
		{
			itr->second->Reset();
			//Update_AirDrop();
			//Update_CraterCapture();
			//Update_BloodSailBootyCapture();
		}

		//Update  Banes
		if(itr->first == UPDATE_BANES && itr->second->Passed())
		{
			itr->second->Reset();
			UpdateBanes();
		}

		//Update Sige Control
		if(itr->first == UPDATE_SIEGECONTROL && itr->second->Passed())
		{
	        itr->second->Reset();	
			if(WorldGames.CurrentSiegeCity !=0)
				UpdateCityBattle();

		}

		//Display Siege Status
		if(itr->first == UPDATE_SIEGEDISPLAY && itr->second->Passed())
		{
	        itr->second->Reset();
			BroadcastSiegeStatus(0);
		}
		//HotZone Update
		if(itr->first == UPDATE_HOTZONE && itr->second->Passed())
		{
	        itr->second->Reset();
		    UpdateHotZone();
		}
 
		//General Update
		if(itr->first == UPDATE_GENERAL && itr->second->Passed())
		{
	        itr->second->Reset();
			BroadcastServerInfo();
			//UpdateWorldGames();
			BroadcastBanes();
		}

		//Update World Games
		/*if(itr->first == UPDATE_WORLDGAMES && itr->second->Passed())
		{
			itr->second->Reset();

			//Redneck Rampage
			if(WorldGames.Game_RR.gamestage==2) // Game In Progress
			{
				WorldGames.Game_RR.secondssincelastgame=0;
				//Update_RedneckRampage();
			}
			else
			{
				WorldGames.Game_RR.secondssincelastgame=WorldGames.Game_RR.secondssincelastgame+5;
			}
			if(WorldGames.Game_RR.secondssincelastgame>=600)//Prestart the game
			{
				WorldGames.Game_RR.secondssincelastgame=0;
				//PreStart_RedneckRampage();
			}
			if(WorldGames.Game_RR.secondssincelastgame>=30 && WorldGames.Game_RR.gamestage==1)//Start the game
			{
				WorldGames.Game_RR.secondssincelastgame=0;
				//Start_RedneckRampage();
			}

		}*/
    }
}
void uowToolSet::InitizlizeEvents()
{
	//5 second for World Game Updates
	IntervalTimer* u_wc = new IntervalTimer;
	u_wc->SetInterval(5*IN_MILISECONDS);
	Events.insert(EventCollection::value_type(UPDATE_WORLDGAMES,u_wc));

	//5 minute for Bane Updates
	IntervalTimer* u_wb = new IntervalTimer;
	u_wb->SetInterval(5*MINUTE*IN_MILISECONDS);
	Events.insert(EventCollection::value_type(UPDATE_BANES,u_wb));

	//1 minutes siege control updates
	IntervalTimer* u_sc = new IntervalTimer;
	u_sc->SetInterval(1*MINUTE*IN_MILISECONDS);
	Events.insert(EventCollection::value_type(UPDATE_SIEGECONTROL,u_sc));

	//45 minutes siege display
	IntervalTimer* u_sd = new IntervalTimer;
	u_sd->SetInterval(45*MINUTE*IN_MILISECONDS);
	Events.insert(EventCollection::value_type(UPDATE_SIEGEDISPLAY,u_sd));

	//HotZone 35 minutes
	UpdateHotZone();
	IntervalTimer* u_hz = new IntervalTimer;
	u_hz->SetInterval(35*MINUTE*IN_MILISECONDS);
	Events.insert(EventCollection::value_type(UPDATE_HOTZONE,u_hz));	

	//General Updates every 60 minutes
	IntervalTimer* g_general = new IntervalTimer;
	g_general->SetInterval(30*MINUTE*IN_MILISECONDS);
	Events.insert(EventCollection::value_type(UPDATE_GENERAL,g_general));	
}
void uowToolSet::Update(uint32 diff)
{  
	UpdateEvents(diff);
}
void uowToolSet::Initialize()
{
	Random.seed(time(NULL));
	LoadCityData();
	WorldGames.CurrentBonusGame=0;
	InitizlizeEvents();
	SetAttackableCity();
	LoadPlayerHousing();
	Load_RedneckRampage();
	Load_CraterCapture();
	Load_BloodSailBootyCapture();
	Load_AirDrop();
	InitializeCommonMarks();
	LoadCraftables();
}
void uowToolSet::UpdateHotZone()
{
	uint32 zones[] = {394,66,65,3537,3711,46,51,1377};
	WorldGames.CurrentHotZone = zones[Random.randInt(7)];
	//if (WorldGames.CurrentHotZone>4 || WorldGames.CurrentHotZone < 1) WorldGames.CurrentHotZone=0;
	AreaTableEntry const* current_zone = GetAreaEntryByAreaID(WorldGames.CurrentHotZone);
	std::stringstream extratext;
	if(!current_zone)
	{
		extratext << "ERROR (UpdateHotZone): AreaID = " << WorldGames.CurrentHotZone;
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,extratext.str().c_str());
		return;
	}	
	extratext << MSG_COLOR_ORANGE << "[" << current_zone->area_name[0] << "]" << MSG_COLOR_YELLOW << " is a HOT ZONE for the next 35 minutes. Kill as many monsters and people as fast as you can! Get extra x2 EXP for kills in this zone during this time!";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,extratext.str().c_str());
}
bool uowToolSet::OpenSiegeDoor(Player *player, GameObject *gameobject)
{
	bool open = false;
	CityCollection::const_iterator itr = WorldGames.SiegeCities.find(player->GetAreaId());
	if( itr == WorldGames.SiegeCities.end() )
		return open;
	//Can open
	if(itr->second.guildid==0 || itr->second.guildid == player->GetGuildId())
		open=true;

	//Handle Door Open
	if(open)
	{
		gameobject->UseDoorOrButton();
		ChatHandler(player).PSendSysMessage("You have been granted access to this object.");
	}
	else
	{
		ChatHandler(player).PSendSysMessage("You are not allowed to use this object at this time.");
	}
	return open;
}

bool uowToolSet::UseSiegeControl(Player *player, GameObject *gameobject)
{
	bool open = false;
	CityCollection::iterator itr = WorldGames.SiegeCities.find(player->GetAreaId());
 	if( itr == WorldGames.SiegeCities.end() || WorldGames.CurrentSiegeCity != player->GetAreaId())
		return open;
	open=true;

	//Check for Paladin Bubble
	if(player->HasAura(642))
	{
		ChatHandler(player).PSendSysMessage("You may not do this while using Divine Shield.");
		return false;
	}

	//Already controlled by this guild
	if(itr->second.contestedby == player->GetGuildId())
	{
		ChatHandler(player).PSendSysMessage("This siege stone is already under control of your guild or no guild.");
		return open;
	}
	else//New Contester
	{
		if(itr->second.guildid ==  player->GetGuildId())
			itr->second.contestedby = 0;				
		else
			itr->second.contestedby =  player->GetGuildId();
		itr->second.contestedmins = 0;
		std::ostringstream contesterinfo;
		if (itr->second.contestedby!=0)
		{
			Guild * owner = objmgr.GetGuildById(itr->second.contestedby);
			contesterinfo << MSG_COLOR_RED << "[" << owner->GetName() << "]";
		}
		else
		{
			contesterinfo << MSG_COLOR_RED << "[No Guild]";
		}
		std::ostringstream msg;
		AreaTableEntry const* current_zone = GetAreaEntryByAreaID(WorldGames.CurrentSiegeCity);
		msg << MSG_COLOR_GREEN << "[" << current_zone->area_name[0] << "]" << MSG_COLOR_LIGHTRED << " is now being contested by " << contesterinfo.str( ).c_str( );
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));		
	}
	return open;
}

void uowToolSet::BroadcastAttackbleCity()
{
	if (WorldGames.CurrentSiegeCity==0)
		return;
	CityCollection::const_iterator itr = WorldGames.SiegeCities.find(WorldGames.CurrentSiegeCity);
	std::ostringstream contesterinfo;
	if(itr->second.contestedby!=0)
	{
		Guild * attacker = objmgr.GetGuildById(itr->second.contestedby);
		contesterinfo << MSG_COLOR_LIGHTRED << "[" << attacker->GetName() << "]";
	}
	else
	{
		contesterinfo << MSG_COLOR_LIGHTRED << "[No Guild]";
	}
	std::ostringstream defenderinfo;
	if(itr->second.guildid!=0)
	{
		Guild * owner = objmgr.GetGuildById(itr->second.guildid);
		if(!owner)
			defenderinfo << MSG_COLOR_LIGHTRED << "[No Defenders]";
		else
			defenderinfo << MSG_COLOR_LIGHTRED << "[" << owner->GetName() << "]";
	}
	else
	{
		defenderinfo << MSG_COLOR_LIGHTRED << "[No Defenders]";
	}
	std::ostringstream msg;
	AreaTableEntry const* current_zone = GetAreaEntryByAreaID(WorldGames.CurrentSiegeCity);
	msg << MSG_COLOR_GREEN << "[" << current_zone->area_name[0] << "]" << MSG_COLOR_RED << " is now attackable! " << defenderinfo.str( ).c_str( ) << MSG_COLOR_RED << " will be moving in soon. All attackers have 1 hour to siege this city. " << contesterinfo.str( ).c_str( ) << MSG_COLOR_RED << " is currently in the lead for control.";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
}
void uowToolSet::SetAttackableCity()
{
    QueryResult *result = CharacterDatabase.PQuery("SELECT areaid from uow_SiegeCities where areaid > %u order by areaid",WorldGames.CurrentSiegeCity);
    if(!result)
		result = CharacterDatabase.PQuery("SELECT areaid from uow_SiegeCities order by areaid");
	if(!result)
		return;
    Field *fields = result->Fetch();
	CharacterDatabase.DirectPExecute("update uow_variables set LastCitySieged=%u",WorldGames.CurrentSiegeCity);
	WorldGames.CurrentSiegeCity = fields[0].GetUInt32();
    delete result;	
	CityCollection::iterator itr = WorldGames.SiegeCities.find(WorldGames.CurrentSiegeCity);
	itr->second.contestedmins=0;
	itr->second.contestedby=0;
	itr->second.minslefttoattack= 15 + Random.randInt(15);
	BroadcastAttackbleCity();
}
void uowToolSet::UpdateCityBattle()
{
	CityCollection::iterator itr = WorldGames.SiegeCities.find(WorldGames.CurrentSiegeCity);
	if( itr == WorldGames.SiegeCities.end() )
		return;
	if (itr->second.contestedby != 0 && itr->second.guildid != itr->second.contestedby)
		itr->second.contestedmins++;
	itr->second.minslefttoattack=itr->second.minslefttoattack-1;
	//winner check
	if (itr->second.contestedmins >= 10 && itr->second.contestedby != 0 && itr->second.guildid != itr->second.contestedby)
	{//attack winner
		//Reward each winner from this guild if they are online
		itr->second.guildid = itr->second.contestedby;
		RewardSiegeWinners(itr->second.guildid,WorldGames.CurrentSiegeCity,false);
		CharacterDatabase.PExecute("Update uow_SiegeCities set guildid=%u where areaid=%u",itr->second.guildid,itr->second.areaid);
		std::ostringstream ownerinfo;
		if(itr->second.guildid!=0)
		{
			Guild * owner = objmgr.GetGuildById(itr->second.guildid);
			if(!owner)
				ownerinfo << MSG_COLOR_LIGHTRED << "[No Owner]";
			else
				ownerinfo << MSG_COLOR_LIGHTRED << "[" << owner->GetName() << "]";
		}
		else
		{
			ownerinfo << MSG_COLOR_LIGHTRED << "[No Owner]";
		}
		std::ostringstream msg;
		AreaTableEntry const* current_zone = GetAreaEntryByAreaID(WorldGames.CurrentSiegeCity);
		msg << MSG_COLOR_YELLOW << "The battle for " << MSG_COLOR_GREEN "[" << current_zone->area_name[0] << "]" << MSG_COLOR_YELLOW << " is now over! The winners are: " << MSG_COLOR_RED << ownerinfo.str( ).c_str( );
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));		
		EventCollection::iterator iter = Events.find(UPDATE_SIEGECONTROL);
		iter->second->Reset();
		itr->second.minslefttoattack=0;
		SetAttackableCity();
		return;
	}
	//defend winner (or default timer)
	if(itr->second.minslefttoattack<=0 && itr->second.contestedmins==0 && itr->second.contestedby==0)
	{
		std::ostringstream ownerinfo;
		if(itr->second.guildid!=0)
		{
			Guild * owner = objmgr.GetGuildById(itr->second.guildid);
			if(!owner)
			{
				ownerinfo << MSG_COLOR_LIGHTRED << "[No Owner]";
			}
			else
			{
				ownerinfo << MSG_COLOR_LIGHTRED << "[" << owner->GetName() << "]";
				RewardSiegeWinners(itr->second.guildid,WorldGames.CurrentSiegeCity,true);
			}
		}
		else
		{
			ownerinfo << MSG_COLOR_LIGHTRED << "[No Owner]";
		}
		std::ostringstream msg;
		AreaTableEntry const* current_zone = GetAreaEntryByAreaID(WorldGames.CurrentSiegeCity);
		msg << MSG_COLOR_YELLOW << "The battle for " << MSG_COLOR_GREEN "[" << current_zone->area_name[0] << "]" << MSG_COLOR_YELLOW << " is now over! " << MSG_COLOR_RED << ownerinfo.str( ).c_str( ) << MSG_COLOR_YELLOW << " has defended the city!";
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));		
		EventCollection::iterator iter = Events.find(UPDATE_SIEGECONTROL);
		iter->second->Reset();
		itr->second.minslefttoattack=0;
		SetAttackableCity();
	}
}


void uowToolSet::RewardSiegeWinners(uint32 guildid, uint32 areaid, bool defenders)
{
	Guild * g = objmgr.GetGuildById(guildid);
	if(!g)
		return;
	for (Guild::MemberList::const_iterator itr = g->members.begin(); itr !=  g->members.end(); ++itr)
    {
        if (Player *pl = ObjectAccessor::FindPlayer(MAKE_NEW_GUID(itr->first, 0, HIGHGUID_PLAYER)))
        {
			if(pl->IsInWorld() && pl->GetAreaId()==areaid && pl->getLevel() == 80 && pl->GetMaxHealth() > 15000)
			{
				if(defenders)
				{
					GiveItems(pl,UOW_WSGMARK,5);
					GiveItems(pl,70028,1);
					ChatHandler(pl).PSendSysMessage("You've gained 5 WSG Marks and a Platinum Ticket for defending a city.");
				}
				else
				{
					GiveItems(pl,UOW_WSGMARK,2);
					ChatHandler(pl).PSendSysMessage("You've gained 2 WSG marks for attacking this city. Defend this city for even greater rewards!");
				}
			}
		}
	}

}
uint32 uowToolSet::GetGuildMembersOnline(uint32 guildid)
{
	uint32 count = 0;
	Guild * g = objmgr.GetGuildById(guildid);
	if(!g)
		return count;
	for (Guild::MemberList::const_iterator itr = g->members.begin(); itr !=  g->members.end(); ++itr)
    {
        if (Player *pl = ObjectAccessor::FindPlayer(MAKE_NEW_GUID(itr->first, 0, HIGHGUID_PLAYER)))
        {
			if(pl->IsInWorld())
				count++;
		}
	}
	return count;
}
void uowToolSet::GiveItems(Player * player, uint32 itementry, uint32 quantity)
{
	uint32 noSpaceForCount = 0;
	ItemPosCountVec dest;
	uint8 msg = player->CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, itementry, quantity, &noSpaceForCount );
	if( msg != EQUIP_ERR_OK )   // convert to possible store amount
		quantity = noSpaceForCount;
	if( quantity == 0 || dest.empty()) // can't add any
	{
		ChatHandler(player).PSendSysMessage("You don't have any space in your bags for a reward.");
		return;
	}
	Item* item = player->StoreNewItem( dest, itementry, true, Item::GenerateItemRandomPropertyId(itementry));
	player->SendNewItem(item,quantity,true,false);
}
bool uowToolSet::HasMats(Player * player, uint32 itementry, uint32 quantity)
{
	if (player->HasItemCount(itementry,quantity,false))
		return true;
	ItemPrototype const* matProto = objmgr.GetItemPrototype(itementry);
	std::ostringstream msg;
	msg << MSG_COLOR_RED << "[MISSING] " << MSG_COLOR_LIGHTRED << matProto->Name1 << MSG_COLOR_YELLOW << " x " << quantity;
	ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
	return false;
}
void uowToolSet::CraftItem(Player * player, uint32 itemid)
{
	if (!player->HasSpell(69541))
	{
		ChatHandler(player).PSendSysMessage("You must posses the power of the Pandaren Monk before you can craft new items.");
		return;
	}
	CraftingCollection::const_iterator itr =  Craftables.find(itemid);
	if( itr ==  Craftables.end() )
	{
		ChatHandler(player).PSendSysMessage("The ID you entered can't be crafted or you entered an invalid ID, check the number and try again.");
		return;
	}

	CraftingData craftdata = itr->second;
	//Check Resources
	if((craftdata.resourceid1>0 && !HasMats(player,craftdata.resourceid1,craftdata.quantity1))
		||(craftdata.resourceid2>0 && !HasMats(player,craftdata.resourceid2,craftdata.quantity2))
		||(craftdata.resourceid3>0 && !HasMats(player,craftdata.resourceid3,craftdata.quantity3))
		||(craftdata.resourceid4>0 && !HasMats(player,craftdata.resourceid4,craftdata.quantity4)))
		return;
	//Needs guild city
	if(craftdata.tier > 2 && !uowTools.GetGuildCityByGuildID(player->GetGuildId()))
	{
		ChatHandler(player).PSendSysMessage("You must be in a guild with a guild city in order to craft this tier of items.");
		return;
	}
	if(craftdata.resourceid1>0)
		player->DestroyItemCount(craftdata.resourceid1,craftdata.quantity1,true,false);
	if(craftdata.resourceid2>0)
		player->DestroyItemCount(craftdata.resourceid2,craftdata.quantity2,true,false);
	if(craftdata.resourceid3>0)
		player->DestroyItemCount(craftdata.resourceid3,craftdata.quantity3,true,false);
	if(craftdata.resourceid4>0)
		player->DestroyItemCount(craftdata.resourceid4,craftdata.quantity4,true,false);
	GiveItems(player,craftdata.itemid,1);
}
uint32 uowToolSet::SpawnGO(uint32 entry, Map *map ,float x,float y,float z,float o, bool SaveToDB)
{
    const GameObjectInfo *gInfo = objmgr.GetGameObjectInfo(entry);
    if (!gInfo)
        return 0;
    if (gInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(gInfo->displayId))
        return 0;
    GameObject* pGameObj = new GameObject;
    uint32 db_lowGUID = objmgr.GenerateLowGuid(HIGHGUID_GAMEOBJECT);
    if(!pGameObj->Create(db_lowGUID, gInfo->id, map, PHASEMASK_NORMAL, x, y, z, o, 0.0f, 0.0f, 0.0f, 0.0f, 0, GO_STATE_READY))
    {
        delete pGameObj;
        return 0;
    }
    // fill the gameobject data and save to the db
    pGameObj->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()),PHASEMASK_NORMAL);
    // this will generate a new guid if the object is in an instance
    if(!pGameObj->LoadFromDB(db_lowGUID, map))
    {
        delete pGameObj;
        return 0;
    }
    map->Add(pGameObj);
    objmgr.AddGameobjectToGrid(db_lowGUID, objmgr.GetGOData(db_lowGUID));
	if(!SaveToDB)
		pGameObj->DeleteFromDB();
    return db_lowGUID;
}

GameObject * uowToolSet::SpawnTempGO(uint32 entry, Map *map ,float x,float y,float z,float o, uint32 despawntime, uint64 ownerguid)
{
    float rot2 = sin(o/2);
    float rot3 = cos(o/2);
    GameObjectInfo const* goinfo = objmgr.GetGameObjectInfo(entry);
    if(!goinfo)
    {
        sLog.outErrorDb("Gameobject template %u not found in database!", entry);
        return 0;
    }
  
    GameObject *go = new GameObject();
    if(!go->Create(objmgr.GenerateLowGuid(HIGHGUID_GAMEOBJECT), entry, map, PHASEMASK_NORMAL, x,y,z,o,0,0,rot2,rot3,100,GO_STATE_READY))
    {
        delete go;
        return 0;
    }
	go->SetOwnerGUID(ownerguid);
    go->SetRespawnTime(despawntime);
    map->Add(go);
    return go;
}

uint32 uowToolSet::SpawnNPC(uint32 entry, Map *map ,float x,float y,float z,float o, bool SaveToDB)
{
    Creature* pCreature = new Creature;
    if (!pCreature->Create(objmgr.GenerateLowGuid(HIGHGUID_UNIT), map, PHASEMASK_NORMAL, entry, 0, 0, x, y, z, o))
    {
        delete pCreature;
        return 0;
    }
	pCreature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), PHASEMASK_NORMAL);
    uint32 db_guid = pCreature->GetDBTableGUIDLow();
    pCreature->LoadFromDB(db_guid, map);
    map->Add(pCreature);
    objmgr.AddCreatureToGrid(db_guid, objmgr.GetCreatureData(db_guid));
	if(!SaveToDB)
		pCreature->DeleteFromDB();
    return db_guid;
}
uint32 uowToolSet::SpawnTempNPC(uint32 entry, Map *map ,float x,float y,float z,float o, uint32 despawntime, Player * Owner)
{
	Position pos = {x,y,z,o};
	if(TempSummon *summon = map->SummonCreature(entry, pos, NULL, despawntime * IN_MILISECONDS,(Unit*)Owner))
	{
		summon->SetTempSummonType(TEMPSUMMON_TIMED_OR_DEAD_DESPAWN);
		return summon->GetGUID();
	}
	return 0;
}
void uowToolSet::LoadPlayerHousing()
{
 
	PlayerHousing.clear();
	//Housing
	QueryResult *result = CharacterDatabase.Query("SELECT * FROM uow_player_housing");
	if(result)
	do
	{
		Field *fields = result->Fetch();
		PlayerHousingData * house=new PlayerHousingData();
		house->id = fields[0].GetInt32();
		house->accountid=fields[1].GetInt32();
		house->type = fields[2].GetInt32();
		house->ispublic = fields[3].GetInt32();
		house->isforguild = fields[4].GetInt32();
		house->guildid = fields[5].GetInt32();
		house->banewindow = fields[6].GetString();

		//Player Housing Access List
		house->accesslist.clear();
		QueryResult *aresult = CharacterDatabase.PQuery("SELECT * FROM uow_player_housing_access where houseid = %u", house->id);
		if(aresult)
		do
		{
			Field *afields = aresult->Fetch();
			house->accesslist[afields[1].GetInt32()]=afields[2].GetInt32();
		}
		while (aresult->NextRow());		
		delete aresult;

		//Housing Objects
		aresult = CharacterDatabase.PQuery("SELECT * FROM uow_player_housing_objects where houseid = %u", house->id);
		if(aresult)
		do
		{
			Field *fields = aresult->Fetch();
			PlayerHousingObjects* houseobject=new PlayerHousingObjects();
			houseobject->id = fields[0].GetInt32();
			houseobject->houseid = fields[1].GetInt32();
			houseobject->gospawnid = fields[2].GetInt32();
			houseobject->tx = fields[3].GetFloat();
			houseobject->ty = fields[4].GetFloat();
			houseobject->tz = fields[5].GetFloat();
			houseobject->selectable = fields[6].GetInt32();
			house->objectlist[houseobject->id]=houseobject;
		}
		while (aresult->NextRow());		
		delete aresult;

		//Housing NPC's
		aresult = CharacterDatabase.PQuery("SELECT * FROM uow_player_housing_npc where houseid = %u", house->id);
		if(aresult)
		do
		{
			Field *fields = aresult->Fetch();
			PlayerHousingNPCS* housenpcs=new PlayerHousingNPCS();
			housenpcs->id = fields[0].GetInt32();
			housenpcs->houseid = fields[1].GetInt32();
			housenpcs->npcspawnid = fields[2].GetInt32();
			housenpcs->selectable = fields[3].GetInt32();
			house->npclist[housenpcs->id]=housenpcs;
		}
		while (aresult->NextRow());		
		delete aresult;

		//add the finished product to the player's housing list
		PlayerHousing.insert(HouseCollection::value_type(house->id,house));
	}
	while (result->NextRow());		
	delete result;
}

float uowToolSet::GetHeight(float x, float y, Player * player)
{
	if (!player->GetMap())
		return 0.0f;
	return player->GetMap()->GetHeight(x,y,player->GetPositionZ(),false);
}
float uowToolSet::GetHeight(float x, float y, float z, Map  * map)
{
	if (!map)
		return 0.0f;
	return map->GetHeight(x,y,z,false);
}
void uowToolSet::MakeHouse(uint32 housetype, Player * player)
{
	//Check for existing home
	if(HasHouse(player) && housetype < 100)
	{
		ChatHandler(player).PSendSysMessage("You already own a home.");
		return;
	}

	//build it
	switch(housetype)
	{
		case 100: //Guild City Flag
			{
				if(!HasMats(player,70004,1))
				{
					ChatHandler(player).PSendSysMessage("Page a GM and ask about a Guild City Charter.");
					return;
				}
				if(GetGuildCity(player))
				{
					ChatHandler(player).PSendSysMessage("You already own a guild city.");
					return;
				}
				if(GetGuildCityByGuildID(player->GetGuildId()))
				{
					ChatHandler(player).PSendSysMessage("Your guild already owns a guild city.");
					return;
				}
				if (player->GetMoney() < 25000000)
				{
					player->SendBuyError( BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
					return;
				}
				player->DestroyItemCount(70004,1,true,false);
				player->ModifyMoney(-(25000000));
				//Write new house to the database ad get the houseid
				CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing (accountid, type, isforguild, guildid) VALUES (%u,%u,%u,%u)",player->GetSession()->GetAccountId(),housetype,1,player->GetGuildId());
				QueryResult *result = CharacterDatabase.PQuery("SELECT id FROM uow_player_housing where accountid = %u and type = 100", player->GetSession()->GetAccountId());
				if(!result)
				{
					ChatHandler(player).PSendSysMessage("Error creating home. No results for new house ID.");
					return;
				}
				Field *fields = result->Fetch();
				uint32 houseid = fields[0].GetInt32();

				//create go's and write each to the database
				uint32 ret;
				ret = this->SpawnGO(GUILDCITY_MARKER, player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(),  player->GetOrientation(),true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
			}
			break;
		case 0: //Shack
			{
				if(!HasMats(player,UOW_WRIT,20))
				{
					ChatHandler(player).PSendSysMessage("Page a GM and ask about purchasing Writs!.");
					return;
				}
				player->DestroyItemCount(UOW_WRIT,20,true,false);
				//Write new house to the database ad get the houseid
				CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing (accountid, type, isforguild, guildid) VALUES (%u,%u,%u,%u)",player->GetSession()->GetAccountId(),housetype,0,player->GetGuildId());
				QueryResult *result = CharacterDatabase.PQuery("SELECT id FROM uow_player_housing where accountid = %u and type = 0", player->GetSession()->GetAccountId());
				if(!result)
				{
					ChatHandler(player).PSendSysMessage("Error creating home. No results for new house ID.");
					return;
				}
				Field *fields = result->Fetch();
				uint32 houseid = fields[0].GetInt32();

				//create go's and write each to the database
				uint32 ret;
				ret = this->SpawnGO(70125, player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 0.647958f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(300311,  player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()+9, 3.67696f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(191164,  player->GetMap(), player->GetPositionX() - 3.691f, player->GetPositionY() - 2.67f, player->GetPositionZ() + 21.7972f, 3.79918f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(191956,  player->GetMap(), player->GetPositionX() - 2.501f, player->GetPositionY() + 4.69f, player->GetPositionZ()+ 21.7972f, 1.60399f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(183450,  player->GetMap(), player->GetPositionX() + 6.069f, player->GetPositionY() + 2.98f, player->GetPositionZ()+ 21.7772f, 0.655791f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
			
				//create NPC's and write each to the database
				ret = this->SpawnNPC(28676,  player->GetMap(), player->GetPositionX() + 4.154f, player->GetPositionY() - 2.16f, player->GetPositionZ() + 21.7972f, 2.83314f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(29532,  player->GetMap(), player->GetPositionX() + 5.072f, player->GetPositionY() - 0.02f, player->GetPositionZ() + 21.7972f, 2.86221f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

			}
			break;
		case 1: //Donator Tower
			{
				if(!HasMats(player,UOW_WRIT,150))
				{
					ChatHandler(player).PSendSysMessage("Page a GM and ask about purchasing Writs!.");
					return;
				}
				player->DestroyItemCount(UOW_WRIT,150,true,false);
				//Write new house to the database ad get the houseid
				CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing (accountid, type, isforguild, guildid) VALUES (%u,%u,%u,%u)",player->GetSession()->GetAccountId(),housetype,0,player->GetGuildId());
				QueryResult *result = CharacterDatabase.PQuery("SELECT id FROM uow_player_housing where accountid = %u and type = 1", player->GetSession()->GetAccountId());
				if(!result)
				{
					ChatHandler(player).PSendSysMessage("Error creating home. No results for new house ID.");
					return;
				}
				Field *fields = result->Fetch();
				uint32 houseid = fields[0].GetInt32();

				//create go's and write each to the database
				uint32 ret;
				ret = this->SpawnGO(70126,  player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()+2, 3.67696f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(300311,  player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()+9, 3.67696f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);


				ret = this->SpawnGO(191956,  player->GetMap(), player->GetPositionX() - 7.291f, player->GetPositionY() - 8.19f, player->GetPositionZ() + 2.0501f, 3.66126f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(1685,  player->GetMap(), player->GetPositionX() - 9.584f, player->GetPositionY() + 4.29f, player->GetPositionZ()+ 2.0519f, 2.70857f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(192697,  player->GetMap(), player->GetPositionX() - 4.801f, player->GetPositionY() + 8.18f, player->GetPositionZ()+ 2.0517f, 2.15093f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(176296,  player->GetMap(), player->GetPositionX() - 4.533f, player->GetPositionY() + 5.36f, player->GetPositionZ() + 37.1407f, 2.35514f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
			
				ret = this->SpawnGO(13965,  player->GetMap(), player->GetPositionX() - 13.606f, player->GetPositionY() - 8.03f, player->GetPositionZ() + 1.954f, 3.67174f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
				//------------------------------------------------------------------------------------------------------------------------------

				//create NPC's and write each to the database
				ret = this->SpawnNPC(29532,  player->GetMap(), player->GetPositionX() + 3.651f, player->GetPositionY() + 0.61f, player->GetPositionZ() + 2.0535f, 3.57199f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);
			
				//create NPC's and write each to the database
				ret = this->SpawnNPC(28676,  player->GetMap(), player->GetPositionX() + 4.08f, player->GetPositionY() - 9.23f, player->GetPositionZ() + 2.052f, 2.19754f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

 

			}		
			break;
		case 3: //Zep
			{
				//Write new house to the database ad get the houseid
				CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing (accountid, type, isforguild, guildid) VALUES (%u,%u,%u,%u)",player->GetSession()->GetAccountId(),housetype,0,player->GetGuildId());
				QueryResult *result = CharacterDatabase.PQuery("SELECT id FROM uow_player_housing where accountid = %u and type = 3", player->GetSession()->GetAccountId());
				if(!result)
				{
					ChatHandler(player).PSendSysMessage("Error creating home. No results for new house ID.");
					return;
				}
				Field *fields = result->Fetch();
				uint32 houseid = fields[0].GetInt32();

				//create go's and write each to the database
				uint32 ret;
				ret = this->SpawnGO(70024,  player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 3.11018f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(193089,  player->GetMap(), player->GetPositionX() - 4.123f, player->GetPositionY() + 2.16f, player->GetPositionZ() - 20.443f, 1.55666f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(191956,  player->GetMap(), player->GetPositionX() - 0.945f, player->GetPositionY() + 2.54f, player->GetPositionZ() - 23.723f, 4.72108f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(13965,  player->GetMap(), player->GetPositionX() - 11.441f, player->GetPositionY() +8.32f, player->GetPositionZ() - 24.401f, 3.08744f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(192697,  player->GetMap(), player->GetPositionX() - 3.556f, player->GetPositionY() + 12.14f, player->GetPositionZ() - 17.919f, 3.38039f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
			
				ret = this->SpawnGO(1685,  player->GetMap(), player->GetPositionX() - 4.358f, player->GetPositionY() + 4.7f, player->GetPositionZ() - 17.897f, 3.29321f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
			
				ret = this->SpawnGO(183384,  player->GetMap(), player->GetPositionX() + 15.486f, player->GetPositionY() + 4.4f, player->GetPositionZ() -15.578f, 6.15972f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
			
				ret = this->SpawnGO(191164,  player->GetMap(), player->GetPositionX() + 15.989f, player->GetPositionY() + 9.78f, player->GetPositionZ() - 15.479f, 0.0650248f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);


				//Teleporter Top
				//Teleport to Bottom XO's
				float tx = player->GetPositionX() - 14.09f;
				float ty = player->GetPositionY() + 8.305;
				float tz = GetHeight(player->GetPositionX()  - 14.09f, player->GetPositionY() + 8.305, player) + 2;
				ret = this->SpawnGO(192951,  player->GetMap(), player->GetPositionX()  - 14.09f, player->GetPositionY() +  8.305f , player->GetPositionZ() - 24.186f,  6.12582f ,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
				ret = this->SpawnGO(194437,  player->GetMap(), player->GetPositionX()  - 14.09f, player->GetPositionY()+  8.305f , player->GetPositionZ() - 24.186f,  0.188496f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid,tx,ty,tz ) VALUES (%u,%u,%f,%f,%f)",houseid,ret,tx,ty,tz);

				//Teleporter Bottom
				//Teleport to Top XO's
				tx = player->GetPositionX() - 14.09f;
				ty = player->GetPositionY() + 8.305;
				tz = player->GetPositionZ() - 22.186f;
				// Get the current terrain height at this position
				float z = GetHeight(player->GetPositionX()  - 14.09f, player->GetPositionY() + 8.305f,player);
				ret = this->SpawnGO(192951,  player->GetMap(), player->GetPositionX()  - 14.09f, player->GetPositionY() +  8.305f, z,  6.12582f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
				ret = this->SpawnGO(194437,  player->GetMap(), player->GetPositionX()  - 14.09f, player->GetPositionY() +  8.305f , z,  0.188496f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid,tx,ty,tz) VALUES (%u,%u,%f,%f,%f)",houseid,ret,tx,ty,tz);
				


				//create NPC's and write each to the database
				ret = this->SpawnNPC(28676,  player->GetMap(), player->GetPositionX() - 7.006f, player->GetPositionY() + 2.68f, player->GetPositionZ() - 23.631f, 1.35482f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);
	 
				//create NPC's and write each to the database
				ret = this->SpawnNPC(45001,  player->GetMap(), player->GetPositionX() + 9.009f, player->GetPositionY() + 10.85f, player->GetPositionZ() - 22.677f, 3.06309f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(9856,  player->GetMap(), player->GetPositionX() + 4.953f, player->GetPositionY() + 13.17f, player->GetPositionZ() - 22.677f, 4.69495f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(45002,  player->GetMap(), player->GetPositionX() + 6.942f, player->GetPositionY() - 12.99f, player->GetPositionZ() - 22.677f, 4.62034f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(29532,  player->GetMap(), player->GetPositionX() + 3.01f, player->GetPositionY() - 7.62f, player->GetPositionZ() - 23.78f, 3.02928f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

			}
			break;
		case 4: //Kingpin
			{
				if(!HasMats(player,UOW_WRIT,200))
				{
					ChatHandler(player).PSendSysMessage("Page a GM and ask about purchasing Writs!.");
					return;
				}
				player->DestroyItemCount(UOW_WRIT,200,true,false);
				//Write new house to the database ad get the houseid
				CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing (accountid, type, isforguild, guildid) VALUES (%u,%u,%u,%u)",player->GetSession()->GetAccountId(),housetype,0,player->GetGuildId());
				QueryResult *result = CharacterDatabase.PQuery("SELECT id FROM uow_player_housing where accountid = %u and type = 4", player->GetSession()->GetAccountId());
				if(!result)
				{
					ChatHandler(player).PSendSysMessage("Error creating home. No results for new house ID.");
					return;
				}
				Field *fields = result->Fetch();
				uint32 houseid = fields[0].GetInt32();

				//create go's and write each to the database
				uint32 ret;
				ret = this->SpawnGO(190373,  player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 1.50168f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
	 
				ret = this->SpawnGO(300311,  player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()+9, 3.67696f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(13965,  player->GetMap(), player->GetPositionX() - 9.286f, player->GetPositionY() - 9.51f, player->GetPositionZ() + 4.3044f, 3.8241f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(13965,  player->GetMap(), player->GetPositionX() + 14.445f, player->GetPositionY() + 10.9f, player->GetPositionZ() + 4.6043f, 0.729631f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(192697 ,  player->GetMap(), player->GetPositionX() + 3.604f, player->GetPositionY() - 26.68f, player->GetPositionZ() + 30.6564f, 4.64878f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(193089,  player->GetMap(), player->GetPositionX() + 1.24f, player->GetPositionY() - 14.95f, player->GetPositionZ() + 25.3264f, 1.52288f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(13965,  player->GetMap(), player->GetPositionX() - 7.507f, player->GetPositionY() + 12.29f, player->GetPositionZ() + 4.3043f, 2.32006f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(13965,  player->GetMap(), player->GetPositionX() + 12.854f, player->GetPositionY() - 11.09f, player->GetPositionZ()+ 4.3044f, 5.4538f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(1685,  player->GetMap(), player->GetPositionX() + 1.569f, player->GetPositionY() - 26.41f, player->GetPositionZ()+ 30.6564f, 4.66841f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(183384,  player->GetMap(), player->GetPositionX() - 10.074f, player->GetPositionY() - 5.09f, player->GetPositionZ()+ 30.6564f, 3.64739f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(191164,  player->GetMap(), player->GetPositionX() - 4.382f, player->GetPositionY() - 10.82f, player->GetPositionZ()+ 30.6564f, 4.10292f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(191956,  player->GetMap(), player->GetPositionX() - 2.895f, player->GetPositionY() + 13.48f, player->GetPositionZ()+ 30.6564f, 5.21898f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(173197,  player->GetMap(), player->GetPositionX() + 9.839f, player->GetPositionY() + 13.1f, player->GetPositionZ()+ 30.6564f, 4.088f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(173197,  player->GetMap(), player->GetPositionX() + 15.524f, player->GetPositionY() + 6.29f, player->GetPositionZ()+ 30.6465f, 0.414689f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(180324,  player->GetMap(), player->GetPositionX() - 23.787, player->GetPositionY() + 2.37f, player->GetPositionZ() + 30.6564f, 3.08112f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(29532,  player->GetMap(), player->GetPositionX() - 9.272f, player->GetPositionY() + 8.04f, player->GetPositionZ() + 30.6564f, 5.57241f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(9856,  player->GetMap(), player->GetPositionX() + 2.942f, player->GetPositionY() + 6.72f, player->GetPositionZ() + 17.1544f, 1.49932f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(28676,  player->GetMap(), player->GetPositionX() + 5.946f, player->GetPositionY() - 12.53f, player->GetPositionZ() + 21.1964f, 2.32399f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(45002,  player->GetMap(), player->GetPositionX() + 8.27f, player->GetPositionY() - 11.55f, player->GetPositionZ() + 30.6564f, 2.13629f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(45001,  player->GetMap(), player->GetPositionX() + 14.395f, player->GetPositionY() - 6.13f, player->GetPositionZ() + 30.6564f, 2.42297f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);


			}
			break;
		case 5:
			{
				if(!HasMats(player,UOW_WRIT,250))
				{
					ChatHandler(player).PSendSysMessage("Page a GM and ask about purchasing Writs!.");
					return;
				}
				player->DestroyItemCount(UOW_WRIT,250,true,false);
				//Write new house to the database ad get the houseid
				CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing (accountid, type, isforguild, guildid) VALUES (%u,%u,%u,%u)",player->GetSession()->GetAccountId(),housetype,0,player->GetGuildId());
				QueryResult *result = CharacterDatabase.PQuery("SELECT id FROM uow_player_housing where accountid = %u and type = 5", player->GetSession()->GetAccountId());
				if(!result)
				{
					ChatHandler(player).PSendSysMessage("Error creating home. No results for new house ID.");
					return;
				}
				Field *fields = result->Fetch();
				uint32 houseid = fields[0].GetInt32();

				//create go's and write each to the database
				uint32 ret;
				ret = this->SpawnGO(70106, player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation(),true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(300311,  player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()+9, 3.67696f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
			}
			break;
		case 20: //Guild Airship
			{
				if(!HasMats(player,UOW_WRIT,300))
				{
					ChatHandler(player).PSendSysMessage("Page a GM and ask about purchasing Writs!.");
					return;
				}
				player->DestroyItemCount(UOW_WRIT,300,true,false);
				//Write new house to the database ad get the houseid
				CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing (accountid, type, isforguild, guildid) VALUES (%u,%u,%u,%u)",player->GetSession()->GetAccountId(),housetype,0,player->GetGuildId());
				QueryResult *result = CharacterDatabase.PQuery("SELECT id FROM uow_player_housing where accountid = %u and type = 20", player->GetSession()->GetAccountId());
				if(!result)
				{
					ChatHandler(player).PSendSysMessage("Error creating home. No results for new house ID.");
					return;
				}
				Field *fields = result->Fetch();
				uint32 houseid = fields[0].GetInt32();

				//create go's and write each to the database
				uint32 ret;
				ret = this->SpawnGO(77814,  player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 3.33559f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(193089,  player->GetMap(), player->GetPositionX() -32.709f, player->GetPositionY() -6.65f, player->GetPositionZ() + 0.737f, 0.193984f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(191956,  player->GetMap(), player->GetPositionX() -38.318f, player->GetPositionY() -7.44f, player->GetPositionZ() + 9.612f, 0.193988f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(191956,  player->GetMap(), player->GetPositionX() +16.867f, player->GetPositionY() -0.66f, player->GetPositionZ() + 20.792f, 3.28846f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(192697,  player->GetMap(), player->GetPositionX() +66.202f , player->GetPositionY() + 12.98f, player->GetPositionZ() + 23.486f, 0.217557f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(1685,  player->GetMap(), player->GetPositionX() +63.143f, player->GetPositionY() +8.81f, player->GetPositionZ() + 23.486f, 6.01065f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(173197,  player->GetMap(), player->GetPositionX() +59.81f, player->GetPositionY() +18.21f , player->GetPositionZ() + 23.517f, 1.80563f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(190894,  player->GetMap(), player->GetPositionX() +14.345f, player->GetPositionY() + 2.69f , player->GetPositionZ() + 25.209f, 0.145901f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(176296,  player->GetMap(), player->GetPositionX() -38.139f, player->GetPositionY() +3.71f, player->GetPositionZ() + 40.417f, 1.61634f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(176499,  player->GetMap(), player->GetPositionX() -32.929f, player->GetPositionY() -17.75f, player->GetPositionZ() + 40.473f, 4.97784f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(191164,  player->GetMap(), player->GetPositionX() -29.84f, player->GetPositionY() -5.94f, player->GetPositionZ() + 40.503f, 0.171206f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(183384,  player->GetMap(), player->GetPositionX() -56.479f, player->GetPositionY() -5.94f, player->GetPositionZ() + 40.527f, 3.33636f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(176501,  player->GetMap(), player->GetPositionX() -57.409f, player->GetPositionY() -5.94f, player->GetPositionZ() + 40.511f, 2.91225f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(176497,  player->GetMap(), player->GetPositionX() -55.285f, player->GetPositionY() -16.68f, player->GetPositionZ() + 40.514f, 3.59162f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(176500,  player->GetMap(), player->GetPositionX() -46.475f, player->GetPositionY() -20.86f, player->GetPositionZ() + 40.514f, 4.88361f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	

				ret = this->SpawnGO(176498,  player->GetMap(), player->GetPositionX() -50.924f, player->GetPositionY() +1.53f, player->GetPositionZ() + 40.514f, 1.74987f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(182351,  player->GetMap(), player->GetPositionX() -30.937f, player->GetPositionY() +0.15f, player->GetPositionZ() + 40.514f, 0.218343f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				ret = this->SpawnGO(182352,  player->GetMap(), player->GetPositionX() -28.491f, player->GetPositionY() -12.33f, player->GetPositionZ() + 40.502f, 0.147652f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);

				//Doors
				ret = this->SpawnGO(190192,  player->GetMap(), player->GetPositionX() -25.08f, player->GetPositionY() -4.862f, player->GetPositionZ() -4.892f, 3.32293f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	
				ret = this->SpawnGO(177246,  player->GetMap(), player->GetPositionX() +15.24f, player->GetPositionY() +2.966f, player->GetPositionZ() +20.97f, 3.37241f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	
				ret = this->SpawnGO(177246,  player->GetMap(), player->GetPositionX() -19.85f, player->GetPositionY() -3.894f, player->GetPositionZ() +20.97f, 3.3143f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);	
				

				//Teleporter Top
				//Teleport to Bottom XO's
				float tx = player->GetPositionX()-41.84f;
				float ty = player->GetPositionY()-8;
				float tz = GetHeight(player->GetPositionX()  -41.84f, player->GetPositionY()-8,player) + 2;
				ret = this->SpawnGO(192951,  player->GetMap(), player->GetPositionX()  -41.84f, player->GetPositionY()-8 , player->GetPositionZ() + 40.0f,  0.188496f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
				ret = this->SpawnGO(194437,  player->GetMap(), player->GetPositionX()  -41.84f, player->GetPositionY()-8 , player->GetPositionZ() + 40.0f,  0.188496f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid,tx,ty,tz ) VALUES (%u,%u,%f,%f,%f)",houseid,ret,tx,ty,tz);

				//Teleporter Bottom
				//Teleport to Top XO's
				tx = player->GetPositionX()-41.84f;
				ty = player->GetPositionY()-8;
				tz = player->GetPositionZ() + 42.0f;
				// Get the current terrain height at this position
				float z = GetHeight(player->GetPositionX()  -41.84f, player->GetPositionY()-8, player);
				ret = this->SpawnGO(192951,  player->GetMap(), player->GetPositionX()  -41.84f, player->GetPositionY()-8 , z,  0.188496f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",houseid,ret);
				ret = this->SpawnGO(194437,  player->GetMap(), player->GetPositionX()  -41.84f, player->GetPositionY()-8 , z,  0.188496f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid,tx,ty,tz) VALUES (%u,%u,%f,%f,%f)",houseid,ret,tx,ty,tz);
				
				//create NPC's and write each to the database
				ret = this->SpawnNPC(45006,  player->GetMap(), player->GetPositionX() + 58.612f, player->GetPositionY() + 7.57f, player->GetPositionZ() -5.169, 3.06227f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(45007,  player->GetMap(), player->GetPositionX() + 57.347f, player->GetPositionY() +14.2f, player->GetPositionZ() -5.251f, 3.27826f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(45008,  player->GetMap(), player->GetPositionX() -24.03f, player->GetPositionY() + 14.7f, player->GetPositionZ() + 9.696f, 5.67511f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(72000,  player->GetMap(), player->GetPositionX() -20.03f, player->GetPositionY() + 14.7f, player->GetPositionZ() + 9.696f, 5.67511f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(17213,  player->GetMap(), player->GetPositionX() + 22.876f, player->GetPositionY() + 16.62f, player->GetPositionZ() + 21.502f, 3.11078f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(17213,  player->GetMap(), player->GetPositionX() + 42.033f, player->GetPositionY() -3.84f, player->GetPositionZ() + 20.465f, 3.08564f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(70006,  player->GetMap(), player->GetPositionX() -21.665f, player->GetPositionY() -21.1f, player->GetPositionZ() + 9.715f, 1.04518f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(29532,  player->GetMap(), player->GetPositionX() -47.917f, player->GetPositionY() -11.9f, player->GetPositionZ() + 9.267f,  0.480649f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(28676,  player->GetMap(), player->GetPositionX() -32.015f, player->GetPositionY() -4.53f, player->GetPositionZ() -5.253f, 0.127222f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(45007,  player->GetMap(), player->GetPositionX() + 52.722f, player->GetPositionY() + 21.64f, player->GetPositionZ() -5.137f, 4.09115f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(45010,  player->GetMap(), player->GetPositionX() -27.727f, player->GetPositionY() + 10.95f, player->GetPositionZ() + 9.71f, 5.52981f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawni, selectabled) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(17213,  player->GetMap(), player->GetPositionX() -1.552f, player->GetPositionY() + 15.66f, player->GetPositionZ() + 9.686f, 3.42571f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(9856,  player->GetMap(), player->GetPositionX() -50.691f, player->GetPositionY() -1.65f, player->GetPositionZ() + 9.249f, 0.162566f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(45002,  player->GetMap(), player->GetPositionX() -44.472f, player->GetPositionY() +  4.46f, player->GetPositionZ() + 9.341f, 4.90245f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(45011,  player->GetMap(), player->GetPositionX() -18.046f, player->GetPositionY() -23.18f, player->GetPositionZ() + 9.707f, 1.04911f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(45012,  player->GetMap(), player->GetPositionX() + 57.398f, player->GetPositionY() + 0.33f, player->GetPositionZ() -5.139f, 2.75343f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

				//create NPC's and write each to the database
				ret = this->SpawnNPC(45001,  player->GetMap(), player->GetPositionX() -46.818f, player->GetPositionY() + 3.76f, player->GetPositionZ() + 9.308f, 5.07916f,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,1)",houseid,ret);

			}
			break;
		}
		//Reload Playerhousing and access lists
		LoadPlayerHousing();
	}

bool uowToolSet::HasHouse(Player * player)
{
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
		{
			if( itr == PlayerHousing.end() )
				return false;
			if(itr->second->accountid==player->GetSession()->GetAccountId() && itr->second->type<100)
				return true;
		}
	return false;
}
bool uowToolSet::CanOpenHouseDoor(Player *player, GameObject *gameobject)
{
	bool open = false;
	//spin houses
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return open;
		//spin objects (doors)
		for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
			{
				if(itr2->second->gospawnid == gameobject->GetDBTableGUIDLow())
				{
					//Owner Check
					if(itr->second->accountid == player->GetSession()->GetAccountId())
						open = true;
					//Public House Check
					if(itr->second->ispublic)
						open = true;
					//Guild Use & Memeber check
					if(itr->second->isforguild &&itr->second->guildid!=0)
						if(itr->second->guildid==player->GetGuildId())
							open = true;
					//Access List Check
					for(std::map<uint32, uint32>::iterator itr3 =  itr->second->accesslist.begin(); itr3 != itr->second->accesslist.end(); ++itr3)
					{
						if(itr3->second==player->GetGUIDLow())
							open = true;
					}
				}
			}
	}
	/*/Handle Door Open
	if(open)
		ChatHandler(player).PSendSysMessage("You have been granted access to this object.");
	else
		ChatHandler(player).PSendSysMessage("You are not allowed to use this door at this time.");
		*/
	return open;
}

bool uowToolSet::DeleteNPC(Map* map, uint32 lowguid,bool RemoveFromDB)
{
	Creature* unit = NULL;
    if (CreatureData const* cr_data = objmgr.GetCreatureData(lowguid))
		unit = map->GetCreature(MAKE_NEW_GUID(lowguid, cr_data->id, HIGHGUID_UNIT));
    if(!unit || unit->isPet() || unit->isTotem())
        return false;
    unit->CombatStop();
	if(RemoveFromDB)
	{
		CharacterDatabase.DirectPExecute("delete from uow_player_housing_npc where npcspawnid=%u", unit->GetDBTableGUIDLow());			
		unit->DeleteFromDB();
	}
	unit->RemoveFromWorld();
	if(!unit->isSummon())
		unit->AddObjectToRemoveList();
    return true;
}
bool uowToolSet::DeleteGO(Map* map , uint32 lowguid,bool RemoveFromDB)
{
    GameObject* obj = NULL;
    if (GameObjectData const* go_data = objmgr.GetGOData(lowguid))
		obj = map->GetGameObject(MAKE_NEW_GUID(lowguid, go_data->id, HIGHGUID_GAMEOBJECT));
    if(!obj)
        return false;
	obj->RemoveFromWorld();
    obj->SetRespawnTime(0);                               
    obj->Delete();
	if(RemoveFromDB)
	{
		CharacterDatabase.DirectPExecute("delete from uow_player_housing_objects where gospawnid=%u", obj->GetDBTableGUIDLow());					
		obj->DeleteFromDB();
	}
    return true;
}
PlayerHousingData* uowToolSet::GetHouse(Player * player)
{
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
		{
			if( itr == PlayerHousing.end() )
				return NULL;
			if(itr->second->accountid==player->GetSession()->GetAccountId())
				return itr->second;
		}
	return NULL;
}

PlayerHousingData* uowToolSet::GetGuildCity(Player * player)
{
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
		{
			if( itr == PlayerHousing.end() )
				return NULL;
			if(itr->second->accountid==player->GetSession()->GetAccountId() && itr->second->type==100)
				return itr->second;
		}
	return NULL;
}
PlayerHousingData* uowToolSet::GetGuildCity(uint32 houseid)
{
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
		{
			if( itr == PlayerHousing.end() )
				return NULL;
			if(itr->second->id==houseid && itr->second->type==100)
				return itr->second;
		}
	return NULL;
}
PlayerHousingData* uowToolSet::GetGuildCityByGuildID(uint32 guildid)
{
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
		{
			if( itr == PlayerHousing.end() )
				return NULL;
			if(itr->second->guildid==guildid && itr->second->type==100)
				return itr->second;
		}
	return NULL;
}
PlayerHousingData* uowToolSet::GetGuildCityByGO(uint32 gospawnid)
{
	//spin and remove building
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return NULL;
		if (itr->second->type==100)
		{
			//Got a city , lets check out the objects
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
			{
				if(itr2->second->gospawnid==gospawnid)
				{
					return itr->second;
				}

			}
		}
	}
	return NULL;
}
void uowToolSet::ToggleHousePublic(Player * player, bool GuildCity)
{
	PlayerHousingData* ph = GetHouse(player);
	ph->ispublic = !ph->ispublic;
	if (GuildCity)
		CharacterDatabase.DirectPExecute("UPDATE uow_player_housing set ispublic = %u where id = %u and type = 100", ph->ispublic, ph->id);
	else
		CharacterDatabase.DirectPExecute("UPDATE uow_player_housing set ispublic = %u where id = %u", ph->ispublic, ph->id);
	LoadPlayerHousing();
}
void uowToolSet::ToggleHouseForGuild(Player * player)
{
	PlayerHousingData* ph = GetHouse(player);
	ph->isforguild = !ph->isforguild;
	CharacterDatabase.DirectPExecute("UPDATE uow_player_housing set isforguild = %u , guildid = %u where id = %u", ph->isforguild, player->GetGuildId(), ph->id);
	LoadPlayerHousing();
}
void uowToolSet::UseGate(Player * player, uint32 GateGUID)
{
	if(player->isInCombat())
	{
		ChatHandler(player).PSendSysMessage("You may not do this whilst in combat.");
		return;
	}
	MarkCollection::const_iterator itr;
    for(itr = ActiveGates.begin(); itr != ActiveGates.end(); ++itr)
    {
      if(itr->second->id == GateGUID && CanRecall(player,itr->second,0,0))
	  {
	   	DismissHelper(player); //Remove ze panda!
		player->SendPlaySound(7756,false);
		player->TeleportTo(itr->second->mapid,itr->second->x,itr->second->y,itr->second->z,itr->second->o,0);
		player->SendPlaySound(7756,false);		
        break;
	  }
    }
}

bool uowToolSet::CanRecall(Player * player, Mark * mark , uint32 item, uint32 quantity)
{
	if(player->isGameMaster())
		return true;
	if (item > 0)
	{
		bool canrecall = false;
		//CD vs Consumption of item
		if(!HasCoolDown(CD_RECALL,player))
			canrecall=true;
		if(!canrecall && HasMats(player,item,quantity))
			canrecall=true;
		if(!canrecall)
		{
			std::ostringstream msg2;
			msg2 << MSG_COLOR_MAGENTA << "You may not gate for 15 minutes after a previous recall. It costs [" << quantity << "] WSG Mark(s) of honor to skip this check.";
			ChatHandler(player).PSendSysMessage(msg2.str( ).c_str( ));
			player->SendPlaySound(42,true);
			return false;
		}
	}


	if(player->isInCombat())
	{
		ChatHandler(player).PSendSysMessage("You may not do this whilst in combat.");
		return false;
	}
	//Check if the spots valid to recall into
	Map *map = MapManager::Instance().FindMap(mark->mapid);
	if (!map)
	{
		ChatHandler(player).PSendSysMessage("You may not recall to this area, Page a GM, this should not happen.");
		return false;
	}
	//Off limit Zones (ZoneID)
	if (map->GetZoneId(mark->x,mark->y,mark->z)==616)
	{
		ChatHandler(player).PSendSysMessage("You may not recall to this area.");
		return false;
	}
	GameObject * ret=SpawnTempGO(185935,map,mark->x,mark->y,mark->z,mark->o,10,player->GetGUID());
	if(!ret)
	{
		ChatHandler(player).PSendSysMessage("You may not recall to this area, Page a GM, this should not happen.");
		return false;
	}
	if (ret->FindNearestGameObject(NOMARK_MARKER,1024))
	{
		ChatHandler(player).PSendSysMessage("There is a Guild City or No Mark flag at this recall location. As such, DENIED!");
		player->SendPlaySound(42,true);
		return false;
	}
	//Cannot mark or recall near shield bubbles
	if (ret->FindNearestGameObject(HOUSE_SHIELD,2048))
	{
		ChatHandler(player).PSendSysMessage("There is a Player House at this recall location. As such, DENIED!");
		player->SendPlaySound(42,true);
		return false;
	}

	return true;
}
void uowToolSet::MakeGate(Player * player)
{
	if(!player->m_currentmark)
		return;

	if(player->isInCombat())
	{
		ChatHandler(player).PSendSysMessage("You may not do this whilst in combat.");
		return;
	}
	
	bool canrecall = false;
	if(!HasCoolDown(CD_RECALL,player))
		canrecall=true;
	if(!canrecall && HasMats(player,UOW_WSGMARK,3))
		canrecall=true;

	if(canrecall)
	{	
		Map *map = MapManager::Instance().FindMap(player->m_currentmark->mapid);
		if (!map)
			map = MapManager::Instance().CreateMap(player->m_currentmark->mapid, player, 0);
		if (!map)
		{
			ChatHandler(player).PSendSysMessage("You may not open a gate to this area, Page a GM, this should not happen.");
			return;
		}
		if(HasCoolDown(CD_RECALL,player))
		{
			player->DestroyItemCount(UOW_WSGMARK,3,true,false);
			ChatHandler(player).PSendSysMessage("3 WSG Mark of Honor has been consumed whilst casting this spell.");
		}
		else
		{
			AddPlayerCooldown(CD_RECALL,900,player);
		}
		uowTools.DismissHelper(player); //Remove ze panda!
		player->SendPlaySound(12326,false);
		GameObject * ret;
		ret=SpawnTempGO(211024,player->GetMap(),player->GetPositionX(),player->GetPositionY(),player->GetPositionZ(),0,60,player->GetGUID());
		if(ret)
		{
			Mark * dest = new Mark();
			dest->mapid =map->GetId();
			dest->x = player->m_currentmark->x;
			dest->y = player->m_currentmark->y;
			dest->z = player->m_currentmark->z;
			dest->o = 0;
			dest->name="GATE";
			dest->id = ret->GetGUID();
			ActiveGates.insert(GateCollection::value_type(dest->id,dest));
		}
		ret=SpawnTempGO(211024,map,player->m_currentmark->x,player->m_currentmark->y,player->m_currentmark->z,player->m_currentmark->o,60,player->GetGUID());
		if(ret)
		{
			Mark * dest = new Mark();
			dest->mapid = player->GetMap()->GetId();
			dest->x = player->GetPositionX();
			dest->y = player->GetPositionY();
			dest->z = player->GetPositionZ();
			dest->o=0;
			dest->name="GATE";
			dest->id = ret->GetGUID();
			ActiveGates.insert(GateCollection::value_type(dest->id,dest));
		}
	}
	else
	{
		ChatHandler(player).PSendSysMessage("You may not gate for 15 minutes after a previous recall. It costs [3] WSG Mark of honor to skip this check.");
	}
}
void uowToolSet::Recall(Player * player, bool bypassrules)
{
	
	if(!player->m_currentmark)
		return;

	if(bypassrules)
	{
		uowTools.DismissHelper(player); //Remove ze panda!
		player->SendPlaySound(7756,false);
		player->TeleportTo(player->m_currentmark->mapid,player->m_currentmark->x,player->m_currentmark->y,player->m_currentmark->z,player->m_currentmark->o,0);
		player->SendPlaySound(7756,false);	
		return;
	}

	if(CanRecall(player,player->m_currentmark,0,0))
	{		
		uowTools.DismissHelper(player); //Remove ze panda!
		player->SendPlaySound(7756,false);
		player->TeleportTo(player->m_currentmark->mapid,player->m_currentmark->x,player->m_currentmark->y,player->m_currentmark->z,player->m_currentmark->o,0);
		player->SendPlaySound(7756,false);		
		/*
		if(HasCoolDown(CD_RECALL,player))
		{
			player->DestroyItemCount(UOW_WSGMARK,1,true,false);
			ChatHandler(player).PSendSysMessage("1 WSG Mark of Honor has been consumed whilst casting this spell.");
		}
		else
		{
			AddPlayerCooldown(CD_RECALL,900,player);
		}*/
	}

}

void uowToolSet::UpdatePlayer(Player *player)
{
	UpdatePlayerCoolDowns(player);
	UpdatePlayerVehicle(player);
}

void uowToolSet::UpdatePlayerVehicle(Player *player)
{
	if (player->GetZoneId()==616) //hyjal
		return;
	 //if (player->GetVehicle() && (!GetNearestGOOwnedByOther(player) || !GetNearestOwnedGO(player,false)))
	 //if(player->GetVehicle() && GetGuildCityFlagDistance())
	 if(player->GetVehicle() && !player->FindNearestGameObject(VEHICLEAREA_MARKER,8192) && !GetNearestGOOwnedByOther(player) && !GetNearestOwnedGO(player,false) && !player->FindNearestGameObject(GUILDCITY_BANE,8192))
	 {
		Vehicle * veh = player->GetVehicle();
		veh->RemoveAllPassengers();
		veh->Dismiss();	
		ChatHandler(player).PSendSysMessage("You are to far from the Vehicle control flag and this vehicle is no longer valid.");		
	 }
}

void uowToolSet::UpdatePlayerCoolDowns(Player *player)
{
	if (player->realPrevTime == 0 )
		player->realPrevTime = getMSTime();
    uint32 realCurrTime = getMSTime();
    uint32 diff = getMSTimeDiff(player->realPrevTime,realCurrTime);
	player->realPrevTime = getMSTime();
	CoolDownCollection::const_iterator itr;
	if( player->m_cooldowns.empty())
		return;
    for(itr = player->m_cooldowns.begin(); itr != player->m_cooldowns.end(); ++itr)
    {
		//Update all timers
		if (itr->second->GetCurrent()>=0)
			itr->second->Update(diff);
		else 
			itr->second->SetCurrent(0);

		//Clear Cooldown
		if(itr->second->Passed())
		{
			IntervalTimer* t = itr->second;
			delete t;
			player->m_cooldowns.erase(itr);
			return;
	    }
	}
}
bool uowToolSet::HasCoolDown(uint32 CoolDownID, Player * player)
{
	CoolDownCollection::const_iterator itr =  player->m_cooldowns.find(CoolDownID);
	if( itr ==  player->m_cooldowns.end() )
		return false;
	else
		return true;
}

void uowToolSet::AddPlayerCooldown(uint32 CoolDownID, uint32 seconds, Player * player)
{
	IntervalTimer* nt = new IntervalTimer;
	nt->SetInterval(seconds*IN_MILISECONDS);
	nt->SetCurrent(0);
	player->m_cooldowns.insert(CoolDownCollection::value_type(CoolDownID,nt));
}
void uowToolSet::LoadPlayerCooldowns(Player * player)
{
	player->realPrevTime = 0 ;
    QueryResult *result = CharacterDatabase.PQuery("SELECT cooldownid, elapsedseconds + TIMESTAMPDIFF(SECOND,stamp,NOW()) as elapsedseconds from uow_playercooldowns where playerguid = %u or accountguid=%u ",player->GetGUIDLow(),player->GetSession()->GetAccountId());
    if(!result)
        return;
    do
    {
        Field *fields = result->Fetch();
		switch(fields[0].GetUInt32())
		{
		case CD_SHRINES:
			{
				int et = 14400 - fields[1].GetUInt32();
				if(et > 0)
					AddPlayerCooldown(CD_SHRINES,et,player);
			}
			break;
		case CD_SIEGESUMMON:
			{
				int et = 300 - fields[1].GetUInt32();
				if(et > 0)
					AddPlayerCooldown(CD_SIEGESUMMON,et,player);
			}
			break;
		case CD_PVPDEATH:
			{
				int et = 300 - fields[1].GetUInt32();
				if(et > 0)
					AddPlayerCooldown(CD_PVPDEATH,et,player);
			}
			break;
		case CD_RECALL:
			{
				int et = 900 - fields[1].GetUInt32();
				if(et > 0)
					AddPlayerCooldown(CD_RECALL,et,player);
			}
			break;
		case CD_WSGMOBTOKEN:
			{
				int et = 120 - fields[1].GetUInt32();
				if(et > 0)
					AddPlayerCooldown(CD_WSGMOBTOKEN,et,player);
			}
			break;
		case CD_ADKILL:
			{
				int et = 300 - fields[1].GetUInt32();
				if(et > 0)
					AddPlayerCooldown(CD_ADKILL,et,player);
			}
			break;
		case CD_CRIMINAL:
			{
				int et = 120 - fields[1].GetUInt32();
				if(et > 0)
					AddPlayerCooldown(CD_CRIMINAL,et,player);
			}
			break;
		case CD_GUILDCITYREWARDS:
			{
				int et = 86400 - fields[1].GetUInt32();
				if(et > 0)
					AddPlayerCooldown(CD_GUILDCITYREWARDS,et,player);
			}
			break;
		case CD_BANEWINDOW:
			{
				int et = 259200 - fields[1].GetUInt32();
				if(et > 0)
					AddPlayerCooldown(CD_BANEWINDOW,et,player);
			}
			break;
		case CD_WHO:
			{
				int et = 300 - fields[1].GetUInt32();
				if(et > 0)
					AddPlayerCooldown(CD_WHO,et,player);
			}
			break;
		}
		
    } while (result->NextRow());
    delete result;
}
void uowToolSet::SavePlayerCooldowns(Player * player)
{
	CharacterDatabase.PExecute("DELETE FROM uow_playercooldowns Where playerguid=%u or accountguid=%u", player->GetGUIDLow(),player->GetSession()->GetAccountId());
	for (CoolDownCollection::iterator itr =  player->m_cooldowns.begin(); itr !=   player->m_cooldowns.end(); ++itr)
	{
		if( itr ==  player->m_cooldowns.end() )
			return;
		uint32 et;
		switch(itr->first)
		{
		case CD_SHRINES:
			et = (itr->second->GetCurrent() / 1000) + (14400-(itr->second->GetInterval() / 1000));
			break;
		case CD_SIEGESUMMON:
			et = (itr->second->GetCurrent() / 1000) + (300-(itr->second->GetInterval() / 1000));
			break;
		case CD_PVPDEATH:
			et = (itr->second->GetCurrent() / 1000) + (300-(itr->second->GetInterval() / 1000));
			break;
		case CD_RECALL:
			et = (itr->second->GetCurrent() / 1000) + (900-(itr->second->GetInterval() / 1000));
			break;
		case CD_WSGMOBTOKEN:
			et = (itr->second->GetCurrent() / 1000) + (120-(itr->second->GetInterval() / 1000));
			break;
		case CD_ADKILL:
			et = (itr->second->GetCurrent() / 1000) + (300-(itr->second->GetInterval() / 1000));
			break;
		case CD_CRIMINAL:
			et = (itr->second->GetCurrent() / 1000) + (120-(itr->second->GetInterval() / 1000));
			break;
		case CD_GUILDCITYREWARDS:
			et = (itr->second->GetCurrent() / 1000) + (86400-(itr->second->GetInterval() / 1000));
			break;
		case CD_BANEWINDOW:
			et = (itr->second->GetCurrent() / 1000) + (259200-(itr->second->GetInterval() / 1000));
			break;
		case CD_WHO:
			et = (itr->second->GetCurrent() / 1000) + (300-(itr->second->GetInterval() / 1000));
			break;
		}
		switch(itr->first)
		{
			case CD_BANEWINDOW:
			case CD_GUILDCITYREWARDS:
				CharacterDatabase.PExecute("INSERT INTO uow_playercooldowns (accountguid, cooldownid, elapsedseconds, stamp) VALUES (%u,%u,%u,NOW())",player->GetSession()->GetAccountId(),itr->first,et);
				break;
			default:
				CharacterDatabase.PExecute("INSERT INTO uow_playercooldowns (playerguid, cooldownid, elapsedseconds, stamp) VALUES (%u,%u,%u,NOW())", player->GetGUIDLow(),itr->first,et);
				break;
		}
		
	}
}

void uowToolSet::DemolishHome(Player * Owner, bool GuildCity)
{
	PlayerHousingData * ph;
	if (!GuildCity)
	{
		if(!HasHouse(Owner))
			return;
		ph = GetHouse(Owner);
	}
	else
	{
		if(!GetGuildCity(Owner))
			return;
		ph = GetGuildCity(Owner);
	}
	//spin and remove objects
	for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  ph->objectlist.begin(); itr2 != ph->objectlist.end(); ++itr2)
		DeleteGO(Owner->GetMap(),itr2->second->gospawnid,true);
	//spin and remove NPC
	for(std::map<uint32, PlayerHousingNPCS*>::iterator itr2 =  ph->npclist.begin(); itr2 != ph->npclist.end(); ++itr2)
		DeleteNPC(Owner->GetMap(),itr2->second->npcspawnid,true);
	//Clear House Entry
	CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing where id =%u",ph->id);
	//Clear GO's
	CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing_objects where houseid =%u",ph->id);
	//Clear NPC's
	CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing_npc where houseid =%u",ph->id);
	//Clear Acces List
	CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing_access where houseid =%u",ph->id);		

	LoadPlayerHousing();
}
void uowToolSet::DemolishGuildCity(uint32 houseid, Map *map)
{
	PlayerHousingData * ph;
	ph=GetGuildCity(houseid);
	if(!ph)
		return;
		//spin and remove objects
	for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  ph->objectlist.begin(); itr2 != ph->objectlist.end(); ++itr2)
		DeleteGO(map,itr2->second->gospawnid,true);
	//spin and remove NPC
	for(std::map<uint32, PlayerHousingNPCS*>::iterator itr2 =  ph->npclist.begin(); itr2 != ph->npclist.end(); ++itr2)
		DeleteNPC(map,itr2->second->npcspawnid,true);
	//Clear House Entry
	CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing where id =%u",ph->id);
	//Clear GO's
	CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing_objects where houseid =%u",ph->id);
	//Clear NPC's
	CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing_npc where houseid =%u",ph->id);
	//Clear Acces List
	CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing_access where houseid =%u",ph->id);		

	LoadPlayerHousing();
}
void uowToolSet::RepairBuildings(Player * Owner)
{
	PlayerHousingData * ph;
	if(!GetGuildCity(Owner))
			return;
	ph = GetGuildCity(Owner);
	if(HasActiveBane(ph->id))
	{
		ChatHandler(Owner).PSendSysMessage("You may not repair while under siege.");
		return;
	}
	//spin and rebuild objects
	for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  ph->objectlist.begin(); itr2 != ph->objectlist.end(); ++itr2)
	{
		GameObject * tgo=NULL;
		if (GameObjectData const* go_data = objmgr.GetGOData(itr2->second->gospawnid))
			tgo = Owner->GetMap()->GetGameObject(MAKE_NEW_GUID(itr2->second->gospawnid, go_data->id , HIGHGUID_GAMEOBJECT));
		if(tgo)
			tgo->Rebuild();
	}
 
}
void uowToolSet::RemoveBuilding(GameObject * go)
{
	//spin and remove building
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return;
		if (itr->second->type==100)
		{
			//Got a city , lets check out the objects
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
			{
				if(itr2->second->gospawnid==go->GetDBTableGUIDLow())
				{
					
					DeleteGO(go->GetMap(),itr2->second->gospawnid,true);					
				}

			}
		}
	}
	LoadPlayerHousing();
}
bool uowToolSet::OwnsBuilding(Player* player, uint32 gospawnid)
{
	//spin and remove building
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return false;
		if (itr->second->type==100 && itr->second->accountid == player->GetSession()->GetAccountId())
		{
			//Got a city , lets check out the objects
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
			{
				if(itr2->second->gospawnid==gospawnid)
					return true;
			}
		}
	}
	return false;
}
void uowToolSet::UseHouseTeleporter(Player *player, GameObject *gameobject)
{
	//spin houses
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return;
		//spin objects (doors)
		for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
			{
				if(itr2->second->gospawnid == gameobject->GetDBTableGUIDLow())
				{
					uowTools.DismissHelper(player); //Remove ze panda!
					player->SendPlaySound(7756,false);
					player->TeleportTo(player->GetMapId(),itr2->second->tx,itr2->second->ty,itr2->second->tz,player->GetOrientation(),0);
					player->SendPlaySound(7756,false);
				}
			}
	}
}

void uowToolSet::AddToHouseList(Player * Owner)
{
	if(!Owner->GetSelectedPlayer())
	{
		ChatHandler(Owner).PSendSysMessage("No target selected.");
		return;
	}
	uint32 tid = Owner->GetSelectedPlayer()->GetGUIDLow();
	uint32 hid = GetHouse(Owner)->id;
	CharacterDatabase.PExecute("DELETE FROM uow_player_housing_access where houseid=%u and characterid=%u",GetHouse(Owner)->id,Owner->GetGUIDLow());
	CharacterDatabase.PExecute("INSERT INTO uow_player_housing_access (houseid,characterid) VALUES(%u,%u)",GetHouse(Owner)->id,Owner->GetGUIDLow());
	GetHouse(Owner)->accesslist.insert(std::map<uint32, uint32>::value_type(hid,tid));
	ChatHandler(Owner).PSendSysMessage("Player added to your list.");
	ChatHandler(Owner->GetSelectedPlayer()).PSendSysMessage("You have been granted access to this house.");
}
void uowToolSet::RemoveFromHouseList(Player * Owner)
{
	if(!Owner->GetSelectedPlayer())
	{
		ChatHandler(Owner).PSendSysMessage("No target selected.");
		return;
	}
	uint32 tid = Owner->GetSelectedPlayer()->GetGUIDLow();
	uint32 hid = GetHouse(Owner)->id;
	CharacterDatabase.PExecute("DELETE FROM uow_player_housing_access where houseid=%u and characterid=%u",hid,tid);
	for (std::map<uint32, uint32>::iterator itr =  GetHouse(Owner)->accesslist.begin(); itr !=  GetHouse(Owner)->accesslist.end(); ++itr)
	{
		if( itr == GetHouse(Owner)->accesslist.end() )
			return;
		if( itr->second==tid && itr->first == hid)
		{
			GetHouse(Owner)->accesslist.erase(itr);
			ChatHandler(Owner).PSendSysMessage("Player removed from your list.");
			ChatHandler(Owner->GetSelectedPlayer()).PSendSysMessage("You have been removed from the access list of this house.");
			return;
		}
	}
}

void uowToolSet::ChangeOwner(Player * Owner, bool ishouse)
{
	Player * NewOwner = Owner->GetSelectedPlayer();
	if(!NewOwner)
	{
		ChatHandler(Owner).PSendSysMessage("No target selected.");
		return;
	}

	//See if the new owner can have a house
	if(HasHouse(NewOwner) && ishouse)
	{
		ChatHandler(Owner).PSendSysMessage("Target already owns a home.");
		return;
	}
	else if(GetGuildCity(NewOwner))
	{
		ChatHandler(Owner).PSendSysMessage("Target already owns a guild city.");
		return;
	}
	if(ishouse)
	{
		//Clear Acces List
		CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing_access where houseid =%u",GetHouse(Owner)->id);
		//Set new owner
		CharacterDatabase.DirectPExecute("UPDATE uow_player_housing set  isforguild=0, ispublic=0, accountid=%u where id=%u",NewOwner->GetSession()->GetAccountId(),GetHouse(Owner)->id);
		ChatHandler(Owner).PSendSysMessage("%s is now the owner of this house.", NewOwner->GetName());
		ChatHandler(NewOwner).PSendSysMessage("You are now the owner of this house.");
	}
	else
	{
		//Clear Acces List
		CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing_access where houseid =%u",GetGuildCity(Owner)->id);
		//Set new owner
		CharacterDatabase.DirectPExecute("UPDATE uow_player_housing set  isforguild=0, ispublic=0, accountid=%u where id=%u",NewOwner->GetSession()->GetAccountId(),GetGuildCity(Owner)->id);
		ChatHandler(Owner).PSendSysMessage("%s is now the owner of this house.", NewOwner->GetName());
		ChatHandler(NewOwner).PSendSysMessage("You are now the owner of this guild city.");
	}
	LoadPlayerHousing();
}

int32 uowToolSet::GetTokenCount(Player * player)
{
	QueryResult *result = CharacterDatabase.PQuery("select sum(votestoprocess) from uow_votes,realmd.account where realmd.account.username = name and realmd.account.id=%u",player->GetSession()->GetAccountId());
    if(!result)
        return 0;
	Field *fields = result->Fetch();
	int32 total = fields[0].GetInt32();
	delete result;
    return total;
}
void uowToolSet::GetToken(Player * player)
{
	//Get Token Data
	QueryResult *result = CharacterDatabase.PQuery("select sum(votestoprocess),name from uow_votes,realmd.account where realmd.account.username = name and realmd.account.id=%u group by name",player->GetSession()->GetAccountId());
    if(!result)
        return;
	Field *fields = result->Fetch();
	int32 total = fields[0].GetInt32();
 	const char *  name = fields[1].GetString();
	delete result;


	//dish out token
	if(total>0)
	{
		QueryResult *result = CharacterDatabase.PQuery("select name, votetype from uow_votes,realmd.account where realmd.account.username = name and realmd.account.id=%u and votestoprocess>0",player->GetSession()->GetAccountId());
		if(!result)
			return;
		Field *fields = result->Fetch();
		name = fields[0].GetString();
		int32 vt = fields[1].GetInt32();
		GiveItems(player,44990,1);
		CharacterDatabase.DirectPExecute("update uow_votes set votestoprocess=votestoprocess-1 where name ='%s' and votetype=%i",name,vt);
		ChatHandler(player).PSendSysMessage("You have been granted 1 reward token.");
		delete result;
		//CharacterDatabase.DirectExecute("delete from uow_votes where votestoprocess<1");
	}
	
    
}
void uowToolSet::GiveAllGlyphs(Player * player)
{
	if (HasMats(player,INSCRIPTION_RESEARCH_BAG,1))
	{
		player->learnSpell(56944,false);
		player->learnSpell(56946,false);
		player->learnSpell(56947,false);
		player->learnSpell(56949,false);
		player->learnSpell(56950,false);
		player->learnSpell(56954,false);
		player->learnSpell(56958,false);
		player->learnSpell(56960,false);
		player->learnSpell(56975,false);
		player->learnSpell(56977,false);
		player->learnSpell(56983,false);
		player->learnSpell(56986,false);
		player->learnSpell(56988,false);
		player->learnSpell(56989,false);
		player->learnSpell(56996,false);
		player->learnSpell(56998,false);
		player->learnSpell(56999,false);
		player->learnSpell(57010,false);
		player->learnSpell(57011,false);
		player->learnSpell(57012,false);
		player->learnSpell(57013,false);
		player->learnSpell(57014,false);
		player->learnSpell(57019,false);
		player->learnSpell(57021,false);
		player->learnSpell(57028,false);
		player->learnSpell(57034,false);
		player->learnSpell(57035,false);
		player->learnSpell(57112,false);
		player->learnSpell(57115,false);
		player->learnSpell(57116,false);
		player->learnSpell(57117,false);
		player->learnSpell(57124,false);
		player->learnSpell(57126,false);
		player->learnSpell(57127,false);
		player->learnSpell(57128,false);
		player->learnSpell(57130,false);
		player->learnSpell(57152,false);
		player->learnSpell(57153,false);
		player->learnSpell(57155,false);
		player->learnSpell(57159,false);
		player->learnSpell(57160,false);
		player->learnSpell(57164,false);
		player->learnSpell(57166,false);
		player->learnSpell(57169,false);
		player->learnSpell(57170,false);
		player->learnSpell(57181,false);
		player->learnSpell(57189,false);
		player->learnSpell(57190,false);
		player->learnSpell(57191,false);
		player->learnSpell(57193,false);
		player->learnSpell(57195,false);
		player->learnSpell(57199,false);
		player->learnSpell(57202,false);
		player->learnSpell(57207,false);
		player->learnSpell(57208,false);
		player->learnSpell(57211,false);
		player->learnSpell(57212,false);
		player->learnSpell(57214,false);
		player->learnSpell(57218,false);
		player->learnSpell(57220,false);
		player->learnSpell(57223,false);
		player->learnSpell(57232,false);
		player->learnSpell(57233,false);
		player->learnSpell(57234,false);
		player->learnSpell(57235,false);
		player->learnSpell(57237,false);
		player->learnSpell(57243,false);
		player->learnSpell(57247,false);
		player->learnSpell(57250,false);
		player->learnSpell(57258,false);
		player->learnSpell(57260,false);
		player->learnSpell(57261,false);
		player->learnSpell(57263,false);
		player->learnSpell(57264,false);
		player->learnSpell(57267,false);
		player->learnSpell(57268,false);
		player->learnSpell(57273,false);
		player->learnSpell(57276,false);
		player->learnSpell(57719,false);
		player->learnSpell(59559,false);
		player->learnSpell(59560,false);
		player->learnSpell(59561,false);
		player->learnSpell(61677,false);
		player->learnSpell(56965,false);
		player->learnSpell(56990,false);
		player->learnSpell(56991,false);
		player->learnSpell(57209,false);
		player->learnSpell(57215,false);
		player->learnSpell(57217,false);
		player->learnSpell(57228,false);
		player->learnSpell(57229,false);
		player->learnSpell(57230,false);
		player->learnSpell(57253,false);
		player->learnSpell(58253,false);
		player->learnSpell(58286,false);
		player->learnSpell(58287,false);
		player->learnSpell(58288,false);
		player->learnSpell(58289,false);
		player->learnSpell(58296,false);
		player->learnSpell(58297,false);
		player->learnSpell(58298,false);
		player->learnSpell(58299,false);
		player->learnSpell(58300,false);
		player->learnSpell(58301,false);
		player->learnSpell(58302,false);
		player->learnSpell(58303,false);
		player->learnSpell(58305,false);
		player->learnSpell(58306,false);
		player->learnSpell(58307,false);
		player->learnSpell(58308,false);
		player->learnSpell(58310,false);
		player->learnSpell(58311,false);
		player->learnSpell(58312,false);
		player->learnSpell(58313,false);
		player->learnSpell(58314,false);
		player->learnSpell(58315,false);
		player->learnSpell(58316,false);
		player->learnSpell(58317,false);
		player->learnSpell(58318,false);
		player->learnSpell(58319,false);
		player->learnSpell(58320,false);
		player->learnSpell(58321,false);
		player->learnSpell(58323,false);
		player->learnSpell(58324,false);
		player->learnSpell(58325,false);
		player->learnSpell(58326,false);
		player->learnSpell(58327,false);
		player->learnSpell(58328,false);
		player->learnSpell(58329,false);
		player->learnSpell(58330,false);
		player->learnSpell(58331,false);
		player->learnSpell(58332,false);
		player->learnSpell(58333,false);
		player->learnSpell(58336,false);
		player->learnSpell(58337,false);
		player->learnSpell(58338,false);
		player->learnSpell(58339,false);
		player->learnSpell(58340,false);
		player->learnSpell(58341,false);
		player->learnSpell(58342,false);
		player->learnSpell(58343,false);
		player->learnSpell(58344,false);
		player->learnSpell(58345,false);
		player->learnSpell(58346,false);
		player->learnSpell(58347,false);
		player->learnSpell(59315,false);
		player->learnSpell(59326,false);
		player->learnSpell(62162,false);
		player->learnSpell(64297,false);
		player->learnSpell(64300,false);
		player->learnSpell(64298,false);
		player->learnSpell(64299,false);
		player->learnSpell(64256,false);
		player->learnSpell(64268,false);
		player->learnSpell(64313,false);
		player->learnSpell(64307,false);
		player->learnSpell(65245,false);
		player->learnSpell(64270,false);
		player->learnSpell(64271,false);
		player->learnSpell(64273,false);
		player->learnSpell(64253,false);
		player->learnSpell(64304,false);
		player->learnSpell(64246,false);
		player->learnSpell(64249,false);
		player->learnSpell(64276,false);
		player->learnSpell(64274,false);
		player->learnSpell(64257,false);
		player->learnSpell(64275,false);
		player->learnSpell(64314,false);
		player->learnSpell(64277,false);
		player->learnSpell(64305,false);
		player->learnSpell(64279,false); 
		player->learnSpell(64278,false);
		player->learnSpell(64254,false);
		player->learnSpell(64251,false);
		player->learnSpell(64308,false);
		player->learnSpell(64280,false);
		player->learnSpell(64281,false);
		player->learnSpell(64283,false);
		player->learnSpell(64309,false);
		player->learnSpell(64282,false);
		player->learnSpell(64303,false);
		player->learnSpell(64315,false);
		player->learnSpell(64284,false);
		player->learnSpell(64285,false);
		player->learnSpell(64286,false); 
		player->learnSpell(64310,false);
		player->learnSpell(64288,false);
		player->learnSpell(64316,false);
		player->learnSpell(64289,false);
		player->learnSpell(64247,false);
		player->learnSpell(64287,false);
		player->learnSpell(64294,false);
		player->learnSpell(64317,false);  
		player->learnSpell(64291,false); 
		player->learnSpell(64248,false);
		player->learnSpell(64318,false); 
		player->learnSpell(64311,false);
		player->learnSpell(64250,false);
		player->learnSpell(64295,false); 
		player->learnSpell(64312,false); 
		player->learnSpell(64252,false);
		player->learnSpell(64296,false);
		player->learnSpell(64302,false); 
		player->learnSpell(64255,false); 
		player->learnSpell(64260,false);      
	}
}
void uowToolSet::GiveAllAlchemy(Player * player)
{
	if (HasMats(player,ALCHEMIST_RESEARCH_FLASK,1))
	{
		player->learnSpell(4508,false);
		player->learnSpell(60354,false);
		player->learnSpell(53895,false);
		player->learnSpell(60365,false);
		player->learnSpell(60365,false);
		player->learnSpell(60357,false);
		player->learnSpell(60356,false);
		player->learnSpell(60366,false);
		player->learnSpell(56519,false);
		player->learnSpell(54220,false);
		player->learnSpell(54221,false);
		player->learnSpell(54222,false);
		player->learnSpell(53904,false);
		player->learnSpell(41458,false);
		player->learnSpell(41500,false);
		player->learnSpell(41501,false);
		player->learnSpell(41502,false);
		player->learnSpell(41503,false);
		player->learnSpell(53777,false);
		player->learnSpell(53776,false);
		player->learnSpell(53781,false);
		player->learnSpell(53782,false);
		player->learnSpell(53775,false);
		player->learnSpell(53774,false);
		player->learnSpell(28590,false);
		player->learnSpell(28587,false);
		player->learnSpell(28588,false);
		player->learnSpell(28591,false);
		player->learnSpell(28589,false);
		player->learnSpell(28586,false);
		player->learnSpell(53773,false);
		player->learnSpell(53779,false);
		player->learnSpell(53780,false);
		player->learnSpell(53783,false);
		player->learnSpell(53784,false);
		player->learnSpell(66659,false);
		player->learnSpell(28585,false);
		player->learnSpell(28583,false);
		player->learnSpell(28584,false);
		player->learnSpell(28582,false);
		player->learnSpell(28580,false);
		player->learnSpell(28581,false);
		player->learnSpell(66658,false);
		player->learnSpell(66662,false);
		player->learnSpell(66664,false);
		player->learnSpell(66660,false);
		player->learnSpell(66663,false);		
	}
}
void uowToolSet::GiveAllCooking(Player * player)
{
	if (HasMats(player,FELS_KITCHEN_COOKBOOK,1))
	{
		player->learnSpell(33277,false);
		player->learnSpell(6412,false);
		player->learnSpell(2795,false);
		player->learnSpell(2542,false);
		player->learnSpell(6416,false);
		player->learnSpell(3371,false);
		player->learnSpell(28267,false);
		player->learnSpell(9513,false);
		player->learnSpell(2543,false);
		player->learnSpell(3370,false);
		player->learnSpell(6417,false);
		player->learnSpell(3372,false);
		player->learnSpell(2547,false);
		player->learnSpell(2549,false);
		player->learnSpell(2548,false);
		player->learnSpell(3377,false);
		player->learnSpell(3373,false);
		player->learnSpell(3398,false);
		player->learnSpell(3376,false);
		player->learnSpell(3399,false);
		player->learnSpell(3400,false);
		player->learnSpell(24801,false);
		player->learnSpell(33279,false);
		player->learnSpell(25659,false);
		player->learnSpell(38868,false);
		player->learnSpell(38867,false);
		player->learnSpell(53056,false);
		player->learnSpell(64054,false);
		player->learnSpell(57421,false);
	}
}
void uowToolSet::GiveAllEnchanting(Player * player)
{
	if (HasMats(player,MAJESTICS_ENCHANTING_ROD,1))
	{
		player->learnSpell(13419,false);
		player->learnSpell(34006,false);
		player->learnSpell(20008,false);
		player->learnSpell(13841,false);
		player->learnSpell(27982,false);
		player->learnSpell(27968,false);
		player->learnSpell(20031,false);
		player->learnSpell(20013,false);
		player->learnSpell(20032,false);
		player->learnSpell(27971,false);
		player->learnSpell(27981,false);
		player->learnSpell(20035,false);
		player->learnSpell(20016,false);
		player->learnSpell(20028,false);
		player->learnSpell(27951,false);
		player->learnSpell(13898,false);
		player->learnSpell(47051,false);
		player->learnSpell(27954,false);
		player->learnSpell(27984,false);
		player->learnSpell(15596,false);
		player->learnSpell(20012,false);
		player->learnSpell(27948,false);
		player->learnSpell(34007,false);
		player->learnSpell(34008,false);
		player->learnSpell(46578,false);
		player->learnSpell(27950,false);
		player->learnSpell(27977,false);
		player->learnSpell(25072,false);
		player->learnSpell(25084,false);
		player->learnSpell(34005,false);
		player->learnSpell(20010,false);
		player->learnSpell(44483,false);
		player->learnSpell(44590,false);
		player->learnSpell(44494,false);
		player->learnSpell(20036,false);
		player->learnSpell(27914,false);
		player->learnSpell(27917,false);
		player->learnSpell(20030,false);
		player->learnSpell(20014,false);
		player->learnSpell(20029,false);
		player->learnSpell(7443,false);
		player->learnSpell(7766,false);
		player->learnSpell(7782,false);
		player->learnSpell(7786,false);
		player->learnSpell(13380,false);
		player->learnSpell(13464,false);
		player->learnSpell(7859,false);
		player->learnSpell(13522,false);
		player->learnSpell(13612,false);
		player->learnSpell(13617,false);
		player->learnSpell(13620,false);
		player->learnSpell(13655,false);
		player->learnSpell(13653,false);
		player->learnSpell(21931,false);
		player->learnSpell(13687,false);
		player->learnSpell(13689,false);
		player->learnSpell(13698,false);
		player->learnSpell(13817,false);
		player->learnSpell(13846,false);
		player->learnSpell(13868,false);
		player->learnSpell(13882,false);
		player->learnSpell(13915,false);
		player->learnSpell(13933,false);
		player->learnSpell(13945,false);
		player->learnSpell(13947,false);
		player->learnSpell(20020,false);
		player->learnSpell(20009,false);
		player->learnSpell(20024,false);
		player->learnSpell(20033,false);
		player->learnSpell(20023,false);
		player->learnSpell(20034,false);
		player->learnSpell(20025,false);
		player->learnSpell(20025,false);
		player->learnSpell(20011,false);
		player->learnSpell(25080,false);
		player->learnSpell(25086,false);
		player->learnSpell(27948,false);
		player->learnSpell(27906,false);
		player->learnSpell(27962,false);
		player->learnSpell(27913,false);
		player->learnSpell(27946,false);
		player->learnSpell(33992,false);
		player->learnSpell(27972,false);
		player->learnSpell(27975,false);
		player->learnSpell(28003,false);
		player->learnSpell(28004,false);
		player->learnSpell(27947,false);
		player->learnSpell(42974,false);
		player->learnSpell(44596,false);
		player->learnSpell(64579,false);
		player->learnSpell(64441,false);
		player->learnSpell(62257,false);
		player->learnSpell(44625,false);
		player->learnSpell(47899,false);
		player->learnSpell(44591,false);
		player->learnSpell(44631,false);
		player->learnSpell(47672,false);
		player->learnSpell(47898,false);
		player->learnSpell(60692,false);
		player->learnSpell(44588,false);
		player->learnSpell(44575,false);
		player->learnSpell(60767,false);
		player->learnSpell(47901,false);
		player->learnSpell(60763,false);
		player->learnSpell(60707,false);
		player->learnSpell(44595,false);
		player->learnSpell(60714,false);
		player->learnSpell(44576,false);
		player->learnSpell(44524,false);
		player->learnSpell(44621,false);
		player->learnSpell(59625,false);
		player->learnSpell(59621,false);
		player->learnSpell(59619,false);
		player->learnSpell(62948,false);
		player->learnSpell(62256,false);
		player->learnSpell(60691,false);
	}
}
void uowToolSet::GiveAllJewelcrafting(Player * player)
{
	if (HasMats(player,BADOMENS_JEWELCRAFTING_GEM,1))
	{
		player->learnSpell(56500,false);
		player->learnSpell(56499,false);
		player->learnSpell(56501,false);
		player->learnSpell(56497,false);
		player->learnSpell(56496,false);
		player->learnSpell(56498,false);
		player->learnSpell(58954,false);
		player->learnSpell(58147,false);
		player->learnSpell(58150,false);
		player->learnSpell(58148,false);
		player->learnSpell(58507,false);
		player->learnSpell(58492,false);
		player->learnSpell(55401,false);
		player->learnSpell(55405,false);
		player->learnSpell(55397,false);
		player->learnSpell(55389,false);
		player->learnSpell(55390,false);
		player->learnSpell(55384,false);
		player->learnSpell(55392,false);
		player->learnSpell(55393,false);
		player->learnSpell(55398,false);
		player->learnSpell(55387,false);
		player->learnSpell(55388,false);
		player->learnSpell(55396,false);
		player->learnSpell(55404,false);
		player->learnSpell(55400,false);
		player->learnSpell(55407,false);
		player->learnSpell(55395,false);
		player->learnSpell(55403,false);
		player->learnSpell(53994,false);
		player->learnSpell(53830,false);
		player->learnSpell(53977,false);
		player->learnSpell(53979,false);
		player->learnSpell(53972,false);
		player->learnSpell(53982,false);
		player->learnSpell(53945,false);
		player->learnSpell(53986,false);
		player->learnSpell(53990,false);
		player->learnSpell(53998,false);
		player->learnSpell(54011,false);
		player->learnSpell(53976,false);
		player->learnSpell(54019,false);
		player->learnSpell(53950,false);
		player->learnSpell(53949,false);
		player->learnSpell(54001,false);
		player->learnSpell(53993,false);
		player->learnSpell(53980,false);
		player->learnSpell(53965,false);
		player->learnSpell(53974,false);
		player->learnSpell(53970,false);
		player->learnSpell(53975,false);
		player->learnSpell(54006,false);
		player->learnSpell(53996,false);
		player->learnSpell(54009,false);
		player->learnSpell(53981,false);
		player->learnSpell(53983,false);
		player->learnSpell(53954,false);
		player->learnSpell(54003,false);
		player->learnSpell(53968,false);
		player->learnSpell(53960,false);
		player->learnSpell(54010,false);
		player->learnSpell(53984,false);
		player->learnSpell(53957,false);
		player->learnSpell(53973,false);
		player->learnSpell(53966,false);
		player->learnSpell(53961,false);
		player->learnSpell(54012,false);
		player->learnSpell(53987,false);
		player->learnSpell(53971,false);
		player->learnSpell(54023,false);
		player->learnSpell(53978,false);
		player->learnSpell(53958,false);
		player->learnSpell(53967,false);
		player->learnSpell(53946,false);
		player->learnSpell(54002,false);
		player->learnSpell(54014,false);
		player->learnSpell(53963,false);
		player->learnSpell(54004,false);
		player->learnSpell(53957,false);
		player->learnSpell(53952,false);
		player->learnSpell(53962,false);
		player->learnSpell(53992,false);
		player->learnSpell(53991,false);
		player->learnSpell(54000,false);
		player->learnSpell(53955,false);
		player->learnSpell(53948,false);
		player->learnSpell(54008,false);
		player->learnSpell(54013,false);
		player->learnSpell(53964,false);
		player->learnSpell(53959,false);
		player->learnSpell(53995,false);
		player->learnSpell(54005,false);
		player->learnSpell(53985,false);
		player->learnSpell(53997,false);
		player->learnSpell(53988,false);
		player->learnSpell(66576,false);
		player->learnSpell(66553,false);
		player->learnSpell(66447,false);
		player->learnSpell(66449,false);
		player->learnSpell(66503,false);
		player->learnSpell(66579,false);
		player->learnSpell(66430,false);
		player->learnSpell(66568,false);
		player->learnSpell(66560,false);
		player->learnSpell(66584,false);
		player->learnSpell(66448,false);
		player->learnSpell(66571,false);
		player->learnSpell(66580,false);
		player->learnSpell(66338,false);
		player->learnSpell(66442,false);
		player->learnSpell(66572,false);
		player->learnSpell(66583,false);
		player->learnSpell(66453,false);
		player->learnSpell(66434,false);
		player->learnSpell(66451,false);
		player->learnSpell(66578,false);
		player->learnSpell(66575,false);
		player->learnSpell(66555,false);
		player->learnSpell(66561,false);
		player->learnSpell(66564,false);
		player->learnSpell(66567,false);
		player->learnSpell(66440,false);
		player->learnSpell(66431,false);
		player->learnSpell(66439,false);
		player->learnSpell(66585,false);
		player->learnSpell(66566,false);
		player->learnSpell(66500,false);
		player->learnSpell(66435,false);
		player->learnSpell(66562,false);
		player->learnSpell(66505,false);
		player->learnSpell(68253,false);
		player->learnSpell(66444,false);
		player->learnSpell(66569,false);
		player->learnSpell(66450,false);
		player->learnSpell(66573,false);
		player->learnSpell(66563,false);
		player->learnSpell(66556,false);
		player->learnSpell(66506,false);
		player->learnSpell(66441,false);
		player->learnSpell(66574,false);
		player->learnSpell(66559,false);
		player->learnSpell(66586,false);
		player->learnSpell(66582,false);
		player->learnSpell(66501,false);
		player->learnSpell(66558,false);
		player->learnSpell(66446,false);
		player->learnSpell(66433,false);
		player->learnSpell(66443,false);
		player->learnSpell(66557,false);
		player->learnSpell(66437,false);
		player->learnSpell(66502,false);
		player->learnSpell(66497,false);
		player->learnSpell(66554,false);
		player->learnSpell(66498,false);
		player->learnSpell(66581,false);
		player->learnSpell(66587,false);
		player->learnSpell(66428,false);
		player->learnSpell(66499,false);
		player->learnSpell(66452,false);
		player->learnSpell(66436,false);
		player->learnSpell(66438,false);
		player->learnSpell(66565,false);
		player->learnSpell(66504,false);
		player->learnSpell(66432,false);
		player->learnSpell(66445,false);
		player->learnSpell(66570,false);
		player->learnSpell(66429,false);
		player->learnSpell(66577,false);
	}
}
void uowToolSet::CleanUpQueues()
{
	//Clean up the queue
	bool cleanagain = false;
	for (PlayerCollection::iterator itr =  WorldGames.Game_RR.queuedplayers.begin(); itr !=   WorldGames.Game_RR.queuedplayers.end(); ++itr)
	{
		if(itr->second==NULL)
		{
			WorldGames.Game_RR.queuedplayers.erase(itr);
			cleanagain=true;
			break;
		}
	}
	PlayerCollection pc;
	//82nd Air Drop
	for (ADTeamCollection::iterator itr =  WorldGames.Game_AD.teams.begin(); itr !=   WorldGames.Game_AD.teams.end(); ++itr)
	{
		/*/Double grouped?? Clean
		bool forcedel = false;
		for(GroupReference *gitr = itr->second.team->GetFirstMember(); gitr != NULL; gitr = gitr->next())
		{
			Player* player = gitr->getSource();
			PlayerCollection::iterator fitr = pc.find(player->GetGUIDLow());
			if( fitr == pc.end() )
			{
				pc.insert(PlayerCollection::value_type(player->GetGUIDLow(),player));
			}
			else
			{
				forcedel=true;
				player->RemoveFromGroup();
			}
		}

		//Standard Clean
		if(!itr->second.team || (itr->second.team != NULL && itr->second.team->m_disbanded) || itr->second.team->GetMembersCount()>2 )
		{
			WorldGames.Game_AD.teams.erase(itr);
			cleanagain=true;
			break;
		}*/
	}

	if(cleanagain)
		CleanUpQueues();
}
void uowToolSet::PreStart_RedneckRampage()
{
	uint32 c = 0;
	WorldGames.Game_RR.gamestage=1;
	for (PlayerCollection::iterator itr =  WorldGames.Game_RR.queuedplayers.begin(); itr !=   WorldGames.Game_RR.queuedplayers.end(); ++itr)
	{
		RRStartingLocationCollection::iterator sitr =  WorldGames.Game_RR.startinglocations.find(c);
		if( sitr == WorldGames.Game_RR.startinglocations.end() )
		{
			break;
		}
		else
		{
			//Valid starting location add them to this location
			if(itr->second->isAlive() && itr->second->GetSession())
			{
				//Spawn objeccts for this location/player
				//sitr->second.vehicle = SpawnNPC(27881,itr->second->GetMap(),sitr->second.x,sitr->second.y,sitr->second.z,sitr->second.o,true);
				//Teleport the player to the gamezone
				Map *map = MapManager::Instance().FindMap(sitr->second.mapid);
				uowTools.DismissHelper(itr->second); //Remove ze panda!
				itr->second->TeleportTo(sitr->second.mapid,sitr->second.x,sitr->second.y,sitr->second.z,sitr->second.o,0);
				SpawnTempGO(184719,map,sitr->second.x,sitr->second.y,sitr->second.z,sitr->second.o,30,itr->second->GetGUID());	
				SpawnTempNPC(27881,map,sitr->second.x,sitr->second.y,sitr->second.z,sitr->second.o,300,itr->second);
				sitr->second.player = itr->second;

				//Tell the player they are ready
				ChatHandler(itr->second).PSendSysMessage("The game will start in less than 30 seconds!!");
				//clear the queue of this player (marks for deletion)
				itr->second=NULL;
				c++;
			}
		}
		//Max xx players get out
		if (c>=WorldGames.Game_RR.startinglocations.size())
			break;
	}
	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "[Redneck Rampage] Pre-game startup has completed. The game will start in 30 seconds.";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));

	CleanUpQueues();
}
void uowToolSet::Start_RedneckRampage()
{
	WorldGames.Game_RR.gamestage=2;
	
	for (RRStartingLocationCollection::iterator itr =  WorldGames.Game_RR.startinglocations.begin(); itr !=   WorldGames.Game_RR.startinglocations.end(); ++itr)
	{
		if(itr->second.player && itr->second.player->IsInWorld())
		{
			ChatHandler(itr->second.player).PSendSysMessage("LET THE RAMPAGE BEGIN!!");
			itr->second.player->SendPlaySound(10843,false);
		}
	}

}
void uowToolSet::Update_RedneckRampage()
{
	if(WorldGames.Game_RR.gamestage!=2)
		return;
	uint32 pl = WorldGames.Game_RR.startinglocations.size();
	for (RRStartingLocationCollection::iterator itr =  WorldGames.Game_RR.startinglocations.begin(); itr !=   WorldGames.Game_RR.startinglocations.end(); ++itr)
	{
		//Player is not in the vehicle kick'em out
		if(itr->second.player && (!itr->second.player->GetVehicle() || !itr->second.player->IsInWorld() || itr->second.player->isDead()))
		{
			//DeleteNPC(itr->second.player->GetMap(),itr->second.vehicle,true);
			itr->second.player->ModifyArenaPoints(5);
			GiveItems(itr->second.player,20558,1);
			ChatHandler(itr->second.player).PSendSysMessage("You're out of here sucker!");
			itr->second.player->SendPlaySound(10860,false);	 
			itr->second.player->TeleportTo(itr->second.player->GetStartPosition(),0);
			itr->second.player=NULL;
			
		}

		if(itr->second.player == NULL)
			pl--;
	}
	//Winner check
	if(pl<=1)
		Reward_RedneckRampage();
}
void uowToolSet::Reward_RedneckRampage()
{
	WorldGames.Game_RR.gamestage=0; // Game Over
	for (RRStartingLocationCollection::iterator itr =  WorldGames.Game_RR.startinglocations.begin(); itr !=   WorldGames.Game_RR.startinglocations.end(); ++itr)
	{
		//Player is not in the vehicle kick'em out
		if(itr->second.player)
		{
			itr->second.player->GetVehicle()->RemoveAllPassengers();
			ChatHandler(itr->second.player).PSendSysMessage("Cya later champ! Thanks for playing!");
			uowTools.DismissHelper(itr->second.player); //Remove ze panda!
			itr->second.player->TeleportTo(itr->second.player->GetStartPosition(),0);
			std::ostringstream msg;
			msg << MSG_COLOR_GREEN << "[Redneck Rampage] Champion is [" << itr->second.player->GetName() << "]";
			sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
			itr->second.player->ModifyArenaPoints(20);
			GiveItems(itr->second.player,20558,5);
		}
		itr->second.player=NULL;
 
	}
	CleanUpQueues();
}
void uowToolSet::Load_RedneckRampage()
{
	WorldGames.Game_RR.startinglocations.clear();
	WorldGames.Game_RR.gamestage=0;
	WorldGames.Game_RR.secondssincelastgame=0;
	uint32 c=0;
    QueryResult *result = CharacterDatabase.PQuery("SELECT * from uow_rr_startinglocations");
    if(!result)
        return;
    do
    {
        Field *fields = result->Fetch();
		RRGameStartData startloc;
		startloc.id = fields[0].GetUInt32();
		startloc.mapid = fields[1].GetUInt32();
		startloc.x = fields[2].GetFloat();
		startloc.y = fields[3].GetFloat();
		startloc.z = fields[4].GetFloat();
		startloc.o = fields[5].GetFloat();
		startloc.player = NULL;
 
		WorldGames.Game_RR.startinglocations.insert(RRStartingLocationCollection::value_type(c,startloc));
		c++;
    } while (result->NextRow());
    delete result;
}

void uowToolSet::Queue_RedneckRampage(Player * player)
{
	PlayerCollection::iterator itr = WorldGames.Game_RR.queuedplayers.find(player->GetGUIDLow());

	if( itr == WorldGames.Game_RR.queuedplayers.end() )
	{
		WorldGames.Game_RR.queuedplayers.insert(PlayerCollection::value_type(player->GetGUIDLow(),player));
		ChatHandler(player).PSendSysMessage("You have been queued for the next match.");
		std::ostringstream msg;
		msg << MSG_COLOR_GREEN << "[" << player->GetName() << "] has joined the queue for [Redneck Rampage]";
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
	}
	else
	{
		ChatHandler(player).PSendSysMessage("You are already queued for Redneck Rampage.");
	}
}
int32 uowToolSet::GetGuildCityFlagDistance(uint32 houseid, Player * player)
{
	GameObject * go = NULL;
	uint32 distance = 10000;
	//spin houses
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return NULL;
		if (itr->second->id==houseid)
		{
			//Good owner, lets check out the objects
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
				{
					GameObject * tgo=NULL;
					if (GameObjectData const* go_data = objmgr.GetGOData(itr2->second->gospawnid))
						tgo = player->GetMap()->GetGameObject(MAKE_NEW_GUID(itr2->second->gospawnid, go_data->id , HIGHGUID_GAMEOBJECT));
					if(tgo && tgo->GetEntry() == GUILDCITY_MARKER)
						return GetDistance2D(tgo->GetPositionX(),tgo->GetPositionY(),player->GetPositionX(), player->GetPositionY());
				}
		}
	}
	return distance;
}
GameObject * uowToolSet::GetGuildCityFlag(Player * player)
{
	GameObject * go = NULL;
	//spin houses
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return NULL;
		if (itr->second->accountid==player->GetSession()->GetAccountId())
		{
			//Good owner, lets check out the objects
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
				{
					GameObject * tgo=NULL;
					if (GameObjectData const* go_data = objmgr.GetGOData(itr2->second->gospawnid))
						tgo = player->GetMap()->GetGameObject(MAKE_NEW_GUID(itr2->second->gospawnid, go_data->id , HIGHGUID_GAMEOBJECT));
					if(tgo && tgo->GetEntry() == GUILDCITY_MARKER)
						return tgo;
				}
		}
	}
	return NULL;
}
GameObject * uowToolSet::GetNearestOwnedGO(Player *player, bool honorselectable)
{
	GameObject * go = NULL;
	float nearest = 100000;
	//spin houses
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return NULL;
		if (itr->second->accountid==player->GetSession()->GetAccountId())
		{
			//Good owner, lets check out the objects
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
				{
					if(honorselectable && itr2->second->selectable==0)
						continue;
					GameObject * tgo=NULL;
					if (GameObjectData const* go_data = objmgr.GetGOData(itr2->second->gospawnid))
						tgo = player->GetMap()->GetGameObject(MAKE_NEW_GUID(itr2->second->gospawnid, go_data->id , HIGHGUID_GAMEOBJECT));
					if(tgo)
					{
						float dst = tgo->GetDistance2d(player);
						if (dst<nearest)
						{
							nearest=dst;
							go = tgo;
						}
					}
				}
		}
	}
	//Threshold
	if (go && nearest > 100)
		return NULL;
	return go;
}
Creature * uowToolSet::GetNearestOwnedNPC(Player *player, bool honorselectable)
{
	Creature * c = NULL;
	float nearest = 100000;
	//spin houses
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return NULL;
		if (itr->second->accountid==player->GetSession()->GetAccountId())
		{
			//Good owner, lets check out the objects
			for(std::map<uint32, PlayerHousingNPCS*>::iterator itr2 =  itr->second->npclist.begin(); itr2 != itr->second->npclist.end(); ++itr2)
				{
					if(honorselectable && itr2->second->selectable==0)
						continue;
					Creature * cre=NULL;
					if (CreatureData const* c_data = objmgr.GetCreatureData(itr2->second->npcspawnid))
						cre = player->GetMap()->GetCreature(MAKE_NEW_GUID(itr2->second->npcspawnid, c_data->id , HIGHGUID_UNIT));
					if(cre)
					{
						float dst = cre->GetDistance2d(player);
						if (dst<nearest)
						{
							nearest=dst;
							c = cre;
						}
					}
				}
		}
	}
	//Threshold
	if (c && nearest > 100)
		return NULL;
	return c;
}
GameObject * uowToolSet::GetNearestGOOwnedByOther(Player *player)
{
	GameObject * go = NULL;
	float nearest = 100000;
	//spin houses
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return NULL;
		if (itr->second->accountid!=player->GetSession()->GetAccountId())
		{
			//Good owner, lets check out the objects
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
				{
					GameObject * tgo=NULL;
					if (GameObjectData const* go_data = objmgr.GetGOData(itr2->second->gospawnid))
						tgo = player->GetMap()->GetGameObject(MAKE_NEW_GUID(itr2->second->gospawnid, go_data->id , HIGHGUID_GAMEOBJECT));
					if(tgo)
					{
						float dst = tgo->GetDistance2d(player);
						if (dst<nearest)
						{
							nearest=dst;
							go = tgo;
						}
					}
				}
		}
	}
	//Threshold
	if (go && nearest > 100)
		return NULL;
	return go;
}
GameObject * uowToolSet::GetGateHolder(GameObject * gatego)
{
	GameObject * go = NULL;
	float nearest = 10000;
	//spin houses
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return NULL;
		if (itr->second->type==100)
		{
			//Good owner, lets check out the objects
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
				{
					GameObject * tgo=NULL;
					if (GameObjectData const* go_data = objmgr.GetGOData(itr2->second->gospawnid))
						tgo = gatego->GetMap()->GetGameObject(MAKE_NEW_GUID(itr2->second->gospawnid, go_data->id , HIGHGUID_GAMEOBJECT));
					if(tgo && (tgo->GetEntry() == 191797 || tgo->GetEntry() ==  190221))//Wallgate or Tower
					{
						float dst = tgo->GetDistance2d(gatego);
						if (dst<nearest)
						{
							nearest=dst;
							go = tgo;
						}
					}
				}
		}
	}
	//Threshold
	if (go && nearest > 25)
		return NULL;
	return go;
}
PlayerHousingObjects * uowToolSet::GetGuildCityObject(uint32 gospawnid)
{
	//spin and remove building
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return NULL;
		if (itr->second->type==100)
		{
			//Got a city , lets check out the objects
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
			{
				if(itr2->second->gospawnid==gospawnid)
					return itr2->second;
			}
		}
	}
	return NULL;
}


bool uowToolSet::PlaceHousingNPC(Player * player, uint32 entry, int cost, int maxcount, bool isplayerhouse, bool isselectable)
{
	if (player->GetMoney() < cost * 10000)
	{
		player->SendBuyError( BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
		return false;
	}
	PlayerHousingData * gc;
	if(isplayerhouse)
		gc =  GetHouse(player);
	else
		gc =  GetGuildCity(player);

	//Building restrictions
	if(uowTools.GetHousingNPCCount(gc->id,entry)>=maxcount) // Max count on this entry
	{
		std::ostringstream msg;
		msg << "You may not build any more of this type of NPC, max count is: " << maxcount;
		ChatHandler(player).PSendSysMessage(msg.str().c_str());
		return false;

	}
	if(isplayerhouse)
	{//distance check from shield, and shield mus be active to build
		if(!CanBuildHere(player))
		{
			ChatHandler(player).PSendSysMessage("You are not allowed to build in this are or your Housing Shield is not activated. Activate your housing shield in order to place new housing items.");
			return false;
		}
	}
	else
	{
		if(!gc || !GetNearestOwnedGO(player,false))
		{
			ChatHandler(player).PSendSysMessage("You do not own a guild city or you are to far away from your city to build a node.");
			return false;
		}
		if(HasActiveBane(gc->id))
		{
			ChatHandler(player).PSendSysMessage("You may not place npc while under siege.");
			return false;
		}
	}
	player->ModifyMoney(-(cost*10000));
	//create go's and write each to the database
	uint32 ret;
	ret = this->SpawnNPC(entry, player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(),player->GetOrientation(),true);
	if (ret!=0)
	{
		CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_npc (houseid, npcspawnid, selectable) VALUES (%u,%u,%u)",gc->id,ret,(uint32)isselectable);
		LoadPlayerHousing();
		return true;
	}
	return false;
}
bool uowToolSet::PlaceHousingObject(Player * player, uint32 entry, int cost, int maxcount, bool isplayerhouse, bool isselectable)
{
	
	if (player->GetMoney() < cost * 10000)
	{
		player->SendBuyError( BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
		return false;
	}

	PlayerHousingData * gc;
	if(isplayerhouse)
		gc =  GetHouse(player);
	else
		gc =  GetGuildCity(player);
	
	//Building restrictions
	if(uowTools.GetHousingObjectCount(gc->id,entry)>=maxcount) // Max count walls
	{
		std::ostringstream msg;
		msg << "You may not build any more of this type of object, max count is: " << maxcount;
		ChatHandler(player).PSendSysMessage(msg.str().c_str());
		return false;

	}
	if(isplayerhouse)
	{//distance check from shield, and shield mus be active to build
		if(!CanBuildHere(player))
		{
			ChatHandler(player).PSendSysMessage("You are not allowed to build in this are or your Housing Shield is not activated. Activate your housing shield in order to place new housing items.");
			return false;
		}
	}
	else
	{
		if(!gc || !GetNearestOwnedGO(player,false))
		{
			ChatHandler(player).PSendSysMessage("You do not own a guild city or you are to far away from your city to build a node.");
			return false;
		}
		if(HasActiveBane(gc->id))
		{
			ChatHandler(player).PSendSysMessage("You may not place npc while under siege.");
			return false;
		}
	}
	player->m_currentselectedgo=NULL;
	player->ModifyMoney(-(cost*10000));
	//create go's and write each to the database
	uint32 ret;
	ret = this->SpawnGO(entry, player->GetMap(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(),player->GetOrientation(),true);
	if (ret!=0)
	{
		CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid, selectable) VALUES (%u,%u,%u)",gc->id,ret,(uint32)isselectable);
		LoadPlayerHousing();
		GameObject * tgo=NULL;
		if (GameObjectData const* go_data = objmgr.GetGOData(ret))
			tgo = player->GetMap()->GetGameObject(MAKE_NEW_GUID(ret, entry , HIGHGUID_GAMEOBJECT));
		if(tgo)
			player->m_currentselectedgo=tgo;
		else
			player->m_currentselectedgo=NULL;
		return true;
	}
	return false;
}
void uowToolSet::ReorientObject(Player * player, GameObject * go)
{
	if(!go)
		return;
	if(go->GetEntry() == GUILDCITY_MARKER)
		return;
	bool ishouse=true;
	if(GetHouseFromGO(go)->type==100)
		ishouse=false;
	if(PlaceHousingObject(player,go->GetEntry(),0,250,ishouse,true))
	{
		DeleteGO(player->GetMap(),go->GetGUIDLow(),true);
		CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing_objects where gospawnid=%u",go->GetGUIDLow());
		LoadPlayerHousing();
	}
}
void uowToolSet::ReorientNPC(Player * player, Creature * cre)
{
	if(!cre)
		return;
	
	bool ishouse=true;
	if(GetHouseFromNPC(cre)->type==100)
		ishouse=false;
	if(PlaceHousingNPC(player,cre->GetEntry(),0,250,ishouse,true))
	{
		DeleteNPC(player->GetMap(),cre->GetGUIDLow(),true);
		CharacterDatabase.DirectPExecute("DELETE FROM uow_player_housing_npc where npcspawnid=%u",cre->GetGUIDLow());
		LoadPlayerHousing();
	}
}
bool uowToolSet::HasPendingBane(uint32 houseid)
{
	bool active = false;
    QueryResult *result = CharacterDatabase.PQuery("SELECT * from uow_banes where phase<1 and houseid=%u and ended='0000-00-00 00:00:00'",houseid);
    if(result)
	{
        active = true;
		delete result;
	}
	return active;
}
bool uowToolSet::HasActiveBane(uint32 houseid)
{
	bool active = false;
    QueryResult *result = CharacterDatabase.PQuery("SELECT * from uow_banes where phase=1 and houseid=%u and ended='0000-00-00 00:00:00'",houseid);
    if(result)
	{
        active = true;
		delete result;
	}
	return active;
}
void uowToolSet::BaneCity(Player * player)
{
	if(player->GetGuildId()==0)
	{
		ChatHandler(player).PSendSysMessage("In order to place a bane, you must be in a guild.");
		return;
	};
	//Make sure this is a valid guild city
	if(!GetGuildCityByGuildID(player->GetGuildId()))
	{
		ChatHandler(player).PSendSysMessage("You are not a member of a guild that owns a guild city.");
		return;
	}
	if(!uowTools.HasMats(player,BANE_CHARTER,1))
		return;
	GameObject * go = GetNearestGOOwnedByOther(player);
	if(!go)
	{
		ChatHandler(player).PSendSysMessage("You must be near the guild city you wish to bane.");
		return;
	}
	float dst = go->GetDistance2d(player);
	if(dst < 70)
	{
		ChatHandler(player).PSendSysMessage("You are to close to the city, back up a bit and re-bane.");
		return;
	}
	PlayerHousingObjects * pho = GetGuildCityObject(go->GetDBTableGUIDLow());
	if(!pho)
		return;
	if(HasActiveBane(pho->houseid) || HasPendingBane(pho->houseid))
	{
		ChatHandler(player).PSendSysMessage("This city is under a pending bane.");
		return;
	}
	Guild* dg = objmgr.GetGuildById(uowTools.GetGuildCity(pho->houseid)->guildid);
	if(!dg)
		return;
	Guild* ag = objmgr.GetGuildById(player->GetGuildId());
	if(!ag)
		return;
	if(ag->GetId() == dg->GetId())
	{
		ChatHandler(player).PSendSysMessage("You may not bane your own guild silly billy!");
		return;
	}
	//guild city bane window check
	//Close down any challenges that have expired, phase 0 and pase 1, announce dodgers and winners
    QueryResult *result = CharacterDatabase.PQuery("SELECT HOUR(TIMEDIFF(CURTIME(),banewindow)) from uow_player_housing where id = %u", pho->houseid);
    if(!result)
		return;
	Field *fields = result->Fetch();
	int windowframe = fields[0].GetInt32();
	delete result;
	if(windowframe>=0 && windowframe < 6)
	{
		ChatHandler(player).PSendSysMessage("Trying to place bane...");
	}
	else
	{
		std::ostringstream msg;
		msg << "You may not bane this city at this time, the current window for this city to be baned starts at:" << uowTools.GetGuildCity(pho->houseid)->banewindow << " server time, and lasts 6 hours.";
		ChatHandler(player).PSendSysMessage(msg.str().c_str());
		return;
	}

	uint32 ret=0;
	ret = SpawnGO(GUILDCITY_BANE,player->GetMap(),player->GetPositionX(),player->GetPositionY(),player->GetPositionZ(),player->GetOrientation(),true);
	if(!ret)
		return;
	//Do data work here
	CharacterDatabase.DirectPExecute("INSERT INTO uow_banes (houseid,started,attacker,defender,winner,phase,banespawnid,mapid) VALUES (%u,NOW(),%u,%u,0,0,%u,%u)",pho->houseid,ag->GetId(),dg->GetId(),ret,player->GetMapId());

	std::ostringstream msg;
	msg << MSG_COLOR_GOLD << "[" << player->GetGuildName() << "] " << MSG_COLOR_YELLOW << " Has just baned the City that belongs to " << MSG_COLOR_GOLD << "[" << dg->GetName() << "] " << MSG_COLOR_YELLOW << ". This battle will start 48 hours from NOW. Server time:" << GetServerTime() ;
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str().c_str());
}
void uowToolSet::AcceptBane(uint32 houseid)
{
	PlayerHousingData * phd = GetGuildCity(houseid);
	if(!phd)
		return;
	if(HasPendingBane(phd->id))
	{	
		CharacterDatabase.DirectPExecute("UPDATE uow_banes set phase=1, started=NOW() where phase<1 and houseid=%u and ended='0000-00-00 00:00:00'", phd->id);
		std::ostringstream msg;
		msg << MSG_COLOR_GOLD << "[" << objmgr.GetGuildById(phd->guildid)->GetName() << "] " << MSG_COLOR_YELLOW << " has been attacked! There guild city is now open to attack. All attackers and defenders are urged to move in!";
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str().c_str());
	}
}
void uowToolSet::UpdateBanes()
{
	//Start any challenges that has hit its window for phase 0  
    QueryResult *result = CharacterDatabase.PQuery("SELECT *,TIMESTAMPDIFF(HOUR,started,now())as elapsedtime from uow_banes where phase<1 and ended='0000-00-00 00:00:00'");
    if(result)
	{
		do
		{
			Field *fields = result->Fetch();
			uint32 id = fields[0].GetUInt32();
			uint32 hid = fields[1].GetUInt32();
			uint32 at = fields[4].GetUInt32();
			uint32 df = fields[5].GetUInt32();
			uint32 sp = fields[8].GetUInt32();
			uint32 mapid = fields[9].GetUInt32();
			uint32 et = fields[10].GetUInt32();
			if(et>=48)
				AcceptBane(hid);
				
		} while (result->NextRow());
		delete result;
	}

	//Elapsed time on accepted matches
    result = CharacterDatabase.PQuery("SELECT *,TIMESTAMPDIFF(HOUR,started,now())as elapsedtime from uow_banes where phase=1 and ended='0000-00-00 00:00:00'");
    if(result)
	{
		do
		{
			Field *fields = result->Fetch();
			uint32 id = fields[0].GetUInt32();
			uint32 at = fields[4].GetUInt32();
			uint32 df = fields[5].GetUInt32();
			uint32 sp = fields[8].GetUInt32();
			uint32 mapid = fields[9].GetUInt32();
			uint32 et = fields[10].GetUInt32();
			if(et>=2)
			{
				Map *map = MapManager::Instance().FindMap(mapid);
				GameObject * tgo=NULL;
				if (GameObjectData const* go_data = objmgr.GetGOData(sp))
					tgo = map->GetGameObject(MAKE_NEW_GUID(sp,GUILDCITY_BANE , HIGHGUID_GAMEOBJECT));
				if(tgo)
					DeleteGO(tgo->GetMap(),tgo->GetDBTableGUIDLow(),true);
				CharacterDatabase.DirectPExecute("UPDATE uow_banes set ended=NOW(), winner=%u where id=%u", df, id);
				Guild* ag = objmgr.GetGuildById(at);
				Guild* dg = objmgr.GetGuildById(df);
				if(dg && ag)
				{
					std::ostringstream msg;
					msg << MSG_COLOR_GOLD << "[" << dg->GetName() << "] " << MSG_COLOR_YELLOW << " defended there city from " <<  MSG_COLOR_GOLD << "[" << ag->GetName() << "] " << MSG_COLOR_YELLOW << ". The cowards that attacked have been forced to flee like the dogs they are, put down by the better guild!";
					sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str().c_str());
				}
			}
				
		} while (result->NextRow());
		delete result;
	}
}
uint32 uowToolSet::GetBaneID(GameObject * bane)
{
	QueryResult *result = CharacterDatabase.PQuery("SELECT id from uow_banes where banespawnid=%u", bane->GetDBTableGUIDLow());
	if(result)
	{
		Field *fields = result->Fetch();
		uint32 ret = fields[0].GetUInt32();
		delete result;
		return ret;
	}
	return 0;
}

void uowToolSet::RewardBaneDefenders(GameObject * go)
{
	uint32 bcid = GetBaneID(go);
	if(bcid)
	{
		CharacterDatabase.DirectPExecute("UPDATE uow_banes set ended=NOW(), winner=defender where id=%u", bcid);
		QueryResult *result = CharacterDatabase.PQuery("SELECT attacker,defender from uow_banes where id=%u", bcid);
		if(result)
		{
			Field *fields = result->Fetch();
			uint32 at = fields[0].GetUInt32();
			uint32 df = fields[1].GetUInt32();
			Guild* ag = objmgr.GetGuildById(at);
			Guild* dg = objmgr.GetGuildById(df);
			if(dg && ag)
			{
				std::ostringstream msg;
				msg << MSG_COLOR_GOLD << "[" << dg->GetName() << "] " << MSG_COLOR_YELLOW << " defended there city from " <<  MSG_COLOR_GOLD << "[" << ag->GetName() << "] " << MSG_COLOR_YELLOW << ". The cowards that attacked have been forced to flee like the dogs they are, put down by the better guild!";
				sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str().c_str());
			}
			delete result;
			DeleteGO(go->GetMap(),go->GetDBTableGUIDLow(),true);
		}
	}
}

uint32 uowToolSet::GetHousingNPCCount(uint32 houseid, uint32 entry)
{
	//spin 
	uint32 count = 0;
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return count;
		if (itr->second->id == houseid)
		{
			//Got a city , lets check out the objects
			for(std::map<uint32, PlayerHousingNPCS*>::iterator itr2 =  itr->second->npclist.begin(); itr2 != itr->second->npclist.end(); ++itr2)
			{
				Creature * tc=NULL;
				if (CreatureData const* c_data = objmgr.GetCreatureData(itr2->second->npcspawnid))
				{
					if(c_data->id == entry)
						count++;
				}
			}
		}
	}
	return count;
}
uint32 uowToolSet::GetHousingObjectCount(uint32 houseid, uint32 entry)
{
	//spin 
	uint32 count = 0;
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return count;
		if (itr->second->id == houseid)
		{
			//Got a city , lets check out the objects
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
			{
				GameObject * tgo=NULL;
				if (GameObjectData const* go_data = objmgr.GetGOData(itr2->second->gospawnid))
				{
					if(go_data->id == entry)
						count++;
				}
			}
		}
	}
	return count;
}
uint32 uowToolSet::GetHousingObjectCount(uint32 houseid)
{
	//spin 
	uint32 count = 0;
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return count;
		if (itr->second->id == houseid)
		{
			//Got a city , lets check out the objects
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
				count++;
		}
	}
	return count;
}
void uowToolSet::RewardBaneAttackers(uint32 houseid, GameObject * cityflag)
{
	QueryResult *result = CharacterDatabase.PQuery("SELECT * from uow_banes where houseid=%u and phase=1 and ended='0000-00-00 00:00:00'", houseid);
	if(result)
	{
		Field *fields = result->Fetch();
		uint32 bid = fields[0].GetUInt32();
		uint32 at = fields[4].GetUInt32();
		uint32 df = fields[5].GetUInt32();
		uint32 bspid = fields[8].GetUInt32();
		uint32 mapid = fields[9].GetUInt32();
		Guild* ag = objmgr.GetGuildById(at);
		Guild* dg = objmgr.GetGuildById(df);
		CharacterDatabase.DirectPExecute("UPDATE uow_banes set ended=NOW(), winner=attacker where id=%u and phase=1 and ended='0000-00-00 00:00:00'", bid);
		if(dg && ag)
		{
			std::ostringstream msg;
			msg << MSG_COLOR_GOLD << "[" << dg->GetName() << "] " << MSG_COLOR_YELLOW << " has been defeated by " <<  MSG_COLOR_GOLD << "[" << ag->GetName() << "] " << MSG_COLOR_YELLOW << ". The defenders have been beaten down like maggots! The city they once called home is in shambles, wiped from the face of this earth!";
			sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str().c_str());
		}
		delete result;
		DeleteGO(cityflag->GetMap(),bspid,true);
		uowTools.DemolishGuildCity(houseid,cityflag->GetMap());
	}
}

GameObject * uowToolSet::GetNearestBane(Player *player)
{
	GameObject * go = NULL;
	go = player->FindNearestGameObject(GUILDCITY_BANE,8192);
	return go;
}
void uowToolSet::SummonBattleGO(Player * player, uint32 entry, uint32 dspawntime, int32 cost, bool ignorecooldown)
{
	if(HasCoolDown(CD_SIEGESUMMON,player) && !ignorecooldown)
	{
		ChatHandler(player).PSendSysMessage("Cooldown has not expired. You must wait!");
		return;
	}
	GameObject * go;
	go = GetNearestOwnedGO(player,false);
	if(!go)
		go = GetNearestGOOwnedByOther(player);
	if(!go)
		go = GetNearestBane(player);
	if(!go)
	{
		ChatHandler(player).PSendSysMessage("You must be closer to a city that is under attack in oder to summon this unit");
		return;
	}
	PlayerHousingObjects * pho = GetGuildCityObject(go->GetDBTableGUIDLow());
	if(!pho)
		return;
	if(HasActiveBane(pho->houseid))
	{
		if (player->GetMoney() < cost * 10000)
		{
			player->SendBuyError( BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
			return;
		}
		player->ModifyMoney(-(cost*10000));
		SpawnTempGO(entry,player->GetMap(),player->GetPositionX(),player->GetPositionY(),player->GetPositionZ(),player->GetOrientation(),dspawntime,player->GetGUID());
		if (!ignorecooldown)
			AddPlayerCooldown(CD_SIEGESUMMON, dspawntime, player);
		std::ostringstream msg;
		msg << MSG_COLOR_GOLD << "this object will despawn in " << dspawntime << " seconds.";
		ChatHandler(player).PSendSysMessage(msg.str().c_str());
	}
	else
	{
		ChatHandler(player).PSendSysMessage("You may only summon during an active siege");
	}
}

void uowToolSet::SummonBattleNPC(Player * player, uint32 entry, uint32 dspawntime, int32 cost, bool ignorecooldown)
{
	if(HasCoolDown(CD_SIEGESUMMON,player) && !ignorecooldown)
	{
		ChatHandler(player).PSendSysMessage("Cooldown has not expired. You must wait!");
		return;
	}
	GameObject * go;
	go = GetNearestOwnedGO(player,false);
	if(!go)
		go = GetNearestGOOwnedByOther(player);
	if(!go)
		go = GetNearestBane(player);
	if(!go)
	{
		ChatHandler(player).PSendSysMessage("You must be closer to a city that is under attack in oder to summon this unit");
		return;
	}
	bool activebane = false;
	PlayerHousingObjects * pho = GetGuildCityObject(go->GetDBTableGUIDLow());
	if(pho!=NULL)
	{
		if(HasActiveBane(pho->houseid))
			activebane=true;
	}
	else
	{
		 if(GetNearestBane(player)!=NULL)
			 activebane=true;
	}
	
	if(activebane)
	{
		if (player->GetMoney() < cost * 10000)
		{
			player->SendBuyError( BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
			return;
		}
		player->ModifyMoney(-(cost*10000));
		SpawnTempNPC(entry,player->GetMap(),player->GetPositionX(),player->GetPositionY(),player->GetPositionZ(),player->GetOrientation(),dspawntime,player);
		if (!ignorecooldown)
			AddPlayerCooldown(CD_SIEGESUMMON, dspawntime, player);
		std::ostringstream msg;
		msg << MSG_COLOR_GOLD << "this NPC will despawn in " << dspawntime << " seconds.";
		ChatHandler(player).PSendSysMessage(msg.str().c_str());
	}
	else
	{
		ChatHandler(player).PSendSysMessage("You may only summon during an active siege");
	}
}

void uowToolSet::PruchaseItems(Player * player, uint32 entry, int32 quantity, int32 cost)
{
	if (player->GetMoney() < (cost * 10000) * quantity)
	{
		player->SendBuyError( BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
		return;
	}
	player->ModifyMoney(-((cost * 10000) * quantity));
	GiveItems(player, entry, quantity);
}
int32 uowToolSet::GetRandomInt(int32 data[])
{
	return (int32)data[Random.randInt(sizeof(data))];
}

void uowToolSet::ClaimGuildCityRewards(Player * player)
{
	//Guild City Rewards
	if(HasCoolDown(CD_GUILDCITYREWARDS,player) || player->GetGuildId()==0)
	{
		ChatHandler(player).PSendSysMessage("You have already recieved rewards in the last 24 hours on this account, or you are not in a guild.");
		return;
	}
	//Make sure this is a valid guild city
	if(!GetGuildCityByGuildID(player->GetGuildId()))
	{
		ChatHandler(player).PSendSysMessage("You are not a member of a guild that owns a guild city.");
		return;
	}
	//Get guild data
	Guild * g = objmgr.GetGuildById(player->GetGuildId());
	if(!g)
		return;
	uint32 houseid = GetGuildCityByGuildID(player->GetGuildId())->id;
	//Apply Cooldown to this account
	player->SendPlaySound(8173,false);	 
	ChatHandler(player).PSendSysMessage("You have been granted rewards for the day! You may claim these rewards again in 24 hours. These rewards will show up in your mailbox in the next 5-10 minutes.");
	AddPlayerCooldown(CD_GUILDCITYREWARDS,86400,player);
	//Standard Rewards AV and WSG marks
	std::ostringstream msg;
	msg << player->GetName() << " \"Guild City Rewards\"" << " \"Since you are a member of a guild that owns a guild city you are granted the following rerwards.\"" << " 20560:2 20558:5";
	ChatHandler(player).HandleSendItemsCommand(msg.str().c_str());
	//Factory Rewards
	if(GetHousingObjectCount(houseid,192028)>0) //Alch
	{
		int32 data[] = {46377,46376,46379,46378,41166,42545,40077,5634,2459};
		int32 data2[] = {3,3,3,3,5,5,5,3,3};
		int32 rnd = uowTools.Random.randInt(8);
		int32 reward=data[rnd];
		int32 qty=data2[rnd];
		std::ostringstream msg;
		msg << player->GetName() << " \"Guild City Rewards\"" << " \"Alchemy Factory.\" " << reward << ":" << qty;
		ChatHandler(player).HandleSendItemsCommand(msg.str().c_str());
	}
	if(GetHousingObjectCount(houseid,192029)>0) //food
	{
		int32 data[] = {42999,34767,42995,34769,43480,43478,43015};
		int32 data2[] = {10,10,10,10,3,3,3};
		int32 rnd = uowTools.Random.randInt(6);
		int32 reward=data[rnd];
		int32 qty=data2[rnd];
		std::ostringstream msg;
		msg << player->GetName() << " \"Guild City Rewards\"" << " \"Food Factory.\" " << reward << ":" << qty;
		ChatHandler(player).HandleSendItemsCommand(msg.str().c_str());
	}
	if(GetHousingObjectCount(houseid,192030)>0) //Eng
	{
		int32 data[] = {44506,44507,23841,42641,44951,40772};
		int32 data2[] = {1,1,1,1,1,1};
		int32 rnd = uowTools.Random.randInt(5);
		int32 reward=data[rnd];
		int32 qty=data2[rnd];
		std::ostringstream msg;
		msg << player->GetName() << " \"Guild City Rewards\"" << " \"Engineering Factory.\" " << reward << ":" << qty;
		ChatHandler(player).HandleSendItemsCommand(msg.str().c_str());
	}
	if(GetHousingObjectCount(houseid,192031)>0) //heal
	{
		int32 data[] = {36900,25521,23576,43466,37092,37098,43464,37094};
		int32 data2[] = {1,1,1,5,5,5,5,5};
		int32 rnd = uowTools.Random.randInt(7);
		int32 reward=data[rnd];
		int32 qty=data2[rnd];
		std::ostringstream msg;
		msg << player->GetName() << " \"Guild City Rewards\"" << " \"Healing Factory.\" " << reward << ":" << qty;
		ChatHandler(player).HandleSendItemsCommand(msg.str().c_str());
	}
	if(GetHousingObjectCount(houseid,192032)>0) //ore and gem
	{
		int32 data[] = {36910,36912,36928,36919,36925,36922,36934,36931,42225};
		int32 data2[] = {20,20,1,1,1,1,1,1,1};
		int32 rnd = uowTools.Random.randInt(8);
		int32 reward=data[rnd];
		int32 qty=data2[rnd];
		std::ostringstream msg;
		msg << player->GetName() << " \"Guild City Rewards\"" << " \"Gem Factory.\" " << reward << ":" << qty;
		ChatHandler(player).HandleSendItemsCommand(msg.str().c_str());
	}
	if(GetHousingObjectCount(houseid,192033)>0) //wmd
	{
		int32 data[] = {29532,29530,49634,41509};
		int32 data2[] = {1,1,3,3};
		int32 rnd = uowTools.Random.randInt(3);
		int32 reward=data[rnd];
		int32 qty=data2[rnd];
		std::ostringstream msg;
		msg << player->GetName() << " \"Guild City Rewards\"" << " \"WMD Factory.\" " << reward << ":" << qty;
		ChatHandler(player).HandleSendItemsCommand(msg.str().c_str());

		//Send out a Electrum Ticket
		std::ostringstream msg2;
		reward=70030;
		qty=3;
		msg2 << player->GetName() << " \"Guild City Rewards\"" << " \"WMD Factory.\" " << reward << ":" << qty;
		ChatHandler(player).HandleSendItemsCommand(msg2.str().c_str());

	}

}

uint32 uowToolSet::GetAccountIDByCharacterName(std::string name)
{
	QueryResult *result = CharacterDatabase.PQuery("SELECT account from characters where name ='%s'",name.c_str());
	if(result)
	{
		Field *fields = result->Fetch();
		uint32 id = fields[0].GetUInt32();
		delete result;
		return id;
	}
	return 0;
}
void uowToolSet::RewardShrine(Player * player)
{
	uint32 count=0;
	PlayerVariablesCollection::const_iterator iitr;
	iitr = player->m_uowvariables.find("300300");
	if( iitr != player->m_uowvariables.end() )
		count++;
	iitr = player->m_uowvariables.find("300301");
	if( iitr != player->m_uowvariables.end() )
		count++;
	iitr = player->m_uowvariables.find("300302");
	if( iitr != player->m_uowvariables.end() )
		count++;
	iitr = player->m_uowvariables.find("300303");
	if( iitr != player->m_uowvariables.end() )
		count++;
	iitr = player->m_uowvariables.find("300304");
	if( iitr != player->m_uowvariables.end() )
		count++;
	iitr = player->m_uowvariables.find("300305");
	if( iitr != player->m_uowvariables.end() )
		count++;
	iitr = player->m_uowvariables.find("300306");
	if( iitr != player->m_uowvariables.end() )
		count++;
	iitr = player->m_uowvariables.find("300307");
	if( iitr != player->m_uowvariables.end() )
		count++;
	if(count==8)
	{
		//Update stats
		uowTools.UpdatePlayerStats(player,0,0,0,0,0,0,0,0,0,1);
		ChatHandler(player).PSendSysMessage("You clicked all 8 shrines! Congratulations! You get 1 Champion's seal and 5 WSG tokens!");
		uowTools.GiveItems(player,UOW_WSGMARK,5);
		uowTools.GiveItems(player,44990,1);
		player->ModifyHonorPoints(500);
		uowTools.AddPlayerCooldown(CD_SHRINES,14400,player);
		player->SendPlaySound(7054,true);
		iitr = player->m_uowvariables.find("300300");
			player->m_uowvariables.erase(iitr);
		iitr = player->m_uowvariables.find("300301");
			player->m_uowvariables.erase(iitr);
		iitr = player->m_uowvariables.find("300302");
			player->m_uowvariables.erase(iitr);
		iitr = player->m_uowvariables.find("300303");
			player->m_uowvariables.erase(iitr);
		iitr = player->m_uowvariables.find("300304");
			player->m_uowvariables.erase(iitr);
		iitr = player->m_uowvariables.find("300305");
			player->m_uowvariables.erase(iitr);
		iitr = player->m_uowvariables.find("300306");
			player->m_uowvariables.erase(iitr);
		iitr = player->m_uowvariables.find("300307");
			player->m_uowvariables.erase(iitr);
	}
}

void uowToolSet::InitializeCommonMarks()
{
	CommonMarks.clear();
	Mark * newmark;
	//City Siege
	newmark = new Mark(); newmark->id=1; newmark->mapid=0; newmark->name="Stonewatch Keep"; newmark->x=-9323.959961; newmark->y=-3030.909912; newmark->z=132.669006; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=2; newmark->mapid=0; newmark->name="Fenris Keep"; newmark->x=852.763855; newmark->y=688.620728; newmark->z=53.425354; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=3; newmark->mapid=0; newmark->name="Hearthglen Keep"; newmark->x=2793.090088; newmark->y=-1621.400024; newmark->z=129.332993; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=4; newmark->mapid=0; newmark->name="Stromgarde Keep"; newmark->x=-1551.199951; newmark->y=-1808.099976; newmark->z=67.521896; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=5; newmark->mapid=1; newmark->name="Tirigarde Keep"; newmark->x=-202.190994; newmark->y=-5025.830078; newmark->z=21.631001; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=6; newmark->mapid=0; newmark->name="Ravenholdt Manor"; newmark->x=-9.508620; newmark->y=-1596.459961; newmark->z=194.606003; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=7; newmark->mapid=0; newmark->name="Hillsbrad Fields"; newmark->x=-663.816467; newmark->y=40.729897; newmark->z=46.926456; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=8; newmark->mapid=0; newmark->name="The Farstrider Lodge"; newmark->x=-5491.970215; newmark->y=-4063.183350; newmark->z=364.629150; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	//Shrines
	newmark = new Mark(); newmark->id=9; newmark->mapid=0; newmark->name="Shrine of Compassion"; newmark->x=255.604279; newmark->y=378.648102; newmark->z=-68.702682; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=10; newmark->mapid=0; newmark->name="Shrine of Honesty"; newmark->x=-2125.121582; newmark->y=-1994.519653; newmark->z=4.217486; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=11; newmark->mapid=0; newmark->name="Shrine of Honor"; newmark->x=-1328.420532; newmark->y=548.128357; newmark->z=100.999344; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=12; newmark->mapid=0; newmark->name="Shrine of Humility"; newmark->x=-4750.767090; newmark->y=-3283.322021; newmark->z=310.256012; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=13; newmark->mapid=0; newmark->name="Shrine of Justice"; newmark->x=-9135.679688; newmark->y=-1050.379517; newmark->z=70.584587; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=14; newmark->mapid=0; newmark->name="Shrine of Sacrafice"; newmark->x=2282.522461; newmark->y=-5289.480957; newmark->z=83.319611; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=15; newmark->mapid=0; newmark->name="Shrine of Spirituality"; newmark->x=-5644.189941; newmark->y=-494.630463; newmark->z=399; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));
	newmark = new Mark(); newmark->id=16; newmark->mapid=0; newmark->name="Shrine of Valor"; newmark->x=-14748.302734; newmark->y=-432.138947; newmark->z=4.097762; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));

	//Special
	newmark = new Mark(); newmark->id=17; newmark->mapid=0; newmark->name="Gurubashi Duel Pit"; newmark->x=-13227.930664; newmark->y=228.202423; newmark->z=32.939613; newmark->o=0;
	CommonMarks.insert(MarkCollection::value_type(newmark->id,newmark));

}
Mark * uowToolSet::GetCommonMark(uint32 id)
{
	MarkCollection::iterator itr = CommonMarks.find(id);
	if( itr == CommonMarks.end() )
		return NULL;
	return itr->second;
}

int32 uowToolSet::GetDistance2D(float x1, float y1,float x2, float y2)
{
    float dx = x1 - x2; 
    float dy = y1 - y2; 
	int32 ret = abs((int32)sqrt(dx*dx + dy*dy));
	return ret;
}

void uowToolSet::DismissHelper(Player * player)
{
	player->RemoveAllMinionsByEntry(11325);
	player->RemoveAllMinionsByEntry(34694);
	player->RemoveAllMinionsByEntry(29147);
	player->RemoveAllMinionsByEntry(36911);
}
void uowToolSet::Queue_CraterCapture(Player * player)
{
	//Make sure they are not already part of the game
	Remove_CraterCapture(player);

	//Return Mark
	Mark * ret = new Mark();
	ret->id = 0; ret->mapid = player->GetMap()->GetId(); ret->name="return recall"; ret->x = player->GetPositionX(); ret->y = player->GetPositionY(); ret->z = player->GetPositionZ(); ret->o = player->GetOrientation();
	//New Player Setup
	CCPlayerDataStruct newplayer;
	newplayer.captures=0; newplayer.deaths=0; newplayer.kills=0; newplayer.markhome = ret; newplayer.player = player;
	WorldGames.Game_CC.players.insert(CCPlayerCollection::value_type(player->GetGUIDLow(),newplayer));
	ChatHandler(player).PSendSysMessage("You are now in the Crater Capture Queue!");

	//if the game is in progress, send them in
	if(WorldGames.Game_CC.gamestage==1)
	{
		std::ostringstream msg;
		msg << MSG_COLOR_GREEN << "[" << player->GetName() << "] has joined [Crater Capture]";
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
		TeleportTo_CraterCapture(player);
	}
	else
	{
		std::ostringstream msg;
		msg << MSG_COLOR_GREEN << "[" << player->GetName() << "] has joined the [Crater Capture Queue]";
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
	}
}
void uowToolSet::TeleportTo_CraterCapture(Player * player)
{
		Remove_BloodSailBootyCapture(player);
		uint32 ran = Random.randInt(4);
		Position pos;
		switch (ran)
		{
		case 0:
			pos = WorldGames.Game_CC.s1;
			break;
		case 1:
			pos = WorldGames.Game_CC.s2;
			break;
		case 2:
			pos = WorldGames.Game_CC.s3;
			break;
		default:
			pos = WorldGames.Game_CC.s4;
			break;
		}
		player->RemoveFromGroup();
		player->DestroyConjuredItems(true);
		player->UnsummonPetTemporaryIfAny();
		DismissHelper(player); //Remove ze panda!
		player->SendPlaySound(7756,false);
		player->TeleportTo(1,pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ(),pos.GetOrientation(),0);
		player->SendPlaySound(7756,false);
}
void uowToolSet::Start_CraterCapture()
{
	WorldGames.Game_CC.turnintrigger=0;
	WorldGames.Game_CC.gamestage=1;
	WorldGames.Game_CC.elapsedseconds=0;
	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "[Crater Capture] has begun! Open your companion helper and select world games and then crater capture to join!";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
	//Teleport them to the battlefield
	for (CCPlayerCollection::iterator itr = WorldGames.Game_CC.players.begin(); itr != WorldGames.Game_CC.players.end(); ++itr)
		TeleportTo_CraterCapture(itr->second.player);
}
void uowToolSet::Load_CraterCapture()
{
	WorldGames.Game_CC.turnintrigger=0;
	WorldGames.Game_CC.gamestage=0;
	WorldGames.Game_CC.elapsedseconds=0;
	WorldGames.Game_CC.players.clear();
	WorldGames.Game_CC.s1.Relocate(5354.951172,-2818.271240,1466.567139,0);
	WorldGames.Game_CC.s2.Relocate(5391.978027,-2689.667725,1458.410278,0);
	WorldGames.Game_CC.s3.Relocate(5536.804688,-2768.441406,1462.284912,0);
	WorldGames.Game_CC.s4.Relocate(5486.061035,-2899.370850,1482.449097,0);
}
void uowToolSet::Update_CraterCapture()
{
	if (WorldGames.Game_CC.gamestage < 1 && WorldGames.Game_CC.players.size() < 3) // The game is not active so lets traack the time and activate as necessary
	{
		WorldGames.Game_CC.elapsedseconds=WorldGames.Game_CC.elapsedseconds + 5;
		int32 elapsedminutes=WorldGames.Game_CC.elapsedseconds / 60;
		//Has 20 minutes passed? start a new game if so...
		if(elapsedminutes>=20)
		{
			WorldGames.Game_CC.elapsedseconds=0;
			std::ostringstream msg;
			msg << MSG_COLOR_GREEN << "[Crater Capture] Join the queue for some FFA CTF action!";
			sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
		}			
		return;
	}
	if (WorldGames.Game_CC.gamestage < 1 && WorldGames.Game_CC.players.size() >= 3) // Start it up!
		Start_CraterCapture();
	WorldGames.Game_CC.elapsedseconds=WorldGames.Game_CC.elapsedseconds+5;
	WorldGames.Game_CC.turnintrigger=WorldGames.Game_CC.turnintrigger+5;
	int32 elapsedminutes=WorldGames.Game_CC.elapsedseconds / 60;
	//Has 20 minutes passed? Game over if so.
	if(elapsedminutes>=20)
	{
		//Game over
		Reward_CraterCapture();
		return;
	}
	//check to see if anyone is near a capture point who has a flag and reward them.
	for (CCPlayerCollection::iterator itr = WorldGames.Game_CC.players.begin(); itr != WorldGames.Game_CC.players.end(); ++itr)
	{
		itr->second.player->RemoveFromGroup();
		//if(itr->second.player->HasInvisibilityAura() && itr->second.player->HasAura(45450))
		if((itr->second.player->HasAura(642) || itr->second.player->GetVehicle() || itr->second.player->IsMounted()) && itr->second.player->HasAura(45450))		
			itr->second.player->RemoveAura(45450);

		if (itr->second.player->FindNearestGameObject(186469,4) &&  itr->second.player->HasAura(45450))
		{
			if(WorldGames.Game_CC.players.size()<3)
			{
				itr->second.player->RemoveAura(45450);
				itr->second.player->SendPlaySound(42,false);
				ChatHandler(itr->second.player).PSendSysMessage("There are not enough players in this game for a capture to count. Removing the flag.");
				return;
			}
			itr->second.player->RemoveAura(45450);
			itr->second.player->SendPlaySound(11988,false);
			itr->second.captures++;
			std::ostringstream msg;
			msg << MSG_COLOR_GREEN << "[Crater Capture] " <<  itr->second.player->GetName() << " has captured a flag!";
			sWorld.SendZoneText(616,msg.str( ).c_str( ));
			//Update stats
			uowTools.UpdatePlayerStats(itr->second.player,1,0,0,0,0,0,0,0);
		}
	}
}
void uowToolSet::BroadcastCraterScore(Player * player)
{
	for (CCPlayerCollection::iterator itr = WorldGames.Game_CC.players.begin(); itr != WorldGames.Game_CC.players.end(); ++itr)
	{
		//Ranks
		int score =  (itr->second.captures * 4) + itr->second.kills - itr->second.deaths;
		//Display Score
		std::ostringstream msg;
		msg << MSG_COLOR_RED << "Name:" << MSG_COLOR_WHITE << itr->second.player->GetName() << MSG_COLOR_RED << " Captures:" << MSG_COLOR_WHITE << itr->second.captures << MSG_COLOR_RED << " Kills:" << MSG_COLOR_WHITE << itr->second.kills << MSG_COLOR_RED << " Deaths:" << MSG_COLOR_WHITE << itr->second.deaths << MSG_COLOR_RED << " Total Score:" << MSG_COLOR_WHITE << score;
		if(!player)
			sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
		else
			ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
	}
}
void uowToolSet::Reward_CraterCapture()
{
	//check to see if anyone is near a capture point who has a flag and reward them.
	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "[Crater Capture] has ended! Please watch chat for another gaming starting in about 20 minutes!";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
	Player * Rank1p = NULL; int Rank1s=-999;
	Player * Rank2p = NULL; int Rank2s=-999;
	Player * Rank3p = NULL; int Rank3s=-999;
	for (CCPlayerCollection::iterator itr = WorldGames.Game_CC.players.begin(); itr != WorldGames.Game_CC.players.end(); ++itr)
	{
		//Ranks
		int score =  (itr->second.captures * 4) + itr->second.kills - itr->second.deaths;
		if(score > Rank1s) //rank1 bump
		{
			Rank3s = Rank2s;
			Rank3p = Rank2p;
			Rank2p = Rank1p;
			Rank2s = Rank1s;
			Rank1p = itr->second.player;
			Rank1s = score;
		}
		else if(score > Rank2s) //rank2 bump
		{
			Rank3s = Rank2s;
			Rank3p = Rank2p;
			Rank2p = itr->second.player;
			Rank2s = score;
		}
		else if(score > Rank3s) //rank2 bump
		{
			Rank3p = itr->second.player;
			Rank3s = score;
		}
		//Display Score
		std::ostringstream msg;
		msg << MSG_COLOR_RED << "Name:" << MSG_COLOR_WHITE << itr->second.player->GetName() << MSG_COLOR_RED << " Captures:" << MSG_COLOR_WHITE << itr->second.captures << MSG_COLOR_RED << " Kills:" << MSG_COLOR_WHITE << itr->second.kills << MSG_COLOR_RED << " Deaths:" << MSG_COLOR_WHITE << itr->second.deaths << MSG_COLOR_RED << " Total Score:" << MSG_COLOR_WHITE << score;
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));

		//Destroy VEH/Mount
		if(itr->second.player->GetVehicle())
		{
			Vehicle * veh = itr->second.player->GetVehicle();
			veh->RemoveAllPassengers();
			//veh->Dismiss();	
			veh->GetBase()->Kill(veh->GetBase(),false);
		}

		//Recall the player home
		if(itr->second.player && itr->second.player->IsInWorld() && itr->second.player->GetZoneId() == 616)
		{
			//Update stats
			uowTools.UpdatePlayerStats(itr->second.player,0,0,0,1,0,0,0,0);

			if(itr->second.player->isDead())
				itr->second.player->ResurrectPlayer(100,false);
			itr->second.player->m_currentmark = itr->second.markhome;
			uowTools.Recall(itr->second.player,true);
		}

	}
	//Display Top 3 and reward
	std::ostringstream msg2;
	msg2 << MSG_COLOR_GREEN << "The top players that earned rewards are...\n";
	if(Rank1p)
	{
		ChatHandler(Rank1p).PSendSysMessage("You win 100 gold and 5 WSG tokens!");
		uowTools.GiveItems(Rank1p,UOW_WSGMARK,3);
		Rank1p->ModifyMoney(25*10000);
		msg2 << MSG_COLOR_RED << "Name:" << MSG_COLOR_WHITE << Rank1p->GetName() << MSG_COLOR_RED << " Total Score:" << MSG_COLOR_WHITE << Rank1s << "\n";
	}
	if(Rank2p)
	{
		ChatHandler(Rank2p).PSendSysMessage("You win 50 gold and 3 WSG tokens!");
		uowTools.GiveItems(Rank2p,UOW_WSGMARK,2);
		Rank2p->ModifyMoney(15*10000);
		msg2 << MSG_COLOR_RED << "Name:" << MSG_COLOR_WHITE << Rank2p->GetName() << MSG_COLOR_RED << " Total Score:" << MSG_COLOR_WHITE << Rank2s << "\n";
	}
	if(Rank3p)
	{
		ChatHandler(Rank3p).PSendSysMessage("You win 25 gold and 1 WSG token!");
		uowTools.GiveItems(Rank3p,UOW_WSGMARK,1);
		Rank3p->ModifyMoney(10*10000);
		msg2 << MSG_COLOR_RED << "Name:" << MSG_COLOR_WHITE << Rank3p->GetName() << MSG_COLOR_RED << " Total Score:" << MSG_COLOR_WHITE << Rank3s << "\n";
	}
	msg2 << MSG_COLOR_GREEN << "Hail the victors!";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg2.str( ).c_str( ));
	WorldGames.Game_CC.players.clear();
	WorldGames.Game_CC.gamestage=0;
	WorldGames.Game_CC.elapsedseconds=0;
	WorldGames.Game_CC.turnintrigger=0;

}
void uowToolSet::Remove_CraterCapture(Player * player)
{
	//Make sure they are not already part of the game
	CCPlayerCollection::iterator itr = WorldGames.Game_CC.players.find(player->GetGUIDLow());
	if( itr != WorldGames.Game_CC.players.end() ) // in the list let's remove/readd them to the game
		WorldGames.Game_CC.players.erase(itr);
}

void uowToolSet::Load_BloodSailBootyCapture()
{
	WorldGames.Game_BB.gamestage=0;
	WorldGames.Game_BB.elapsedseconds=0;
	WorldGames.Game_BB.players.clear();
	WorldGames.Game_BB.RedStart.Relocate(-15026.277344,265.190735,7.517748,0);
	WorldGames.Game_BB.BlueStart.Relocate(-14926.458984,358.304169,7.539866,0);
	WorldGames.Game_BB.RedFlag=NULL;
	WorldGames.Game_BB.BlueFlag=NULL;
	WorldGames.Game_BB.Bubble=NULL;
	WorldGames.Game_BB.BlueTurnin=NULL;
	WorldGames.Game_BB.RedTurnin=NULL;
}
void uowToolSet::TeleportTo_BootyCapture(BBPlayerDataStruct playerdata)
{
	Remove_CraterCapture(playerdata.player);
	ChatHandler(playerdata.player).PSendSysMessage("Teleporting you to the game!");
	//Team Assignment
	if(WorldGames.Game_BB.BlueTeam.size() < WorldGames.Game_BB.RedTeam.size())
	{
		playerdata.team=0;
		WorldGames.Game_BB.BlueTeam.insert(BBPlayerCollection::value_type(playerdata.player->GetGUIDLow(),playerdata));
	}
	else
	{
		playerdata.team=1;
		WorldGames.Game_BB.RedTeam.insert(BBPlayerCollection::value_type(playerdata.player->GetGUIDLow(),playerdata));
	}
	Position pos;
	if(playerdata.team==0)
		pos = WorldGames.Game_BB.BlueStart;
	else
		pos = WorldGames.Game_BB.RedStart;	
	playerdata.player->DestroyConjuredItems(true);
	DismissHelper(playerdata.player);
	playerdata.player->SendPlaySound(7756,false);
	playerdata.player->TeleportTo(0,pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ(),pos.GetOrientation(),0);
	playerdata.player->SendPlaySound(7756,false);
}
void uowToolSet::Start_BloodSailBootyCapture()
{
	WorldGames.Game_BB.gamestage=1;
	WorldGames.Game_BB.elapsedseconds=0;
	Map *map = MapManager::Instance().FindMap(0);
	WorldGames.Game_BB.RedFlag = SpawnTempGO(HYJAL_FLAG,map,-15014.901367,265.532990,0.198196,0,1200,0);
	WorldGames.Game_BB.RedTurnin = SpawnTempGO(192496,map,-15014.901367,265.532990,0.198196,0,1200,0);
	WorldGames.Game_BB.BlueFlag = SpawnTempGO(HYJAL_FLAG,map,-14936.555664,355.150055,0.219793,0,1200,0);
	WorldGames.Game_BB.BlueTurnin = SpawnTempGO(20917,map,-14936.555664,355.150055,0.219793,0,1200,0);
	WorldGames.Game_BB.Bubble = SpawnTempGO(300308,map,-14959.351563,302.752960,2.81747,0,1200,0);

	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "[Bloodsail Booty Capture] has begun! Open your companion helper and select world games and then bloodsail capture to join!";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
	WorldGames.Game_BB.BlueTeam.clear();
	WorldGames.Game_BB.RedTeam.clear();
	//Teleport them to the field of battle, set the factions and place them on a team
	for (BBPlayerCollection::iterator itr = WorldGames.Game_BB.players.begin(); itr != WorldGames.Game_BB.players.end(); ++itr)
		TeleportTo_BootyCapture(itr->second);
}

void uowToolSet::Queue_BloodSailBootyCapture(Player * player)
{
	//Make sure they are not already part of the game
	Remove_BloodSailBootyCapture(player);
	//Teleport them to the field of battle, set the factions and place them on a team
	//Return Mark
	Mark * ret = new Mark();
	ret->id = 0; ret->mapid = player->GetMap()->GetId(); ret->name="return recall"; ret->x = player->GetPositionX(); ret->y = player->GetPositionY(); ret->z = player->GetPositionZ(); ret->o = player->GetOrientation();
	//New Player Setup
	BBPlayerDataStruct newplayer;
	newplayer.captures=0; newplayer.deaths=0; newplayer.kills=0; newplayer.markhome = ret; newplayer.player = player;
	WorldGames.Game_BB.players.insert(BBPlayerCollection::value_type(player->GetGUIDLow(),newplayer));
	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "[" << player->GetName() << "] has joined [Bloodsail Booty Capture]";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));

}
void uowToolSet::Remove_AllQueues(Player * player)
{
	Remove_BloodSailBootyCapture(player);
	Remove_CraterCapture(player);
}
void uowToolSet::Remove_BloodSailBootyCapture(Player * player)
{
	//Make sure they are not already part of the game
	BBPlayerCollection::iterator itr = WorldGames.Game_BB.players.find(player->GetGUIDLow());
	if( itr != WorldGames.Game_BB.players.end() ) // in the list let's remove/readd them to the game
	{
		//itr->second.player->RemoveFromGroup();
		//itr->second.player->CastSpell(itr->second.player,26013,true,NULL,NULL,itr->second.player->GetGUID());	
		if(itr->second.markhome)
			delete itr->second.markhome;
		WorldGames.Game_BB.players.erase(itr);
	}
}

bool uowToolSet::isinBloodSailBootyCapture(Player * player)
{
	//Make sure they are not already part of the game
	BBPlayerCollection::iterator itr = WorldGames.Game_BB.players.find(player->GetGUIDLow());
	if( itr != WorldGames.Game_BB.players.end() ) // in the list let's remove/readd them to the game
		return true;
	else
		return false;
}
void uowToolSet::Update_BloodSailBootyCapture()
{
	if (WorldGames.Game_BB.gamestage < 1 && WorldGames.Game_BB.players.size() < 4) // The game is not active so lets traack the time and activate as necessary
	{
		WorldGames.Game_BB.elapsedseconds=WorldGames.Game_BB.elapsedseconds + 5;
		int32 elapsedminutes=WorldGames.Game_CC.elapsedseconds / 60;
		//Has 20 minutes passed? start a new game if so...
		if(elapsedminutes>=20)
		{
			WorldGames.Game_BB.elapsedseconds=0;
			std::ostringstream msg;
			msg << MSG_COLOR_GREEN << "[Booty Capture] Join the queue for some TEAM CTF action!";
			sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
		}			
		return;
	}
	if (WorldGames.Game_BB.gamestage < 1 && WorldGames.Game_BB.players.size() >= 4) // Start it up!
		Start_BloodSailBootyCapture();
	WorldGames.Game_BB.elapsedseconds=WorldGames.Game_BB.elapsedseconds+5;
	int32 elapsedminutes=WorldGames.Game_BB.elapsedseconds / 60;
	//Has 20 minutes passed? Game over if so.
	if(elapsedminutes>=20)
	{
		//Game over
		Reward_BloodSailBootyCapture();
		return;
	}
 
	//check to see if anyone is near a capture point who has a flag and reward them.
	for (BBPlayerCollection::iterator itr = WorldGames.Game_BB.players.begin(); itr != WorldGames.Game_BB.players.end(); ++itr)
	{
		//rulesfor flag
		if((itr->second.player->HasAura(642) || itr->second.player->GetVehicle() || itr->second.player->IsMounted()) && itr->second.player->HasAura(45450))
			itr->second.player->RemoveAura(45450);
		uint32 turnin;
		if(itr->second.team==0)
			turnin=20917;
		else
			turnin=192496;
		if (itr->second.player->FindNearestGameObject(turnin,4) &&  itr->second.player->HasAura(45450))
		{
			itr->second.player->RemoveAura(45450);
			itr->second.player->SendPlaySound(11988,false);
			itr->second.captures++;
			std::ostringstream msg;
			msg << MSG_COLOR_GREEN << "[Bloodsail Booty Capture] " <<  itr->second.player->GetName() << " has captured a flag!";
			sWorld.SendZoneText(33,msg.str( ).c_str( ));
			//Update stats
			uowTools.UpdatePlayerStats(itr->second.player,1,0,0,0,0,0,0,0);
			/*
			//respawn the flag for the other team
			Map *map = MapManager::Instance().FindMap(0);
			if(itr->second.team=0)
				WorldGames.Game_BB.BlueFlag = SpawnTempGO(HYJAL_FLAG,map,-14936.555664,355.150055,0.219793,0,1200,0);
			else
				WorldGames.Game_BB.RedFlag = SpawnTempGO(HYJAL_FLAG,map,-15014.901367,265.532990,0.198196,0,1200,0);
			*/

		}
	}
}
void uowToolSet::Reward_BloodSailBootyCapture()
{
	//check to see if anyone is near a capture point who has a flag and reward them.
	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "[Bloodsail Booty Capture] has ended! Please watch chat for another gaming starting in about 20 minutes!";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
	for (BBPlayerCollection::iterator itr = WorldGames.Game_BB.players.begin(); itr != WorldGames.Game_BB.players.end(); ++itr)
	{
		//Ranks
		int score =  (itr->second.captures * 4) + itr->second.kills - itr->second.deaths;
		//Display Score
		std::ostringstream msg;
		msg << MSG_COLOR_RED << "Name:" << MSG_COLOR_WHITE << itr->second.player->GetName() << MSG_COLOR_RED << " Captures:" << MSG_COLOR_WHITE << itr->second.captures << MSG_COLOR_RED << " Kills:" << MSG_COLOR_WHITE << itr->second.kills << MSG_COLOR_RED << " Deaths:" << MSG_COLOR_WHITE << itr->second.deaths << MSG_COLOR_RED << " Total Score:" << MSG_COLOR_WHITE << score;
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));

		//Recall the player home
		if(itr->second.player && itr->second.player->IsInWorld() && itr->second.player->GetZoneId() == 33)
		{
			//Update stats
			uowTools.UpdatePlayerStats(itr->second.player,0,0,0,0,1,0,0,0);
			if(itr->second.player->isDead())
				itr->second.player->ResurrectPlayer(100,false);
			itr->second.player->m_currentmark = itr->second.markhome;
			uowTools.Recall(itr->second.player,true);
		}
	}
	WorldGames.Game_BB.BlueFlag->Delete();
	WorldGames.Game_BB.BlueTurnin->Delete();
	WorldGames.Game_BB.Bubble->Delete();
	WorldGames.Game_BB.RedFlag->Delete();
	WorldGames.Game_BB.RedTurnin->Delete();
	WorldGames.Game_BB.BlueFlag=NULL;
	WorldGames.Game_BB.BlueTurnin=NULL;
	WorldGames.Game_BB.Bubble=NULL;
	WorldGames.Game_BB.RedFlag=NULL;
	WorldGames.Game_BB.RedTurnin=NULL;
	WorldGames.Game_BB.players.clear();
	WorldGames.Game_BB.gamestage=0;
	WorldGames.Game_BB.elapsedseconds=0;
}
void uowToolSet::BroadcastBloodSailBootyScore(Player * player)
{
	int red = 0;
	int blue =0;

	for (BBPlayerCollection::iterator itr = WorldGames.Game_BB.players.begin(); itr != WorldGames.Game_BB.players.end(); ++itr)
	{
		//Ranks
		int score =  (itr->second.captures * 4) + itr->second.kills - itr->second.deaths;
		//Display Score
		char * teamstr = NULL;
		if(itr->second.team==0)
		{
			teamstr = "|cff00ccff[TEAM BLUE] ";
			blue=blue+(itr->second.captures*4) + itr->second.kills - itr->second.deaths;
		}
		else
		{
			teamstr = "|cffff0000[TEAM RED] ";
			red=red+(itr->second.captures*4) + itr->second.kills - itr->second.deaths;
		}
		std::ostringstream msg;
		msg << teamstr << MSG_COLOR_RED << "Name:" << MSG_COLOR_WHITE << itr->second.player->GetName() << MSG_COLOR_RED << " Captures:" << MSG_COLOR_WHITE << itr->second.captures << MSG_COLOR_RED << " Kills:" << MSG_COLOR_WHITE << itr->second.kills << MSG_COLOR_RED << " Deaths:" << MSG_COLOR_WHITE << itr->second.deaths << MSG_COLOR_RED << " Total Score:" << MSG_COLOR_WHITE << score;
		if(!player)
			sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
		else
			ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
	}
	std::ostringstream msg;
	msg << MSG_COLOR_ORANGE << "Red Total: " << MSG_COLOR_WHITE << red;
	if(!player)
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
	else
		ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));

	std::ostringstream msg2;
	msg2 << MSG_COLOR_ORANGE << "Blue Total: " << MSG_COLOR_WHITE << blue;
	if(!player)
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg2.str( ).c_str( ));
	else
		ChatHandler(player).PSendSysMessage(msg2.str( ).c_str( ));

}
void uowToolSet::BroadcastBloodSailBootyStatus(Player * player)
{
	//If the game is not active display activation time left etc
	if(WorldGames.Game_BB.gamestage == 0)
	{
		std::ostringstream msg;
		msg << MSG_COLOR_RED << "[Bloodsail Booty Capture] is currently not active. Time until next game " << MSG_COLOR_WHITE << "[" << (int)(10 - (WorldGames.Game_BB.elapsedseconds / 60)) << " minutes]."  << MSG_COLOR_RED " If another world game is active this game will not start until that one is complete.";
		if(!player)
			sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
		else
			ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
		return;
	}
	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "[Bloodsail Booty Capture] is currently active. Time left in this game" << "[" << MSG_COLOR_WHITE << (int)(20 - (WorldGames.Game_BB.elapsedseconds / 60)) << " minutes]";
	if(!player)
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
	else
		ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
	BroadcastBloodSailBootyScore(player);
}
void uowToolSet::BroadcastAirdropStatus(Player * player)
{
	std::ostringstream msg;
	//Broadcast crater queue
	
	msg << MSG_COLOR_ORANGE << "[World Game Queue Status Update. To join the queue for ANY Battleground, Arena or World Game just use your companion helper and select the World Games Option for a list of queues you can join!]\n"; 
	//Broadcast crater queue
	if(WorldGames.Game_CC.gamestage==0)
		msg	<< MSG_COLOR_WHITE << "[Crater Capture] " <<  MSG_COLOR_RED << "Waiting for players.\n";
	else
		msg	<< MSG_COLOR_WHITE << "[Crater Capture] " <<  MSG_COLOR_GREEN << "Game in progress.\n";
	//Broadcast airdrop queue
	if(WorldGames.Game_AD.gamestage==0)
		msg	<< MSG_COLOR_WHITE << "[82nd Airdrop] " <<  MSG_COLOR_RED << "Waiting for teams.\n";
	else if(WorldGames.Game_AD.gamestage==1)
		msg	<< MSG_COLOR_WHITE << "[82nd Airdro] " <<  MSG_COLOR_YELLOW << "Last chance to join the queue! Planes are in the air!\n";
	else
		msg	<< MSG_COLOR_WHITE << "[82nd Airdro] " <<  MSG_COLOR_GREEN << "Game in progress.\n";
	if(!player)
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
	else
		ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));

}
void uowToolSet::UpdatePlayerStats(Player * player, uint32 captures, uint32 kills, uint32 deaths, uint32 cratergames, uint32 bootygames, uint32 worldkills, uint32 worlddeaths, uint32 hotzonecreaturekills, uint32 baneswon, uint32 shrinescompleted)
{
	bool hasdata = false;
    QueryResult *result = CharacterDatabase.PQuery("SELECT * from uow_player_scores where playerguid = %u",player->GetGUIDLow());
    if(result)
	{
        hasdata = true;
		delete result;
	}
	if(!hasdata)
		CharacterDatabase.DirectPExecute("INSERT INTO uow_player_scores (playerguid, captures, kills, deaths, cratergames, bootygames, worldkills, worlddeaths, hotzonecreaturekills,baneswon,shrinescompleted) VALUES (%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u)",player->GetGUIDLow(),captures,kills,deaths,cratergames,bootygames,worldkills,worlddeaths,hotzonecreaturekills, baneswon, shrinescompleted);
	else
		CharacterDatabase.DirectPExecute("UPDATE uow_player_scores SET captures=captures+%u, kills=kills+%u, deaths=deaths+%u, cratergames=cratergames+%u, bootygames=bootygames+%u, worldkills=worldkills+%u, worlddeaths=worlddeaths+%u,hotzonecreaturekills=hotzonecreaturekills+%u,baneswon=baneswon+%u , shrinescompleted=shrinescompleted+%u where playerguid=%u",captures,kills,deaths,cratergames,bootygames,worldkills,worlddeaths,hotzonecreaturekills, baneswon, shrinescompleted,player->GetGUIDLow());
	
}
void uowToolSet::BroadcastStats(StatStyleEnum style,Player * player)
{
	switch (style)
	{
	case STAT_TOPWORLDPVPKILLS:
		{
			QueryResult *result = CharacterDatabase.Query("Select Characters.Name, Uow_player_scores.worldkills from characters,Uow_player_scores where Uow_player_scores.playerguid = characters.guid order by Uow_player_scores.worldkills desc limit 0,10");
			if(result)
			{
				int count=0;
				std::ostringstream msg;
				msg << MSG_COLOR_ORANGE << "[Top 10 World Killers]";
				if(!player)
					sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
				else
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			    do
				{
					count++;
					Field *fields = result->Fetch();
					std::ostringstream msg;					
					msg << MSG_COLOR_WHITE << "[#" << count << "] " << MSG_COLOR_GREEN << fields[0].GetString() << MSG_COLOR_WHITE << " Kills: " << MSG_COLOR_GREEN << fields[1].GetInt32();
					if(!player)
						sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
					else
						ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
				} while (result->NextRow());
				delete result;
			}
		}
		break;
	case STAT_TOPWORLDPVPDEATHS:
		{
			QueryResult *result = CharacterDatabase.Query("Select Characters.Name, Uow_player_scores.worlddeaths from characters,Uow_player_scores where Uow_player_scores.playerguid = characters.guid order by Uow_player_scores.worlddeaths desc limit 0,10");
			if(result)
			{
				int count=0;
				std::ostringstream msg;
				msg << MSG_COLOR_ORANGE << "[Top 10 World Deaths]";
				if(!player)
					sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
				else
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			    do
				{
					count++;
					Field *fields = result->Fetch();
					std::ostringstream msg;
					msg << MSG_COLOR_WHITE << "[#" << count << "] " << MSG_COLOR_GREEN << fields[0].GetString() << MSG_COLOR_WHITE << " Deaths: " << MSG_COLOR_GREEN << fields[1].GetInt32();
					if(!player)
						sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
					else
						ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
				} while (result->NextRow());
				delete result;
			}
		}
		break;
	case STAT_TOPPVPKILL:
		{
			QueryResult *result = CharacterDatabase.Query("Select Characters.Name, Uow_player_scores.kills from characters,Uow_player_scores where Uow_player_scores.playerguid = characters.guid order by Uow_player_scores.kills desc limit 0,10");
			if(result)
			{
				int count=0;
				std::ostringstream msg;
				msg << MSG_COLOR_ORANGE << "[Top 10 Mini-Game Killers]";
				if(!player)
					sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
				else
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			    do
				{
					count++;
					Field *fields = result->Fetch();
					std::ostringstream msg;
					msg << MSG_COLOR_WHITE << "[#" << count << "] " << MSG_COLOR_GREEN << fields[0].GetString() << MSG_COLOR_WHITE << " Kills: " << MSG_COLOR_GREEN << fields[1].GetInt32();
					if(!player)
						sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
					else
						ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
				} while (result->NextRow());
				delete result;
			}
		}
		break;
	case STAT_TOPPVPDEATHS:
		{
			QueryResult *result = CharacterDatabase.Query("Select Characters.Name, Uow_player_scores.deaths from characters,Uow_player_scores where Uow_player_scores.playerguid = characters.guid order by Uow_player_scores.deaths desc limit 0,10");
			if(result)
			{
				int count=0;
				std::ostringstream msg;
				msg << MSG_COLOR_ORANGE << "[Top 10 Mini-Game Deaths]";
				if(!player)
					sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
				else
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			    do
				{
					count++;
					Field *fields = result->Fetch();
					std::ostringstream msg;
					msg << MSG_COLOR_WHITE << "[#" << count << "] " << MSG_COLOR_GREEN << fields[0].GetString() << MSG_COLOR_WHITE << " Deaths: " << MSG_COLOR_GREEN << fields[1].GetInt32();
					if(!player)
						sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
					else
						ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
				} while (result->NextRow());
				delete result;
			}
		}
		break;
	case STAT_TOPCAPTURES:
		{
			QueryResult *result = CharacterDatabase.Query("Select Characters.Name, Uow_player_scores.captures from characters,Uow_player_scores where Uow_player_scores.playerguid = characters.guid order by Uow_player_scores.captures desc limit 0,10");
			if(result)
			{
				int count=0;
				std::ostringstream msg;
				msg << MSG_COLOR_ORANGE << "[Top 10 Mini-Game Captures]";
				if(!player)
					sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
				else
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			    do
				{
					count++;
					Field *fields = result->Fetch();
					std::ostringstream msg;
					msg << MSG_COLOR_WHITE << "[#" << count << "] " << MSG_COLOR_GREEN << fields[0].GetString() << MSG_COLOR_WHITE << " Captures: " << MSG_COLOR_GREEN << fields[1].GetInt32();
					if(!player)
						sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
					else
						ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
				} while (result->NextRow());
				delete result;
			}
		}
		break;
	case STAT_TOPOVERALL:
		{
			QueryResult *result = CharacterDatabase.Query("Select Characters.Name, ((Uow_player_scores.captures/2) + Uow_player_scores.kills - Uow_player_scores.deaths + Uow_player_scores.worldkills - Uow_player_scores.worlddeaths + (Uow_player_scores.hotzonecreaturekills / 20) + Uow_player_scores.shrinescompleted ) as score from characters,Uow_player_scores where Uow_player_scores.playerguid = characters.guid order by score desc limit 0,10");
			if(result)
			{
				int count=0;
				std::ostringstream msg;
				msg << MSG_COLOR_ORANGE << "[Top 10 Players (Combined Scoring)]";
				if(!player)
					sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
				else
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			    do
				{
					count++;
					Field *fields = result->Fetch();
					std::ostringstream msg;
					msg << MSG_COLOR_WHITE << "[#" << count << "] " << MSG_COLOR_GREEN << fields[0].GetString() << MSG_COLOR_WHITE << " Score: " << MSG_COLOR_GREEN << fields[1].GetInt32();
					if(!player)
						sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
					else
						ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
				} while (result->NextRow());
				delete result;
			}
		}
		break;
	case STAT_TOPHZ:
		{
			QueryResult *result = CharacterDatabase.Query("Select Characters.Name, Uow_player_scores.hotzonecreaturekills from characters,Uow_player_scores where Uow_player_scores.playerguid = characters.guid order by Uow_player_scores.hotzonecreaturekills desc limit 0,10");
			if(result)
			{
				int count=0;
				std::ostringstream msg;
				msg << MSG_COLOR_ORANGE << "[Top 10 Hot Zone Creature Killers]";
				if(!player)
					sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
				else
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			    do
				{
					count++;
					Field *fields = result->Fetch();
					std::ostringstream msg;
					msg << MSG_COLOR_WHITE << "[#" << count << "] " << MSG_COLOR_GREEN << fields[0].GetString() << MSG_COLOR_WHITE << " Captures: " << MSG_COLOR_GREEN << fields[1].GetInt32();
					if(!player)
						sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
					else
						ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
				} while (result->NextRow());
				delete result;
			}
		}
		break;
	case STAT_TOPSHRINERS:
		{
			QueryResult *result = CharacterDatabase.Query("Select Characters.Name, Uow_player_scores.shrinescompleted from characters,Uow_player_scores where Uow_player_scores.playerguid = characters.guid order by Uow_player_scores.shrinescompleted desc limit 0,10");
			if(result)
			{
				int count=0;
				std::ostringstream msg;
				msg << MSG_COLOR_ORANGE << "[Top 10 Shrine Users]";
				if(!player)
					sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
				else
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			    do
				{
					count++;
					Field *fields = result->Fetch();
					std::ostringstream msg;
					msg << MSG_COLOR_WHITE << "[#" << count << "] " << MSG_COLOR_GREEN << fields[0].GetString() << MSG_COLOR_WHITE << " Shrines Completed (All 8): " << MSG_COLOR_GREEN << fields[1].GetInt32();
					if(!player)
						sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
					else
						ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
				} while (result->NextRow());
				delete result;
			}
		}
		break;
	case STAT_TOPMINIGAMERS:
		{
			QueryResult *result = CharacterDatabase.Query("Select Characters.Name, Uow_player_scores.bootygames+Uow_player_scores.cratergames as score from characters,Uow_player_scores where Uow_player_scores.playerguid = characters.guid order by score desc limit 0,10");
			if(result)
			{
				int count=0;
				std::ostringstream msg;
				msg << MSG_COLOR_ORANGE << "[Top 10 Mini-Game Users]";
				if(!player)
					sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
				else
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			    do
				{
					count++;
					Field *fields = result->Fetch();
					std::ostringstream msg;
					msg << MSG_COLOR_WHITE << "[#" << count << "] " << MSG_COLOR_GREEN << fields[0].GetString() << MSG_COLOR_WHITE << " Total Games Played: " << MSG_COLOR_GREEN << fields[1].GetInt32();
					if(!player)
						sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
					else
						ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
				} while (result->NextRow());
				delete result;
			}
		}
		break;
	}
}
void uowToolSet::Load_AirDrop()
{
	WorldGames.Game_AD.gamestage=0;
	WorldGames.Game_AD.elapsedseconds=0;
	WorldGames.Game_AD.teams.clear();
	WorldGames.Game_AD.gamezone=0;
}
void uowToolSet::PreStart_AirDrop()
{
	WorldGames.Game_AD.gamestage=1;
	WorldGames.Game_AD.elapsedseconds=0;
	std::ostringstream msg;
	msg << MSG_COLOR_ORANGE << "[82nd Airdrop] will begin in 3 minutes! Open your companion helper and select world games and then 82nd Airdrop to join. (Requires a team of 2 players only)";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
}
void uowToolSet::Queue_AirDrop(Player * player)
{	
	bool canqueue=true;
	if (WorldGames.Game_AD.gamestage > 1)
		canqueue=false;
	else if(!player->GetGroup())
		 canqueue=false;
	else if(player->GetGroup()->GetMembersCount() !=2)
		canqueue=false;
	else if(player->GetGroup()->isRaidGroup())
		canqueue=false;
	if(canqueue) //Check to make sure they are not alreadt queued.
	{
		ADTeamCollection::iterator itr = WorldGames.Game_AD.teams.find(player->GetGroup()->GetLeaderGUID());
		if( itr != WorldGames.Game_AD.teams.end() ) 
		{
			ChatHandler(player).PSendSysMessage("[82nd Airdrop] You are already queued.");
			return;
		}	
	}
	if(!canqueue)
	{
		ChatHandler(player).PSendSysMessage("[82nd Airdrop] you must be in a group of 2 people only and the game must not be active.");
		return;
	}
	//Return Mark
	Mark * ret = new Mark();
	ret->id = 0; ret->mapid = 1; ret->name="return recall"; ret->x = -11833.536133; ret->y = -4794.843750; ret->z =6.031844 ; ret->o = player->GetOrientation();
	//New Player Setup
	ADTeamDataStruct newteam;
	newteam.deaths=0; newteam.kills=0; newteam.markhome = ret; newteam.team = player->GetGroup();
	WorldGames.Game_AD.teams.insert(ADTeamCollection::value_type(player->GetGroup()->GetLeaderGUID(),newteam));
	//Team member itteration
    std::vector<Player*> TeamMembers;
    TeamMembers.reserve(player->GetGroup()->GetMembersCount());
    for(GroupReference *itr = player->GetGroup()->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* Target = itr->getSource();
		ChatHandler(Target).PSendSysMessage("[82nd Airdrop] You are now queued for 82nd Airdrop!");
        TeamMembers.push_back(Target);
    }
	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "[" << TeamMembers[0]->GetName() <<  " & " << TeamMembers[1]->GetName() << "] have joined the [82nd Airdrop] queue.";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
}
void uowToolSet::Start_AirDrop()
{
	WorldGames.Game_AD.gamestage=2;
	WorldGames.Game_AD.elapsedseconds=0;
	std::ostringstream msg;
	msg << MSG_COLOR_GREEN << "[82nd Airdrop] has begun!";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));

	uint32 gz[] = {493,41};
	WorldGames.Game_AD.gamezone = gz[Random.randInt(0)];
	CleanUpQueues();
	for (ADTeamCollection::iterator titr = WorldGames.Game_AD.teams.begin(); titr != WorldGames.Game_AD.teams.end(); ++titr)
	{
		float x=0;
		float y=0;
		float z=0;;
		switch(WorldGames.Game_AD.gamezone)
		{
		case 41:
			{
				do{x=uowTools.Random.randInt(50);}while(x<37);
				do{y=uowTools.Random.randInt(75);}while(y<21);				
			}
			break;
		case 493:
			{
				do{x=uowTools.Random.randInt(60);}while(x<34);
				do{y=uowTools.Random.randInt(72);}while(y<23);				
			}
			break;
		}
		AreaTableEntry const* current_zone = GetAreaEntryByAreaID(WorldGames.Game_AD.gamezone);
		uint32 mapid=current_zone[0].mapid;
		Map *map = MapManager::Instance().FindMap(mapid);
		Zone2MapCoordinates(x,y,WorldGames.Game_AD.gamezone);
		z=map->GetGrid(x, y)->getHeight(x,y)+ 50;
		for(GroupReference *itr = titr->second.team->GetFirstMember(); itr != NULL; itr = itr->next())
		{
			Player* Target = itr->getSource();
			//Air Drop!!!
			ChatHandler(Target).PSendSysMessage("[82nd Airdrop] Here is the situation soldier. You and your team must seek and destroy all other teams dropped into this zone. Your team has 5 minuts to score a kills or be thrown out of the game. Everytime your team scores a kill your 5 minute timer is reset. If your team sustains 2 deaths you are thrown out of the game. The last team standing wins. Good Luck!");
			Target->CastSpell(Target,53208,true,NULL,NULL,Target->GetGUID());	
			Target->CastSpell(Target,19883,true,NULL,NULL,Target->GetGUID());	
			uowTools.DismissHelper(Target); //Remove ze panda!
			Target->SendPlaySound(7756,false);
			Target->TeleportTo(mapid,x,y,z,Target->GetOrientation(),0);
			Target->SendPlaySound(7756,false);	
			uowTools.AddPlayerCooldown(CD_ADKILL,300,Target);
		}
	}
}
void uowToolSet::Update_AirDrop()
{
	if (WorldGames.Game_AD.gamestage == 0 && WorldGames.Game_AD.teams.size() < 3) // The game is not active so lets traack the time and activate as necessary
	{
		WorldGames.Game_AD.elapsedseconds=WorldGames.Game_AD.elapsedseconds + 5;
		int32 elapsedminutes=WorldGames.Game_AD.elapsedseconds / 60;
		CleanUpQueues();
		//Has 20 minutes passed? 
		if(elapsedminutes>=20)
		{
			WorldGames.Game_AD.elapsedseconds=0;
			std::ostringstream msg;
			msg << MSG_COLOR_ORANGE << "[82nd Airdrop] " << MSG_COLOR_WHITE << "Queue now for 82nd Airdrop! People are in the queue and waiting on challengers.";
			sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( )); 
		}
		return;
	}
	if (WorldGames.Game_AD.gamestage == 0 && WorldGames.Game_AD.teams.size() >= 3) // More than 3 teams, flip the countdown
		PreStart_AirDrop();
	if (WorldGames.Game_AD.gamestage == 1) // The game is in PreStart stage
	{
		WorldGames.Game_AD.elapsedseconds=WorldGames.Game_AD.elapsedseconds + 5;
		int32 elapsedminutes=WorldGames.Game_AD.elapsedseconds / 60;
		//Has 3 minutes passed? start a new game if so...
		if(elapsedminutes>=3)
			Start_AirDrop();
		return;
	}
	if (WorldGames.Game_AD.gamestage == 2)
	{
		//add a death to any team that loses the ADKILL cooldown
		for (ADTeamCollection::iterator titr = WorldGames.Game_AD.teams.begin(); titr != WorldGames.Game_AD.teams.end(); ++titr)
		{
			if(!titr->second.team)
				continue;
			//Team member itteration
			for(GroupReference *itr = titr->second.team->GetFirstMember(); itr != NULL; itr = itr->next())
			{
				Player* Target = itr->getSource();
				if(!uowTools.HasCoolDown(CD_ADKILL,Target))
				{
					ChatHandler(Target).PSendSysMessage("[82nd Airdrop] You took to long to kill someone. You have been killed for your lack of effort!");
					Target->Kill(Target,false);
					titr->second.deaths++;
				}
			}
			
			//if a team has 2 deaths set them to null for removal and teleport them out
			if(titr->second.deaths>=2)
			{
				//Team member itteration
				for(GroupReference *itr = titr->second.team->GetFirstMember(); itr != NULL; itr = itr->next())
				{
					Player* Target = itr->getSource();
					if(Target->isDead())
					{
						Target->ResurrectPlayer(1);
						Target->SpawnCorpseBones();
						Target->SaveToDB();
					}
					ChatHandler(Target).PSendSysMessage("[82nd Airdrop] Your team has been removed from [82nd Airdrop]");
					uowTools.DismissHelper(Target); //Remove ze panda!
					Target->SendPlaySound(7756,false);
					Target->TeleportTo(titr->second.markhome->mapid,titr->second.markhome->x,titr->second.markhome->y,titr->second.markhome->z,Target->GetOrientation(),0);
					Target->SendPlaySound(7756,false);	
				}
				titr->second.team=NULL;
			}
		}
		
		//throw out any deadbeats
		CleanUpQueues();

		//Winner check
		if(WorldGames.Game_AD.teams.size()<=1)
			Reward_AirDrop();
	}

}
void uowToolSet::Reward_AirDrop()
{
	WorldGames.Game_AD.gamestage=0;
	WorldGames.Game_AD.elapsedseconds=0;
	std::ostringstream msg2;
	msg2 << MSG_COLOR_GREEN << "[82nd Airdrop] has ended!";
	sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg2.str( ).c_str( ));
	//add a death to any team that loses the ADKILL cooldown
	for (ADTeamCollection::iterator titr = WorldGames.Game_AD.teams.begin(); titr != WorldGames.Game_AD.teams.end(); ++titr)
	{
		//Team member itteration
		uint32 c = 0;
		std::ostringstream msg;
		for(GroupReference *itr = titr->second.team->GetFirstMember(); itr != NULL; itr = itr->next())
		{
			c++;
			Player* Target = itr->getSource();
			ChatHandler(Target).PSendSysMessage("[82nd Airdrop] Game over, you're team won!");
			if(c==1)
				msg << MSG_COLOR_ORANGE << "[82nd Airdrop] " << MSG_COLOR_WHITE "[" << Target->GetName() << "] " << MSG_COLOR_CYAN << "has overcome all the odds with his partner ";
			else
				msg << MSG_COLOR_WHITE << "[" << Target->GetName() << "] " << MSG_COLOR_CYAN << "proving that they are indeed Internet Champions. ALL HAIL THE VICTORS!";
			if(Target->isDead())
			{
				Target->ResurrectPlayer(1);
				Target->SpawnCorpseBones();
				Target->SaveToDB();
			}
			//Reward
			uowTools.GiveItems(Target,UOW_WSGMARK,2);
			uowTools.GiveItems(Target,UOW_AVMARK,1);
			Target->ModifyMoney(150*10000);

			//Port out
			uowTools.DismissHelper(Target); //Remove ze panda!
			Target->SendPlaySound(7756,false);
			Target->TeleportTo(titr->second.markhome->mapid,titr->second.markhome->x,titr->second.markhome->y,titr->second.markhome->z,Target->GetOrientation(),0);
			Target->SendPlaySound(7756,false);	
		}
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
	}

	WorldGames.Game_AD.teams.clear();
}

void uowToolSet::ResetHonor(Player * target)
{
    target->SetUInt32Value(PLAYER_FIELD_KILLS, 0);
    target->SetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS, 0);
    target->SetUInt32Value(PLAYER_FIELD_HONOR_CURRENCY, 0);
    target->SetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, 0);
    target->SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, 0);
    target->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL);
}

void uowToolSet::ResetArenaPoints(Player * target)
{
    target->SetUInt32Value(PLAYER_FIELD_ARENA_CURRENCY, 0);
}

void uowToolSet::AtLogin(Player * player)
{
	player->m_currentselectedgo=NULL;
	player->m_resatgy=true;
	uowTools.GiveFreeStuff(player);
	//uowTools.GiveMaxRep(player);
	uowTools.LoadPlayerMarks(player);
	uowTools.LoadPlayerCooldowns(player);
	player->UpdateSkillsToMaxSkillsForLevel();
	//All players get disenchanting
	player->learnSpell(13262,false);

	//World Games
	player->RemoveAura(45450);
	if((player->GetZoneId() == 616) || (player->GetZoneId() == 33 && player->GetAreaId() == 43 ))
	{
		player->TeleportTo(player->m_homebindMapId,player->m_homebindX, player->m_homebindY, player->m_homebindZ, player->GetOrientation());
	}

	//Login Commands (DB/uow_atlogin)
	QueryResult *result = CharacterDatabase.PQuery("Select Command from uow_atlogin where playerguid=%u",player->GetGUIDLow());
	if(result)
	{
		do
		{
			Field *fields = result->Fetch();
			uint32 command = fields[0].GetUInt32();
			switch (command)
			{
			case LOGIN_RESETHONOR://Reset Honor
				ResetHonor(player);
				break;
			case LOGIN_RESETARENA://Reset Arena
				ResetArenaPoints(player);
				break;
			case LOGIN_TEMPREWARD:
				{
					//Xmas Rewards/Gifts
					uowTools.GiveItems(player,49284,1); 
					uowTools.GiveItems(player,70026,10);
					uowTools.GiveItems(player,70028,10);
					uowTools.GiveItems(player,70030,10);
					uowTools.GiveItems(player,34057,10);
					ChatHandler(player).PSendSysMessage("Happy 1 Year Aniversery from UOWOW!");
				}
				break;
			}
		} while (result->NextRow());
		delete result;
	}
	CharacterDatabase.DirectPExecute("DELETE FROM uow_atlogin where playerguid=%u",player->GetGUIDLow());

	player->UpdateZone(player->GetZoneId(), player->GetAreaId());
}
void uowToolSet::BroadcastWorldGames(Player * player)
{
	std::ostringstream msg;
	//Broadcast crater queue
	
	msg << MSG_COLOR_WHITE << "[World Game Information]\n"; 
	msg	<< MSG_COLOR_ORANGE << "[City Siege] " <<  MSG_COLOR_GREEN << "Always in progress.\n";
	msg	<< MSG_COLOR_ORANGE << "[Warsong Gulch] " <<  MSG_COLOR_YELLOW << "People are in the Queue and waiting for others to join! " <<  MSG_COLOR_WHITE << "Press H and go to the battlegrounds tab and select Warsong Gulch!\n";
	msg	<< MSG_COLOR_ORANGE << "[Arathi Basin] " <<  MSG_COLOR_YELLOW << "People are in the Queue and waiting for others to join! " <<  MSG_COLOR_WHITE << "Press H and go to the battlegrounds tab and select Arathi Basin!\n";
	//Broadcast crater queue
	if(WorldGames.Game_CC.gamestage==0)
		msg	<< MSG_COLOR_ORANGE << "[Crater Capture] " <<  MSG_COLOR_RED << "Waiting for players. " <<  MSG_COLOR_WHITE << "Use your companion pet and select world games then Crater Capture!\n";
	else
		msg	<< MSG_COLOR_ORANGE << "[Crater Capture] " <<  MSG_COLOR_GREEN << "Game in progress. " <<  MSG_COLOR_WHITE << "Use your companion pet and select world games then Crater Capture!\n";
	//Broadcast airdrop queue
	if(WorldGames.Game_AD.gamestage==0)
		msg	<< MSG_COLOR_ORANGE << "[82nd Airdrop] " <<  MSG_COLOR_RED << "Waiting for teams. " <<  MSG_COLOR_WHITE << "Use your companion pet and select world games then 82nd Airdrop!\n";
	else if(WorldGames.Game_AD.gamestage==1)
		msg	<< MSG_COLOR_ORANGE << "[82nd Airdrop] " <<  MSG_COLOR_YELLOW << "Last chance to join the queue! Planes are in the air! " <<  MSG_COLOR_WHITE << "Use your companion pet and select world games then 82nd Airdrop!\n";
	else
		msg	<< MSG_COLOR_ORANGE << "[82nd Airdrop] " <<  MSG_COLOR_GREEN << "Game in progress.\n";
	//Broadcast bootycap queue
	if(WorldGames.Game_BB.gamestage==0)
		msg	<< MSG_COLOR_ORANGE << "[Bloodsail Booty Capture] " <<  MSG_COLOR_RED << "Waiting for players. " <<  MSG_COLOR_WHITE << "Use your companion pet and select world games then Bloodsail Booty Capture!\n";
	else
		msg	<< MSG_COLOR_ORANGE << "[Bloodsail Booty Capture] " <<  MSG_COLOR_GREEN << "Game in progress.\n";
	msg << MSG_COLOR_WHITE << "The game " 	<< MSG_COLOR_ORANGE << "[" << GetGameName(WorldGames.CurrentBonusGame) << "] " << MSG_COLOR_WHITE << "is the current Bonus game running. If you play this game you will be eligable for bonus rewards for each kill and maybe even a random uber reward!";
	if(player)
		ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
	else
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
}
std::string uowToolSet::GetGameName(uint32 game)
{
	switch (game)
	{
	case GAME_WSG:
		return "Warsong Gulch";
	case GAME_ARATHIBASIN:
		return "Arathi Basin";
	case GAME_REDNECKRAMPAGE:
		return "Redneck Rampage";
	case GAME_BLOODSAILBOOTYCAPTURE:
		return "Bloodsail Booty Capture";
	case GAME_AIRDROP:
		return "82nd Airdrop";
	case GAME_CRATERCAPTURE:
		return "Crater Capture";
	case GAME_CITYSIEGE:
		return "City Siege";
	}
	return "No game selected";
}
void uowToolSet::UpdateWorldGames()
{
	WorldGames.CurrentBonusGame++;
	if(WorldGames.CurrentBonusGame==GAME_REDNECKRAMPAGE || WorldGames.CurrentBonusGame==GAME_BLOODSAILBOOTYCAPTURE)
		WorldGames.CurrentBonusGame++;
	if(WorldGames.CurrentBonusGame>6)
		WorldGames.CurrentBonusGame=0;
	//BroadcastWorldGames();
}
void uowToolSet::ToggleHomeShield(Player * player)
{
	//spin houses
	GameObject * HouseGO=NULL;
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return;
		//spin objects (looking for a shield)
		if(itr->second->type!=100 && itr->second->accountid == player->GetSession()->GetAccountId())
		{
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
				{
						GameObject * tgo=NULL;
						if (GameObjectData const* go_data = objmgr.GetGOData(itr2->second->gospawnid))
							tgo = player->GetMap()->GetGameObject(MAKE_NEW_GUID(itr2->second->gospawnid, go_data->id , HIGHGUID_GAMEOBJECT));
						if(tgo && tgo->GetEntry() == HOUSE_SHIELD)
						{
							DeleteGO(tgo->GetMap(),itr2->second->gospawnid,true);
							return;
						}
						else if(tgo && ( tgo->GetEntry() == 70125 || tgo->GetEntry() == 70126 || tgo->GetEntry() == 190373 || tgo->GetEntry() == 70106))
							HouseGO=tgo;					
				}
			//New field based on the house
			if(HouseGO)
			{
				uint32 ret = this->SpawnGO(HOUSE_SHIELD,  HouseGO->GetMap(), HouseGO->GetPositionX(), HouseGO->GetPositionY(), HouseGO->GetPositionZ()+9, 0,true);
				if (ret!=0)
					CharacterDatabase.DirectPExecute("INSERT INTO uow_player_housing_objects (houseid, gospawnid) VALUES (%u,%u)",itr->second->id,ret);
				LoadPlayerHousing();
				return;
			}
		}
 
	}
	
}
void uowToolSet::ZapHomeShield(Player * player)
{
	//spin houses
	GameObject * HouseGO=NULL;
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return;
		//spin objects (looking for a shield)
		if(itr->second->type!=100 && itr->second->accountid == player->GetSession()->GetAccountId())
		{
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
				{
					GameObject * tgo=NULL;
					if (GameObjectData const* go_data = objmgr.GetGOData(itr2->second->gospawnid))
						tgo = player->GetMap()->GetGameObject(MAKE_NEW_GUID(itr2->second->gospawnid, go_data->id , HIGHGUID_GAMEOBJECT));
					if(tgo && tgo->GetEntry() == HOUSE_SHIELD)
					{
						ChatHandler(player).PSendSysMessage("Cleansing this area of all intruders!");
						Map::PlayerList const &PlList = tgo->GetMap()->GetPlayers();
						if(PlList.isEmpty())
							return;
						for(Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
						{
							if(Player* pPlayer = i->getSource())
							{
								if(tgo->GetDistance2d(pPlayer) < 58 && !CanOpenHouseDoor(pPlayer,tgo))
								{
									ChatHandler(pPlayer).PSendSysMessage("Intruder!! You were killed by a Shield Zapper.");
									pPlayer->Kill(pPlayer,true);
								}
							}
						}
						return;
					}
				}
		}
	}	
	ChatHandler(player).PSendSysMessage("Your home shield is not active");
}
bool uowToolSet::CanBuildHere(Player * player)
{
	//spin houses
	GameObject * HouseGO=NULL;
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return false;
		//spin objects (looking for a shield)
		if(itr->second->type!=100 && itr->second->accountid == player->GetSession()->GetAccountId())
		{
			for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
				{
					GameObject * tgo=NULL;
					if (GameObjectData const* go_data = objmgr.GetGOData(itr2->second->gospawnid))
						tgo = player->GetMap()->GetGameObject(MAKE_NEW_GUID(itr2->second->gospawnid, go_data->id , HIGHGUID_GAMEOBJECT));
					if(tgo && tgo->GetEntry() == HOUSE_SHIELD && tgo->GetDistance2d(player) < 58)
						return true;
				}
		}
	}	
	return false;
}
PlayerHousingData * uowToolSet::GetHouseFromGO(GameObject * go)
{
	//spin houses
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return 0;
		//spin objects
		for(std::map<uint32, PlayerHousingObjects*>::iterator itr2 =  itr->second->objectlist.begin(); itr2 != itr->second->objectlist.end(); ++itr2)
				if(go->GetDBTableGUIDLow() == itr2->second->gospawnid)
					return itr->second;
	}
	return NULL;
}

PlayerHousingData * uowToolSet::GetHouseFromNPC(Creature * cre)
{
	//spin houses
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return 0;
		//spin objects
		for(std::map<uint32, PlayerHousingNPCS*>::iterator itr2 =  itr->second->npclist.begin(); itr2 != itr->second->npclist.end(); ++itr2)
				if(cre->GetDBTableGUIDLow() == itr2->second->npcspawnid)
					return itr->second;
	}
	return 0;
}

uint32 uowToolSet::GetNPCOwnerAccountID(Creature *cre)
{
	//spin houses
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
	{
		if( itr == PlayerHousing.end() )
			return 0;
		//spin objects
		for(std::map<uint32, PlayerHousingNPCS*>::iterator itr2 =  itr->second->npclist.begin(); itr2 != itr->second->npclist.end(); ++itr2)
				if(cre->GetDBTableGUIDLow() == itr2->second->npcspawnid)
					return itr->second->accountid;
	}
	return 0;
}

std::string uowToolSet::GetServerTime()
{
	QueryResult *result = CharacterDatabase.Query("SELECT CURTIME()");
	Field *fields = result->Fetch();
	std::string stime = fields[0].GetString();
	delete result;
	return stime;
}
uint32 uowToolSet::GetHoursUntilBane(uint32 houseid)
{
    QueryResult *result = CharacterDatabase.PQuery("SELECT TIMESTAMPDIFF(HOUR,started,now()) as elapsedtime from uow_banes where phase<1 and ended='0000-00-00 00:00:00' and houseid=%u",houseid);
	if(!result)
		return 9999;
	Field *fields = result->Fetch();
	uint32 stime = fields[0].GetUInt32();
	delete result;
	return 48-stime;
}
void uowToolSet::SetGuildCityBaneWindow(Player * player, uint32 window)
{
	std::stringstream windowstring;
	if(window==1)
		windowstring << "00:00:00";
	else if(window==2)
		windowstring << "06:00:00";
	else if(window==3)
		windowstring << "12:00:00";
	else if(window==4)
		windowstring << "18:00:00";

	if(!uowTools.GetGuildCity(player))
	{
		ChatHandler(player).PSendSysMessage("Only the owner can set this.");
		return;
	}
	if(!HasCoolDown(CD_BANEWINDOW,player))
	{
		if(GetGuildCity(player))
		{
			AddPlayerCooldown(CD_BANEWINDOW,259200,player);
			std::ostringstream msg;
			msg << "Guild bane window set to: " << windowstring.str( ).c_str( );
			ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			CharacterDatabase.DirectPExecute("UPDATE uow_player_housing set banewindow = '%s' where id = %u and type = 100", windowstring.str( ).c_str( ), GetGuildCity(player)->id); 
		}
	}
	else
	{
		ChatHandler(player).PSendSysMessage("This window has been recently changed and cannot be changed until 72 hours pass from the last set time.");
	}
}
void uowToolSet::BroadcastBanes(Player * player)
{
	if(!player)
		sWorld.SendWorldText(LANG_AUTO_BROADCAST,"[Guild City Status]");
	else
		ChatHandler(player).PSendSysMessage("[Guild City Status]");
	for (HouseCollection::const_iterator itr = PlayerHousing.begin(); itr !=  PlayerHousing.end(); ++itr)
		{
			if( itr == PlayerHousing.end() )
				return;
			if(itr->second->type==100)
			{
				std::ostringstream msg;
				Guild * g = objmgr.GetGuildById(itr->second->guildid);
				if(!g)
					continue;
				if(HasPendingBane(itr->second->id))
					msg << MSG_COLOR_CYAN << "[City - " << g->GetName() << "] " <<  MSG_COLOR_YELLOW << "has a PENDING bane. This bane will be active in " << GetHoursUntilBane(itr->second->id) << " hours." ;
				else if(HasActiveBane(itr->second->id))
					msg << MSG_COLOR_CYAN << "[City - " << g->GetName() << "] " <<  MSG_COLOR_RED << "has an ACTIVE bane. All players can attack or defend at this guild city.";
				else 
					msg << MSG_COLOR_CYAN << "[City - " << g->GetName() << "] " <<  MSG_COLOR_WHITE << "is currently clear of any attackers. No banes have been placed on this city. Any guild owning a guild city can bane this city.";
				if(!player)
					sWorld.SendWorldText(LANG_AUTO_BROADCAST,msg.str( ).c_str( ));
				else
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			}
		}
}
void uowToolSet::PlaceItemInChest(uint32 entry, uint32 count, GameObject * GO)
{
	if (entry==0)
		return;
	LootStoreItem storeitem = LootStoreItem(entry, 100,0, 7, 0, 1, count);
	GO->loot.AddItem(storeitem);
	GO->loot.unlootedCount++;
	GO->SetLootState(GO_ACTIVATED);
}
void uowToolSet::DropItemsOnDeath(Player * player, GameObject * chest)
{

	//Equipment? Gems? Enchant?
	uint32 dropcount=1;//RandomUInt(1);//1 items
	Item * dropItem=NULL;
	for (uint32 i=0;i<dropcount;i++)
	{
		uint32 count=0;
		uint32 dropSlot=0;	
		dropSlot=uowTools.Random.randInt(EQUIPMENT_SLOT_END-1);
		//Spin Start
		for(int x=dropSlot; x<EQUIPMENT_SLOT_START; x--)
		{
			dropItem=player->GetItemByPos( INVENTORY_SLOT_BAG_0, x);//->GetItemInterface()->GetInventoryItem(INVENTORY_SLOT_NOT_SET,randomSlot);
			if (dropItem!=NULL)
				if (dropItem->IsBoundAccountWide() || dropItem->IsBag())
					dropItem=NULL;
			if (dropItem!=NULL)
				break;
		}
		if (!dropItem)//Spin End
		{
			for(int x=dropSlot; x<EQUIPMENT_SLOT_END; x++)
			{
				dropItem=player->GetItemByPos( INVENTORY_SLOT_BAG_0, x);//->GetItemInterface()->GetInventoryItem(INVENTORY_SLOT_NOT_SET,randomSlot);
				if (dropItem!=NULL)
					if (dropItem->IsBoundAccountWide() || dropItem->IsBag())
						dropItem=NULL;
				if (dropItem!=NULL)
					break;
			}
		}
		/* 
		do
		{
			randomSlot=uowTools.Random.randInt(EQUIPMENT_SLOT_END-1);
			count++;
			dropItem=player->GetItemByPos( INVENTORY_SLOT_BAG_0, randomSlot);//->GetItemInterface()->GetInventoryItem(INVENTORY_SLOT_NOT_SET,randomSlot);
			if (dropItem!=NULL)
				if (dropItem->IsBoundAccountWide() || dropItem->IsBag())
					dropItem=NULL;
		}
		while((dropItem==NULL)&&(count<EQUIPMENT_SLOT_END-1));
		if ((dropItem==NULL)||(count>=EQUIPMENT_SLOT_END-1))
			continue;
		*/
		if ((dropItem==NULL))
			continue;
		std::ostringstream msg;
	
		//Special Handler for Weapons
		if(dropSlot == EQUIPMENT_SLOT_MAINHAND || dropSlot == EQUIPMENT_SLOT_OFFHAND || dropSlot == EQUIPMENT_SLOT_RANGED)
		{
			bool dropenchant = true;
			bool dropit = true;
			if(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT))//gem1
			{
				SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT));
				if(enchantEntry)
				{
					msg << MSG_COLOR_YELLOW << "[LOST GEM] " << MSG_COLOR_WHITE << enchantEntry->description[0];
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
					dropItem->ClearEnchantment(SOCK_ENCHANTMENT_SLOT);
					PlaceItemInChest(enchantEntry->GemID,1,chest);
					dropenchant=false;
				}
			}
			if(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2))//gem2
			{
				SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2));
				if(enchantEntry)
				{
					msg << MSG_COLOR_YELLOW << "[LOST GEM] " << MSG_COLOR_WHITE << enchantEntry->description[0];
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
					dropItem->ClearEnchantment(SOCK_ENCHANTMENT_SLOT_2);
					PlaceItemInChest(enchantEntry->GemID,1,chest);
					dropenchant=false;
				}
			}
			if(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3))//gem3
			{
				SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3));
				if(enchantEntry)
				{
					msg << MSG_COLOR_YELLOW << "[LOST GEM] " << MSG_COLOR_WHITE << enchantEntry->description[0];
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
					dropItem->ClearEnchantment(SOCK_ENCHANTMENT_SLOT_3);
					PlaceItemInChest(enchantEntry->GemID,1,chest);
					dropenchant=false;
				}
			}
			//Check for Enchantments
			if(dropItem->GetEnchantmentId(PERM_ENCHANTMENT_SLOT) && dropenchant)//enchants
			{
				SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(dropItem->GetEnchantmentId(PERM_ENCHANTMENT_SLOT));
				if(enchantEntry)
				{
					msg << MSG_COLOR_ORANGE << "[LOST ENCHANT] " << MSG_COLOR_WHITE << enchantEntry->description[0];
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
					dropItem->ClearEnchantment(PERM_ENCHANTMENT_SLOT);
					dropit=false;
				}
			}
			if(dropenchant==true && dropit==true)
			{
				msg << MSG_COLOR_RED << "[LOST ITEM] " << MSG_COLOR_WHITE << dropItem->GetProto()->Name1;
				ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
				player->DestroyItemCount(dropItem->GetEntry(),1,true,true);
				PlaceItemInChest(dropItem->GetEntry(),1,chest);
			}
		}
		else
		{
			if(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT))//gem1
			{
				SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT));
				if(enchantEntry)
				{
					msg << MSG_COLOR_YELLOW << "[LOST GEM] " << MSG_COLOR_WHITE << enchantEntry->description[0];
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
					dropItem->ClearEnchantment(SOCK_ENCHANTMENT_SLOT);
					PlaceItemInChest(enchantEntry->GemID,1,chest);
				}
			}
			if(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2))//gem2
			{
				SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2));
				if(enchantEntry)
				{
					msg << MSG_COLOR_YELLOW << "[LOST GEM] " << MSG_COLOR_WHITE << enchantEntry->description[0];
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
					dropItem->ClearEnchantment(SOCK_ENCHANTMENT_SLOT_2);
					PlaceItemInChest(enchantEntry->GemID,1,chest);
				}
			}
			if(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3))//gem3
			{
				SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(dropItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3));
				if(enchantEntry)
				{
					msg << MSG_COLOR_YELLOW << "[LOST GEM] " << MSG_COLOR_WHITE << enchantEntry->description[0];
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
					dropItem->ClearEnchantment(SOCK_ENCHANTMENT_SLOT_3);
					PlaceItemInChest(enchantEntry->GemID,1,chest);
				}
			}
			if(dropItem->GetEnchantmentId(PRISMATIC_ENCHANTMENT_SLOT))//prismatic
			{
				SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(dropItem->GetEnchantmentId(PRISMATIC_ENCHANTMENT_SLOT));
				if(enchantEntry)
				{
					msg << MSG_COLOR_ORANGE << "[LOST PRISMATIC] " << MSG_COLOR_WHITE << enchantEntry->description[0];
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
					dropItem->ClearEnchantment(PRISMATIC_ENCHANTMENT_SLOT);
					PlaceItemInChest(enchantEntry->GemID,1,chest);
				}
			}
			//Check for Enchantments
			if(dropItem->GetEnchantmentId(PERM_ENCHANTMENT_SLOT))//enchants
			{
				SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(dropItem->GetEnchantmentId(PERM_ENCHANTMENT_SLOT));
				if(enchantEntry)
				{
					msg << MSG_COLOR_ORANGE << "[LOST ENCHANT] " << MSG_COLOR_WHITE << enchantEntry->description[0];
					ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
					dropItem->ClearEnchantment(PERM_ENCHANTMENT_SLOT);
				}
			}
			//Item Drop
			msg << MSG_COLOR_RED << "[LOST ITEM] " << MSG_COLOR_WHITE << dropItem->GetProto()->Name1;
			ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
			player->DestroyItemCount(dropItem->GetEntry(),1,true,true);
			PlaceItemInChest(dropItem->GetEntry(),1,chest);
		}

	}

	//Bags
	std::ostringstream msg;
    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item *item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!item) continue;
        if (!item->IsBag())
        {
            if (!item->IsBoundAccountWide() && item->GetEntry() != 6948)//Drop+destroy
			{
				msg << MSG_COLOR_RED << "[LOST ITEM] " << MSG_COLOR_WHITE << item->GetProto()->Name1 << "/n";
				player->DestroyItemCount(item->GetEntry(),1,true,true);
				PlaceItemInChest(item->GetEntry(),1,chest);
			} 
        }
		else
		{
            Bag *bag = (Bag*)item;
			//Drop items from bags, but not ammo bags.
			if( bag->GetProto()->InventoryType!= INVTYPE_AMMO )
			{
				for (uint8 j = 0; j < bag->GetBagSize(); ++j)
				{
					Item* item2 = bag->GetItemByPos(j);
					if (item2 && !item2->IsBoundAccountWide() && item2->GetEntry() != 6948)//Drop+destroy
					{
						msg << MSG_COLOR_RED << "[LOST ITEM] " << MSG_COLOR_WHITE << item2->GetProto()->Name1 << "/n";
						player->DestroyItemCount(item2->GetEntry(),1,true,true);
						PlaceItemInChest(item2->GetEntry(),1,chest);
					}                 
				}
			}
		}
    }
	if(msg.str( ).c_str( )!="")
		ChatHandler(player).PSendSysMessage(msg.str( ).c_str( ));
}
void uowToolSet::CraftingSearch(Player *player, const char* text)
{
    std::string namepart = text;
    std::wstring wnamepart;
    if(!Utf8toWStr(namepart,wnamepart))
        return;
    wstrToLower(wnamepart);
	for (CraftingCollection::const_iterator itr = Craftables.begin(); itr !=  Craftables.end(); ++itr)
	{
		if( itr == Craftables.end() )
			return;			
		std::string name = itr->second.itemname;
		if (Utf8FitTo(name, wnamepart))
			ChatHandler(player).PSendSysMessage(LANG_ITEM_LIST_CHAT, itr->second.itemid, itr->second.itemid, itr->second.itemname.c_str());
	}
}
void uowToolSet::LoadCraftables()
{
    QueryResult *result = CharacterDatabase.PQuery("SELECT * from uow_craftables");
    if(!result)
        return;
    do
    {		
        Field *fields = result->Fetch();
		CraftingData craftitem;
		craftitem.itemid = fields[0].GetUInt32();
		craftitem.tradeskill = fields[1].GetUInt32();
		craftitem.skilllevel = fields[2].GetUInt32();
		craftitem.resourceid1 = fields[3].GetUInt32();
		craftitem.quantity1 = fields[4].GetUInt32();
		craftitem.resourceid2 = fields[5].GetUInt32();
		craftitem.quantity2 = fields[6].GetUInt32();
		craftitem.resourceid3 = fields[7].GetUInt32();
		craftitem.quantity3 = fields[8].GetUInt32();
		craftitem.resourceid4 = fields[9].GetUInt32();
		craftitem.quantity4 = fields[10].GetUInt32();
		craftitem.tier = fields[11].GetUInt32();
		ItemPrototype const* i = objmgr.GetItemPrototype(craftitem.itemid);
		if(i)
			craftitem.itemname = i->Name1;

		i = objmgr.GetItemPrototype(craftitem.resourceid1);
		if(i)
			craftitem.resourcename1 = i->Name1;

		i = objmgr.GetItemPrototype(craftitem.resourceid2);
		if(i)
			craftitem.resourcename2 = i->Name1;

		i = objmgr.GetItemPrototype(craftitem.resourceid3);
		if(i)
			craftitem.resourcename3 = i->Name1;

		i = objmgr.GetItemPrototype(craftitem.resourceid4);
		if(i)
			craftitem.resourcename4 = i->Name1;

		Craftables.insert(CraftingCollection::value_type(craftitem.itemid,craftitem));
    } while (result->NextRow());
    delete result;
}
