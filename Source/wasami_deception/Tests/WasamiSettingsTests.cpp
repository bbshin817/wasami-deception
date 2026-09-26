#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Tests/AutomationCommon.h"
#include "../WasamiAssets.h"
#include "../WasamiGameInstance.h"
#include "../WasamiPlayerCharacter.h"
#include "../WasamiSettingsSaveGame.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FString SettingsTestSlotName(TEXT("WasamiTest_Settings"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSettingsDefaultsTest, "Wasami.Settings.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	// BP_DD_Settings_SaveGame's class defaults; Inverted Y Axis and Toggle Sprint are not among them.
	const UWasamiSettingsSaveGame* Settings = GetDefault<UWasamiSettingsSaveGame>();
	TestEqual(TEXT("the slot"), UWasamiSettingsSaveGame::SlotName, FString(TEXT("Settings")));
	TestEqual(TEXT("QUALITY HIGH"), Settings->Quality, 2);
	TestEqual(TEXT("RESOLUTION SCALE 1"), Settings->ResolutionScale, 1.f);
	TestEqual(TEXT("BRIGHTNESS 1"), Settings->Brightness, 1.f);
	TestEqual(TEXT("MUSIC 1"), Settings->Music, 1.f);
	TestEqual(TEXT("SFX 1"), Settings->SFX, 1.f);
	TestEqual(TEXT("DIALOGUE 1"), Settings->Dialogue, 1.f);
	TestTrue(TEXT("SUBTITLES on"), Settings->bSubtitles);
	TestEqual(TEXT("MOUSE SENSITIVITY 0.5"), Settings->MouseSensitivity, 0.5f);
	TestTrue(TEXT("HEAD BOBBING on"), Settings->bHeadBobbing);
	TestTrue(TEXT("MOUSE SMOOTHING on"), Settings->bMouseSmoothing);
	TestFalse(TEXT("INVERTED Y AXIS off"), Settings->bInvertedYAxis);
	TestFalse(TEXT("TOGGLE SPRINT off"), Settings->bToggleSprint);
	TestTrue(TEXT("DIFFICULTY NORMAL"), Settings->Difficulty == EWasamiDifficulty::Normal);
	TestFalse(TEXT("GOD MODE off"), Settings->bGodMode);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSettingsRulesTest, "Wasami.Settings.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSettingsRulesTest::RunTest(const FString& Parameters)
{
	using S = UWasamiSettingsSaveGame;
	// The sliders' snap to 1/9, within 0 to 1.
	TestEqual(TEXT("0.3 snaps to 3/9"), S::Snap(0.3f), 3.f / 9.f, 1e-5f);
	TestEqual(TEXT("0.95 snaps to 1"), S::Snap(0.95f), 1.f, 1e-5f);
	TestEqual(TEXT("0.04 snaps to 0"), S::Snap(0.04f), 0.f, 1e-5f);
	TestEqual(TEXT("past 1 stays at 1"), S::Snap(1.2f), 1.f, 1e-5f);
	TestEqual(TEXT("below 0 stays at 0"), S::Snap(-0.1f), 0.f, 1e-5f);
	// Their value boxes: rounded to one decimal, no decimals for a whole number.
	TestEqual(TEXT("0"), S::SliderText(0.f).ToString(), FString(TEXT("0")));
	TestEqual(TEXT("3/9 reads 0.3"), S::SliderText(3.f / 9.f).ToString(), FString(TEXT("0.3")));
	TestEqual(TEXT("4/9 reads 0.4"), S::SliderText(4.f / 9.f).ToString(), FString(TEXT("0.4")));
	TestEqual(TEXT("5/9 reads 0.6"), S::SliderText(5.f / 9.f).ToString(), FString(TEXT("0.6")));
	TestEqual(TEXT("the default sensitivity reads 0.5"), S::SliderText(0.5f).ToString(), FString(TEXT("0.5")));
	TestEqual(TEXT("1"), S::SliderText(1.f).ToString(), FString(TEXT("1")));
	// The arrows.
	TestEqual(TEXT("HIGH up to VERY HIGH"), S::StepValue(2, 1, S::QualityMax), 3);
	TestEqual(TEXT("no higher than VERY HIGH"), S::StepValue(3, 1, S::QualityMax), 3);
	TestEqual(TEXT("no lower than LOW"), S::StepValue(0, -1, S::QualityMax), 0);
	TestEqual(TEXT("NORMAL does not go up to HARD"), S::StepValue(1, 1, S::DifficultyMax), 1);
	TestEqual(TEXT("NORMAL down to EASY"), S::StepValue(1, -1, S::DifficultyMax), 0);
	TestEqual(TEXT("LOW"), S::QualityText(0).ToString(), FString(TEXT("LOW")));
	TestEqual(TEXT("MEDIUM"), S::QualityText(1).ToString(), FString(TEXT("MEDIUM")));
	TestEqual(TEXT("HIGH"), S::QualityText(2).ToString(), FString(TEXT("HIGH")));
	TestEqual(TEXT("VERY HIGH"), S::QualityText(3).ToString(), FString(TEXT("VERY HIGH")));
	TestEqual(TEXT("EASY"), S::DifficultyText(EWasamiDifficulty::Easy).ToString(), FString(TEXT("EASY")));
	TestEqual(TEXT("NORMAL"), S::DifficultyText(EWasamiDifficulty::Normal).ToString(), FString(TEXT("NORMAL")));
	TestEqual(TEXT("HARD"), S::DifficultyText(EWasamiDifficulty::Hard).ToString(), FString(TEXT("HARD")));
	// Set Settings.
	TestEqual(TEXT("the brightness at 0 is gamma 1.8"), S::GammaFor(0.f), 1.8f, 1e-5f);
	TestEqual(TEXT("at 0.5, 2.0"), S::GammaFor(0.5f), 2.f, 1e-5f);
	TestEqual(TEXT("at 1, UE's 2.2"), S::GammaFor(1.f), 2.2f, 1e-5f);
	TestEqual(TEXT("clamped"), S::GammaFor(2.f), 2.2f, 1e-5f);
	TestEqual(TEXT("post-processing 2 for LOW"), S::PostProcessingQualityFor(0), 2);
	TestEqual(TEXT("and for HIGH"), S::PostProcessingQualityFor(2), 2);
	TestEqual(TEXT("3 for VERY HIGH"), S::PostProcessingQualityFor(3), 3);
	// The player.
	TestEqual(TEXT("the default sensitivity is today's view speed"), S::PlayerSensitivityFor(0.5f), 1.f, 1e-6f);
	TestEqual(TEXT("1 doubles it"), S::PlayerSensitivityFor(1.f), 2.f, 1e-6f);
	TestEqual(TEXT("the smoothing lags at 12.5"), S::RotationLagSpeedFor(true), 12.5f);
	TestEqual(TEXT("without, at 50"), S::RotationLagSpeedFor(false), 50.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSettingsSoundMixTest, "Wasami.Settings.SoundMix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSettingsSoundMixTest::RunTest(const FString& Parameters)
{
	// DD_SoundMix as the original's: SFX, Dialogue and Music at 1, none of them to its children.
	using S = UWasamiSettingsSaveGame;
	const USoundMix* Mix = S::LoadSoundMix();
	USoundClass* Music = S::LoadMusicClass();
	USoundClass* SFX = S::LoadSFXClass();
	USoundClass* Dialogue = S::LoadDialogueClass();
	if (!TestNotNull(TEXT("the mix"), Mix) || !TestNotNull(TEXT("Music"), Music) || !TestNotNull(TEXT("SFX"), SFX)
		|| !TestNotNull(TEXT("Dialogue"), Dialogue))
	{
		return false;
	}
	if (TestEqual(TEXT("three classes"), Mix->SoundClassEffects.Num(), 3))
	{
		const USoundClass* Order[] = {SFX, Dialogue, Music};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FSoundClassAdjuster& Adjuster = Mix->SoundClassEffects[Index];
			TestTrue(FString::Printf(TEXT("adjuster %d's class"), Index), Adjuster.SoundClassObject == Order[Index]);
			TestEqual(FString::Printf(TEXT("adjuster %d's volume"), Index), Adjuster.VolumeAdjuster, 1.f);
			TestFalse(FString::Printf(TEXT("adjuster %d not to the children"), Index), Adjuster.bApplyToChildren);
		}
	}
	// The classes' tree and properties.
	TestNull(TEXT("Music has no parent"), Music->ParentClass.Get());
	TestFalse(TEXT("Music without reverb"), Music->Properties.bReverb);
	TestTrue(TEXT("SFX with the ambient volumes"), SFX->Properties.bApplyAmbientVolumes);
	TestEqual(TEXT("SFX has SFX_Movies and SFX_UI"), SFX->ChildClasses.Num(), 2);
	for (const USoundClass* Child : SFX->ChildClasses)
	{
		if (TestNotNull(TEXT("a child of SFX"), Child))
		{
			TestTrue(FString::Printf(TEXT("%s's parent is SFX"), *Child->GetName()), Child->ParentClass == SFX);
			if (Child->GetName() == TEXT("DD_SoundClass_SFX_UI"))
			{
				TestTrue(TEXT("SFX_UI is a UI sound"), Child->Properties.bIsUISound);
			}
			else
			{
				TestEqual(TEXT("SFX_Movies at 0.5"), Child->Properties.Volume, 0.5f);
			}
		}
	}

	// Sounds carry their exports' classes (none where the original has none).
	const struct
	{
		const TCHAR* Sound;
		const TCHAR* Class;
	} Cases[] = {
		{TEXT("/Game/DD/Audio/UI/UI_Select_V3"), TEXT("DD_SoundClass_SFX")},
		{TEXT("/Game/DD/Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue"), TEXT("DD_SoundClass_SFX")},
		{TEXT("/Game/DD/Audio/UI/UI_YouEscaped"), TEXT("DD_SoundClass_SFX_UI")},
		{TEXT("/Game/DD/Audio/UI/Pause_Sound_v1"), TEXT("DD_SoundClass_Music")},
		{TEXT("/Game/DD/Audio/DD_-_Dark_Deception_-_Theme_v1_3"), TEXT("DD_SoundClass_Music")},
		{TEXT("/Game/DD/Audio/Titlescreen/Bierce_Title_Modified_03"), TEXT("DD_SoundClass_Dialogue")},
		{TEXT("/Game/DD/Audio/06_Hospital/DD_TT_Lift_Loop"), TEXT("")},
	};
	for (const auto& Case : Cases)
	{
		const USoundBase* Sound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(Case.Sound)).LoadSynchronous();
		if (TestNotNull(Case.Sound, Sound))
		{
			TestEqual(FString::Printf(TEXT("%s's class"), Case.Sound), Sound->SoundClassObject ? Sound->SoundClassObject->GetName() : FString(), FString(Case.Class));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSettingsSlotTest, "Wasami.Settings.Slot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSettingsSlotTest::RunTest(const FString& Parameters)
{
	const float Gamma = GEngine->DisplayGamma;
	UGameplayStatics::DeleteGameInSlot(SettingsTestSlotName, UWasamiSettingsSaveGame::UserIndex);

	// Check Settings Save: none to read, so the defaults are made and written.
	UWasamiSettingsSaveGame* Made = UWasamiSettingsSaveGame::Check(SettingsTestSlotName);
	if (!TestNotNull(TEXT("made"), Made))
	{
		return false;
	}
	TestTrue(TEXT("and written"), UGameplayStatics::DoesSaveGameExist(SettingsTestSlotName, UWasamiSettingsSaveGame::UserIndex));
	TestEqual(TEXT("at the defaults"), Made->MouseSensitivity, 0.5f);

	// The game instance's Check Settings Save reads them back and applies them; SAVE & EXIT writes them.
	UWasamiGameInstance* Instance = NewObject<UWasamiGameInstance>();
	Instance->SettingsSlotName = SettingsTestSlotName;
	UWasamiSettingsSaveGame* Read = Instance->CheckSettingsSave();
	if (!TestNotNull(TEXT("read"), Read))
	{
		return false;
	}
	TestTrue(TEXT("kept as the settings"), Instance->GetSettings() == Read);
	TestFalse(TEXT("NORMAL is not EASY"), Instance->IsEasy());
	TestEqual(TEXT("the brightness applied as the display gamma"), GEngine->DisplayGamma, 2.2f, 1e-5f);
	Read->Brightness = 0.5f;
	Read->MouseSensitivity = 1.f;
	Read->bInvertedYAxis = true;
	Read->Difficulty = EWasamiDifficulty::Easy;
	Instance->SaveSettings();
	TestEqual(TEXT("SAVE & EXIT applies the gamma"), GEngine->DisplayGamma, 2.f, 1e-5f);
	TestTrue(TEXT("EASY"), Instance->IsEasy());

	UWasamiSettingsSaveGame* Again = UWasamiSettingsSaveGame::Check(SettingsTestSlotName);
	if (TestNotNull(TEXT("read again"), Again))
	{
		TestTrue(TEXT("a new object"), Again != Read);
		TestEqual(TEXT("the brightness kept"), Again->Brightness, 0.5f);
		TestEqual(TEXT("the sensitivity kept"), Again->MouseSensitivity, 1.f);
		TestTrue(TEXT("the inverted Y kept"), Again->bInvertedYAxis);
		TestTrue(TEXT("the difficulty kept"), Again->Difficulty == EWasamiDifficulty::Easy);
		TestEqual(TEXT("the rest as they were"), Again->Quality, 2);
	}

	UGameplayStatics::DeleteGameInSlot(SettingsTestSlotName, UWasamiSettingsSaveGame::UserIndex);
	GEngine->DisplayGamma = Gamma;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSettingsPlayerTest, "Wasami.Settings.Player",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSettingsPlayerTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the player"), Player))
	{
		return false;
	}
	const USpringArmComponent* Arm = Player->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("the spring arm"), Arm))
	{
		return false;
	}

	// The defaults give what the player had before there were settings.
	UWasamiSettingsSaveGame* Settings = NewObject<UWasamiSettingsSaveGame>();
	Player->ApplySettings(*Settings);
	TestEqual(TEXT("the sensitivity at 1"), Player->MouseSensitivity, 1.f);
	TestFalse(TEXT("Y not inverted"), Player->bInvertY);
	TestTrue(TEXT("the head bob on"), Player->bHeadBob);
	TestFalse(TEXT("the sprint held"), Player->bToggleSprint);
	TestEqual(TEXT("the lag untouched"), Arm->CameraRotationLagSpeed, 20.f);

	Settings->MouseSensitivity = 1.f;
	Settings->bInvertedYAxis = true;
	Settings->bHeadBobbing = false;
	Settings->bToggleSprint = true;
	Settings->bMouseSmoothing = false;
	Player->ApplySettings(*Settings);
	TestEqual(TEXT("the sensitivity doubled"), Player->MouseSensitivity, 2.f);
	TestTrue(TEXT("Y inverted"), Player->bInvertY);
	TestFalse(TEXT("the head bob off"), Player->bHeadBob);
	TestTrue(TEXT("the sprint toggled"), Player->bToggleSprint);
	TestEqual(TEXT("the lag waits for Set Up Mouse Smoothing"), Arm->CameraRotationLagSpeed, 20.f);
	Player->SetUpMouseSmoothing(*Settings);
	TestEqual(TEXT("no smoothing: 50"), Arm->CameraRotationLagSpeed, 50.f);
	Settings->bMouseSmoothing = true;
	Player->SetUpMouseSmoothing(*Settings);
	TestEqual(TEXT("the smoothing: 12.5"), Arm->CameraRotationLagSpeed, 12.5f);
	return true;
}

#endif
