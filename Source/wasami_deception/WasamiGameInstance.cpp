#include "WasamiGameInstance.h"

#include "AudioDevice.h"
#include "AudioThread.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundClass.h"
#include "UObject/UnrealType.h"
#include "WasamiCapture.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiSettingsSaveGame.h"

DEFINE_LOG_CATEGORY_STATIC(LogWasamiSettings, Log, All);

namespace
{
	FAutoConsoleCommandWithWorldAndArgs SettingsCommand(TEXT("Wasami.Settings"),
		TEXT("Wasami.Settings [Name Value]: prints the settings (the OPTIONS' save) and the volumes the audio device has for the Music, SFX and Dialogue classes; with one of their names (Quality, MouseSensitivity, bInvertedYAxis, Difficulty, ...) and a value, sets it and saves as SAVE & EXIT does."),
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
			// The volumes the audio device works out for the sliders' classes (they reach them over the overrides' 1 s).
			if (FAudioDevice* Device = World->GetAudioDeviceRaw())
			{
				// The classes load on the game thread, but GetSoundClassCurrentProperties has a check(IsInAudioThread()): the
				// editor runs the audio on the game thread (so it passes), a packaged build does not, so the read goes over there.
				TArray<USoundClass*> Classes({UWasamiSettingsSaveGame::LoadMusicClass(), UWasamiSettingsSaveGame::LoadSFXClass(), UWasamiSettingsSaveGame::LoadDialogueClass()});
				Classes.RemoveAll([](const USoundClass* Class) { return Class == nullptr; });
				TArray<float> Volumes;
				Volumes.Init(TNumericLimits<float>::Lowest(), Classes.Num());
				FAudioThread::RunCommandOnAudioThread([Device, &Classes, &Volumes]()
				{
					for (int32 Index = 0; Index < Classes.Num(); ++Index)
					{
						if (const FSoundClassProperties* Properties = Device->GetSoundClassCurrentProperties(Classes[Index]))
						{
							Volumes[Index] = Properties->Volume;
						}
					}
				});
				// The command is batched when the audio thread runs; the fence retires it before the values are read back.
				FAudioCommandFence Fence;
				Fence.BeginFence();
				Fence.Wait();
				for (int32 Index = 0; Index < Classes.Num(); ++Index)
				{
					if (Volumes[Index] > TNumericLimits<float>::Lowest())
					{
						UE_LOG(LogWasamiSettings, Display, TEXT("%s volume now %.3f"), *Classes[Index]->GetName(), Volumes[Index]);
					}
				}
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
		Settings->Apply(this);
	}
	// BeginPlay goes on to SetBaseSoundMix(DD_SoundMix) (@27511), which the classes' volumes are overrides of.
	if (GetWorld())
	{
		UGameplayStatics::SetBaseSoundMix(this, UWasamiSettingsSaveGame::LoadSoundMix());
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
	Current->Apply(this);
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

bool UWasamiGameInstance::IsEasy()
{
	const UWasamiSettingsSaveGame* Current = GetSettings();
	return Current && Current->Difficulty == EWasamiDifficulty::Easy;
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
	// Every place that forgets the shards is a fresh start (RESTART, QUIT TO TITLE, the loading screen to the next
	// level), and none of them should hand the next player the tablet and the sprint a death left behind.
	bCarriedPlayerState = false;
}

void UWasamiGameInstance::RememberPlayerState(bool bTabletUp, bool bSprintOn)
{
	bCarriedPlayerState = true;
	bCarriedTabletUp = bTabletUp;
	bCarriedSprintOn = bSprintOn;
}

void UWasamiGameInstance::RememberPlayerStateOnce(bool bTabletUp, bool bSprintOn)
{
	if (!bCarriedPlayerState)
	{
		RememberPlayerState(bTabletUp, bSprintOn);
	}
}

bool UWasamiGameInstance::TakeCarriedPlayerState(bool& bOutTabletUp, bool& bOutSprintOn)
{
	bOutTabletUp = bCarriedTabletUp;
	bOutSprintOn = bCarriedSprintOn;
	const bool bCarried = bCarriedPlayerState;
	bCarriedPlayerState = false;
	return bCarried;
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
