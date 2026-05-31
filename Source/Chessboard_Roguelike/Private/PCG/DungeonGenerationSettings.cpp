#include "PCG/DungeonGenerationSettings.h"

UDungeonGenerationSettings::UDungeonGenerationSettings()
{
	ProductionRules.Add(TEXT("F"), TEXT("F[+F]F[-F]F"));
}
