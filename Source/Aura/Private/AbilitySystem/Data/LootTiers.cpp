// Copyright XXX


#include "AbilitySystem/Data/LootTiers.h"

TArray<FLootItem> ULootTiers::GetLootItems()
{
    TArray<FLootItem> ReturnItems;

    for (FLootItem& Item : LootItems)
    {
        for (int32 i=0; i<Item.MaxNumberToSpawn; i++)
        {
            float Chance = FMath::FRandRange(1.f, 100.f);
            if (Chance < Item.ChanceToSpawn)
            {
                FLootItem NewItem = Item;
                ReturnItems.Add(NewItem);
            }
        }
    }

    return ReturnItems;
}
