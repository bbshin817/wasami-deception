#include "WasamiGameInstance.h"

#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/UnrealType.h"
#include "WasamiCapture.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiSettingsSaveGame.h"

DEFINE_LOG_CATEGORY_STATIC(LogWasamiSettings, Log, All);

namespace
{
	FAutoConsoleCommandWithWorldAndArgs SettingsCommand(TEXT("Wasami.Settings"),
		TEXT("Wasami.Settings [Name Value]: prints the settings (the OPTIONS' save); with one of their names (Quality, MouseSensitivity, bInvertedYAxis, Difficulty, ...) and a value, sets it and saves as SAVE & EXIT does."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UWasamiGameInstance* Instance = World ? World->GetGameInstance<UWasamiGameInstance>() : nullptr;
			UWasamiSettingsSaveGame* Settings = Instance ? Instance->GetSettings() : nullptr;
			if (!Settings)
			{
				return;
			}
			if (Args.Num() >= 2)
			{
				FProperty* Property = FindFProperty<FProperty>(UWasamiSettingsSaveGame::StaticClass(), FName(*Args[0]));
				if (!Property || !Property->ImportText_Direct(*Args[1], Property->ContainerPtrToValuePtr<void>(Settings), Settings, PPF_None))
				{
					UE_LOG(LogWasamiSettings, Warning, TEXT("Wasami.Settings: cannot set '%s' to '%s'."), *Args[0], *Args[1]);
					return;
				}
				Instance->SaveSettings();
			}
			for (TFieldIterator<FProperty> It(UWasamiSettingsSaveGame::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
			{
				FString Value;
				It->ExportTextItem_Direct(Value, It->ContainerPtrToValuePtr<void>(Settings), nullptr, Settings, PPF_None);
				UE_LOG(LogWasamiSettings, Display, TEXT("%s = %s"), *It->GetName(), *Value);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ResetSettingsCommand(TEXT("Wasami.ResetSettings"),
		TEXT("Starts the settings over at their defaults and saves them as SAVE & EXIT does."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UWasamiGameInstance* Instance = World ? World->GetGameInstance<UWasamiGameInstance>() : nullptr;
			UWasamiSettingsSaveGame* Settings = Instance ? Instance->GetSettings() : nullptr;
			if (Settings)
			{
				for (TFieldIterator<FProperty> It(UWasamiSettingsSaveGame::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
				{
					It->CopyCompleteValue_InContainer(Settings, GetDefault<UWasamiSettingsSaveGame>());
				}
				Instance->SaveSettings();
			}
		}));
}

void UWasamiGameInstance::Init()
{
	Super::Init();
	if (GIsEditor && GEngine)
	{
		EditorDisplayGamma = GEngine->DisplayGamma;
	}
}

void UWasamiGameInstance::Shutdown()
{
	if (GIsEditor && GEngine && EditorDisplayGamma > 0.f)
	{
		GEngine->DisplayGamma = EditorDisplayGamma;
	}
	Super::Shutdown();
}

UWasamiSettingsSaveGame* UWasamiGameInstance::CheckSettingsSave()
{
	Settings = UWasamiSettingsSaveGame::Check(GetSettingsSlot());
	if (Settings)
	{
		Settings->Apply();
	}
	return Settings;
}

void UWasamiGameInstance::SaveSettings()
{
	UWasamiSettingsSaveGame* Current = GetSettings();
	if (!Current)
	{
		return;
	}
	Current->Apply();
	UGameplayStatics::SaveGameToSlot(Current, GetSettingsSlot(), UWasamiSettingsSaveGame::UserIndex);
	UWorld* World = GetWorld();
	if (AWasamiPlayerCharacter* Player = World ? Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)) : nullptr)
	{
		Player->ApplySettings(*Current);
		Player->SetUpMouseSmoothing(*Current);
	}
}

UWasamiSettingsSaveGame* UWasamiGameInstance::GetSettings()
{
	return Settings ? Settings.Get() : CheckSettingsSave();
}

FString UWasamiGameInstance::GetSettingsSlot() const
{
	return SettingsSlotName.IsEmpty() ? UWasamiSettingsSaveGame::SlotName : SettingsSlotName;
}

void UWasamiGameInstance::DecrementLives()
{
	Lives = FMath::Clamp(Lives - 1, 0, MaxLives);
}

void UWasamiGameInstance::IncrementLives()
{
	Lives = FMath::Clamp(Lives + 1, 0, MaxLives);
}

void UWasamiGameInstance::ResetLives()
{
	Lives = StartingLives;
}

void UWasamiGameInstance::RememberCollectedShard(const FVector& StartLocation)
{
	ShardsToBeRemoved.AddUnique(ShardKey(StartLocation));
}

void UWasamiGameInstance::ForgetCollectedShards()
{
	ShardsToBeRemoved.Reset();
}

FVector UWasamiGameInstance::ShardKey(const FVector& Location)
{
	const FIntVector Truncated = UKismetMathLibrary::FTruncVector(Location);
	return FVector(Truncated.X, Truncated.Y, Truncated.Z);
}

int32 UWasamiGameInstance::TakeCaptureChoice()
{
	return TakeNoRepeat(CaptureChoices, bCaptureChoicesStarted, 0, AWasamiCapture::NumChoices - 1, CaptureStream);
}

int32 UWasamiGameInstance::TakeNoRepeat(TArray<int32>& Remaining, bool& bStarted, int32 Min, int32 Max, const FRandomStream& Stream)
{
	if (Max - Min <= 0)
	{
		// The macro warns that the range is not valid and gives the value it chose last; one number is all there is.
		return Min;
	}
	if (!bStarted)
	{
		for (int32 Value = Min; Value <= Max; ++Value)
		{
			Remaining.Add(Value);
		}
		bStarted = true;
	}
	// Array_Shuffle's swaps, then the last.
	const int32 LastIndex = Remaining.Num() - 1;
	for (int32 Index = 0; Index <= LastIndex; ++Index)
	{
		const int32 Other = Stream.RandRange(Index, LastIndex);
		if (Other != Index)
		{
			Remaining.Swap(Index, Other);
		}
	}
	const int32 Chosen = Remaining.Last();
	if (Remaining.Num() < 2)
	{
		Remaining.Reset();
		bStarted = false;
	}
	else
	{
		Remaining.RemoveAt(LastIndex);
	}
	return Chosen;
}
