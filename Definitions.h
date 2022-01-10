#pragma once

#include <vector>
#include <iostream>
#include <string>
#define RandomInt(min, max) (rand() % (max - min + 1) + min)

struct ChampMinimal
{
	bool active;
	std::string alias;
	std::string banVoPath;
	std::string baseLoadScreenPath;
	bool botEnabled;
	std::string chooseVoPath;
	//disabledQueues
	bool freeToPlay;
	int id;
	std::string name;

	//ownership
	bool freeToPlayReward;
	int owned;

	std::string purchased;
	bool rankedPlayEnabled;
	std::pair<std::string, std::string>roles;
	std::string squarePortraitPath;
	std::string stingerSfxPath;
	std::string title;
};

struct ChampMastery
{
	int championId;
	int championLevel;
	int championPoints = 0;
	int championPointsSinceLastLevel;
	int championPointsUntilNextLevel;
	bool chestGranted;
	std::string formattedChampionPoints;
	std::string formattedMasteryGoal;
	std::string highestGrade;
	std::string lastPlayTime;
	std::string playerId;
	int tokensEarned;
};

struct ChampAll
{
	ChampMinimal min;
	ChampMastery mas;
};

inline std::vector<ChampMinimal>champsMinimal;
inline std::vector<ChampMastery>champsMastery;
inline std::vector<ChampAll>champsAll;

struct Skin
{
	std::string name;
	std::string inventoryType;
	int itemId;
	std::string ownershipType;
	bool isVintage;
	tm purchaseDate;
	int qunatity;
	std::string uuid;
};

inline std::vector<Skin>ownedSkins;






struct Champ
{
	int key;
	std::string name;
	std::vector < std::pair<std::string, std::string>>skins;
};

inline std::vector<Champ>champSkins;
