#include "WasamiSettingsSaveGame.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "WasamiAssets.h"

const FString UWasamiSettingsSaveGame::SlotName(TEXT("Settings"));

UWasamiSettingsSaveGame* UWasamiSettingsSaveGame::Check(const FString& Slot)
{
	// BP_DD_GameMode's Check Settings Save (@33036).
	if (UWasamiSettingsSaveGame* Loaded = Cast<UWasamiSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, UserIndex)))
	{
		return Loaded;
	}
	UWasamiSettingsSaveGame* Made = Cast<UWasamiSettingsSaveGame>(UGameplayStatics::CreateSaveGameObject(StaticClass()));
	UGameplayStatics::SaveGameToSlot(Made, Slot, UserIndex);
	return Made;
}

namespace
{
	template <typename T>
	T* LoadSettingsAsset(const TCHAR* Package)
	{
		return TSoftObjectPtr<T>(WasamiAssets::Path(Package)).LoadSynchronous();
	}
}

USoundMix* UWasamiSettingsSaveGame::LoadSoundMix()
{
	return LoadSettingsAsset<USoundMix>(TEXT("/Game/DD/Audio/SoundMix/DD_SoundMix"));
}

USoundClass* UWasamiSettingsSaveGame::LoadMusicClass()
{
	return LoadSettingsAsset<USoundClass>(TEXT("/Game/DD/Audio/SoundMix/DD_SoundClass_Music"));
}

USoundClass* UWasamiSettingsSaveGame::LoadSFXClass()
{
	return LoadSettingsAsset<USoundClass>(TEXT("/Game/DD/Audio/SoundMix/DD_SoundClass_SFX"));
}

USoundClass* UWasamiSettingsSaveGame::LoadDialogueClass()
{
	return LoadSettingsAsset<USoundClass>(TEXT("/Game/DD/Audio/SoundMix/DD_SoundClass_Dialogue"));
}

void UWasamiSettingsSaveGame::Apply(const UObject* WorldContextObject) const
{
	// Set Settings (@33342).
	if (!GIsEditor)
	{
		if (UGameUserSettings* User = UGameUserSettings::GetGameUserSettings())
		{
			User->SetOverallScalabilityLevel(Quality);
			User->SetResolutionScaleValueEx(ResolutionScale * 100.f);
			User->SetViewDistanceQuality(3);
			User->SetPostProcessingQuality(PostProcessingQualityFor(Quality));
			User->ApplySettings(false);
		}
	}
	UGameplayStatics::SetSubtitlesEnabled(bSubtitles);
	// The console's gamma X, which sets the engine's display gamma.
	if (GEngine)
	{
		GEngine->DisplayGamma = GammaFor(Brightness);
	}
	// On the world's audio device (there is none for a game instance a test makes without a world).
	if (GEngine && GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		USoundMix* Mix = LoadSoundMix();
		UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, Mix, LoadMusicClass(), Music, 1.f, 1.f, false);
		UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, Mix, LoadSFXClass(), SFX, 1.f, 1.f, false);
		UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, Mix, LoadDialogueClass(), Dialogue, 1.f, 1.f, false);
	}
}

float UWasamiSettingsSaveGame::Snap(float Value)
{
	return FMath::Clamp(static_cast<float>(UKismetMathLibrary::GridSnap_Float(Value, SliderGrid)), 0.f, 1.f);
}

FText UWasamiSettingsSaveGame::SliderText(float Value)
{
	const float Rounded = FMath::RoundToFloat(Value * 10.f) / 10.f;
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 0;
	Options.MaximumFractionalDigits = 3;
	return FText::AsNumber(Rounded, &Options);
}

int32 UWasamiSettingsSaveGame::StepValue(int32 Value, int32 Delta, int32 Max)
{
	return FMath::Clamp(Value + Delta, 0, Max);
}

FText UWasamiSettingsSaveGame::QualityText(int32 Level)
{
	switch (Level)
	{
	case 0:
		return FText::FromString(TEXT("LOW"));
	case 1:
		return FText::FromString(TEXT("MEDIUM"));
	case 2:
		return FText::FromString(TEXT("HIGH"));
	case 3:
		return FText::FromString(TEXT("VERY HIGH"));
	default:
		return FText::GetEmpty();
	}
}

FText UWasamiSettingsSaveGame::DifficultyText(EWasamiDifficulty Level)
{
	switch (Level)
	{
	case EWasamiDifficulty::Easy:
		return FText::FromString(TEXT("EASY"));
	case EWasamiDifficulty::Normal:
		return FText::FromString(TEXT("NORMAL"));
	case EWasamiDifficulty::Hard:
		return FText::FromString(TEXT("HARD"));
	default:
		return FText::GetEmpty();
	}
}

float UWasamiSettingsSaveGame::GammaFor(float Brightness)
{
	return static_cast<float>(UKismetMathLibrary::MapRangeClamped(Brightness, 0., 1., 1.8, 2.2));
}

int32 UWasamiSettingsSaveGame::PostProcessingQualityFor(int32 Quality)
{
	return Quality >= 3 ? 3 : 2;
}

float UWasamiSettingsSaveGame::PlayerSensitivityFor(float Setting)
{
	return Setting / 0.5f;
}

float UWasamiSettingsSaveGame::RotationLagSpeedFor(bool bSmoothing)
{
	return bSmoothing ? 12.5f : 50.f;
}
