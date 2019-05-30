#ifndef _UOWOW_H
#define _UOWOW_H
#include "Common.h"
#include "UpdateFields.h"
#include "UpdateData.h"
#include "GameSystem/GridReference.h"
#include "ObjectDefines.h"
#include "GridDefines.h"
#include "Map.h"
#include "DBCStores.h"
#include "Gameobject.h"
#include <set>
#include <string>

//Uowow Declerations
#define MSG_COLOR_LIGHTRED	  "|cffff6060"
#define MSG_COLOR_LIGHTBLUE	 "|cff00ccff"
#define MSG_COLOR_BLUE		  "|cff0000ff"
#define MSG_COLOR_GREEN		 "|cff00ff00"
#define MSG_COLOR_RED		   "|cffff0000"
#define MSG_COLOR_GOLD		  "|cffffcc00"
#define MSG_COLOR_GREY		  "|cff888888"
#define MSG_COLOR_WHITE		 "|cffffffff"
#define MSG_COLOR_SUBWHITE	  "|cffbbbbbb"
#define MSG_COLOR_MAGENTA	   "|cffff00ff"
#define MSG_COLOR_YELLOW		"|cffffff00"
#define MSG_COLOR_CYAN		  "|cff00ffff"
#define MSG_COLOR_ORANGE	  "|cffff9900"

//Custom defines 
#define REWARD_XP_SHIRT 60000
#define SIEGE_GATE 175352
#define SIEGE_CONTROLUNIT 180090
#define UOW_WSGMARK 20558
#define UOW_AVMARK 20560
#define UOW_WRIT 46114 //Champion's Writ
#define INSCRIPTION_RESEARCH_BAG 70000
#define ALCHEMIST_RESEARCH_FLASK 70001
#define FELS_KITCHEN_COOKBOOK 70002
#define BADOMENS_JEWELCRAFTING_GEM 70003
#define MAJESTICS_ENCHANTING_ROD 70006
#define NOMARK_MARKER 211029
#define VEHICLEAREA_MARKER 70565
#define VEHICLEAREA_TOWER 190221
#define GUILDCITY_MARKER 179830
#define GUILDCITY_BANE 190357
#define BANE_CHARTER 70005
#define HYJAL_FLAG 300309
#define HOUSE_SHIELD 300311
#define PVPLOOT_CHEST 77816
#define ARENA_VOUCHER 70021

#pragma region "Recall Structure"
struct Mark
{
	uint32 id;
	uint32 mapid;
	std::string name;
	float x;
	float y;
	float z;
	float o;
};
typedef std::multimap<uint32,Mark*> MarkCollection;
typedef std::multimap<uint32,Mark*> GateCollection;
#pragma endregion "Recall Structure"
#pragma region "City Siege Structure"
struct CityInfo
{
	uint32 areaid;
	uint32 guildid;
	uint32 rank;
	uint32 contestedby;
	uint32 contestedmins;
	int minslefttoattack;
};
typedef std::multimap<uint32,CityInfo> CityCollection;
#pragma endregion "City Siege Structure"
#pragma region "Player Housing"

struct PlayerHousingObjects
{
	uint32 id;
	uint32 houseid;
	uint64 gospawnid;
	float tx,ty,tz;
	uint32 selectable;
};

struct PlayerHousingNPCS
{
	uint32 id;
	uint32 houseid;
	uint64 npcspawnid;
	uint32 selectable;
};
struct PlayerHousingData
{
	uint32 id;
	uint32 accountid;
	uint32 type;
	uint32 ispublic;
	uint32 isforguild;
	uint32 guildid;
	std::string banewindow;
	std::map<uint32, uint32> accesslist;
	std::map<uint32, PlayerHousingNPCS*> npclist;
	std::map<uint32, PlayerHousingObjects*> objectlist;
};
typedef std::multimap<uint32,PlayerHousingData*> HouseCollection;

#pragma endregion "Player Housing"
#pragma region "Redneck Rampage"
struct RRGameStartData
{
	uint32 id,mapid;
	float x,y,z,o;
	Player * player;
};
typedef std::map<uint32,Player*> PlayerCollection;
typedef std::multimap<uint32,RRGameStartData> RRStartingLocationCollection;
struct RRGameData
{
	uint32 gamestage;
	uint32 secondssincelastgame;
	RRStartingLocationCollection startinglocations;
	PlayerCollection queuedplayers;
};
#pragma endregion "Redneck Rampage"
#pragma region "Crater Capture"
struct CCPlayerDataStruct
{
	Mark * markhome;
	Player * player;
	uint32 captures;
	uint32 kills;
	uint32 deaths;
};
typedef std::multimap<uint32,CCPlayerDataStruct> CCPlayerCollection;
struct CCGameData
{
	Position s1,s2,s3,s4;
	uint32 gamestage;
	uint32 elapsedseconds;
	uint32 turnintrigger;
	CCPlayerCollection players;
};
#pragma endregion "Crater Capture"
#pragma region "Bloodsail BootyCapture"
struct BBPlayerDataStruct
{
	Mark * markhome;
	Player * player;
	uint32 captures;
	uint32 kills;
	uint32 deaths;
	uint32 team;
};
typedef std::multimap<uint32,BBPlayerDataStruct> BBPlayerCollection;
struct BBGameData
{
	Position RedStart;
	Position BlueStart;
	GameObject * RedFlag;
	GameObject * BlueFlag;
	GameObject * RedTurnin;
	GameObject * BlueTurnin;
	GameObject * Bubble;
	BBPlayerCollection RedTeam;
	BBPlayerCollection BlueTeam;
	uint32 gamestage;
	uint32 elapsedseconds;
	BBPlayerCollection players;
};
#pragma endregion "Bloodsail BootyCapture"
#pragma region "AirDrop"
struct ADTeamDataStruct
{
	Mark * markhome;
	Group * team;
	uint32 kills;
	uint32 deaths;
};
typedef std::multimap<uint32,ADTeamDataStruct> ADTeamCollection;
struct ADGameData
{
	uint32 gamestage;
	uint32 elapsedseconds;
	ADTeamCollection teams;
	uint32 gamezone;
};
#pragma endregion "AirDrop"

#pragma region "UOWow Tools"
enum ToolTimerEnum
{
    UPDATE_SIEGEDISPLAY = 0,
    UPDATE_HOTZONE,
	UPDATE_GENERAL,
	UPDATE_SIEGECONTROL,
	UPDATE_WORLDGAMES,
	UPDATE_BANES,
	UPDATE_EVENTCOUNT
};

enum CoolDownEnum
{
    CD_PVPDEATH = 0,
	CD_RECALL,
	CD_WSGMOBTOKEN,
	CD_SIEGESUMMON,
	CD_SHRINES,
	CD_ADKILL,
	CD_CRIMINAL,
	CD_GUILDCITYREWARDS,
	CD_BANEWINDOW,
	CD_WHO
};

enum StatStyleEnum
{
    STAT_TOPWORLDPVPKILLS = 0,
	STAT_TOPWORLDPVPDEATHS,
	STAT_TOPPVPKILL,
	STAT_TOPPVPDEATHS,
	STAT_TOPOVERALL,
	STAT_TOPCAPTURES,
	STAT_TOPHZ,
	STAT_TOPSHRINERS,
	STAT_TOPMINIGAMERS
};

enum AtLoginEnum
{
	LOGIN_RESETHONOR = 1,
	LOGIN_RESETARENA = 2,
	LOGIN_TEMPREWARD = 3
};


enum GamesEnum
{
	GAME_WSG = 0,
	GAME_ARATHIBASIN = 1,
	GAME_REDNECKRAMPAGE = 2,
	GAME_BLOODSAILBOOTYCAPTURE = 3,
	GAME_AIRDROP = 4,
	GAME_CRATERCAPTURE = 5,
	GAME_CITYSIEGE = 6
};
struct WorldGameData
{
	CityCollection SiegeCities;
	uint32 CurrentHotZone;
	uint32 CurrentSiegeCity;
	uint32 CurrentBonusGame;
	RRGameData Game_RR;
	CCGameData Game_CC;
	BBGameData Game_BB;
	ADGameData Game_AD;
};
struct CraftingData
{
	uint32 itemid;
	std::string itemname;
	uint32 resourceid1;
	uint32 resourceid2;
	uint32 resourceid3;
	uint32 resourceid4;
	std::string resourcename1;
	std::string resourcename2;
	std::string resourcename3;
	std::string resourcename4;
	uint32 quantity1;
	uint32 quantity2;
	uint32 quantity3;
	uint32 quantity4;
	uint32 tradeskill;
	uint32 skilllevel;
	uint32 tier;
};

typedef std::multimap<uint32,CraftingData> CraftingCollection;
typedef std::multimap<uint32,IntervalTimer*> CoolDownCollection;
typedef std::multimap<uint32,IntervalTimer*> EventCollection;
typedef std::map<std::string,std::string> PlayerVariablesCollection;
class TRINITY_DLL_SPEC uowToolSet
{
public:
	~uowToolSet();
	CraftingCollection Craftables;
	WorldGameData WorldGames;
	GateCollection ActiveGates;
	void LoadPlayerMarks(Player * player);
	void MakeMark(Player* player);
	void SelectMark(uint32 id, Player * player);
	void LoadCityData();
	void BroadcastSiegeStatus(uint32 areaid,Player * player=NULL);
	bool IsSiegeCity(uint32 areaid);
	void BroadcastChatHelp(Player * player=NULL);
	void BroadcastHotZone(Player * player=NULL);
	void BroadcastServerInfo();
	void GiveMaxRep(Player* player);
	void GiveFreeStuff(Player* player);
	void AnnounceKill(Player * killer, Player * killed);
	void Update(uint32 diff);
	void Initialize();
	EventCollection Events;
	void UpdateEvents(uint32 diff);
	void InitizlizeEvents();
	void UpdateHotZone();
	bool OpenSiegeDoor(Player *player, GameObject *gameobject);
	bool UseSiegeControl(Player *player, GameObject *gameobject);
	void UpdatePlayerCoolDowns(Player *player);
	bool HasCoolDown(uint32 CoolDownID, Player * player);
	void AddPlayerCooldown(uint32 CoolDownID, uint32 seconds, Player * player);
	void BroadcastAttackbleCity();
	void SetAttackableCity();
	void UpdateCityBattle();
	void RewardSiegeWinners(uint32 guildid, uint32 areaid, bool defenders);
	void GiveItems(Player * player, uint32 itementry, uint32 quantity);
	bool HasMats(Player * player, uint32 itementry, uint32 quantity);
	void CraftItem(Player * player, uint32 itemid);
	GameObject * SpawnTempGO(uint32 entry, Map *map ,float x,float y,float z,float o , uint32 despawntime,uint64 ownerguid);
	uint32 SpawnTempNPC(uint32 entry, Map *map ,float x,float y,float z,float o, uint32 despawntime, Player * Owner);
	uint32 SpawnGO(uint32 entry, Map *map ,float x,float y,float z,float o,bool SaveToDB);
	uint32 SpawnNPC(uint32 entry, Map *map ,float x,float y,float z,float o, bool SaveToDB);
	void LoadPlayerHousing();
	void MakeHouse(uint32 housetype, Player * player);
	float GetHeight(float x, float y, Player * player);
	float GetHeight(float x, float y, float z, Map  * map);
	HouseCollection PlayerHousing;
	bool CanOpenHouseDoor(Player *player, GameObject *gameobject);
	bool DeleteNPC(Map* map, uint32 lowguid,bool RemoveFromDB);
	bool DeleteGO(Map* map , uint32 lowguid,bool RemoveFromDB);
	bool HasHouse(Player * player);
	PlayerHousingData* GetHouse(Player * player);
	void ToggleHousePublic(Player * player, bool GuildCity);
	void ToggleHouseForGuild(Player * player);
	void Recall(Player * player, bool bypassrules);
	bool CanRecall(Player * player, Mark * mark , uint32 item, uint32 quantity);
	void MakeGate(Player * player);
	void UseGate(Player * player, uint32 GateGUID);
	void LoadPlayerCooldowns(Player * player);
	void SavePlayerCooldowns(Player * player);
	void DemolishHome(Player * Owner, bool GuildCity);
	void RepairBuildings(Player * Owner);
	void RemoveBuilding(GameObject * go);
	void UseHouseTeleporter(Player *player, GameObject *gameobject);
	void AddToHouseList(Player * Owner);
	void RemoveFromHouseList(Player * Owner);
	void ChangeOwner(Player * Owner, bool ishouse);
	int32 GetTokenCount(Player * player);
	void GetToken(Player * player);
	void GiveAllGlyphs(Player * player);
	void GiveAllAlchemy(Player * player);
	void GiveAllCooking(Player * player);
	void GiveAllJewelcrafting(Player * player);
	void PreStart_RedneckRampage();
	void Start_RedneckRampage();
	void Update_RedneckRampage();
	void Reward_RedneckRampage();
	void Load_RedneckRampage();
	void Queue_RedneckRampage(Player * player);
	void CleanUpQueues();
	void UpdatePlayer(Player *player);
	void UpdatePlayerVehicle(Player *player);
	PlayerHousingData* GetGuildCity(uint32 houseid);
	PlayerHousingData* GetGuildCity(Player * player);
	PlayerHousingData* GetGuildCityByGO(uint32 gospawnid);
	GameObject * GetNearestOwnedGO(Player *player, bool honorselectable);
	GameObject * GetNearestGOOwnedByOther(Player *player);
	bool PlaceHousingObject(Player *player , uint32 entry, int cost, int maxcount, bool isplayerhouse=false, bool isselectable=false);
	bool PlaceHousingNPC(Player * player, uint32 entry, int cost, int maxcount,bool isplayerhouse=false, bool isselectable=false);
	void ReorientObject(Player * player, GameObject * go);
	void ReorientNPC(Player * player, Creature * cre);
	bool OwnsBuilding(Player* player, uint32 gospawnid);
	GameObject * GetGateHolder(GameObject * gatego);
	PlayerHousingObjects * GetGuildCityObject(uint32 gospawnid);
	PlayerHousingData* GetGuildCityByGuildID(uint32 guildid);
	void BaneCity(Player *player);
	void AcceptBane(uint32 houseid);
	bool HasPendingBane(uint32 houseid);
	bool HasActiveBane(uint32 houseid);
	void UpdateBanes();
	uint32 GetGuildMembersOnline(uint32 guildid);
	uint32 GetBaneID(GameObject * bane);
	void RewardBaneDefenders(GameObject * go);
	uint32 GetHousingNPCCount(uint32 houseid, uint32 entry);
	uint32 GetHousingObjectCount(uint32 houseid);
	void DemolishGuildCity(uint32 houseid, Map *map);
	void RewardBaneAttackers(uint32 houseid, GameObject * cityflag);
	void SummonBattleGO(Player * player, uint32 entry, uint32 dspawntime, int32 cost, bool ignorecooldown);
	void SummonBattleNPC(Player * player, uint32 entry, uint32 dspawntime, int32 cost, bool ignorecooldown);
	void PruchaseItems(Player * player, uint32 entry, int32 quantity, int32 cost);
	GameObject * GetNearestBane(Player *player);
	GameObject * GetGuildCityFlag(Player * player);
	uint32 GetHousingObjectCount(uint32 houseid, uint32 entry);
	uint32 GetAccountIDByCharacterName(std::string name);
	void RewardShrine(Player * player);
	void GiveAllEnchanting(Player * player);
	int32 GetRandomInt(int32 data[]);
	void GetRandomReward(int32 data[],int32& entry, int32& quantity);
	MarkCollection CommonMarks;
	void InitializeCommonMarks();
	Mark * GetCommonMark(uint32 id);
	int32 GetDistance2D(float x1, float y1,float x2, float y2);
	int32 GetGuildCityFlagDistance(uint32 houseid, Player * player);
	void DismissHelper(Player * player);
	void Queue_CraterCapture(Player * player);
	void TeleportTo_CraterCapture(Player * player);
	void Load_CraterCapture();
	void Start_CraterCapture();
	void Update_CraterCapture();
	void Reward_CraterCapture();
	void Remove_CraterCapture(Player * player);
	MTRand Random;
	void BroadcastCraterScore(Player * player = NULL);
	void BroadcastCraterStatus(Player * player = NULL);
	void Load_BloodSailBootyCapture();
	void Queue_BloodSailBootyCapture(Player * player);
	void Remove_BloodSailBootyCapture(Player * player);
	void Start_BloodSailBootyCapture();
	void Update_BloodSailBootyCapture();
	void Reward_BloodSailBootyCapture();
	void BroadcastBloodSailBootyScore(Player * player = NULL);
	void BroadcastBloodSailBootyStatus(Player * player = NULL);
	void UpdatePlayerStats(Player * player, uint32 captures=0, uint32 kills=0, uint32 deaths=0, uint32 cratergames=0, uint32 bootygames=0, uint32 worldkills=0, uint32 worlddeaths=0, uint32 hotzonecreaturekills=0, uint32 baneswon=0, uint32 shrinescompleted=0);
	void BroadcastStats(StatStyleEnum style,Player * player = NULL);
	void Load_AirDrop();
	void PreStart_AirDrop();
	void Queue_AirDrop(Player * player);
	void Start_AirDrop();
	void Update_AirDrop();
	void Reward_AirDrop();
	void Remove_AirDrop(Player * player);
	void ResetHonor(Player * target);
	void ResetArenaPoints(Player * target);
	void AtLogin(Player * player);
	void BroadcastWorldGames(Player * player = NULL);
	void BroadcastAirdropStatus(Player * player = NULL);
	void UpdateWorldGames();
	std::string GetGameName(uint32 game);
	void ToggleHomeShield(Player * player);
	void ZapHomeShield(Player * player);
	bool CanBuildHere(Player * player);
	PlayerHousingData * GetHouseFromGO(GameObject * go);
	PlayerHousingData * GetHouseFromNPC(Creature * cre);
	Creature * GetNearestOwnedNPC(Player *player, bool honorselectable);
	uint32 GetNPCOwnerAccountID(Creature *cre);
	void ClaimGuildCityRewards(Player * player);
	std::string GetServerTime();
	void SetGuildCityBaneWindow(Player * player, uint32 window);
	uint32 GetHoursUntilBane(uint32 houseid);
	void BroadcastBanes(Player * player=NULL);
	bool isinBloodSailBootyCapture(Player * player);
	void Remove_AllQueues(Player * player);
	void TeleportTo_BootyCapture(BBPlayerDataStruct playerdata);
	void PlaceItemInChest(uint32 entry, uint32 count, GameObject * GO);
	void DropItemsOnDeath(Player * player, GameObject * chest);
	void CraftingSearch(Player *player, const char* text);
	void LoadCraftables();
};
#define uowTools TRINITY_DLL_DECL Trinity::Singleton<uowToolSet>::Instance()
#pragma endregion "UOWow Tools"

#endif