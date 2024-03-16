#include "global.h"
#include "wild_encounter.h"
#include "pokemon.h"
#include "metatile_behavior.h"
#include "day_night.h"
#include "fieldmap.h"
#include "random.h"
#include "field_player_avatar.h"
#include "event_data.h"
#include "safari_zone.h"
#include "overworld.h"
#include "pokeblock.h"
#include "battle_setup.h"
#include "roamer.h"
#include "rtc.h"
#include "tv.h"
#include "link.h"
#include "script.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "constants/abilities.h"
#include "constants/game_stat.h"
#include "constants/items.h"
#include "constants/layouts.h"
#include "constants/weather.h"
#include "pokedex.h"
#include "power.h"
#include "constants/power.h"
#include "constants/metatile_behaviors.h"
#include "fldeff.h"
#include "constants/battle.h"
#include "battle.h"
#include "constants/pokemon.h"
#include "item.h"

extern const u8 EventScript_RepelWoreOff[];

#define MAX_RSE_ENCOUNTER_RATE 2880
#define MAX_FRLG_ENCOUNTER_RATE 1600

#define NUM_FEEBAS_SPOTS 6

// Number of accessible fishing spots in each section of Route 119
// Each section is an area of the route between the y coordinates in sRoute119WaterTileData
#define NUM_FISHING_SPOTS_1 131
#define NUM_FISHING_SPOTS_2 167
#define NUM_FISHING_SPOTS_3 149
#define NUM_FISHING_SPOTS (NUM_FISHING_SPOTS_1 + NUM_FISHING_SPOTS_2 + NUM_FISHING_SPOTS_3)

enum {
    WILD_AREA_LAND,
    WILD_AREA_WATER,
    WILD_AREA_ROCKS,
    WILD_AREA_FISHING,
};

#define WILD_CHECK_REPEL    (1 << 0)
#define WILD_CHECK_ABILITY  (1 << 1)

#define HEADER_NONE 0xFFFF

static u16 FeebasRandom(void);
static void FeebasSeedRng(u16 seed);
static bool8 IsWildLevelAllowedByRepel(u8 level);
static void ApplyFluteEncounterRateMod(u32 *encRate);
static void ApplyCleanseTagEncounterRateMod(u32 *encRate);
static bool8 TryGetAbilityInfluencedWildMonIndex(const struct WildPokemon *wildMon, u8 type, u8 ability, u8 *monIndex, u32 size);
static bool8 IsAbilityAllowingEncounter(u8 level);
static bool8 TryToScopeSpecies(const struct WildPokemon *wildMon, u8 *monIndex, u32 size);
static u8 ChooseWildMonIndex_Land(void);
static u8 ChooseWildMonIndex_WaterRock(void);

static u32 GenerateUnownPersonalityByLetter(u8 letter);
static u8 GetUnownLetterByPersonalityLoByte(u32 personality);

static u8 GenerateEncounter(u16 headerId, u16 curMetatileBehavior, u16 prevMetatileBehavior, const struct WildPokemonInfo *wildPokemonInfo, u8 terrain, u8 partySlot);

EWRAM_DATA static u8 sWildEncountersDisabled = 0;
EWRAM_DATA static u32 sFeebasRngValue = 0;
EWRAM_DATA u8 gChainStreak = 0;
EWRAM_DATA u16 gLastEncounteredSpecies = 0;
EWRAM_DATA static u8 sPreviousEncounterZoneDirection = DIR_NORTH;
EWRAM_DATA static u8 sEncounterZoneDirectionReversals = 0;
EWRAM_DATA u8 gEncounterMode = ENCOUNTER_EMERALD;
EWRAM_DATA static u16 sEncounterRateBuff = 0;
EWRAM_DATA static u8 sMaxLevel = 0;
EWRAM_DATA static u32 sIterations = 0;

#include "data/wild_encounters_rs.h"
#include "data/wild_encounters_frlg.h"
#include "data/wild_encounters_e.h"
#include "data/wild_encounters.h"

static const struct WildPokemon sWildFeebas = {20, 25, SPECIES_FEEBAS};

static const u16 sRoute119WaterTileData[] =
{
//yMin, yMax, numSpots in previous sections
     0,  45,  0,
    46,  91,  NUM_FISHING_SPOTS_1,
    92, 139,  NUM_FISHING_SPOTS_1 + NUM_FISHING_SPOTS_2,
};

//Unown table imported from FRLG
static const u8 sUnownLetterSlots[][12] = {
  //  A   A   A   A   A   A   A   A   A   A   A   ?
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 27},
  //  C   C   C   D   D   D   H   H   H   U   U   O
    { 2,  2,  2,  3,  3,  3,  7,  7,  7, 20, 20, 14},
  //  N   N   N   N   S   S   S   S   I   I   E   E
    {13, 13, 13, 13, 18, 18, 18, 18,  8,  8,  4,  4},
  //  P   P   L   L   J   J   R   R   R   Q   Q   Q
    {15, 15, 11, 11,  9,  9, 17, 17, 17, 16, 16, 16},
  //  Y   Y   T   T   G   G   G   F   F   F   K   K
    {24, 24, 19, 19,  6,  6,  6,  5,  5,  5, 10, 10},
  //  V   V   V   W   W   W   X   X   M   M   B   B
    {21, 21, 21, 22, 22, 22, 23, 23, 12, 12,  1,  1},
  //  Z   Z   Z   Z   Z   Z   Z   Z   Z   Z   Z   !
    {25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26},
};

// code
void DisableWildEncounters(bool8 disabled)
{
    sWildEncountersDisabled = disabled;
}

// Each fishing spot on Route 119 is given a number between 1 and NUM_FISHING_SPOTS inclusive.
// The number is determined by counting the valid fishing spots left to right top to bottom.
// The map is divided into three sections, with each section having a pre-counted number of
// fishing spots to start from to avoid counting a large number of spots at the bottom of the map.
// Note that a spot is considered valid if it is surfable and not a waterfall. To exclude all
// of the inaccessible water metatiles (so that they can't be selected as a Feebas spot) they
// use a different metatile that isn't actually surfable because it has MB_NORMAL instead.
// This function is given the coordinates and section of a fishing spot and returns which number it is.
static u16 GetFeebasFishingSpotId(s16 targetX, s16 targetY, u8 section)
{
    u16 x, y;
    u16 yMin = sRoute119WaterTileData[section * 3 + 0];
    u16 yMax = sRoute119WaterTileData[section * 3 + 1];
    u16 spotId = sRoute119WaterTileData[section * 3 + 2];

    for (y = yMin; y <= yMax; y++)
    {
        for (x = 0; x < gMapHeader.mapLayout->width; x++)
        {
            u8 behavior = MapGridGetMetatileBehaviorAt(x + MAP_OFFSET, y + MAP_OFFSET);
            if (MetatileBehavior_IsSurfableAndNotWaterfall(behavior) == TRUE)
            {
                spotId++;
                if (targetX == x && targetY == y)
                    return spotId;
            }
        }
    }
    return spotId + 1;
}

static bool8 CheckFeebas(void)
{
    u8 i;
    u16 feebasSpots[NUM_FEEBAS_SPOTS];
    s16 x, y;
    u8 route119Section = 0;
    u16 spotId;

    if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(ROUTE119)
     && gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE119))
    {
        GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
        x -= MAP_OFFSET;
        y -= MAP_OFFSET;

        // Get which third of the map the player is in
        if (y >= sRoute119WaterTileData[3 * 0 + 0] && y <= sRoute119WaterTileData[3 * 0 + 1])
            route119Section = 0;
        if (y >= sRoute119WaterTileData[3 * 1 + 0] && y <= sRoute119WaterTileData[3 * 1 + 1])
            route119Section = 1;
        if (y >= sRoute119WaterTileData[3 * 2 + 0] && y <= sRoute119WaterTileData[3 * 2 + 1])
            route119Section = 2;

        // 50% chance of encountering Feebas (assuming this is a Feebas spot)
        if (Random() % 100 > 49)
            return FALSE;

        FeebasSeedRng(gSaveBlock1Ptr->dewfordTrends[0].rand);

        // Assign each Feebas spot to a random fishing spot.
        // Randomness is fixed depending on the seed above.
        for (i = 0; i != NUM_FEEBAS_SPOTS;)
        {
            feebasSpots[i] = FeebasRandom() % NUM_FISHING_SPOTS;
            if (feebasSpots[i] == 0)
                feebasSpots[i] = NUM_FISHING_SPOTS;

            // < 1 below is a pointless check, it will never be TRUE.
            // >= 4 to skip fishing spots 1-3, because these are inaccessible
            // spots at the top of the map, at (9,7), (7,13), and (15,16).
            // The first accessible fishing spot is spot 4 at (18,18).
            if (feebasSpots[i] < 1 || feebasSpots[i] >= 4)
                i++;
        }

        // Check which fishing spot the player is at, and see if
        // it matches any of the Feebas spots.
        spotId = GetFeebasFishingSpotId(x, y, route119Section);
        for (i = 0; i < NUM_FEEBAS_SPOTS; i++)
        {
            if (spotId == feebasSpots[i])
                return TRUE;
        }
    }
    return FALSE;
}

static u16 FeebasRandom(void)
{
    sFeebasRngValue = ISO_RANDOMIZE2(sFeebasRngValue);
    return sFeebasRngValue >> 16;
}

static void FeebasSeedRng(u16 seed)
{
    sFeebasRngValue = seed;
}

// LAND_WILD_COUNT
static u8 ChooseWildMonIndex_Land(void)
{
	u8 wildMonIndex = 0;
	bool8 swap = FALSE;
    u8 rand = Random() % ENCOUNTER_CHANCE_LAND_MONS_TOTAL;

    if (rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_0)
        wildMonIndex = 0;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_0 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_1)
        wildMonIndex = 1;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_1 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_2)
        wildMonIndex = 2;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_2 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_3)
        wildMonIndex = 3;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_3 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_4)
        wildMonIndex = 4;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_4 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_5)
        wildMonIndex = 5;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_5 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_6)
        wildMonIndex = 6;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_6 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_7)
        wildMonIndex = 7;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_7 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_8)
        wildMonIndex = 8;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_8 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_9)
        wildMonIndex = 9;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_9 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_10)
        wildMonIndex = 10;
    else
        wildMonIndex = 11;
	
	//if (gPowerType == POWER_LUCKY && gPowerTime > 0)
	//{
	//	switch (gPowerLevel)
	//	{
	//		case 1:
	//			if (Random() % 10 < 2)
	//				swap = TRUE;
	//			break;
	//		case 2:
	//			if (Random() % 10 < 5)
	//				swap = TRUE;
	//			break;
	//		case 3:
	//			swap = TRUE;
	//			break;
	//	}
	//}
	
	//if (swap)
	//	wildMonIndex = 11 - wildMonIndex;
	
	return wildMonIndex;
}

// ROCK_WILD_COUNT / WATER_WILD_COUNT
static u8 ChooseWildMonIndex_WaterRock(void)
{
	u8 wildMonIndex = 0;
	bool8 swap = FALSE;
    u8 rand = Random() % ENCOUNTER_CHANCE_WATER_MONS_TOTAL;

    if (rand < ENCOUNTER_CHANCE_WATER_MONS_SLOT_0)
        wildMonIndex = 0;
    else if (rand >= ENCOUNTER_CHANCE_WATER_MONS_SLOT_0 && rand < ENCOUNTER_CHANCE_WATER_MONS_SLOT_1)
        wildMonIndex = 1;
    else if (rand >= ENCOUNTER_CHANCE_WATER_MONS_SLOT_1 && rand < ENCOUNTER_CHANCE_WATER_MONS_SLOT_2)
        wildMonIndex = 2;
    else if (rand >= ENCOUNTER_CHANCE_WATER_MONS_SLOT_2 && rand < ENCOUNTER_CHANCE_WATER_MONS_SLOT_3)
        wildMonIndex = 3;
    else
        wildMonIndex = 4;
	
		
	//if (gPowerType == POWER_LUCKY && gPowerTime > 0)
	//{
	//	switch (gPowerLevel)
	//	{
	//		case 1:
	//			if (Random() % 10 < 2)
	//				swap = TRUE;
	//			break;
	//		case 2:
	//			if (Random() % 10 < 5)
	//				swap = TRUE;
	//			break;
	//		case 3:
	//			swap = TRUE;
	//			break;
	//	}
	//}
	
	//if (swap)
	//	wildMonIndex = 4 - wildMonIndex;
	
	return wildMonIndex;
}

// FISH_WILD_COUNT
static u8 ChooseWildMonIndex_Fishing(u8 rod)
{
    u8 wildMonIndex = 0;
	bool8 swap = FALSE;
    u8 rand = Random() % max(max(ENCOUNTER_CHANCE_FISHING_MONS_OLD_ROD_TOTAL, ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_TOTAL),
                             ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_TOTAL);

	//if (gPowerType == POWER_LUCKY && gPowerTime > 0)
	//{
	//	switch (gPowerLevel)
	//	{
	//		case 1:
	//			if (Random() % 10 < 2)
	//				swap = TRUE;
	//			break;
	//		case 2:
	//			if (Random() % 10 < 5)
	//				swap = TRUE;
	//			break;
	//		case 3:
	//			swap = TRUE;
	//			break;
	//	}
	//}

    switch (rod)
    {
    case OLD_ROD:
        if (rand < ENCOUNTER_CHANCE_FISHING_MONS_OLD_ROD_SLOT_0)
            wildMonIndex = 0;
        else
            wildMonIndex = 1;
		
		//if (swap)
		//	wildMonIndex = 1 - wildMonIndex;
        break;
    case GOOD_ROD:
        if (rand < ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_SLOT_2)
            wildMonIndex = 2;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_SLOT_2 && rand < ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_SLOT_3)
            wildMonIndex = 3;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_SLOT_3 && rand < ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_SLOT_4)
            wildMonIndex = 4;
		
		//if (swap)
		//	wildMonIndex = 6 - wildMonIndex;
        break;
    case SUPER_ROD:
        if (rand < ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_5)
            wildMonIndex = 5;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_5 && rand < ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_6)
            wildMonIndex = 6;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_6 && rand < ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_7)
            wildMonIndex = 7;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_7 && rand < ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_8)
            wildMonIndex = 8;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_8 && rand < ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_9)
            wildMonIndex = 9;
		
		//if (swap)
		//	wildMonIndex = 14 - wildMonIndex;
        break;
    }
    return wildMonIndex;
}

static u8 ChooseWildMonLevel(const struct WildPokemon *wildPokemon)
{
    u8 min;
    u8 max;
    u8 range;
    u8 rand;

    // Make sure minimum level is less than maximum level
    if (wildPokemon->maxLevel >= wildPokemon->minLevel)
    {
        min = wildPokemon->minLevel;
        max = wildPokemon->maxLevel;
    }
    else
    {
        min = wildPokemon->maxLevel;
        max = wildPokemon->minLevel;
    }

    sMaxLevel = max;
    range = max - min + 1;
    rand = Random() % range;

    // check ability for max level mon
    if (gEncounterMode == ENCOUNTER_EMERALD && !GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
    {
        u8 ability = GetMonAbility(&gPlayerParty[0]);
        if (ability == ABILITY_HUSTLE || ability == ABILITY_VITAL_SPIRIT || ability == ABILITY_PRESSURE)
        {
            if (Random() % 2 == 0)
                return max;

            if (rand != 0)
                rand--;
        }
    }
    return min + rand;
}

static u16 GetRubyWildMonHeaderId(void)
{
    u16 i;

    for (i = 0; ; i++)
    {
        const struct WildPokemonHeader *wildHeader = &gWildMonHeadersRS[i];
        if (wildHeader->mapGroup == MAP_GROUP(UNDEFINED))
            break;

        if (gWildMonHeadersRS[i].mapGroup == gSaveBlock1Ptr->location.mapGroup &&
            gWildMonHeadersRS[i].mapNum == gSaveBlock1Ptr->location.mapNum)
        {

            return i;
        }
    }

    return HEADER_NONE;
}

static u16 GetCorrespondingSapphireWildMonHeaderId(u16 currentId)
{
    u16 i;

    for (i = currentId + 1; ; i++)
    {
        const struct WildPokemonHeader *wildHeader = &gWildMonHeadersRS[i];
        if (wildHeader->mapGroup == MAP_GROUP(UNDEFINED))
            break;

        if (gWildMonHeadersRS[i].mapGroup == gSaveBlock1Ptr->location.mapGroup &&
            gWildMonHeadersRS[i].mapNum == gSaveBlock1Ptr->location.mapNum)
        {

            return i;
        }
    }

    return HEADER_NONE;
}

static u16 GetFireRedWildMonHeaderId(void)
{
    u16 i;
    
    for (i = 0; ; i++)
    {
        const struct WildPokemonHeader *wildHeader = &gWildMonHeadersFRLG[i];

        if (wildHeader->mapGroup == MAP_GROUP(UNDEFINED))
            break;
    
        if (gWildMonHeadersFRLG[i].mapGroup == gSaveBlock1Ptr->location.mapGroup &&
            gWildMonHeadersFRLG[i].mapNum == gSaveBlock1Ptr->location.mapNum)
        {
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(KANTO_SIX_ISLAND_ALTERING_CAVE) &&
                gSaveBlock1Ptr->location.mapNum == MAP_NUM(KANTO_SIX_ISLAND_ALTERING_CAVE))
            {
                u16 alteringCaveId = VarGet(VAR_ALTERING_CAVE_WILD_SET);
                if (alteringCaveId >= NUM_ALTERING_CAVE_TABLES)
                    alteringCaveId = 0;
    
                i += alteringCaveId;
            }
    
            return i;
        }
    }
    
    return HEADER_NONE;
}

static u16 GetCorrespondingLeafGreenWildMonHeaderId(u16 currentId)
{
    u16 i;
    
    for (i = currentId + 1; ; i++)
    {
        const struct WildPokemonHeader *wildHeader = &gWildMonHeadersFRLG[i];
        if (wildHeader->mapGroup == MAP_GROUP(UNDEFINED))
            break;
    
        if (gWildMonHeadersFRLG[i].mapGroup == gSaveBlock1Ptr->location.mapGroup &&
            gWildMonHeadersFRLG[i].mapNum == gSaveBlock1Ptr->location.mapNum)
        {
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(KANTO_SIX_ISLAND_ALTERING_CAVE) &&
                gSaveBlock1Ptr->location.mapNum == MAP_NUM(KANTO_SIX_ISLAND_ALTERING_CAVE))
            {
                u16 alteringCaveId = VarGet(VAR_ALTERING_CAVE_WILD_SET);
                if (alteringCaveId >= NUM_ALTERING_CAVE_TABLES)
                    alteringCaveId = 0;
    
                i += alteringCaveId;
            }
    
            return i;
        }
    }
    
    return HEADER_NONE;
}

static u16 GetEmeraldWildMonHeaderId(void)
{
    u16 i;

    for (i = 0; ; i++)
    {
        const struct WildPokemonHeader *wildHeader = &gWildMonHeadersE[i];
        if (wildHeader->mapGroup == MAP_GROUP(UNDEFINED))
            break;

        if (gWildMonHeadersE[i].mapGroup == gSaveBlock1Ptr->location.mapGroup &&
            gWildMonHeadersE[i].mapNum == gSaveBlock1Ptr->location.mapNum)
        {
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(ALTERING_CAVE) &&
                gSaveBlock1Ptr->location.mapNum == MAP_NUM(ALTERING_CAVE))
            {
                u16 alteringCaveId = VarGet(VAR_ALTERING_CAVE_WILD_SET);
                if (alteringCaveId >= NUM_ALTERING_CAVE_TABLES)
                    alteringCaveId = 0;

                i += alteringCaveId;
            }

            return i;
        }
    }

    return HEADER_NONE;
}

static u16 GetHeliodorWildMonHeaderId(void)
{
    u16 i;

    for (i = 0; ; i++)
    {
        const struct WildPokemonHeader *wildHeader = &gWildMonHeaders[i];
        if (wildHeader->mapGroup == MAP_GROUP(UNDEFINED))
            break;

        if (gWildMonHeaders[i].mapGroup == gSaveBlock1Ptr->location.mapGroup &&
            gWildMonHeaders[i].mapNum == gSaveBlock1Ptr->location.mapNum)
        {
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(ALTERING_CAVE) &&
                gSaveBlock1Ptr->location.mapNum == MAP_NUM(ALTERING_CAVE))
            {
                u16 alteringCaveId = VarGet(VAR_ALTERING_CAVE_WILD_SET);
                if (alteringCaveId >= NUM_ALTERING_CAVE_TABLES)
                    alteringCaveId = 0;

                i += alteringCaveId;
            }

            return i;
        }
    }

    return HEADER_NONE;
}

static u8 PickWildMonNature(void)
{
    u8 i;
    u8 j;
    struct Pokeblock *safariPokeblock;
    u8 natures[NUM_NATURES];

    if (GetSafariZoneFlag() == TRUE && Random() % 100 < 80)
    {
        safariPokeblock = SafariZoneGetActivePokeblock();
        if (safariPokeblock != NULL)
        {
            for (i = 0; i < NUM_NATURES; i++)
                natures[i] = i;
            for (i = 0; i < NUM_NATURES - 1; i++)
            {
                for (j = i + 1; j < NUM_NATURES; j++)
                {
                    if (Random() & 1)
                    {
                        u8 temp;
                        SWAP(natures[i], natures[j], temp);
                    }
                }
            }
            for (i = 0; i < NUM_NATURES; i++)
            {
                if (PokeblockGetGain(natures[i], safariPokeblock) > 0)
                    return natures[i];
            }
        }
    }
    // check synchronize for a Pokémon with the same ability
    if (gEncounterMode == ENCOUNTER_EMERALD
		&& !GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG)
        && GetMonAbility(&gPlayerParty[0]) == ABILITY_SYNCHRONIZE
        && Random() % 2 == 0)
    {
        return GetMonData(&gPlayerParty[0], MON_DATA_PERSONALITY) % NUM_NATURES;
    }

    // random nature
    return Random() % NUM_NATURES;
}

static void CreateWildMon(u16 species, u8 level, u8 partySlot)
{
    bool32 checkCuteCharm;
    u8 version;
    u32 personality;
    struct PIDParameters parameters;
    struct IVs ivs;

    parameters.species = species;
    parameters.pidIVMethod = PIDIV_METHOD_1;
    parameters.shinyLock = GENERATE_SHINY_NORMAL;
    parameters.shinyRolls = 1;
    parameters.forceNature = TRUE;
    parameters.nature = PickWildMonNature();
    parameters.forceGender = FALSE;
    parameters.gender = 0;
    parameters.forceUnownLetter = FALSE;
    parameters.unownLetter = 0;

    ZeroEnemyPartyMons();
    checkCuteCharm = (gEncounterMode == ENCOUNTER_EMERALD);

    if (gLastEncounteredSpecies == species)
    {
        if (gChainStreak < 255)
            gChainStreak++;
    }
    else
    {
        gChainStreak = 0;
        gLastEncounteredSpecies = species;
    }

    switch (gEncounterMode)
    {
        case ENCOUNTER_RUBY:
            version = VERSION_RUBY;
            break;
        case ENCOUNTER_SAPPHIRE:
            version = VERSION_SAPPHIRE;
            break;
        case ENCOUNTER_FIRERED:
            version = VERSION_FIRERED;
            break;
        case ENCOUNTER_LEAFGREEN:
            version = VERSION_LEAFGREEN;
            break;
        case ENCOUNTER_EMERALD:
            version = VERSION_EMERALD;
            break;
        default:
            version = VERSION_COLOXD;    // Shouldn't happen
            break;
    }

    switch (gSpeciesInfo[species].genderRatio)
    {
        case MON_MALE:
        case MON_FEMALE:
        case MON_GENDERLESS:
            checkCuteCharm = FALSE;
            break;
    }

    if (checkCuteCharm
        && !GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG)
        && GetMonAbility(&gPlayerParty[0]) == ABILITY_CUTE_CHARM
        && Random() % 3 != 0)
    {
        u16 leadingMonSpecies = GetMonData(&gPlayerParty[0], MON_DATA_SPECIES);
        u32 leadingMonPersonality = GetMonData(&gPlayerParty[0], MON_DATA_PERSONALITY);

        parameters.forceGender = TRUE;
        parameters.gender = GetGenderFromSpeciesAndPersonality(leadingMonSpecies, leadingMonPersonality);

        // misses mon is genderless check, although no genderless mon can have cute charm as ability
        if (parameters.gender == MON_FEMALE)
            parameters.gender = MON_MALE;
        else
            parameters.gender = MON_FEMALE;
    }

    personality = GeneratePIDMaster(parameters, &ivs);

    CreateMon(&gEnemyParty[partySlot], species, level, USE_RANDOM_IVS, TRUE, personality, OT_ID_PLAYER_ID, 0);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_HP_IV, &ivs.hp);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_ATK_IV, &ivs.atk);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_DEF_IV, &ivs.def);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_SPEED_IV, &ivs.speed);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_SPATK_IV, &ivs.spAtk);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_SPDEF_IV, &ivs.spDef);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_MET_GAME, &version);
}

static void CreateWildUnown(u8 slot, u8 level, u8 partySlot)
{
    u8 version;
    u32 personality;
    struct PIDParameters parameters;
    struct IVs ivs;

    parameters.species = SPECIES_UNOWN;
    parameters.pidIVMethod = PIDIV_METHOD_REVERSE_U;
    parameters.shinyLock = GENERATE_SHINY_NORMAL;
    parameters.shinyRolls = 1;
    parameters.forceNature = FALSE;
    parameters.nature = 0;
    parameters.forceGender = FALSE;
    parameters.gender = 0;
    parameters.forceUnownLetter = TRUE;
    parameters.unownLetter = sUnownLetterSlots[(gSaveBlock1Ptr->location.mapNum - MAP_NUM(KANTO_SEVEN_ISLAND_TANOBY_RUINS_MONEAN_CHAMBER)) % 6][slot];

    ZeroEnemyPartyMons();

    if (gLastEncounteredSpecies == SPECIES_UNOWN)
    {
        if (gChainStreak < 255)
            gChainStreak++;
    }
    else
    {
        gChainStreak = 0;
        gLastEncounteredSpecies = SPECIES_UNOWN;
    }

    switch (gEncounterMode)
    {
        case ENCOUNTER_LEAFGREEN:
            version = VERSION_LEAFGREEN;
            break;
        default:
            version = VERSION_FIRERED;
            break;
    }

    personality = GeneratePIDMaster(parameters, &ivs);
 
	CreateMon(&gEnemyParty[partySlot], SPECIES_UNOWN, level, USE_RANDOM_IVS, TRUE, personality, FALSE, 0);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_HP_IV, &ivs.hp);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_ATK_IV, &ivs.atk);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_DEF_IV, &ivs.def);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_SPEED_IV, &ivs.speed);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_SPATK_IV, &ivs.spAtk);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_SPDEF_IV, &ivs.spDef);
    SetMonData(&gEnemyParty[partySlot], MON_DATA_MET_GAME, &version);
}

//TODO
static bool8 TryGenerateWildMon(const struct WildPokemonInfo *wildMonInfo, u8 area, u8 flags, u8 partySlot)
{
    u8 wildMonIndex = 0;
    u8 level;

    switch (area)
    {
    case WILD_AREA_LAND:
        if (flags & WILD_CHECK_ABILITY && TryGetAbilityInfluencedWildMonIndex(wildMonInfo->wildPokemon, TYPE_STEEL, ABILITY_MAGNET_PULL, &wildMonIndex, LAND_WILD_COUNT))
            break;
        if (flags & WILD_CHECK_ABILITY && TryGetAbilityInfluencedWildMonIndex(wildMonInfo->wildPokemon, TYPE_ELECTRIC, ABILITY_STATIC, &wildMonIndex, LAND_WILD_COUNT))
            break;

        wildMonIndex = ChooseWildMonIndex_Land();
        break;
    case WILD_AREA_WATER:
        if (flags & WILD_CHECK_ABILITY && TryGetAbilityInfluencedWildMonIndex(wildMonInfo->wildPokemon, TYPE_ELECTRIC, ABILITY_STATIC, &wildMonIndex, WATER_WILD_COUNT))
            break;

        wildMonIndex = ChooseWildMonIndex_WaterRock();
        break;
    case WILD_AREA_ROCKS:
        wildMonIndex = ChooseWildMonIndex_WaterRock();
        break;
    }

    level = ChooseWildMonLevel(&wildMonInfo->wildPokemon[wildMonIndex]);
    if (flags & WILD_CHECK_REPEL && !IsWildLevelAllowedByRepel(level))
        return FALSE;
    if (gMapHeader.mapLayoutId != LAYOUT_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS && flags & WILD_CHECK_ABILITY && !IsAbilityAllowingEncounter(level))
        return FALSE;

	if (wildMonInfo->wildPokemon[wildMonIndex].species == SPECIES_UNOWN)
		CreateWildUnown(wildMonIndex, level, partySlot);
	else
		CreateWildMon(wildMonInfo->wildPokemon[wildMonIndex].species, level, partySlot);
    return TRUE;
}

static u16 GenerateFishingWildMon(const struct WildPokemonInfo *wildMonInfo, u8 rod)
{
    u8 timeOfDay;
    u8 wildMonIndex;
    u8 level;

    RtcCalcLocalTime();
    timeOfDay = GetCurrentTimeOfDay();

    wildMonIndex = ChooseWildMonIndex_Fishing(rod);
    level = ChooseWildMonLevel(&wildMonInfo->wildPokemon[wildMonIndex]);

	if (FlagGet(FLAG_UNOWN_RELEASED) && !FlagGet(FLAG_UNOWN_SETTLED) && Random() % 8 == 0)
		CreateWildMon(SPECIES_UNOWN, 25, 0);
	else
		CreateWildMon(wildMonInfo->wildPokemon[wildMonIndex].species, level, 0);
    return wildMonInfo->wildPokemon[wildMonIndex].species;
}

static bool8 SetUpMassOutbreakEncounter(u8 flags)
{
    u16 i;
	u8 levelBonus = Random() % 11;

    if (flags & WILD_CHECK_REPEL && !IsWildLevelAllowedByRepel(gSaveBlock1Ptr->outbreakPokemonLevel + levelBonus))
        return FALSE;

    CreateWildMon(gSaveBlock1Ptr->outbreakPokemonSpecies, gSaveBlock1Ptr->outbreakPokemonLevel + levelBonus, 0);
	gChainStreak++;
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(&gEnemyParty[0], gSaveBlock1Ptr->outbreakPokemonMoves[i], i);

    return TRUE;
}

static bool8 DoMassOutbreakEncounterTest(bool8 water)
{
	if (water && gSaveBlock1Ptr->outbreakEncounterType == 0)
		return FALSE;
	else if (!water && gSaveBlock1Ptr->outbreakEncounterType == 1)
		return FALSE;

    if (gSaveBlock1Ptr->outbreakPokemonSpecies != SPECIES_NONE
     && gSaveBlock1Ptr->location.mapNum == gSaveBlock1Ptr->outbreakLocationMapNum
     && gSaveBlock1Ptr->location.mapGroup == gSaveBlock1Ptr->outbreakLocationMapGroup)
    {
        if (Random() % 100 < gSaveBlock1Ptr->outbreakPokemonProbability)
            return TRUE;
    }
    return FALSE;
}

static bool8 EncounterOddsCheck(u16 encounterRate, u16 maxRate)
{
    if (Random() % maxRate < encounterRate)
        return TRUE;
    else
        return FALSE;
}

// Returns true if it will try to create a wild encounter.
static bool8 WildEncounterCheck(u32 encounterRate, bool8 ignoreAbility)
{
    switch (gEncounterMode)
    {
        case ENCOUNTER_RUBY:
        case ENCOUNTER_SAPPHIRE:
            encounterRate *= 16;
            if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
                encounterRate = encounterRate * 80 / 100;
            ApplyFluteEncounterRateMod(&encounterRate);
            ApplyCleanseTagEncounterRateMod(&encounterRate);
            if (!ignoreAbility && !GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
            {
                u32 ability = GetMonAbility(&gPlayerParty[0]);

                if (ability == ABILITY_STENCH)
                    encounterRate /= 2;
                else if (ability == ABILITY_ILLUMINATE)
                    encounterRate *= 2;
            }
            if (encounterRate > MAX_RSE_ENCOUNTER_RATE)
                encounterRate = MAX_RSE_ENCOUNTER_RATE;
            return EncounterOddsCheck(encounterRate, MAX_RSE_ENCOUNTER_RATE);
        case ENCOUNTER_FIRERED:
        case ENCOUNTER_LEAFGREEN:
            encounterRate *= 16;
            if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
                encounterRate = encounterRate * 80 / 100;
            encounterRate += sEncounterRateBuff * 16 / 200;
            ApplyFluteEncounterRateMod(&encounterRate);
            ApplyCleanseTagEncounterRateMod(&encounterRate);
            if (!ignoreAbility && !GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
            {
                u32 ability = GetMonAbility(&gPlayerParty[0]);

                if (ability == ABILITY_STENCH)
                    encounterRate /= 2;
                else if (ability == ABILITY_ILLUMINATE)
                    encounterRate *= 2;
            }
            if (encounterRate > MAX_FRLG_ENCOUNTER_RATE)
                encounterRate = MAX_FRLG_ENCOUNTER_RATE;
            return EncounterOddsCheck(encounterRate, MAX_FRLG_ENCOUNTER_RATE);
            break;
        case ENCOUNTER_EMERALD:
        default:
            encounterRate *= 16;
            if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
                encounterRate = encounterRate * 80 / 100;
            ApplyFluteEncounterRateMod(&encounterRate);
            ApplyCleanseTagEncounterRateMod(&encounterRate);
            if (!ignoreAbility && !GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
            {
                u32 ability = GetMonAbility(&gPlayerParty[0]);

                if (ability == ABILITY_STENCH)
                    encounterRate /= 2;
                else if (ability == ABILITY_ILLUMINATE)
                    encounterRate *= 2;
                else if (ability == ABILITY_WHITE_SMOKE)
                    encounterRate /= 2;
                else if (ability == ABILITY_ARENA_TRAP)
                    encounterRate *= 2;
                else if (ability == ABILITY_SAND_VEIL && gSaveBlock1Ptr->weather == WEATHER_SANDSTORM)
                    encounterRate /= 2;
            }
            if (encounterRate > MAX_RSE_ENCOUNTER_RATE)
                encounterRate = MAX_RSE_ENCOUNTER_RATE;
            return EncounterOddsCheck(encounterRate, MAX_RSE_ENCOUNTER_RATE);
    }
    
    
    //encounterRate *= 16;
    ////if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
    ////    encounterRate = encounterRate * 80 / 100;
    //ApplyFluteEncounterRateMod(&encounterRate);
    //ApplyCleanseTagEncounterRateMod(&encounterRate);
    //if (!ignoreAbility && !GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
    //{
    //    u32 ability = GetMonAbility(&gPlayerParty[0]);
    //
    //    if (ability == ABILITY_STENCH && gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR)
    //        encounterRate = encounterRate * 3 / 4;
    //    else if (ability == ABILITY_STENCH)
    //        encounterRate /= 2;
    //    else if (ability == ABILITY_ILLUMINATE)
    //        encounterRate *= 2;
    //    else if (ability == ABILITY_WHITE_SMOKE)
    //        encounterRate /= 2;
    //    else if (ability == ABILITY_ARENA_TRAP)
    //        encounterRate *= 2;
    //    else if (ability == ABILITY_SAND_VEIL && gSaveBlock1Ptr->weather == WEATHER_SANDSTORM)
    //        encounterRate /= 2;
    //}
    //
	//if (gPowerType == POWER_ENCOUNTER && gPowerTime > 0)
	//{
	//	switch (gPowerLevel)
	//	{
	//		case 1:
	//			encounterRate = encounterRate * 150 / 100;
	//			break;
	//		case 2:
	//			encounterRate = encounterRate * 2;
	//			break;
	//		case 3:
	//			encounterRate = encounterRate * 3;
	//			break;
	//	}
	//}
	//
	//if (gPowerType == POWER_STEALTH && gPowerTime > 0)
	//{
	//	switch (gPowerLevel)
	//	{
	//		case 1:
	//			encounterRate = encounterRate * 66 / 100;
	//			break;
	//		case 2:
	//			encounterRate = encounterRate * 50 / 100;
	//			break;
	//		case 3:
	//			encounterRate = encounterRate * 33 / 100;
	//			break;
	//	}
	//}
    //
    //if (encounterRate > MAX_ENCOUNTER_RATE)
    //    encounterRate = MAX_ENCOUNTER_RATE;
    //return EncounterOddsCheck(encounterRate);
}

// When you first step on a different type of metatile, there's a 40% chance it
// skips the wild encounter check entirely.
static bool8 AllowWildCheckOnNewMetatile(void)
{
    if (Random() % 100 >= 60)
        return FALSE;
    else
        return TRUE;
}

static bool8 AreLegendariesInSootopolisPreventingEncounters(void)
{
    if (gSaveBlock1Ptr->location.mapGroup != MAP_GROUP(SOOTOPOLIS_CITY)
     || gSaveBlock1Ptr->location.mapNum != MAP_NUM(SOOTOPOLIS_CITY))
    {
        return FALSE;
    }

    return FlagGet(FLAG_LEGENDARIES_IN_SOOTOPOLIS);
}

bool8 StandardWildEncounter(u16 curMetatileBehavior, u16 prevMetatileBehavior)
{
    u16 headerId, oppositeHeaderId;
    u8 encounterResult;
    struct Roamer *roamer;
    const struct WildPokemonInfo *wildPokemonInfo;

    if (sWildEncountersDisabled == TRUE)
        return FALSE;

    headerId = GetEmeraldWildMonHeaderId();
    gEncounterMode = ENCOUNTER_EMERALD;

    if (headerId == HEADER_NONE)
    {
        headerId = GetFireRedWildMonHeaderId();

        if (headerId != HEADER_NONE)
            gEncounterMode = ENCOUNTER_FIRERED;
    }

    if (headerId == HEADER_NONE)
    {
        if (gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS)
        {
            headerId = GetBattlePikeWildMonHeaderId();
            if (prevMetatileBehavior != curMetatileBehavior && !AllowWildCheckOnNewMetatile())
                return FALSE;
            else if (WildEncounterCheck(gBattlePikeWildMonHeadersE[headerId].landMonsInfo->encounterRate, FALSE) != TRUE)
                return FALSE;
            else if (TryGenerateWildMon(gBattlePikeWildMonHeadersE[headerId].landMonsInfo, WILD_AREA_LAND, WILD_CHECK_ABILITY, 0) != TRUE)
                return FALSE;
            else if (!TryGenerateBattlePikeWildMon(TRUE))
                return FALSE;

            BattleSetup_StartBattlePikeWildBattle();
            return TRUE;
        }
        if (gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR)
        {
            headerId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
            if (prevMetatileBehavior != curMetatileBehavior && !AllowWildCheckOnNewMetatile())
                return FALSE;
            else if (WildEncounterCheck(gBattlePyramidWildMonHeadersE[headerId].landMonsInfo->encounterRate, FALSE) != TRUE)
                return FALSE;
            else if (TryGenerateWildMon(gBattlePyramidWildMonHeadersE[headerId].landMonsInfo, WILD_AREA_LAND, WILD_CHECK_ABILITY, 0) != TRUE)
                return FALSE;

            GenerateBattlePyramidWildMon();
            BattleSetup_StartWildBattle();
            return TRUE;
        }
    }
    else
    {
        if (MetatileBehavior_IsLandWildEncounter(curMetatileBehavior) == TRUE)
        {
            if (prevMetatileBehavior != curMetatileBehavior && !AllowWildCheckOnNewMetatile())
                return FALSE;

            if (gEncounterMode == ENCOUNTER_FIRERED)
            {
                wildPokemonInfo = gWildMonHeadersFRLG[headerId].landMonsInfo;

                switch (Random() % 3)
                {
                    case 1:
                        oppositeHeaderId = GetCorrespondingLeafGreenWildMonHeaderId(headerId);

                        if (oppositeHeaderId != HEADER_NONE)
                        {
                            gEncounterMode = ENCOUNTER_LEAFGREEN;
                            wildPokemonInfo = gWildMonHeadersFRLG[oppositeHeaderId].landMonsInfo;
                        }
                        break;
                    case 2:
                        headerId = GetHeliodorWildMonHeaderId();

                        if (headerId != HEADER_NONE)
                        {
                            gEncounterMode = ENCOUNTER_HELIODOR;
                            wildPokemonInfo = gWildMonHeaders[headerId].landMonsInfo;
                        }
                        break;
                }
            }
            else
            {
                wildPokemonInfo = gWildMonHeadersE[headerId].landMonsInfo;

                switch (Random() % 4)
                {
                    case 0:
                        headerId = GetRubyWildMonHeaderId();

                        if (headerId != HEADER_NONE)
                        {
                            gEncounterMode = ENCOUNTER_RUBY;
                            wildPokemonInfo = gWildMonHeadersRS[headerId].landMonsInfo;
                        }
                        break;
                    case 1:
                        headerId = GetRubyWildMonHeaderId();

                        if (headerId != HEADER_NONE)
                        {
                            oppositeHeaderId = GetCorrespondingSapphireWildMonHeaderId(headerId);

                            if (oppositeHeaderId != HEADER_NONE)
                            {
                                gEncounterMode = ENCOUNTER_SAPPHIRE;
                                wildPokemonInfo = gWildMonHeadersRS[oppositeHeaderId].landMonsInfo;
                            }
                        }
                        break;
                    case 3:
                        headerId = GetHeliodorWildMonHeaderId();

                        if (headerId != HEADER_NONE)
                        {
                            gEncounterMode = ENCOUNTER_HELIODOR;
                            wildPokemonInfo = gWildMonHeaders[headerId].landMonsInfo;
                        }
                        break;
                }
            }

            if (wildPokemonInfo == NULL)
                return FALSE;

            encounterResult = GenerateEncounter(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, WILD_AREA_LAND, 0);

            if (encounterResult == ENCOUNTER_ROAMER)
            {
                roamer = &gSaveBlock1Ptr->roamer;
                if (!IsWildLevelAllowedByRepel(roamer->level))
                    return FALSE;
            
                BattleSetup_StartRoamerBattle();
                return TRUE;
            }
            else if (encounterResult == ENCOUNTER_SUCCESS)
            {
                if (GetMonsStateToDoubles() == PLAYER_HAS_TWO_USABLE_MONS)
                {
                    if (IsMonShiny(&gEnemyParty[0]))
                        IncrementGameStat(GAME_STAT_SHINIES_FOUND);

                    if (gEncounterMode == ENCOUNTER_FIRERED || gEncounterMode == ENCOUNTER_LEAFGREEN)
                    {
                        headerId = GetFireRedWildMonHeaderId();
                        gEncounterMode = ENCOUNTER_FIRERED;
                        wildPokemonInfo = gWildMonHeadersFRLG[headerId].landMonsInfo;
                        
                        switch (Random() % 3)
                        {
                            case 1:
                                oppositeHeaderId = GetCorrespondingLeafGreenWildMonHeaderId(headerId);

                                if (oppositeHeaderId != HEADER_NONE)
                                {
                                    gEncounterMode = ENCOUNTER_LEAFGREEN;
                                    wildPokemonInfo = gWildMonHeadersFRLG[oppositeHeaderId].landMonsInfo;
                                }
                                break;
                            case 2:
                                headerId = GetHeliodorWildMonHeaderId();

                                if (headerId != HEADER_NONE)
                                {
                                    gEncounterMode = ENCOUNTER_HELIODOR;
                                    wildPokemonInfo = gWildMonHeaders[headerId].landMonsInfo;
                                }
                                break;
                        }
                    }
                    else
                    {
                        headerId = GetEmeraldWildMonHeaderId();
                        gEncounterMode = ENCOUNTER_EMERALD;
                        wildPokemonInfo = gWildMonHeadersE[headerId].landMonsInfo;

                        switch (Random() % 4)
                        {
                            case 0:
                                headerId = GetRubyWildMonHeaderId();

                                if (headerId != HEADER_NONE)
                                {
                                    gEncounterMode = ENCOUNTER_RUBY;
                                    wildPokemonInfo = gWildMonHeadersRS[headerId].landMonsInfo;
                                }
                                break;
                            case 1:
                                headerId = GetRubyWildMonHeaderId();

                                if (headerId != HEADER_NONE)
                                {
                                    oppositeHeaderId = GetCorrespondingSapphireWildMonHeaderId(headerId);

                                    if (oppositeHeaderId != HEADER_NONE)
                                    {
                                        gEncounterMode = ENCOUNTER_SAPPHIRE;
                                        wildPokemonInfo = gWildMonHeadersRS[oppositeHeaderId].landMonsInfo;
                                    }
                                }
                                break;
                            case 3:
                                headerId = GetHeliodorWildMonHeaderId();

                                if (headerId != HEADER_NONE)
                                {
                                    gEncounterMode = ENCOUNTER_HELIODOR;
                                    wildPokemonInfo = gWildMonHeaders[headerId].landMonsInfo;
                                }
                                break;
                        }
                    }

                    encounterResult = GenerateEncounter(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, WILD_AREA_LAND, 1);

                    if (encounterResult == ENCOUNTER_SUCCESS)
                    {
                        if (IsMonShiny(&gEnemyParty[1]))
                            IncrementGameStat(GAME_STAT_SHINIES_FOUND);
                        DebugPrintf("double", NULL);
                        BattleSetup_StartWildDoubleBattle();
                    }
                    else
                    {
                        BattleSetup_StartWildBattle();
                    }
                }
                else
                {
                    if (IsMonShiny(&gEnemyParty[0]))
                        IncrementGameStat(GAME_STAT_SHINIES_FOUND);
                    BattleSetup_StartWildBattle();
                }
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        else if (MetatileBehavior_IsWaterWildEncounter(curMetatileBehavior) == TRUE
                 || (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING) && MetatileBehavior_IsBridgeOverWater(curMetatileBehavior) == TRUE))
        {
            if (AreLegendariesInSootopolisPreventingEncounters() == TRUE)
                return FALSE;
            else if (prevMetatileBehavior != curMetatileBehavior && !AllowWildCheckOnNewMetatile())
                return FALSE;

            if (gEncounterMode == ENCOUNTER_FIRERED)
            {
                wildPokemonInfo = gWildMonHeadersFRLG[headerId].waterMonsInfo;

                switch (Random() % 3)
                {
                    case 1:
                        oppositeHeaderId = GetCorrespondingLeafGreenWildMonHeaderId(headerId);

                        if (oppositeHeaderId != HEADER_NONE)
                        {
                            gEncounterMode = ENCOUNTER_LEAFGREEN;
                            wildPokemonInfo = gWildMonHeadersFRLG[oppositeHeaderId].waterMonsInfo;
                        }
                        break;
                    case 2:
                        headerId = GetHeliodorWildMonHeaderId();

                        if (headerId != HEADER_NONE)
                        {
                            gEncounterMode = ENCOUNTER_HELIODOR;
                            wildPokemonInfo = gWildMonHeaders[headerId].waterMonsInfo;
                        }
                        break;
                }
            }
            else
            {
                wildPokemonInfo = gWildMonHeadersE[headerId].waterMonsInfo;

                switch (Random() % 4)
                {
                    case 0:
                        headerId = GetRubyWildMonHeaderId();

                        if (headerId != HEADER_NONE)
                        {
                            gEncounterMode = ENCOUNTER_RUBY;
                            wildPokemonInfo = gWildMonHeadersRS[headerId].waterMonsInfo;
                        }
                        break;
                    case 1:
                        headerId = GetRubyWildMonHeaderId();

                        if (headerId != HEADER_NONE)
                        {
                            oppositeHeaderId = GetCorrespondingSapphireWildMonHeaderId(headerId);

                            if (oppositeHeaderId != HEADER_NONE)
                            {
                                gEncounterMode = ENCOUNTER_SAPPHIRE;
                                wildPokemonInfo = gWildMonHeadersRS[oppositeHeaderId].waterMonsInfo;
                            }
                        }
                        break;
                    case 3:
                        headerId = GetHeliodorWildMonHeaderId();

                        if (headerId != HEADER_NONE)
                        {
                            gEncounterMode = ENCOUNTER_HELIODOR;
                            wildPokemonInfo = gWildMonHeaders[headerId].waterMonsInfo;
                        }
                        break;
                }
            }

            if (wildPokemonInfo == NULL)
                return FALSE;

            encounterResult = GenerateEncounter(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, WILD_AREA_WATER, 0);

            if (encounterResult == ENCOUNTER_ROAMER)
            {
                roamer = &gSaveBlock1Ptr->roamer;
                if (!IsWildLevelAllowedByRepel(roamer->level))
                    return FALSE;
            
                BattleSetup_StartRoamerBattle();
                return TRUE;
            }
            else if (encounterResult == ENCOUNTER_SUCCESS)
            {
                if (GetMonsStateToDoubles() == PLAYER_HAS_TWO_USABLE_MONS)
                {
                    if (IsMonShiny(&gEnemyParty[0]))
                        IncrementGameStat(GAME_STAT_SHINIES_FOUND);

                    if (gEncounterMode == ENCOUNTER_FIRERED || gEncounterMode == ENCOUNTER_LEAFGREEN)
                    {
                        headerId = GetFireRedWildMonHeaderId();
                        gEncounterMode = ENCOUNTER_FIRERED;
                        wildPokemonInfo = gWildMonHeadersFRLG[headerId].waterMonsInfo;

                        switch (Random() % 3)
                        {
                            case 1:
                                oppositeHeaderId = GetCorrespondingLeafGreenWildMonHeaderId(headerId);

                                if (oppositeHeaderId != HEADER_NONE)
                                {
                                    gEncounterMode = ENCOUNTER_LEAFGREEN;
                                    wildPokemonInfo = gWildMonHeadersFRLG[oppositeHeaderId].waterMonsInfo;
                                }
                                break;
                            case 2:
                                headerId = GetHeliodorWildMonHeaderId();

                                if (headerId != HEADER_NONE)
                                {
                                    gEncounterMode = ENCOUNTER_HELIODOR;
                                    wildPokemonInfo = gWildMonHeaders[headerId].waterMonsInfo;
                                }
                                break;
                        }
                    }
                    else
                    {
                        headerId = GetEmeraldWildMonHeaderId();
                        gEncounterMode = ENCOUNTER_EMERALD;
                        wildPokemonInfo = gWildMonHeadersE[headerId].waterMonsInfo;

                        switch (Random() % 4)
                        {
                            case 0:
                                headerId = GetRubyWildMonHeaderId();

                                if (headerId != HEADER_NONE)
                                {
                                    gEncounterMode = ENCOUNTER_RUBY;
                                    wildPokemonInfo = gWildMonHeadersRS[headerId].waterMonsInfo;
                                }
                                break;
                            case 1:
                                headerId = GetRubyWildMonHeaderId();

                                if (headerId != HEADER_NONE)
                                {
                                    oppositeHeaderId = GetCorrespondingSapphireWildMonHeaderId(headerId);

                                    if (oppositeHeaderId != HEADER_NONE)
                                    {
                                        gEncounterMode = ENCOUNTER_SAPPHIRE;
                                        wildPokemonInfo = gWildMonHeadersRS[oppositeHeaderId].waterMonsInfo;
                                    }
                                }
                                break;
                            case 3:
                                headerId = GetHeliodorWildMonHeaderId();

                                if (headerId != HEADER_NONE)
                                {
                                    gEncounterMode = ENCOUNTER_HELIODOR;
                                    wildPokemonInfo = gWildMonHeaders[headerId].waterMonsInfo;
                                }
                                break;
                        }
                    }

                    encounterResult = GenerateEncounter(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, WILD_AREA_WATER, 1);

                    if (encounterResult == ENCOUNTER_SUCCESS)
                    {
                        if (IsMonShiny(&gEnemyParty[1]))
                            IncrementGameStat(GAME_STAT_SHINIES_FOUND);
                        BattleSetup_StartWildDoubleBattle();
                    }
                    else
                    {
                        BattleSetup_StartWildBattle();
                    }
                }
                else
                {
                    if (IsMonShiny(&gEnemyParty[0]))
                        IncrementGameStat(GAME_STAT_SHINIES_FOUND);
                    BattleSetup_StartWildBattle();
                }
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
		//else if (MetatileBehavior_IsPuddle(currMetaTileBehavior) == TRUE)
        //{
        //    if (gWildMonHeaders[headerId].puddleMonsInfo == NULL)
        //        return FALSE;
        //    else if (previousMetaTileBehavior != currMetaTileBehavior && !DoGlobalWildEncounterDiceRoll())
        //        return FALSE;
        //
		//	wildPokemonInfo = gWildMonHeaders[headerId].puddleMonsInfo;
        //
		//	if (IsNationalPokedexEnabled())
		//	{
		//		if (GetCurrentTimeOfDay() == TIME_MORNING && gWildMonHeaders[headerId].puddleMonsNatMorningInfo != NULL)
		//			wildPokemonInfo = gWildMonHeaders[headerId].puddleMonsNatMorningInfo;
		//		else if (GetCurrentTimeOfDay() == TIME_NIGHT && gWildMonHeaders[headerId].puddleMonsNatNightInfo != NULL)
		//			wildPokemonInfo = gWildMonHeaders[headerId].puddleMonsNatNightInfo;
		//		else if (gWildMonHeaders[headerId].puddleMonsNatInfo != NULL)
		//			wildPokemonInfo = gWildMonHeaders[headerId].puddleMonsNatInfo;
		//	}
        //
		//	if (wildPokemonInfo == gWildMonHeaders[headerId].puddleMonsInfo)
		//	{
		//		if (GetCurrentTimeOfDay() == TIME_MORNING && gWildMonHeaders[headerId].puddleMonsMorningInfo != NULL)
		//			wildPokemonInfo = gWildMonHeaders[headerId].puddleMonsMorningInfo;
		//		if (GetCurrentTimeOfDay() == TIME_NIGHT && gWildMonHeaders[headerId].puddleMonsNightInfo != NULL)
		//			wildPokemonInfo = gWildMonHeaders[headerId].puddleMonsNightInfo;
		//	}
        //
		//	if (DoWildEncounterRateTest(wildPokemonInfo->encounterRate, FALSE))
		//	{
		//		if (TryGenerateWildMon(wildPokemonInfo, WILD_AREA_WATER, WILD_CHECK_REPEL | WILD_CHECK_KEEN_EYE) == TRUE)
        //        {
		//			if (IsMonShiny(&gEnemyParty[0]))
		//				IncrementGameStat(GAME_STAT_SHINIES_FOUND);
        //            BattleSetup_StartWildBattle();
        //            return TRUE;
        //        }
        //    }
        //}
    }

    return FALSE;
}

void RockSmashWildEncounter(void)
{
    u8 flags = WILD_CHECK_REPEL;
    u16 oppositeHeaderId;
    u16 headerId = GetEmeraldWildMonHeaderId();

    gEncounterMode = ENCOUNTER_EMERALD;

    if (headerId == HEADER_NONE)
    {
        headerId = GetFireRedWildMonHeaderId();

        if (headerId != HEADER_NONE)
            gEncounterMode = ENCOUNTER_FIRERED;
    }

    if (headerId != HEADER_NONE)
    {
        const struct WildPokemonInfo *wildPokemonInfo;

        if (gEncounterMode == ENCOUNTER_FIRERED)
        {
            wildPokemonInfo = gWildMonHeadersFRLG[headerId].rockSmashMonsInfo;

            switch (Random() % 3)
            {
                case 1:
                    oppositeHeaderId = GetCorrespondingLeafGreenWildMonHeaderId(headerId);

                    if (oppositeHeaderId != HEADER_NONE)
                    {
                        gEncounterMode = ENCOUNTER_LEAFGREEN;
                        wildPokemonInfo = gWildMonHeadersFRLG[oppositeHeaderId].rockSmashMonsInfo;
                    }
                    break;
                case 2:
                    headerId = GetHeliodorWildMonHeaderId();

                    if (headerId != HEADER_NONE)
                    {
                        gEncounterMode = ENCOUNTER_HELIODOR;
                        wildPokemonInfo = gWildMonHeaders[headerId].rockSmashMonsInfo;
                    }
                    break;
            }
        }
        else
        {
            wildPokemonInfo = gWildMonHeadersE[headerId].rockSmashMonsInfo;

            switch (Random() % 4)
            {
                case 0:
                    headerId = GetRubyWildMonHeaderId();

                    if (headerId != HEADER_NONE)
                    {
                        gEncounterMode = ENCOUNTER_RUBY;
                        wildPokemonInfo = gWildMonHeadersRS[headerId].rockSmashMonsInfo;
                    }
                    break;
                case 1:
                    headerId = GetRubyWildMonHeaderId();

                    if (headerId != HEADER_NONE)
                    {
                        oppositeHeaderId = GetCorrespondingSapphireWildMonHeaderId(headerId);

                        if (oppositeHeaderId != HEADER_NONE)
                        {
                            gEncounterMode = ENCOUNTER_SAPPHIRE;
                            wildPokemonInfo = gWildMonHeadersRS[oppositeHeaderId].rockSmashMonsInfo;
                        }
                    }
                    break;
                case 2:
                    flags |= WILD_CHECK_ABILITY;
                    break;
                case 3:
                    headerId = GetHeliodorWildMonHeaderId();

                    if (headerId != HEADER_NONE)
                    {
                        gEncounterMode = ENCOUNTER_HELIODOR;
                        wildPokemonInfo = gWildMonHeaders[headerId].rockSmashMonsInfo;
                    }
                    break;
            }
        }

        if (wildPokemonInfo == NULL)
        {
            gSpecialVar_Result = FALSE;
        }
        else if (WildEncounterCheck(wildPokemonInfo->encounterRate, TRUE) == TRUE
         && TryGenerateWildMon(wildPokemonInfo, WILD_AREA_ROCKS, flags, 0) == TRUE)
        {
			if (IsMonShiny(&gEnemyParty[0]))
				IncrementGameStat(GAME_STAT_SHINIES_FOUND);
            BattleSetup_StartWildBattle();
            gSpecialVar_Result = TRUE;
        }
        else
        {
            gSpecialVar_Result = FALSE;
        }
    }
    else
    {
        gSpecialVar_Result = FALSE;
    }
}

static const u16 sCommonHeadbuttEncounters[][3] = {
	{SPECIES_WURMPLE, 2, 40},
	{SPECIES_SEEDOT, 3, 40},
	{SPECIES_TAILLOW, 4, 40},
	{SPECIES_SPINARAK, 14, 40},
};

static const u16 sCommonHeadbuttEncountersMorning[][3] = {
	{SPECIES_WURMPLE, 2, 40},
	{SPECIES_SEEDOT, 3, 40},
	{SPECIES_TAILLOW, 4, 40},
	{SPECIES_SPINARAK, 14, 40},
};

static const u16 sCommonHeadbuttEncountersNight[][3] = {
	{SPECIES_WURMPLE, 2, 40},
	{SPECIES_SEEDOT, 3, 40},
	{SPECIES_TAILLOW, 4, 40},
	{SPECIES_SPINARAK, 14, 40},
};

static const u16 sCommonHeadbuttEncountersNat[][3] = {
	{SPECIES_WURMPLE, 2, 40},
	{SPECIES_SEEDOT, 3, 40},
	{SPECIES_TAILLOW, 4, 40},
	{SPECIES_SPINARAK, 14, 40},
	{SPECIES_CATERPIE, 4, 40},
	{SPECIES_METAPOD, 4, 40},
	{SPECIES_WEEDLE, 4, 40},
	{SPECIES_KAKUNA, 4, 40},
	{SPECIES_SPEAROW, 6, 40},
};

static const u16 sCommonHeadbuttEncountersNatMorning[][3] = {
	{SPECIES_WURMPLE, 2, 40},
	{SPECIES_SEEDOT, 3, 40},
	{SPECIES_TAILLOW, 4, 40},
	{SPECIES_SPINARAK, 14, 40},
	{SPECIES_CATERPIE, 4, 40},
	{SPECIES_METAPOD, 4, 40},
	{SPECIES_WEEDLE, 4, 40},
	{SPECIES_KAKUNA, 4, 40},
	{SPECIES_SPEAROW, 6, 40},
};

static const u16 sCommonHeadbuttEncountersNatNight[][3] = {
	{SPECIES_WURMPLE, 2, 40},
	{SPECIES_SEEDOT, 3, 40},
	{SPECIES_TAILLOW, 4, 40},
	{SPECIES_SPINARAK, 14, 40},
	{SPECIES_CATERPIE, 4, 40},
	{SPECIES_METAPOD, 4, 40},
	{SPECIES_WEEDLE, 4, 40},
	{SPECIES_KAKUNA, 4, 40},
	{SPECIES_SPEAROW, 6, 40},
};

static const u16 sUncommonHeadbuttEncounters[][3] = {
	{SPECIES_SHROOMISH, 5, 40},
	{SPECIES_SLAKOTH, 5, 40},
	{SPECIES_LEDYBA, 2, 40},
	{SPECIES_NATU, 20, 40},
};

static const u16 sUncommonHeadbuttEncountersMorning[][3] = {
	{SPECIES_SHROOMISH, 5, 40},
	{SPECIES_SLAKOTH, 5, 40},
	{SPECIES_LEDYBA, 2, 40},
	{SPECIES_NATU, 20, 40},
};

static const u16 sUncommonHeadbuttEncountersNight[][3] = {
	{SPECIES_SHROOMISH, 5, 40},
	{SPECIES_SLAKOTH, 5, 40},
	{SPECIES_LEDYBA, 2, 40},
	{SPECIES_NATU, 20, 40},
};

static const u16 sUncommonHeadbuttEncountersNat[][3] = {
	{SPECIES_SHROOMISH, 5, 40},
	{SPECIES_SLAKOTH, 5, 40},
	{SPECIES_LEDYBA, 14, 40},
	{SPECIES_NATU, 20, 40},
	{SPECIES_BUTTERFREE, 10, 40},
	{SPECIES_BEEDRILL, 10, 40},
	{SPECIES_EXEGGCUTE, 35, 40},
	{SPECIES_EKANS, 6, 40},
};

static const u16 sUncommonHeadbuttEncountersNatMorning[][3] = {
	{SPECIES_SHROOMISH, 5, 40},
	{SPECIES_SLAKOTH, 5, 40},
	{SPECIES_LEDYBA, 14, 40},
	{SPECIES_NATU, 20, 40},
	{SPECIES_BUTTERFREE, 10, 40},
	{SPECIES_BEEDRILL, 10, 40},
	{SPECIES_EXEGGCUTE, 35, 40},
	{SPECIES_EKANS, 6, 40},
};

static const u16 sUncommonHeadbuttEncountersNatNight[][3] = {
	{SPECIES_SHROOMISH, 5, 40},
	{SPECIES_SLAKOTH, 5, 40},
	{SPECIES_LEDYBA, 14, 40},
	{SPECIES_NATU, 20, 40},
	{SPECIES_BUTTERFREE, 10, 40},
	{SPECIES_BEEDRILL, 10, 40},
	{SPECIES_EXEGGCUTE, 35, 40},
	{SPECIES_EKANS, 6, 40},
};

static const u16 sRareHeadbuttEncounters[][3] = {
	{SPECIES_LEDIAN, 18, 40},
	{SPECIES_ARIADOS, 22, 40},
	{SPECIES_NOCTOWL, 2, 40},
	{SPECIES_HERACROSS, 30, 40},
};

static const u16 sRareHeadbuttEncountersMorning[][3] = {
	{SPECIES_LEDIAN, 18, 40},
	{SPECIES_ARIADOS, 22, 40},
	{SPECIES_HERACROSS, 30, 40},
};

static const u16 sRareHeadbuttEncountersNight[][3] = {
	{SPECIES_LEDIAN, 18, 40},
	{SPECIES_ARIADOS, 22, 40},
	{SPECIES_HERACROSS, 30, 40},
};

static const u16 sRareHeadbuttEncountersNat[][3] = {
	{SPECIES_LEDIAN, 18, 40},
	{SPECIES_ARIADOS, 22, 40},
	{SPECIES_HERACROSS, 30, 40},
	{SPECIES_VENONAT, 22, 40},
};

static const u16 sRareHeadbuttEncountersNatMorning[][3] = {
	{SPECIES_LEDIAN, 18, 40},
	{SPECIES_ARIADOS, 22, 40},
	{SPECIES_HERACROSS, 30, 40},
	{SPECIES_VENONAT, 22, 40},
};

static const u16 sRareHeadbuttEncountersNatNight[][3] = {
	{SPECIES_LEDIAN, 18, 40},
	{SPECIES_ARIADOS, 22, 40},
	{SPECIES_HERACROSS, 30, 40},
	{SPECIES_VENONAT, 22, 40},
};
//TODO
void HeadbuttWildEncounter(void)
{
	//u64 inhabitedX, inhabitedY;
	//u16 treeX, treeY, species;
	//u8 quadrant, slot, min, max, range, level;
	//u16 headerId = GetCurrentMapWildMonHeaderId();
	//u16 trainerId = (gSaveBlock2Ptr->playerTrainerId[1] << 8) | gSaveBlock2Ptr->playerTrainerId[0];
	//u16 secretId = (gSaveBlock2Ptr->playerTrainerId[3] << 8) | gSaveBlock2Ptr->playerTrainerId[2];
    //
	//gSpecialVar_Result = TRUE;
	//GetXYCoordsOneStepInFrontOfPlayer(&treeX, &treeY);
	//quadrant = MapGridGetMetatileBehaviorAt(treeX, treeY);
    //
	//switch (quadrant)
	//{
	//	case MB_TREE2:
	//		treeX--; 
	//		break;
	//	case MB_TREE3:
	//		treeY--;
	//		break;
	//	case MB_TREE4:
	//		treeX--;
	//		treeY--;
	//		break;
	//}
    //
	//treeX = treeX % 64;
	//treeY = treeY % 64;
	//inhabitedX = trainerId * (headerId + 5) * 0x100343FDu + 0x10269EC3u;
	//inhabitedY = secretId * (headerId + 5) * 0x100343FDu + 0x10269EC3u;
    //
	//if ((headerId != 0xFFFF && (Random() % 4)) || (headerId == 0xFFFF && ((Random() % 8) == 0)))
	//{
	//	if (((inhabitedX >> treeX) & 3) == 3 && ((inhabitedY >> treeY) & 3) == 3)
	//	{
	//		if (IsNationalPokedexEnabled() && GetCurrentTimeOfDay() == TIME_MORNING)
	//		{
	//			slot = Random() % ARRAY_COUNT(sRareHeadbuttEncountersNatMorning);
	//			species = sRareHeadbuttEncountersNatMorning[slot][0];
	//			if (sRareHeadbuttEncountersNatMorning[slot][2] >= sRareHeadbuttEncountersNatMorning[slot][1])
	//			{
	//				min = sRareHeadbuttEncountersNatMorning[slot][1];
	//				max = sRareHeadbuttEncountersNatMorning[slot][2];
	//			}
	//			else
	//			{
	//				min = sRareHeadbuttEncountersNatMorning[slot][2];
	//				max = sRareHeadbuttEncountersNatMorning[slot][1];
	//			}
	//		}
	//		else if (IsNationalPokedexEnabled() && GetCurrentTimeOfDay() == TIME_NIGHT)
	//		{
	//			slot = Random() % ARRAY_COUNT(sRareHeadbuttEncountersNatNight);
	//			species = sRareHeadbuttEncountersNatNight[slot][0];
	//			if (sRareHeadbuttEncountersNatNight[slot][2] >= sRareHeadbuttEncountersNatNight[slot][1])
	//			{
	//				min = sRareHeadbuttEncountersNatNight[slot][1];
	//				max = sRareHeadbuttEncountersNatNight[slot][2];
	//			}
	//			else
	//			{
	//				min = sRareHeadbuttEncountersNatNight[slot][2];
	//				max = sRareHeadbuttEncountersNatNight[slot][1];
	//			}
	//		}
	//		else if (IsNationalPokedexEnabled())
	//		{
	//			slot = Random() % ARRAY_COUNT(sRareHeadbuttEncountersNat);
	//			species = sRareHeadbuttEncountersNat[slot][0];
	//			if (sRareHeadbuttEncountersNat[slot][2] >= sRareHeadbuttEncountersNat[slot][1])
	//			{
	//				min = sRareHeadbuttEncountersNat[slot][1];
	//				max = sRareHeadbuttEncountersNat[slot][2];
	//			}
	//			else
	//			{
	//				min = sRareHeadbuttEncountersNat[slot][2];
	//				max = sRareHeadbuttEncountersNat[slot][1];
	//			}
	//		}
	//		else if (GetCurrentTimeOfDay() == TIME_MORNING)
	//		{
	//			slot = Random() % ARRAY_COUNT(sRareHeadbuttEncountersMorning);
	//			species = sRareHeadbuttEncountersMorning[slot][0];
	//			if (sRareHeadbuttEncountersMorning[slot][2] >= sRareHeadbuttEncountersMorning[slot][1])
	//			{
	//				min = sRareHeadbuttEncountersMorning[slot][1];
	//				max = sRareHeadbuttEncountersMorning[slot][2];
	//			}
	//			else
	//			{
	//				min = sRareHeadbuttEncountersMorning[slot][2];
	//				max = sRareHeadbuttEncountersMorning[slot][1];
	//			}
	//		}
	//		else if (GetCurrentTimeOfDay() == TIME_NIGHT)
	//		{
	//			slot = Random() % ARRAY_COUNT(sRareHeadbuttEncountersNight);
	//			species = sRareHeadbuttEncountersNight[slot][0];
	//			if (sRareHeadbuttEncountersNight[slot][2] >= sRareHeadbuttEncountersNight[slot][1])
	//			{
	//				min = sRareHeadbuttEncountersNight[slot][1];
	//				max = sRareHeadbuttEncountersNight[slot][2];
	//			}
	//			else
	//			{
	//				min = sRareHeadbuttEncountersNight[slot][2];
	//				max = sRareHeadbuttEncountersNight[slot][1];
	//			}
	//		}
	//		else
	//		{
	//			slot = Random() % ARRAY_COUNT(sRareHeadbuttEncounters);
	//			species = sRareHeadbuttEncounters[slot][0];
	//			if (sRareHeadbuttEncounters[slot][2] >= sRareHeadbuttEncounters[slot][1])
	//			{
	//				min = sRareHeadbuttEncounters[slot][1];
	//				max = sRareHeadbuttEncounters[slot][2];
	//			}
	//			else
	//			{
	//				min = sRareHeadbuttEncounters[slot][2];
	//				max = sRareHeadbuttEncounters[slot][1];
	//			}
	//		}
	//	}
	//	else if (((inhabitedX >> treeX) & 3) == 2 || ((inhabitedY >> treeY) & 3) == 2)
	//	{
	//		if (IsNationalPokedexEnabled() && GetCurrentTimeOfDay() == TIME_MORNING)
	//		{
	//			slot = Random() % ARRAY_COUNT(sCommonHeadbuttEncountersNatMorning);
	//			species = sCommonHeadbuttEncountersNatMorning[slot][0];
	//			if (sCommonHeadbuttEncountersNatMorning[slot][2] >= sCommonHeadbuttEncountersNatMorning[slot][1])
	//			{
	//				min = sCommonHeadbuttEncountersNatMorning[slot][1];
	//				max = sCommonHeadbuttEncountersNatMorning[slot][2];
	//			}
	//			else
	//			{
	//				min = sCommonHeadbuttEncountersNatMorning[slot][2];
	//				max = sCommonHeadbuttEncountersNatMorning[slot][1];
	//			}
	//		}
	//		else if (IsNationalPokedexEnabled() && GetCurrentTimeOfDay() == TIME_NIGHT)
	//		{
	//			slot = Random() % ARRAY_COUNT(sCommonHeadbuttEncountersNatNight);
	//			species = sCommonHeadbuttEncountersNatNight[slot][0];
	//			if (sCommonHeadbuttEncountersNatNight[slot][2] >= sCommonHeadbuttEncountersNatNight[slot][1])
	//			{
	//				min = sCommonHeadbuttEncountersNatNight[slot][1];
	//				max = sCommonHeadbuttEncountersNatNight[slot][2];
	//			}
	//			else
	//			{
	//				min = sCommonHeadbuttEncountersNatNight[slot][2];
	//				max = sCommonHeadbuttEncountersNatNight[slot][1];
	//			}
	//		}
	//		else if (IsNationalPokedexEnabled())
	//		{
	//			slot = Random() % ARRAY_COUNT(sCommonHeadbuttEncountersNat);
	//			species = sCommonHeadbuttEncountersNat[slot][0];
	//			if (sCommonHeadbuttEncountersNat[slot][2] >= sCommonHeadbuttEncountersNat[slot][1])
	//			{
	//				min = sCommonHeadbuttEncountersNat[slot][1];
	//				max = sCommonHeadbuttEncountersNat[slot][2];
	//			}
	//			else
	//			{
	//				min = sCommonHeadbuttEncountersNat[slot][2];
	//				max = sCommonHeadbuttEncountersNat[slot][1];
	//			}
	//		}
	//		else if (GetCurrentTimeOfDay() == TIME_MORNING)
	//		{
	//			slot = Random() % ARRAY_COUNT(sCommonHeadbuttEncountersMorning);
	//			species = sCommonHeadbuttEncountersMorning[slot][0];
	//			if (sCommonHeadbuttEncountersMorning[slot][2] >= sCommonHeadbuttEncountersMorning[slot][1])
	//			{
	//				min = sCommonHeadbuttEncountersMorning[slot][1];
	//				max = sCommonHeadbuttEncountersMorning[slot][2];
	//			}
	//			else
	//			{
	//				min = sCommonHeadbuttEncountersMorning[slot][2];
	//				max = sCommonHeadbuttEncountersMorning[slot][1];
	//			}
	//		}
	//		else if (GetCurrentTimeOfDay() == TIME_NIGHT)
	//		{
	//			slot = Random() % ARRAY_COUNT(sCommonHeadbuttEncountersNight);
	//			species = sCommonHeadbuttEncountersNight[slot][0];
	//			if (sCommonHeadbuttEncountersNight[slot][2] >= sCommonHeadbuttEncountersNight[slot][1])
	//			{
	//				min = sCommonHeadbuttEncountersNight[slot][1];
	//				max = sCommonHeadbuttEncountersNight[slot][2];
	//			}
	//			else
	//			{
	//				min = sCommonHeadbuttEncountersNight[slot][2];
	//				max = sCommonHeadbuttEncountersNight[slot][1];
	//			}
	//		}
	//		else
	//		{
	//			slot = Random() % ARRAY_COUNT(sCommonHeadbuttEncounters);
	//			species = sCommonHeadbuttEncounters[slot][0];
	//			if (sCommonHeadbuttEncounters[slot][2] >= sCommonHeadbuttEncounters[slot][1])
	//			{
	//				min = sCommonHeadbuttEncounters[slot][1];
	//				max = sCommonHeadbuttEncounters[slot][2];
	//			}
	//			else
	//			{
	//				min = sCommonHeadbuttEncounters[slot][2];
	//				max = sCommonHeadbuttEncounters[slot][1];
	//			}
	//		}
	//	}
	//	else if (((inhabitedX >> treeX) & 3) == 1 || ((inhabitedY >> treeY) & 3) == 1)
	//	{
	//		if (IsNationalPokedexEnabled() && GetCurrentTimeOfDay() == TIME_MORNING)
	//		{
	//			slot = Random() % ARRAY_COUNT(sUncommonHeadbuttEncountersNatMorning);
	//			species = sUncommonHeadbuttEncountersNatMorning[slot][0];
	//			if (sUncommonHeadbuttEncountersNatMorning[slot][2] >= sUncommonHeadbuttEncountersNatMorning[slot][1])
	//			{
	//				min = sUncommonHeadbuttEncountersNatMorning[slot][1];
	//				max = sUncommonHeadbuttEncountersNatMorning[slot][2];
	//			}
	//			else
	//			{
	//				min = sUncommonHeadbuttEncountersNatMorning[slot][2];
	//				max = sUncommonHeadbuttEncountersNatMorning[slot][1];
	//			}
	//		}
	//		else if (IsNationalPokedexEnabled() && GetCurrentTimeOfDay() == TIME_NIGHT)
	//		{
	//			slot = Random() % ARRAY_COUNT(sUncommonHeadbuttEncountersNatNight);
	//			species = sUncommonHeadbuttEncountersNatNight[slot][0];
	//			if (sUncommonHeadbuttEncountersNatNight[slot][2] >= sUncommonHeadbuttEncountersNatNight[slot][1])
	//			{
	//				min = sUncommonHeadbuttEncountersNatNight[slot][1];
	//				max = sUncommonHeadbuttEncountersNatNight[slot][2];
	//			}
	//			else
	//			{
	//				min = sUncommonHeadbuttEncountersNatNight[slot][2];
	//				max = sUncommonHeadbuttEncountersNatNight[slot][1];
	//			}
	//		}
	//		else if (IsNationalPokedexEnabled())
	//		{
	//			slot = Random() % ARRAY_COUNT(sUncommonHeadbuttEncountersNat);
	//			species = sUncommonHeadbuttEncountersNat[slot][0];
	//			if (sUncommonHeadbuttEncountersNat[slot][2] >= sUncommonHeadbuttEncountersNat[slot][1])
	//			{
	//				min = sUncommonHeadbuttEncountersNat[slot][1];
	//				max = sUncommonHeadbuttEncountersNat[slot][2];
	//			}
	//			else
	//			{
	//				min = sUncommonHeadbuttEncountersNat[slot][2];
	//				max = sUncommonHeadbuttEncountersNat[slot][1];
	//			}
	//		}
	//		else if (GetCurrentTimeOfDay() == TIME_MORNING)
	//		{
	//			slot = Random() % ARRAY_COUNT(sUncommonHeadbuttEncountersMorning);
	//			species = sUncommonHeadbuttEncountersMorning[slot][0];
	//			if (sUncommonHeadbuttEncountersMorning[slot][2] >= sUncommonHeadbuttEncountersMorning[slot][1])
	//			{
	//				min = sUncommonHeadbuttEncountersMorning[slot][1];
	//				max = sUncommonHeadbuttEncountersMorning[slot][2];
	//			}
	//			else
	//			{
	//				min = sUncommonHeadbuttEncountersMorning[slot][2];
	//				max = sUncommonHeadbuttEncountersMorning[slot][1];
	//			}
	//		}
	//		else if (GetCurrentTimeOfDay() == TIME_NIGHT)
	//		{
	//			slot = Random() % ARRAY_COUNT(sUncommonHeadbuttEncountersNight);
	//			species = sUncommonHeadbuttEncountersNight[slot][0];
	//			if (sUncommonHeadbuttEncountersNight[slot][2] >= sUncommonHeadbuttEncountersNight[slot][1])
	//			{
	//				min = sUncommonHeadbuttEncountersNight[slot][1];
	//				max = sUncommonHeadbuttEncountersNight[slot][2];
	//			}
	//			else
	//			{
	//				min = sUncommonHeadbuttEncountersNight[slot][2];
	//				max = sUncommonHeadbuttEncountersNight[slot][1];
	//			}
	//		}
	//		else
	//		{
	//			slot = Random() % ARRAY_COUNT(sUncommonHeadbuttEncounters);
	//			species = sUncommonHeadbuttEncounters[slot][0];
	//			if (sUncommonHeadbuttEncounters[slot][2] >= sUncommonHeadbuttEncounters[slot][1])
	//			{
	//				min = sUncommonHeadbuttEncounters[slot][1];
	//				max = sUncommonHeadbuttEncounters[slot][2];
	//			}
	//			else
	//			{
	//				min = sUncommonHeadbuttEncounters[slot][2];
	//				max = sUncommonHeadbuttEncounters[slot][1];
	//			}
	//		}
	//	}
	//	else
	//	{
	//		gSpecialVar_Result = FALSE;
	//	}
	//}
	//else
	//{
	//	gSpecialVar_Result = FALSE;
	//}
    //
	//if (gSpecialVar_Result == TRUE)
	//{
	//	u32 status = STATUS1_SLEEP;
	//	range = max - min + 1;
	//	level = Random() % range + min;
	//	CreateWildMon(species, level, 0);
	//	if (IsMonShiny(&gEnemyParty[0]))
	//		IncrementGameStat(GAME_STAT_SHINIES_FOUND);
	//	if (GetCurrentTimeOfDay() == TIME_NIGHT && (Random() % 10))
	//		SetMonData(&gEnemyParty[0], MON_DATA_STATUS, &status);
	//	BattleSetup_StartWildBattle();
	//}
    
    gSpecialVar_Result = FALSE;
}
//TODO
bool8 SweetScentWildEncounter(void)
{
    //struct Roamer *roamer;
    //s16 x, y;
    //u16 oppositeHeaderId;
    //u16 headerId = GetEmeraldWildMonHeaderId();;
    //
    //gEncounterMode = ENCOUNTER_EMERALD;
    //
    //if (headerId == HEADER_NONE)
    //{
    //    headerId = GetFireRedWildMonHeaderId();
    //
    //    if (headerId != HEADER_NONE)
    //        gEncounterMode = ENCOUNTER_FIRERED;
    //}
    //
    //PlayerGetDestCoords(&x, &y);
    //if (headerId == HEADER_NONE)
    //{
    //    if (gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS)
    //    {
    //        headerId = GetBattlePikeWildMonHeaderId();
    //        if (TryGenerateWildMon(gBattlePikeWildMonHeaders[headerId].landMonsInfo, WILD_AREA_LAND, 0) != TRUE)
    //            return FALSE;
    //
    //        TryGenerateBattlePikeWildMon(FALSE);
    //        BattleSetup_StartBattlePikeWildBattle();
    //        return TRUE;
    //    }
    //    if (gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR)
    //    {
    //        headerId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
    //        if (TryGenerateWildMon(gBattlePyramidWildMonHeaders[headerId].landMonsInfo, WILD_AREA_LAND, 0) != TRUE)
    //            return FALSE;
    //
    //        GenerateBattlePyramidWildMon();
    //        BattleSetup_StartWildBattle();
    //        return TRUE;
    //    }
    //}
    //else
    //{
    //    if (MetatileBehavior_IsLandWildEncounter(MapGridGetMetatileBehaviorAt(x, y)) == TRUE)
    //    {
    //        const struct WildPokemonInfo *wildPokemonInfo;
    //
    //        if (gEncounterMode == ENCOUNTER_FIRERED)
    //        {
    //            wildPokemonInfo = gWildMonHeadersFRLG[headerId].landMonsInfo;
    //
    //            if (Random() % 2)
    //            {
    //                oppositeHeaderId = GetCorrespondingLeafGreenWildMonHeaderId(headerId);
    //
    //                if (oppositeHeaderId != HEADER_NONE)
    //                {
    //                    gEncounterMode = ENCOUNTER_LEAFGREEN;
    //                    wildPokemonInfo = gWildMonHeadersFRLG[oppositeHeaderId].landMonsInfo;
    //                }
    //            }
    //        }
    //        else
    //        {
    //            wildPokemonInfo = gWildMonHeadersE[headerId].landMonsInfo;
    //
    //            switch (Random() % 3)
    //            {
    //                case 0:
    //                    headerId = GetRubyWildMonHeaderId();
    //
    //                    if (headerId != HEADER_NONE)
    //                    {
    //                        gEncounterMode = ENCOUNTER_RUBY;
    //                        wildPokemonInfo = gWildMonHeadersRS[headerId].landMonsInfo;
    //                    }
    //                    break;
    //                case 1:
    //                    headerId = GetRubyWildMonHeaderId();
    //
    //                    if (headerId != HEADER_NONE)
    //                    {
    //                        oppositeHeaderId = GetCorrespondingSapphireWildMonHeaderId(headerId);
    //
    //                        if (oppositeHeaderId != HEADER_NONE)
    //                        {
    //                            gEncounterMode = ENCOUNTER_SAPPHIRE;
    //                            wildPokemonInfo = gWildMonHeadersRS[oppositeHeaderId].landMonsInfo;
    //                        }
    //                    }
    //                    break;
    //            }
    //        }
    //
    //        if (gWildMonHeaders[headerId].landMonsInfo == NULL)
    //            return FALSE;
    //
    //        if (TryStartRoamerEncounter() == TRUE)
    //        {
    //            BattleSetup_StartRoamerBattle();
    //            return TRUE;
    //        }
    //
    //        //if (DoMassOutbreakEncounterTest(FALSE) == TRUE)
    //        //    SetUpMassOutbreakEncounter(0);
    //        //else
    //            TryGenerateWildMon(wildPokemonInfo, WILD_AREA_LAND, 0);
    //
	//		if (IsMonShiny(&gEnemyParty[0]))
	//			IncrementGameStat(GAME_STAT_SHINIES_FOUND);
    //
    //        BattleSetup_StartWildBattle();
    //        return TRUE;
    //    }
    //    else if (MetatileBehavior_IsWaterWildEncounter(MapGridGetMetatileBehaviorAt(x, y)) == TRUE
    //             || (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING) && MetatileBehavior_IsBridgeOverWater(MapGridGetMetatileBehaviorAt(x, y)) == TRUE))
    //    {
    //        const struct WildPokemonInfo *wildPokemonInfo;
    //
    //        if (gEncounterMode == ENCOUNTER_FIRERED)
    //        {
    //            wildPokemonInfo = gWildMonHeadersFRLG[headerId].waterMonsInfo;
    //
    //            if (Random() % 2)
    //            {
    //                oppositeHeaderId = GetCorrespondingLeafGreenWildMonHeaderId(headerId);
    //
    //                if (oppositeHeaderId != HEADER_NONE)
    //                {
    //                    gEncounterMode = ENCOUNTER_LEAFGREEN;
    //                    wildPokemonInfo = gWildMonHeadersFRLG[oppositeHeaderId].waterMonsInfo;
    //                }
    //            }
    //        }
    //        else
    //        {
    //            wildPokemonInfo = gWildMonHeadersE[headerId].waterMonsInfo;
    //
    //            switch (Random() % 3)
    //            {
    //                case 0:
    //                    headerId = GetRubyWildMonHeaderId();
    //
    //                    if (headerId != HEADER_NONE)
    //                    {
    //                        gEncounterMode = ENCOUNTER_RUBY;
    //                        wildPokemonInfo = gWildMonHeadersRS[headerId].waterMonsInfo;
    //                    }
    //                    break;
    //                case 1:
    //                    headerId = GetRubyWildMonHeaderId();
    //
    //                    if (headerId != HEADER_NONE)
    //                    {
    //                        oppositeHeaderId = GetCorrespondingSapphireWildMonHeaderId(headerId);
    //
    //                        if (oppositeHeaderId != HEADER_NONE)
    //                        {
    //                            gEncounterMode = ENCOUNTER_SAPPHIRE;
    //                            wildPokemonInfo = gWildMonHeadersRS[oppositeHeaderId].waterMonsInfo;
    //                        }
    //                    }
    //                    break;
    //            }
    //        }
    //
    //        if (AreLegendariesInSootopolisPreventingEncounters() == TRUE)
    //            return FALSE;
    //        if (gWildMonHeaders[headerId].waterMonsInfo == NULL)
    //            return FALSE;
    //
    //        roamer = &gSaveBlock1Ptr->roamer;
    //
    //        if (TryStartRoamerEncounter() == TRUE && roamer->species != SPECIES_RAIKOU && roamer->species != SPECIES_ENTEI)
    //        {
    //            BattleSetup_StartRoamerBattle();
    //            return TRUE;
    //        }
    //
    //        //if (DoMassOutbreakEncounterTest(TRUE) == TRUE)
    //        //    SetUpMassOutbreakEncounter(0);
	//		//else
	//			TryGenerateWildMon(wildPokemonInfo, WILD_AREA_WATER, 0);
    //
	//		if (IsMonShiny(&gEnemyParty[0]))
	//			IncrementGameStat(GAME_STAT_SHINIES_FOUND);
    //
    //        BattleSetup_StartWildBattle();
    //        return TRUE;
    //    }
    //    //else if (MetatileBehavior_IsPuddle(MapGridGetMetatileBehaviorAt(x, y)) == TRUE)
    //    //{
    //    //    if (gWildMonHeaders[headerId].puddleMonsInfo == NULL)
    //    //        return FALSE;
    //    //
	//	//	if (IsNationalPokedexEnabled() && GetCurrentTimeOfDay() == TIME_MORNING && gWildMonHeaders[headerId].puddleMonsNatMorningInfo != NULL)
	//	//		TryGenerateWildMon(gWildMonHeaders[headerId].puddleMonsNatMorningInfo, WILD_AREA_WATER, 0);
	//	//	else if (IsNationalPokedexEnabled() && GetCurrentTimeOfDay() == TIME_NIGHT && gWildMonHeaders[headerId].puddleMonsNatNightInfo != NULL)
	//	//		TryGenerateWildMon(gWildMonHeaders[headerId].puddleMonsNatNightInfo, WILD_AREA_WATER, 0);
	//	//	else if (IsNationalPokedexEnabled() && gWildMonHeaders[headerId].puddleMonsNatInfo != NULL)
	//	//		TryGenerateWildMon(gWildMonHeaders[headerId].puddleMonsNatInfo, WILD_AREA_WATER, 0);
	//	//	else if (GetCurrentTimeOfDay() == TIME_MORNING && gWildMonHeaders[headerId].puddleMonsMorningInfo != NULL)
	//	//		TryGenerateWildMon(gWildMonHeaders[headerId].puddleMonsMorningInfo, WILD_AREA_WATER, 0);
	//	//	else if (GetCurrentTimeOfDay() == TIME_NIGHT && gWildMonHeaders[headerId].puddleMonsNightInfo != NULL)
	//	//		TryGenerateWildMon(gWildMonHeaders[headerId].puddleMonsNightInfo, WILD_AREA_WATER, 0);
	//	//	else
	//	//		TryGenerateWildMon(gWildMonHeaders[headerId].puddleMonsInfo, WILD_AREA_WATER, 0);
	//	//	if (IsMonShiny(&gEnemyParty[0]))
	//	//		IncrementGameStat(GAME_STAT_SHINIES_FOUND);
    //    //    BattleSetup_StartWildBattle();
    //    //    return TRUE;
    //    //}
    //}

    return FALSE;
}
//TODO
bool8 DoesCurrentMapHaveFishingMons(void)
{
    u16 headerId = GetEmeraldWildMonHeaderId();

    if (headerId == HEADER_NONE)
        headerId = GetFireRedWildMonHeaderId();

    if (headerId != HEADER_NONE && gWildMonHeaders[headerId].fishingMonsInfo != NULL)
        return TRUE;
    else
        return FALSE;
}

void FishingWildEncounter(u8 rod)
{
    const struct WildPokemonInfo *wildPokemonInfo;
    u16 species, oppositeHeaderId;
    u16 headerId = GetEmeraldWildMonHeaderId();

    gEncounterMode = ENCOUNTER_EMERALD;

    if (headerId == HEADER_NONE)
    {
        headerId = GetFireRedWildMonHeaderId();

        if (headerId != HEADER_NONE)
            gEncounterMode = ENCOUNTER_FIRERED;
    }

    if (CheckFeebas() == TRUE)
    {
        u8 level = ChooseWildMonLevel(&sWildFeebas);

        species = sWildFeebas.species;
        CreateWildMon(species, level, 0);
    }
    else
    {
        if (gEncounterMode == ENCOUNTER_FIRERED)
        {
            wildPokemonInfo = gWildMonHeadersFRLG[headerId].fishingMonsInfo;

            switch (Random() % 3)
            {
                case 1:
                    oppositeHeaderId = GetCorrespondingLeafGreenWildMonHeaderId(headerId);

                    if (oppositeHeaderId != HEADER_NONE)
                    {
                        gEncounterMode = ENCOUNTER_LEAFGREEN;
                        wildPokemonInfo = gWildMonHeadersFRLG[oppositeHeaderId].fishingMonsInfo;
                    }
                    break;
                case 2:
                    headerId = GetHeliodorWildMonHeaderId();

                    if (headerId != HEADER_NONE)
                    {
                        gEncounterMode = ENCOUNTER_HELIODOR;
                        wildPokemonInfo = gWildMonHeaders[headerId].fishingMonsInfo;
                    }
                    break;
            }
        }
        else
        {
            wildPokemonInfo = gWildMonHeadersE[headerId].fishingMonsInfo;

            switch (Random() % 4)
            {
                case 0:
                    headerId = GetRubyWildMonHeaderId();

                    if (headerId != HEADER_NONE)
                    {
                        gEncounterMode = ENCOUNTER_RUBY;
                        wildPokemonInfo = gWildMonHeadersRS[headerId].fishingMonsInfo;
                    }
                    break;
                case 1:
                    headerId = GetRubyWildMonHeaderId();

                    if (headerId != HEADER_NONE)
                    {
                        oppositeHeaderId = GetCorrespondingSapphireWildMonHeaderId(headerId);

                        if (oppositeHeaderId != HEADER_NONE)
                        {
                            gEncounterMode = ENCOUNTER_SAPPHIRE;
                            wildPokemonInfo = gWildMonHeadersRS[oppositeHeaderId].fishingMonsInfo;
                        }
                    }
                    break;
                case 3:
                    headerId = GetHeliodorWildMonHeaderId();

                    if (headerId != HEADER_NONE)
                    {
                        gEncounterMode = ENCOUNTER_HELIODOR;
                        wildPokemonInfo = gWildMonHeaders[headerId].fishingMonsInfo;
                    }
                    break;
            }
        }

		species = GenerateFishingWildMon(wildPokemonInfo, rod);
    }

    IncrementGameStat(GAME_STAT_FISHING_ENCOUNTERS);
    SetPokemonAnglerSpecies(species);
	if (IsMonShiny(&gEnemyParty[0]))
		IncrementGameStat(GAME_STAT_SHINIES_FOUND);
    gFishingEncounter = TRUE;
    BattleSetup_StartWildBattle();
}

u16 GetLocalWildMon(bool8 *isWaterMon)
{
    const struct WildPokemonInfo *landMonsInfo;
    const struct WildPokemonInfo *waterMonsInfo;
    u16 oppositeHeaderId;
    u16 headerId = GetEmeraldWildMonHeaderId();

    gEncounterMode = ENCOUNTER_EMERALD;

    if (headerId == HEADER_NONE)
    {
        headerId = GetFireRedWildMonHeaderId();

        if (headerId != HEADER_NONE)
            gEncounterMode = ENCOUNTER_FIRERED;
    }

    if (gEncounterMode == ENCOUNTER_FIRERED)
    {
        landMonsInfo = gWildMonHeadersFRLG[headerId].landMonsInfo;
        waterMonsInfo = gWildMonHeadersFRLG[headerId].waterMonsInfo;
        
        switch (Random() % 3)
        {
            case 1:
                oppositeHeaderId = GetCorrespondingLeafGreenWildMonHeaderId(headerId);
        
                if (oppositeHeaderId != HEADER_NONE)
                {
                    gEncounterMode = ENCOUNTER_LEAFGREEN;
                    landMonsInfo = gWildMonHeadersFRLG[oppositeHeaderId].landMonsInfo;
                    waterMonsInfo = gWildMonHeadersFRLG[oppositeHeaderId].waterMonsInfo;
                }
                break;
            case 2:
                headerId = GetHeliodorWildMonHeaderId();

                if (headerId != HEADER_NONE)
                {
                    gEncounterMode = ENCOUNTER_HELIODOR;
                    landMonsInfo = gWildMonHeaders[headerId].landMonsInfo;
                    waterMonsInfo = gWildMonHeaders[headerId].waterMonsInfo;
                }
                break;
        }
    }
    else
    {
        landMonsInfo = gWildMonHeadersE[headerId].landMonsInfo;
        waterMonsInfo = gWildMonHeadersE[headerId].waterMonsInfo;

        switch (Random() % 4)
        {
            case 0:
                headerId = GetRubyWildMonHeaderId();

                if (headerId != HEADER_NONE)
                {
                    gEncounterMode = ENCOUNTER_RUBY;
                    landMonsInfo = gWildMonHeadersRS[headerId].landMonsInfo;
                    waterMonsInfo = gWildMonHeadersRS[headerId].waterMonsInfo;
                }
                break;
            case 1:
                headerId = GetRubyWildMonHeaderId();

                if (headerId != HEADER_NONE)
                {
                    oppositeHeaderId = GetCorrespondingSapphireWildMonHeaderId(headerId);

                    if (oppositeHeaderId != HEADER_NONE)
                    {
                        gEncounterMode = ENCOUNTER_SAPPHIRE;
                        landMonsInfo = gWildMonHeadersRS[oppositeHeaderId].landMonsInfo;
                        waterMonsInfo = gWildMonHeadersRS[oppositeHeaderId].waterMonsInfo;
                    }
                }
                break;
            case 3:
                headerId = GetHeliodorWildMonHeaderId();

                if (headerId != HEADER_NONE)
                {
                    gEncounterMode = ENCOUNTER_HELIODOR;
                    landMonsInfo = gWildMonHeaders[headerId].landMonsInfo;
                    waterMonsInfo = gWildMonHeaders[headerId].waterMonsInfo;
                }
                break;
        }
    }

    if (headerId == HEADER_NONE)
        return SPECIES_NONE;

    // Neither
    if (landMonsInfo == NULL && waterMonsInfo == NULL)
        return SPECIES_NONE;
    // Land Pokémon
    else if (landMonsInfo != NULL && waterMonsInfo == NULL)
        return landMonsInfo->wildPokemon[ChooseWildMonIndex_Land()].species;
    // Water Pokémon
    else if (landMonsInfo == NULL && waterMonsInfo != NULL)
    {
        *isWaterMon = TRUE;
        return waterMonsInfo->wildPokemon[ChooseWildMonIndex_WaterRock()].species;
    }
    // Either land or water Pokémon
    if ((Random() % 100) < 80)
    {
        return landMonsInfo->wildPokemon[ChooseWildMonIndex_Land()].species;
    }
    else
    {
        *isWaterMon = TRUE;
        return waterMonsInfo->wildPokemon[ChooseWildMonIndex_WaterRock()].species;
    }
}

u16 GetLocalWaterMon(void)
{
    const struct WildPokemonInfo *waterMonsInfo;
    u16 oppositeHeaderId;
    u16 headerId = GetEmeraldWildMonHeaderId();

    gEncounterMode = ENCOUNTER_EMERALD;

    if (headerId == HEADER_NONE)
    {
        headerId = GetFireRedWildMonHeaderId();

        if (headerId != HEADER_NONE)
            gEncounterMode = ENCOUNTER_FIRERED;
    }

    if (gEncounterMode == ENCOUNTER_FIRERED)
    {
        waterMonsInfo = gWildMonHeadersFRLG[headerId].waterMonsInfo;
        
        switch (Random() % 2)
        {
            case 1:
                oppositeHeaderId = GetCorrespondingLeafGreenWildMonHeaderId(headerId);
        
                if (oppositeHeaderId != HEADER_NONE)
                {
                    gEncounterMode = ENCOUNTER_LEAFGREEN;
                    waterMonsInfo = gWildMonHeadersFRLG[oppositeHeaderId].waterMonsInfo;
                }
                break;
            case 2:
                headerId = GetHeliodorWildMonHeaderId();

                if (headerId != HEADER_NONE)
                {
                    gEncounterMode = ENCOUNTER_HELIODOR;
                    waterMonsInfo = gWildMonHeaders[headerId].waterMonsInfo;
                }
                break;
        }
    }
    else
    {
        waterMonsInfo = gWildMonHeadersE[headerId].waterMonsInfo;

        switch (Random() % 4)
        {
            case 0:
                headerId = GetRubyWildMonHeaderId();

                if (headerId != HEADER_NONE)
                {
                    gEncounterMode = ENCOUNTER_RUBY;
                    waterMonsInfo = gWildMonHeadersRS[headerId].waterMonsInfo;
                }
                break;
            case 1:
                headerId = GetRubyWildMonHeaderId();

                if (headerId != HEADER_NONE)
                {
                    oppositeHeaderId = GetCorrespondingSapphireWildMonHeaderId(headerId);

                    if (oppositeHeaderId != HEADER_NONE)
                    {
                        gEncounterMode = ENCOUNTER_SAPPHIRE;
                        waterMonsInfo = gWildMonHeadersRS[oppositeHeaderId].waterMonsInfo;
                    }
                }
                break;
            case 2:
                headerId = GetHeliodorWildMonHeaderId();

                if (headerId != HEADER_NONE)
                {
                    gEncounterMode = ENCOUNTER_HELIODOR;
                    waterMonsInfo = gWildMonHeaders[headerId].waterMonsInfo;
                }
                break;
        }
    }

    if (headerId == HEADER_NONE)
        return SPECIES_NONE;
    if (waterMonsInfo)
        return waterMonsInfo->wildPokemon[ChooseWildMonIndex_WaterRock()].species;

    return SPECIES_NONE;
}

bool8 UpdateRepelCounter(void)
{
    u16 steps;

    if (InBattlePike() || InBattlePyramid())
        return FALSE;
    if (InUnionRoom() == TRUE)
        return FALSE;

    steps = VarGet(VAR_REPEL_STEP_COUNT);

    if (steps != 0)
    {
        steps--;
        VarSet(VAR_REPEL_STEP_COUNT, steps);
        if (steps == 0)
        {
            ScriptContext_SetupScript(EventScript_RepelWoreOff);
            return TRUE;
        }
    }
    return FALSE;
}

static bool8 IsWildLevelAllowedByRepel(u8 wildLevel)
{
    u8 i;

    if (!VarGet(VAR_REPEL_STEP_COUNT))
        return TRUE;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_HP) && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
        {
            u8 ourLevel = GetMonData(&gPlayerParty[i], MON_DATA_LEVEL);

            if (wildLevel < ourLevel)
                return FALSE;
            else
                return TRUE;
        }
    }

    return FALSE;
}

static bool8 IsAbilityAllowingEncounter(u8 level)
{
    u8 ability;

    if (GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
        return TRUE;

    ability = GetMonAbility(&gPlayerParty[0]);
    if (ability == ABILITY_KEEN_EYE || ability == ABILITY_INTIMIDATE)
    {
        u8 playerMonLevel = GetMonData(&gPlayerParty[0], MON_DATA_LEVEL);
        if (playerMonLevel > 5 && level <= playerMonLevel - 5 && !(Random() % 2))
            return FALSE;
    }

    return TRUE;
}

static bool8 TryGetRandomWildMonIndexByType(const struct WildPokemon *wildMon, u8 type, u8 numMon, u8 *monIndex)
{
    u8 validIndexes[numMon]; // variable length array, an interesting feature
    u8 i, validMonCount;

    for (i = 0; i < numMon; i++)
        validIndexes[i] = 0;

    for (validMonCount = 0, i = 0; i < numMon; i++)
    {
        if (gSpeciesInfo[wildMon[i].species].types[0] == type || gSpeciesInfo[wildMon[i].species].types[1] == type)
            validIndexes[validMonCount++] = i;
    }

    if (validMonCount == 0 || validMonCount == numMon)
        return FALSE;

    *monIndex = validIndexes[Random() % validMonCount];
    return TRUE;
}

static bool8 TryGetAbilityInfluencedWildMonIndex(const struct WildPokemon *wildMon, u8 type, u8 ability, u8 *monIndex, u32 size)
{
    if (GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
        return FALSE;
    else if (GetMonAbility(&gPlayerParty[0]) != ability)
        return FALSE;
    else if (Random() % 2 != 0)
        return FALSE;

    return TryGetRandomWildMonIndexByType(wildMon, type, size, monIndex);
}

static void ApplyFluteEncounterRateMod(u32 *encRate)
{
    if (FlagGet(FLAG_SYS_ENC_UP_ITEM) == TRUE)
        *encRate += *encRate / 2;
    else if (FlagGet(FLAG_SYS_ENC_DOWN_ITEM) == TRUE)
        *encRate = *encRate / 2;
}

static void ApplyCleanseTagEncounterRateMod(u32 *encRate)
{
    if (GetMonData(&gPlayerParty[0], MON_DATA_HELD_ITEM) == ITEM_CLEANSE_TAG)
        *encRate = *encRate * 2 / 3;
}

static bool8 TryToScopeSpecies(const struct WildPokemon *wildMon, u8 *monIndex, u32 size)
{
	u8 validIndexes[size];
	u8 i;
	u8 validMonCount;
	
	if (GetMonData(&gPlayerParty[0], MON_DATA_HELD_ITEM) != ITEM_SCOPE_LENS || Random() % 100 < 2)
	{
		return FALSE;
	}
	
	for (i = 0; i < size; i++)
		validIndexes[i] = 0;
	for (validMonCount = 0, i = 0; i < size; i++)
	{
		if (wildMon[i].species == GetMonData(&gPlayerParty[0], MON_DATA_SPECIES))
			validIndexes[validMonCount++] = i;
	}
	if (validMonCount == 0 || validMonCount == size)
		return FALSE;
	
	*monIndex = validIndexes[Random() % validMonCount];
	return TRUE;
}

//static bool8 RubyEncounter(const struct WildPokemonInfo *wildMonInfo, u8 area, u8 partySlot)
//{
//    u32 encounterRate, wildMonIndex, level;
//    struct Roamer *roamer;
//
//    encounterRate = wildMonInfo->encounterRate * 16;
//
//    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
//        encounterRate = encounterRate * 80 / 100;
//
//    ApplyFluteEncounterRateMod(&encounterRate);
//    ApplyCleanseTagEncounterRateMod(&encounterRate);
//
//    if (!GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
//    {
//        u32 ability = GetMonAbility(&gPlayerParty[0]);
//        if (ability == ABILITY_STENCH)
//            encounterRate /= 2;
//        if (ability == ABILITY_ILLUMINATE)
//            encounterRate *= 2;
//    }
//
//    if (encounterRate > MAX_ENCOUNTER_RATE)
//        encounterRate = MAX_ENCOUNTER_RATE;
//
//    if (!EncounterOddsCheck(encounterRate))
//        return ENCOUNTER_FAIL;
//
//    if (TryStartRoamerEncounter() == TRUE)
//    {
//        roamer = &gSaveBlock1Ptr->roamer;
//        if (!IsWildLevelAllowedByRepel(roamer->level) || (area == WILD_AREA_WATER && roamer->species == SPECIES_RAIKOU) || (area == WILD_AREA_WATER && roamer->species == SPECIES_ENTEI))
//            return ENCOUNTER_FAIL;
//
//        return ENCOUNTER_ROAMER;
//    }
//    //else if (DoMassOutbreakEncounterTest(FALSE) == TRUE && SetUpMassOutbreakEncounter(WILD_CHECK_REPEL | WILD_CHECK_KEEN_EYE) == TRUE)
//    //{
//    //    return ENCOUNTER_SWARM;
//    //}
//
//    switch (area)
//    {
//        case 0:
//            wildMonIndex = ChooseWildMonIndex_Land();
//            break;
//        case 1:
//            wildMonIndex = ChooseWildMonIndex_WaterRock();
//            break;
//        case 2:
//            wildMonIndex = ChooseWildMonIndex_WaterRock();
//            break;
//    }
//
//    level = ChooseWildMonLevel(&wildMonInfo->wildPokemon[wildMonIndex]);
//
//    if (IsWildLevelAllowedByRepel(level) == FALSE)
//        return ENCOUNTER_FAIL;
//
//    CreateWildMon(wildMonInfo->wildPokemon[wildMonIndex].species, level, partySlot);
//
//    return ENCOUNTER_SUCCESS;
//}
//
//static u8 FireRedEncounter(const struct WildPokemonInfo *wildMonInfo, u8 area, u8 partySlot)
//{
//    u32 encounterRate, wildMonIndex, level;
//    struct Roamer *roamer;
//
//    encounterRate = wildMonInfo->encounterRate * 16;
//
//    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
//        encounterRate = encounterRate * 80 / 100;
//
//    //encounterRate += sWildEncounterData.encounterRateBuff * 16 / 200;
//    ApplyFluteEncounterRateMod(&encounterRate);
//    ApplyCleanseTagEncounterRateMod(&encounterRate);
//
//    if (!GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
//    {
//        u32 ability = GetMonAbility(&gPlayerParty[0]);
//        if (ability == ABILITY_STENCH)
//            encounterRate /= 2;
//        if (ability == ABILITY_ILLUMINATE)
//            encounterRate *= 2;
//    }
//
//    if (encounterRate > 1600)
//        encounterRate = 1600;
//
//    if (!EncounterOddsCheck(encounterRate))
//        return ENCOUNTER_FAIL;
//
//   //if (TryStartRoamerEncounter() == TRUE)
//   //{
//   //    roamer = &gSaveBlock1Ptr->roamer;
//   //    if (!IsWildLevelAllowedByRepel(roamer->level) || (area == WILD_AREA_WATER && roamer->species == SPECIES_RAIKOU) || (area == WILD_AREA_WATER && roamer->species == SPECIES_ENTEI))
//   //        return ENCOUNTER_FAIL;
//   //
//   //    return ENCOUNTER_ROAMER;
//   //}
//    //else if (DoMassOutbreakEncounterTest(FALSE) == TRUE && SetUpMassOutbreakEncounter(WILD_CHECK_REPEL | WILD_CHECK_KEEN_EYE) == TRUE)
//    //{
//    //    return ENCOUNTER_SWARM;
//    //}
//
//    switch (area)
//    {
//        case 0:
//            wildMonIndex = ChooseWildMonIndex_Land();
//            break;
//        case 1:
//            wildMonIndex = ChooseWildMonIndex_WaterRock();
//            break;
//        case 2:
//            wildMonIndex = ChooseWildMonIndex_WaterRock();
//            break;
//    }
//
//    level = ChooseWildMonLevel(&wildMonInfo->wildPokemon[wildMonIndex]);
//
//    if (IsWildLevelAllowedByRepel(level) == FALSE)
//        return ENCOUNTER_FAIL;
//
//    CreateWildMon(wildMonInfo->wildPokemon[wildMonIndex].species, level, partySlot);
//
//    return ENCOUNTER_SUCCESS;
//}
//
//static bool8 EmeraldEncounter(const struct WildPokemonInfo *wildMonInfo, u8 area, u8 partySlot)
//{
//    u32 encounterRate, level;
//    u8 wildMonIndex;
//    struct Roamer *roamer;
//
//    encounterRate  = wildMonInfo->encounterRate * 16;
//
//    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
//        encounterRate = encounterRate * 80 / 100;
//
//    ApplyFluteEncounterRateMod(&encounterRate);
//    ApplyCleanseTagEncounterRateMod(&encounterRate);
//
//    if (!GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
//    {
//        u32 ability = GetMonAbility(&gPlayerParty[0]);
//        if (ability == ABILITY_STENCH)
//            encounterRate /= 2;
//        if (ability == ABILITY_ILLUMINATE)
//            encounterRate *= 2;
//        else if (ability == ABILITY_WHITE_SMOKE)
//            encounterRate /= 2;
//        else if (ability == ABILITY_ARENA_TRAP)
//            encounterRate *= 2;
//        else if (ability == ABILITY_SAND_VEIL && gSaveBlock1Ptr->weather == WEATHER_SANDSTORM)
//            encounterRate /= 2;
//    }
//
//    if (encounterRate > MAX_ENCOUNTER_RATE)
//        encounterRate = MAX_ENCOUNTER_RATE;
//    if (!EncounterOddsCheck(encounterRate))
//        return ENCOUNTER_FAIL;
//
//    if (TryStartRoamerEncounter() == TRUE)
//    {
//        roamer = &gSaveBlock1Ptr->roamer;
//        if (!IsWildLevelAllowedByRepel(roamer->level) || (area == WILD_AREA_WATER && roamer->species == SPECIES_RAIKOU) || (area == WILD_AREA_WATER && roamer->species == SPECIES_ENTEI))
//            return ENCOUNTER_FAIL;
//
//        return ENCOUNTER_ROAMER;
//    }
//    //else if (DoMassOutbreakEncounterTest(FALSE) == TRUE && SetUpMassOutbreakEncounter(WILD_CHECK_REPEL | WILD_CHECK_KEEN_EYE) == TRUE)
//    //{
//    //    return ENCOUNTER_SWARM;
//    //}
//
//    switch (area)
//    {
//        case WILD_AREA_LAND:
//            if (TryGetAbilityInfluencedWildMonIndex(wildMonInfo->wildPokemon, TYPE_STEEL, ABILITY_MAGNET_PULL, &wildMonIndex, LAND_WILD_COUNT))
//                break;
//            if (TryGetAbilityInfluencedWildMonIndex(wildMonInfo->wildPokemon, TYPE_ELECTRIC, ABILITY_STATIC, &wildMonIndex, LAND_WILD_COUNT))
//                break;
//
//            wildMonIndex = ChooseWildMonIndex_Land();
//            break;
//        case WILD_AREA_WATER:
//            if (TryGetAbilityInfluencedWildMonIndex(wildMonInfo->wildPokemon, TYPE_ELECTRIC, ABILITY_STATIC, &wildMonIndex, WATER_WILD_COUNT))
//                break;
//
//            wildMonIndex = ChooseWildMonIndex_WaterRock();
//            break;
//        case WILD_AREA_ROCKS:
//            wildMonIndex = ChooseWildMonIndex_WaterRock();
//            break;
//    }
//
//    level = ChooseWildMonLevel(&wildMonInfo->wildPokemon[wildMonIndex]);
//
//    if (IsWildLevelAllowedByRepel(level) == FALSE)
//        return ENCOUNTER_FAIL;
//    if (gMapHeader.mapLayoutId != LAYOUT_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS && !IsAbilityAllowingEncounter(level))
//        return ENCOUNTER_FAIL;
//
//    CreateWildMon(wildMonInfo->wildPokemon[wildMonIndex].species, level, partySlot);
//
//    return ENCOUNTER_SUCCESS;
//}
//
//static bool8 HeliodorEncounter(const struct WildPokemonInfo *wildMonInfo, u8 area, u8 partySlot)
//{
//    u32 encounterRate, wildMonIndex, level;
//    struct Roamer *roamer;
//
//    encounterRate = wildMonInfo->encounterRate * 16;
//
//    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
//        encounterRate = encounterRate * 80 / 100;
//
//    ApplyFluteEncounterRateMod(&encounterRate);
//    ApplyCleanseTagEncounterRateMod(&encounterRate);
//
//    if (!GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
//    {
//        u32 ability = GetMonAbility(&gPlayerParty[0]);
//        if (ability == ABILITY_STENCH)
//            encounterRate /= 2;
//        if (ability == ABILITY_ILLUMINATE)
//            encounterRate *= 2;
//    }
//
//    if (encounterRate > MAX_ENCOUNTER_RATE)
//        encounterRate = MAX_ENCOUNTER_RATE;
//
//    if (!EncounterOddsCheck(encounterRate))
//        return ENCOUNTER_FAIL;
//
//    if (TryStartRoamerEncounter() == TRUE)
//    {
//        roamer = &gSaveBlock1Ptr->roamer;
//        if (!IsWildLevelAllowedByRepel(roamer->level) || (area == WILD_AREA_WATER && roamer->species == SPECIES_RAIKOU) || (area == WILD_AREA_WATER && roamer->species == SPECIES_ENTEI))
//            return ENCOUNTER_FAIL;
//
//        return ENCOUNTER_ROAMER;
//    }
//    //else if (DoMassOutbreakEncounterTest(FALSE) == TRUE && SetUpMassOutbreakEncounter(WILD_CHECK_REPEL | WILD_CHECK_KEEN_EYE) == TRUE)
//    //{
//    //    return ENCOUNTER_SWARM;
//    //}
//
//    switch (area)
//    {
//        case 0:
//            wildMonIndex = ChooseWildMonIndex_Land();
//            break;
//        case 1:
//            wildMonIndex = ChooseWildMonIndex_WaterRock();
//            break;
//        case 2:
//            wildMonIndex = ChooseWildMonIndex_WaterRock();
//            break;
//    }
//
//    if (wildMonInfo->wildPokemon[wildMonIndex].species == SPECIES_NONE)
//        return ENCOUNTER_FAIL;
//
//    level = ChooseWildMonLevel(&wildMonInfo->wildPokemon[wildMonIndex]);
//
//    if (IsWildLevelAllowedByRepel(level) == FALSE)
//        return ENCOUNTER_FAIL;
//
//    CreateWildMon(wildMonInfo->wildPokemon[wildMonIndex].species, level, partySlot);
//
//    return ENCOUNTER_SUCCESS;
//}

static u8 EncounterCore(u16 headerId, u16 curMetatileBehavior, u16 prevMetatileBehavior, const struct WildPokemonInfo *wildPokemonInfo, u8 terrain, u8 partySlot)
{
    u32 i;
    u8 flags = WILD_CHECK_REPEL;

    if (gEncounterMode == ENCOUNTER_EMERALD)
        flags |= WILD_CHECK_ABILITY;

    if (prevMetatileBehavior != curMetatileBehavior && !AllowWildCheckOnNewMetatile())
        return ENCOUNTER_FAIL;

    if (WildEncounterCheck(wildPokemonInfo->encounterRate, FALSE) == TRUE)
    {
        if (TryStartRoamerEncounter() == TRUE)
        {
            struct Roamer *roamer = &gSaveBlock1Ptr->roamer;

            if (!IsWildLevelAllowedByRepel(roamer->level))
                return ENCOUNTER_FAIL;

            return ENCOUNTER_ROAMER;
        }
        else
        {
            //if (DoMassOutbreakEncounterTest() == TRUE && SetUpMassOutbreakEncounter(flags, partySlot) == TRUE)
            //    return ENCOUNTER_SWARM;

            if (TryGenerateWildMon(wildPokemonInfo, terrain, flags, partySlot) == TRUE)
                return ENCOUNTER_SUCCESS;

            return ENCOUNTER_FAIL;
        }
    }
    else
    {
        return ENCOUNTER_FAIL;
    }
}

#define FORCE_SPECIES   (1 << 0)
#define FORCE_TYPE      (1 << 1)
#define FORCE_GENDER    (1 << 2)
#define FORCE_NATURE    (1 << 3)
#define FORCE_MAXLEVEL  (1 << 4)

static u8 EncounterLoop(u16 headerId, u16 curMetatileBehavior, u16 prevMetatileBehavior, const struct WildPokemonInfo *wildPokemonInfo, u8 terrain, u8 partySlot, u8 forceFlags, u16 species, u8 type, u8 gender, u8 nature)
{
    do
    {
        sIterations++;
    } while (EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot) != ENCOUNTER_SUCCESS
          || (forceFlags & FORCE_SPECIES && GetMonData(&gEnemyParty[partySlot], MON_DATA_SPECIES) != species)
          || (forceFlags & FORCE_TYPE && (gSpeciesInfo[GetMonData(&gEnemyParty[partySlot], MON_DATA_SPECIES)].types[0] != type && gSpeciesInfo[GetMonData(&gEnemyParty[partySlot], MON_DATA_SPECIES)].types[1] != type))
          || (forceFlags & FORCE_GENDER && GetGenderFromSpeciesAndPersonality(GetMonData(&gEnemyParty[partySlot], MON_DATA_SPECIES), GetMonData(&gEnemyParty[partySlot], MON_DATA_PERSONALITY)) != gender)
          || (forceFlags & FORCE_NATURE && GetMonData(&gEnemyParty[partySlot], MON_DATA_PERSONALITY) % NUM_NATURES != nature)
          || (forceFlags & FORCE_MAXLEVEL && GetMonData(&gEnemyParty[partySlot], MON_DATA_LEVEL) != sMaxLevel));

}

static u8 GenerateEncounter(u16 headerId, u16 curMetatileBehavior, u16 prevMetatileBehavior, const struct WildPokemonInfo *wildPokemonInfo, u8 terrain, u8 partySlot)
{
    u8 result, ability, tableSize, i;
    u16 species = 0;
    u8 type = 0;
    u8 gender = 0;
    u8 nature = 0;
    u32 rolls = 1;
    u8 forceFlags = 0;

    gDisableVBlankRNGAdvance = TRUE;

    if (!GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
        ability = GetMonAbility(&gPlayerParty[0]);
    else
        ability = ABILITY_NONE;

    if (gEncounterMode != ENCOUNTER_EMERALD)
    {
        if (ability == ABILITY_ARENA_TRAP)
        {
            result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);
            if (result == ENCOUNTER_FAIL)
                result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);
        }
        else if (ability == ABILITY_WHITE_SMOKE || (ability == ABILITY_SAND_VEIL && gSaveBlock1Ptr->weather == WEATHER_SANDSTORM))
        {
            result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);
            if (result != ENCOUNTER_FAIL)
                result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);
        }
        else if (ability == ABILITY_KEEN_EYE || ability == ABILITY_INTIMIDATE)
        {
            u8 playerMonLevel = GetMonData(&gPlayerParty[0], MON_DATA_LEVEL);
            result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);
            if (result == ENCOUNTER_SUCCESS && playerMonLevel > 5 && GetMonData(&gEnemyParty[partySlot], MON_DATA_LEVEL) <= playerMonLevel - 5 && !(Random() % 2))
                return ENCOUNTER_FAIL;
        }
        else if (ability == ABILITY_MAGNET_PULL)
        {
            result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);

            if (result == ENCOUNTER_SUCCESS && Random() % 2)
            {
                u8 validIndexes[LAND_WILD_COUNT]; // Watch out if another wild count becomes larger than Land for whatever reason
                u8 validMonCount;

                for (i = 0; i < LAND_WILD_COUNT; i++)
                    validIndexes[i] = 0;

                for (validMonCount = 0, i = 0; i < LAND_WILD_COUNT; i++)
                {
                    if (gSpeciesInfo[wildPokemonInfo->wildPokemon[i].species].types[0] == TYPE_STEEL || gSpeciesInfo[wildPokemonInfo->wildPokemon[i].species].types[1] == TYPE_STEEL)
                        validIndexes[validMonCount++] = i;
                }

                if (validMonCount != 0)
                {
                    CreateWildMon(wildPokemonInfo->wildPokemon[validIndexes[Random() % validMonCount]].species, 5, partySlot); // In case we need to chain, we can pull species now
                    type = TYPE_STEEL;
                    forceFlags |= FORCE_TYPE;
                }
            }
        }
        else if (ability == ABILITY_STATIC || ability == ABILITY_LIGHTNING_ROD)
        {
            result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);

            if (result == ENCOUNTER_SUCCESS && Random() % 2)
            {
                u8 validIndexes[LAND_WILD_COUNT]; // Watch out if another wild count becomes larger than Land for whatever reason
                u8 validMonCount;

                for (i = 0; i < LAND_WILD_COUNT; i++)
                    validIndexes[i] = 0;

                for (validMonCount = 0, i = 0; i < LAND_WILD_COUNT; i++)
                {
                    if (gSpeciesInfo[wildPokemonInfo->wildPokemon[i].species].types[0] == TYPE_ELECTRIC || gSpeciesInfo[wildPokemonInfo->wildPokemon[i].species].types[1] == TYPE_ELECTRIC)
                        validIndexes[validMonCount++] = i;
                }

                if (validMonCount != 0)
                {
                    CreateWildMon(wildPokemonInfo->wildPokemon[validIndexes[Random() % validMonCount]].species, 5, partySlot); // In case we need to chain, we can pull species now
                    type = TYPE_ELECTRIC;
                    forceFlags |= FORCE_TYPE;
                }
            }
        }
        else if (ability == ABILITY_FLASH_FIRE)
        {
            result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);

            if (result == ENCOUNTER_SUCCESS && Random() % 2)
            {
                u8 validIndexes[LAND_WILD_COUNT]; // Watch out if another wild count becomes larger than Land for whatever reason
                u8 validMonCount;

                for (i = 0; i < LAND_WILD_COUNT; i++)
                    validIndexes[i] = 0;

                for (validMonCount = 0, i = 0; i < LAND_WILD_COUNT; i++)
                {
                    if (gSpeciesInfo[wildPokemonInfo->wildPokemon[i].species].types[0] == TYPE_FIRE || gSpeciesInfo[wildPokemonInfo->wildPokemon[i].species].types[1] == TYPE_FIRE)
                        validIndexes[validMonCount++] = i;
                }

                if (validMonCount != 0)
                {
                    CreateWildMon(wildPokemonInfo->wildPokemon[validIndexes[Random() % validMonCount]].species, 5, partySlot); // In case we need to chain, we can pull species now
                    type = TYPE_FIRE;
                    forceFlags |= FORCE_TYPE;
                }
            }
        }
        else
        {
            result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);
        }

        if (ability == ABILITY_CUTE_CHARM && Random() % 3 != 0)
        {
            if (gSpeciesInfo[GetMonData(&gEnemyParty[partySlot], MON_DATA_SPECIES)].genderRatio != MON_MALE
             && gSpeciesInfo[GetMonData(&gEnemyParty[partySlot], MON_DATA_SPECIES)].genderRatio != MON_FEMALE
             && gSpeciesInfo[GetMonData(&gEnemyParty[partySlot], MON_DATA_SPECIES)].genderRatio != MON_GENDERLESS)
            {
                gender = GetGenderFromSpeciesAndPersonality(GetMonData(&gPlayerParty[0], MON_DATA_SPECIES), GetMonData(&gPlayerParty[0], MON_DATA_PERSONALITY));
                
                if (gender == MON_FEMALE)
                    gender = MON_MALE;
                else
                    gender = MON_FEMALE;

                forceFlags |= FORCE_GENDER;
            }
        }

        if ((ability == ABILITY_HUSTLE || ability == ABILITY_VITAL_SPIRIT || ability == ABILITY_PRESSURE) && (Random() % 2 == 0))
            forceFlags |= FORCE_MAXLEVEL;

        if (ability == ABILITY_SYNCHRONIZE && Random() % 2 == 0)
        {
            nature = GetMonData(&gPlayerParty[0], MON_DATA_PERSONALITY) % NUM_NATURES;
            forceFlags |= FORCE_NATURE;
        }
    }
    else
    {
        if (ability == ABILITY_LIGHTNING_ROD) // It probably makes more sense to just toss this with Static in the "legit" code, as you wouldn't be able to tell the difference mathematically
        {
            result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);

            if (result == ENCOUNTER_SUCCESS && Random() % 2)
            {
                u8 validIndexes[LAND_WILD_COUNT]; // Watch out if another wild count becomes larger than Land for whatever reason
                u8 validMonCount;

                for (i = 0; i < LAND_WILD_COUNT; i++)
                    validIndexes[i] = 0;

                for (validMonCount = 0, i = 0; i < LAND_WILD_COUNT; i++)
                {
                    if (gSpeciesInfo[wildPokemonInfo->wildPokemon[i].species].types[0] == TYPE_ELECTRIC || gSpeciesInfo[wildPokemonInfo->wildPokemon[i].species].types[1] == TYPE_ELECTRIC)
                        validIndexes[validMonCount++] = i;
                }

                if (validMonCount != 0)
                {
                    CreateWildMon(wildPokemonInfo->wildPokemon[validIndexes[Random() % validMonCount]].species, 5, partySlot); // In case we need to chain, we can pull species now
                    type = TYPE_ELECTRIC;
                    forceFlags |= FORCE_TYPE;
                }
            }
        }
        else if (ability == ABILITY_FLASH_FIRE)
        {
            result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);

            if (result == ENCOUNTER_SUCCESS && Random() % 2)
            {
                u8 validIndexes[LAND_WILD_COUNT]; // Watch out if another wild count becomes larger than Land for whatever reason
                u8 validMonCount;

                for (i = 0; i < LAND_WILD_COUNT; i++)
                    validIndexes[i] = 0;

                for (validMonCount = 0, i = 0; i < LAND_WILD_COUNT; i++)
                {
                    if (gSpeciesInfo[wildPokemonInfo->wildPokemon[i].species].types[0] == TYPE_FIRE || gSpeciesInfo[wildPokemonInfo->wildPokemon[i].species].types[1] == TYPE_FIRE)
                        validIndexes[validMonCount++] = i;
                }

                if (validMonCount != 0)
                {
                    CreateWildMon(wildPokemonInfo->wildPokemon[validIndexes[Random() % validMonCount]].species, 5, partySlot); // In case we need to chain, we can pull species now
                    type = TYPE_FIRE;
                    forceFlags |= FORCE_TYPE;
                }
            }
        }
        else
        {
            result = EncounterCore(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot);
        }
    }

	if (CheckBagHasItem(ITEM_SHINY_CHARM, 1))
		rolls += SHINY_CHARM_REROLLS;

	if (gChainEncounter && gLastEncounteredSpecies == GetMonData(&gEnemyParty[partySlot], MON_DATA_SPECIES))
    {
		rolls += gChainStreak;
        gChainStreak++;
        species = gLastEncounteredSpecies;
        forceFlags |= FORCE_SPECIES;
    }
    else if (gChainEncounter)
    {
        gChainStreak = 0;
        gLastEncounteredSpecies = GetMonData(&gEnemyParty[partySlot], MON_DATA_SPECIES);
        species = gLastEncounteredSpecies;
        forceFlags |= FORCE_SPECIES;
    }
    else
    {
        gChainStreak = 0;
        gLastEncounteredSpecies = SPECIES_NONE;
    }

	if (gPowerType == POWER_LUCKY && gPowerLevel == 3 && gPowerTime > 0)
		rolls *= 2;

    if (result == ENCOUNTER_SWARM)
    {
        BattleSetup_StartWildBattle();
    }
    else if (result == ENCOUNTER_ROAMER)
    {
        BattleSetup_StartRoamerBattle();
    }
    else if (result == ENCOUNTER_SUCCESS)
    {
        DebugPrintf("%d, %d, %d, %d, %d, %d, %d", partySlot, forceFlags, species, type, gender, nature, rolls);
        gMain.vblankCounter2 = 0;
        for (i = 0; i < rolls; i++)
        {
            EncounterLoop(headerId, curMetatileBehavior, prevMetatileBehavior, wildPokemonInfo, terrain, partySlot, forceFlags, species, type, gender, nature);

            if (IsMonShiny(&gEnemyParty[partySlot]))
                break;
        }
        DebugPrintf("%u", gMain.vblankCounter2 / 60);
    }

    gDisableVBlankRNGAdvance = FALSE;

    return result;
}
