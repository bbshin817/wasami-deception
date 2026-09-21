#include "WasamiGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UObjectIterator.h"
#include "WasamiBlackFadeWidget.h"
#include "WasamiCapture.h"
#include "WasamiChapterPortalWidget.h"
#include "WasamiDeathScreenWidget.h"
#include "WasamiAssets.h"
#include "WasamiGameInstance.h"
#include "WasamiLevelClearWidget.h"
#include "WasamiLevelResults.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiSaveGame.h"
#include "WasamiSavingWidget.h"
#include "WasamiShard.h"
#include "WasamiShardStreakWidget.h"
#include "WasamiTriggerBox.h"
#include "WasamiZoneFlow.h"

DEFINE_LOG_CATEGORY_STATIC(LogWasamiDebug, Log, All);

namespace
{
	// ReceiveBeginPlay's Delay before the collected shards are taken out (@34280 → @4551).
	constexpr float ShardRemovalDelay = 0.2f;

	// Check Shards' Delay before it counts (@34510).
	constexpr float CheckShardsDelay = 0.05f;

	// Zone 1's first checkpoint, the lift's arrival (the entrance's 03_ElevatorEnter saves it).
	constexpr int32 Zone1Arrival = 4;

	// Check Streak's milestones (@30697 …): the current streak each Enum_ShardStreaks value 1..10 is shown at.
	constexpr int32 StreakMilestones[] = {20, 50, 100, 150, 200, 250, 350, 500, 700, 1000};

	int32 CountShards(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<AWasamiShard> It(World); It; ++It)
		{
			++Count;
		}
		return Count;
	}

	AWasamiGameMode* WasamiModeOf(UWorld* World)
	{
		return World ? World->GetAuthGameMode<AWasamiGameMode>() : nullptr;
	}

	// Debug commands. The original's development shortcuts (the Fake checkpoints of an unpackaged game, the J key) are
	// not copied.
	FAutoConsoleCommandWithWorldAndArgs KillCommand(TEXT("Wasami.Kill"),
		TEXT("Kills the player: the game mode's DeathEvent with the player as the cause."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AWasamiGameMode* Mode = WasamiModeOf(World))
			{
				Mode->DeathEvent(UGameplayStatics::GetPlayerCharacter(World, 0));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CaptureCommand(TEXT("Wasami.Capture"),
		TEXT("Wasami.Capture [N]: the player is caught (the black room, the clip, the death screen 3.5 s on); N 0 to 2 picks Capture_1 to Capture_3, 3 the face (the death screen 1.15 s on), none takes the next from the bag."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AWasamiCapture::StartCapture(World, nullptr, Args.Num() > 0 ? FCString::Atoi(*Args[0]) : INDEX_NONE);
		}));

	FAutoConsoleCommandWithWorldAndArgs CheckpointCommand(TEXT("Wasami.Checkpoint"),
		TEXT("Wasami.Checkpoint N: saves checkpoint N as the levels do (SAVING PROGRESS, the time, the slot)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AWasamiGameMode* Mode = WasamiModeOf(World);
			if (Mode && Args.Num() > 0)
			{
				Mode->SaveCheckpoint(FCString::Atoi(*Args[0]));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs EscapeCommand(TEXT("Wasami.Escape"),
		TEXT("The hospital's Escape where the player stands: the game paused, checkpoint 0 saved with the time, the level clear screen; its NEXT empties the save and opens the title."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AWasamiGameMode* Mode = WasamiModeOf(World))
			{
				Mode->Escape();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs TitleCommand(TEXT("Wasami.Title"),
		TEXT("Opens the title (L_Title), as the game over's QUIT TO TITLE does; the save is kept."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (World)
			{
				UGameplayStatics::OpenLevel(World, AWasamiGameMode::TitleLevelName, true);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ResetSaveCommand(TEXT("Wasami.ResetSave"),
		TEXT("Starts the save over (no checkpoint, deaths, time or streaks, no warning) with 3 lives and no shards remembered; open the level again to begin anew."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AWasamiGameMode* Mode = WasamiModeOf(World);
			if (Mode && Mode->GetSave())
			{
				Mode->GetSave()->Hospital = FWasamiLevelProgress();
				Mode->GetSave()->bLastCheckpointWarning = false;
				Mode->WriteSave();
			}
			if (UWasamiGameInstance* Instance = Mode ? Mode->GetWasamiGameInstance() : nullptr)
			{
				Instance->ResetLives();
				Instance->ForgetCollectedShards();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CollectShardsCommand(TEXT("Wasami.CollectShards"),
		TEXT("Wasami.CollectShards [N]: collects the level's shards (as if touched) but N of them (0 by default)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const int32 Keep = Args.Num() > 0 ? FMath::Max(0, FCString::Atoi(*Args[0])) : 0;
			TArray<AWasamiShard*> Shards;
			for (TActorIterator<AWasamiShard> It(World); It; ++It)
			{
				Shards.Add(*It);
			}
			for (int32 Index = Keep; Index < Shards.Num(); ++Index)
			{
				Shards[Index]->Collect(false);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs StreakCommand(TEXT("Wasami.Streak"),
		TEXT("Wasami.Streak N: the save's current streak set to N - 1, then Check Streak (as if the Nth shard in a row were collected: 20, 50 … 1000 show their card, 200 and 500 add a life)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AWasamiGameMode* Mode = WasamiModeOf(World);
			if (!Mode || !Mode->GetSave() || Args.Num() == 0)
			{
				return;
			}
			Mode->GetSave()->Hospital.CurrentStreak = FMath::Max(0, FCString::Atoi(*Args[0]) - 1);
			Mode->CheckStreak();
		}));

	FAutoConsoleCommandWithWorldAndArgs TriggerCommand(TEXT("Wasami.Trigger"),
		TEXT("Wasami.Trigger Name: the trigger box of the original's name (06_CutsceneStart, Trigger_MazeStart, ...) fires as if the player went through it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() == 0)
			{
				return;
			}
			if (AWasamiTriggerBox* Box = Cast<AWasamiTriggerBox>(AWasamiZoneFlow::FindSource(World, FName(*Args[0]))))
			{
				Box->NotifyPlayerOverlap(!Box->bEndOverlap);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs InteractCommand(TEXT("Wasami.Interact"),
		TEXT("Wasami.Interact [N]: presses Interact (F) N times (1 by default), as the key does."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AWasamiPlayerCharacter* Player = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
			const int32 Presses = Args.Num() > 0 ? FMath::Max(0, FCString::Atoi(*Args[0])) : 1;
			for (int32 Press = 0; Player && Press < Presses; ++Press)
			{
				Player->InteractPressed();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs LivesCommand(TEXT("Wasami.Lives"),
		TEXT("Wasami.Lives N: sets the lives to N (0 to 6)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AWasamiGameMode* Mode = WasamiModeOf(World);
			UWasamiGameInstance* Instance = Mode ? Mode->GetWasamiGameInstance() : nullptr;
			if (!Instance || Args.Num() == 0)
			{
				return;
			}
			const int32 Wanted = FMath::Clamp(FCString::Atoi(*Args[0]), 0, UWasamiGameInstance::MaxLives);
			while (Instance->GetLives() > Wanted)
			{
				Instance->DecrementLives();
			}
			while (Instance->GetLives() < Wanted)
			{
				Instance->IncrementLives();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ChapterPortalCommand(TEXT("Wasami.ChapterPortal"),
		TEXT("Puts up the stage's title card (UMG_ChapterPortal) as Zone 1's new start does, without stopping the player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UWasamiChapterPortalWidget::Show(World);
		}));

	FAutoConsoleCommandWithWorldAndArgs DelayCommand(TEXT("Wasami.Delay"),
		TEXT("Wasami.Delay S Command ...: runs the console command S seconds later, so one command line can play the game on ")
		TEXT("(Tools/game_flow.py). The wait is real time on the core ticker, so it outlives a level change and goes on while ")
		TEXT("the game is paused; several can wait at once, and the command cannot hold a comma (-ExecCmds splits on it)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() < 2)
			{
				return;
			}
			TArray<FString> Rest(Args);
			Rest.RemoveAt(0);
			const FString Command = FString::Join(Rest, TEXT(" "));
			const float Seconds = FMath::Max(0.f, FCString::Atof(*Args[0]));
			UE_LOG(LogWasamiDebug, Display, TEXT("Wasami.Delay: '%s' in %.1f s"), *Command, Seconds);
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Command](float)
			{
				// The engine's own deferred commands, the ones -ExecCmds uses: they run on the next tick, in whichever
				// world is loaded by then.
				if (GEngine)
				{
					GEngine->DeferredCommands.Add(Command);
				}
				return false;
			}), Seconds);
		}));

	FAutoConsoleCommandWithWorldAndArgs StatusCommand(TEXT("Wasami.Status"),
		TEXT("Prints one line with where the game is (level, checkpoint, lives, shards, objective, the player, the widgets ")
		TEXT("on screen), the fields Tools/playthrough.py reads from the editor, so a packaged build can be followed in its log."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AWasamiGameMode* Mode = WasamiModeOf(World);
			const UWasamiSaveGame* Save = Mode ? Mode->GetSave() : nullptr;
			// Through the world, not the game mode: the title has its own mode (14 record) but the same lives.
			const UWasamiGameInstance* Instance = World ? World->GetGameInstance<UWasamiGameInstance>() : nullptr;
			const AWasamiPlayerCharacter* Player = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
			TArray<FString> Widgets;
			for (TObjectIterator<UUserWidget> It; It; ++It)
			{
				if (It->GetWorld() == World && It->IsInViewport())
				{
					Widgets.AddUnique(It->GetClass()->GetName());
				}
			}
			Widgets.Sort();
			const FVector Where = Player ? Player->GetActorLocation() : FVector::ZeroVector;
			UE_LOG(LogWasamiDebug, Display,
				TEXT("Wasami.Status level=%s checkpoint=%d start=%d lives=%d shards=%d/%d deaths=%d time=%.1f paused=%d input=%d player=%.0f,%.0f,%.0f yaw=%.0f objective='%s' widgets=%s"),
				World ? *World->GetName() : TEXT("none"),
				Save ? Save->Hospital.LevelCheckpoint : -1,
				Mode ? Mode->GetStartCheckpoint() : -1,
				Instance ? Instance->GetLives() : -1,
				World ? CountShards(World) : -1,
				Mode ? Mode->GetTotalShards() : -1,
				Save ? Save->Hospital.Deaths : -1,
				Mode ? Mode->GetTime() : -1.f,
				World && UGameplayStatics::IsGamePaused(World) ? 1 : 0,
				Player && Player->bHasInput ? 1 : 0,
				Where.X, Where.Y, Where.Z,
				Player ? Player->GetControlRotation().Yaw : 0.0,
				Mode ? *Mode->CurrentObjective.ToString() : TEXT(""),
				Widgets.Num() > 0 ? *FString::Join(Widgets, TEXT(",")) : TEXT("none"));
		}));
}

const TCHAR* AWasamiGameMode::Zone1LevelName = TEXT("L_Hospital_Zone1");
const TCHAR* AWasamiGameMode::Zone2LevelName = TEXT("L_Hospital_Zone2");
const TCHAR* AWasamiGameMode::TitleLevelName = TEXT("L_Title");

AWasamiGameMode::AWasamiGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AWasamiPlayerCharacter::StaticClass();
	SaveSlotName = UWasamiSaveGame::SlotName;
	for (const TCHAR* Path : {TEXT("/Game/DD/Audio/UI/Shard_Streak_Milestone_V1A"), TEXT("/Game/DD/Audio/UI/Shard_Streak_Milestone_V2"),
			 TEXT("/Game/DD/Audio/UI/Shard_Streak_Milestone_V3A"), TEXT("/Game/DD/Audio/UI/Shard_Streak_Milestone_V4")})
	{
		StreakSounds.Add(TSoftObjectPtr<USoundBase>(WasamiAssets::Path(Path)));
	}
	StreakShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak")));
}

void AWasamiGameMode::BeginPlay()
{
	Super::BeginPlay();
	// Check Settings Save → Set Settings (@27445).
	if (UWasamiGameInstance* Instance = GetWasamiGameInstance())
	{
		Instance->CheckSettingsSave();
	}
	PrepareStart();
	LoadedStreakSounds.Reset();
	for (const TSoftObjectPtr<USoundBase>& Sound : StreakSounds)
	{
		LoadedStreakSounds.Add(Sound.LoadSynchronous());
	}
	LoadedStreakShake = StreakShakeClass.LoadSynchronous();
	// The original also sets the tablet's count to the level's shards here; the player's tablet refresh (every 0.1 s)
	// counts what is left.
	GetWorldTimerManager().SetTimer(ShardRemovalTimer, this, &AWasamiGameMode::RemoveShardsToBeRemoved, ShardRemovalDelay, false);
	// @6710: the level opens out of black (UMG_BlackFade_2, Fade in? false, Speed 10, Z 10).
	UWasamiBlackFadeWidget::Show(this, false, OpeningFadeSpeed, OpeningFadeZOrder);

	// A zone's Spawn opens the entrance when the save has no checkpoint; here Zone 2 opens Zone 1 (whose 0 reads as 4).
	const int32 Zone = CurrentZone();
	if (Zone == 2 && StartCheckpoint == 0)
	{
		UGameplayStatics::OpenLevel(this, Zone1LevelName, true);
		return;
	}
	// The rest of the zone's level Blueprint (its Setup → Spawn and the events it binds).
	ZoneFlow = AWasamiZoneFlow::SpawnFor(this, Zone);
}

void AWasamiGameMode::PrepareStart()
{
	if (bStartPrepared)
	{
		return;
	}
	bStartPrepared = true;
	CheckForLevelStructSave();
	if (!StructSave)
	{
		return;
	}
	if (CurrentZone() == 1 && StructSave->Hospital.LevelCheckpoint == 0)
	{
		StructSave->Hospital.LevelCheckpoint = Zone1Arrival;
		WriteSave();
		bNewStart = true;
	}
	StartCheckpoint = StructSave->Hospital.LevelCheckpoint;
}

int32 AWasamiGameMode::CurrentZone() const
{
	return ZoneOf(LevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(this, true) : LevelName);
}

AActor* AWasamiGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// The zones' Spawn teleports the player to the checkpoint's player start and turns the view to it; starting there
	// comes to the same. PIE's Play From Here still wins, and any other checkpoint keeps UE's choice (the original
	// leaves the player where UE put them).
	PrepareStart();
	UWorld* World = GetWorld();
	if (TActorIterator<APlayerStartPIE>(World))
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}
	const FName Tag = PlayerStartTagFor(CurrentZone(), StartCheckpoint);
	if (!Tag.IsNone())
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			if (It->PlayerStartTag == Tag)
			{
				return *It;
			}
		}
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AWasamiGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bTimeCounting)
	{
		Time += DeltaSeconds;
	}
}

void AWasamiGameMode::CheckForLevelStructSave()
{
	StructSave = Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UWasamiSaveGame::UserIndex));
	if (!StructSave)
	{
		StructSave = Cast<UWasamiSaveGame>(UGameplayStatics::CreateSaveGameObject(UWasamiSaveGame::StaticClass()));
		WriteSave();
	}
}

void AWasamiGameMode::WriteSave()
{
	if (StructSave)
	{
		UGameplayStatics::SaveGameToSlot(StructSave, SaveSlotName, UWasamiSaveGame::UserIndex);
	}
}

void AWasamiGameMode::DeathEvent(AActor* Cause)
{
	if (bDeathClosed)
	{
		return;
	}
	bDeathClosed = true;
	// Not here: Bierce's idle lines, whose timer the original clears first (the voices are item 20).
	OnDeath.Broadcast(Cause);
	// The zone's DeathEvent, which listens to the dispatcher in the original. The screen's Construct zeroes the save's
	// streak at once, so with a screen up the best streak below takes 0, as there.
	ShowDeathScreen(Cause);
	if (StructSave)
	{
		ShardStreak = FMath::Max(ShardStreak, StructSave->Hospital.CurrentStreak);
		StructSave->Hospital.CurrentStreak = 0;
	}
}

void AWasamiGameMode::ResetDeath()
{
	bDeathClosed = false;
}

void AWasamiGameMode::ShowDeathScreen(AActor* Cause)
{
	// The zones' DeathEvent (Zone 1 @14791 → @5731, Zone 2 @23519 → @5758): the screen with Level set, added at Z 5, the
	// game paused. Its Respawn Event goes to the zone's Respawn (off the screen, unpaused, Reset Death, Spawn), which
	// the screen follows at once by opening the level again, so nothing listens to it here.
	const bool bCausedByPlayer = Cause && Cause->IsA<AWasamiPlayerCharacter>();
	UWasamiDeathScreenWidget::Show(this, DeathScreenLevelFor(CurrentZone(), bCausedByPlayer));
}

bool AWasamiGameMode::TakeFoundVoice()
{
	// The world's time, which starts over with each play, so the first Found of a run always goes through.
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now - LastFoundVoice < FoundVoiceGap)
	{
		return false;
	}
	LastFoundVoice = Now;
	return true;
}

void AWasamiGameMode::PauseTimeCounter()
{
	bTimeCounting = false;
}

void AWasamiGameMode::UnpauseTimeCounter()
{
	bTimeCounting = true;
}

void AWasamiGameMode::ResetTimeCounter()
{
	Time = 0.f;
}

void AWasamiGameMode::SaveCheckpoint(int32 Checkpoint)
{
	// The levels' checkpoints (Zone 1's 05_Transition, 06_ReachAmbulance, Zone 2's four): CreateAndAddWidget(UMG_Saving,
	// Z 0), the checkpoint, Time += the game mode's Time, Reset Time Counter, SaveGameToSlot.
	UWasamiSavingWidget::Show(this);
	if (!StructSave)
	{
		return;
	}
	StructSave->Hospital.LevelCheckpoint = Checkpoint;
	StructSave->Hospital.Time += Time;
	ResetTimeCounter();
	WriteSave();
}

UWasamiLevelClearWidget* AWasamiGameMode::Escape()
{
	// @55455: SetGamePaused(True), then as a checkpoint's save with 0 (UMG_Saving at Z 0 first, the time added, Reset
	// Time Counter, written). Not copied: SaveSlot's read for the achievements.
	UGameplayStatics::SetGamePaused(this, true);
	SaveCheckpoint(0);
	// The time's rank and the rows (@57305 → @38874), Create(UMG_LevelClear), Finished bound to Finished Level,
	// AddToViewport(6). EASY is the settings' difficulty, which the screen's Construct and Final Rank read.
	UWasamiGameInstance* Instance = GetWasamiGameInstance();
	UWasamiLevelClearWidget* Screen = UWasamiLevelClearWidget::Show(this,
		FWasamiLevelResults::ForHospital(StructSave ? StructSave->Hospital : FWasamiLevelProgress(), Instance && Instance->IsEasy()));
	if (Screen)
	{
		Screen->OnFinished.AddDynamic(this, &AWasamiGameMode::FinishedLevel);
	}
	return Screen;
}

void AWasamiGameMode::FinishedLevel()
{
	// @75934: SaveSlot's Progress and Level Ranks (the level select's) are not copied. Then a DoOnce, SetGamePaused(False)
	// and Delay(1).
	if (bLevelFinished)
	{
		return;
	}
	bLevelFinished = true;
	UGameplayStatics::SetGamePaused(this, false);
	GetWorldTimerManager().SetTimer(FinishedLevelTimer, this, &AWasamiGameMode::LeaveFinishedLevel, FinishedLevelDelay, false);
}

void AWasamiGameMode::LeaveFinishedLevel()
{
	// @17537: levelStruct[the level] = levelStruct[10] (an empty entry), written; Hard Check Point = 0 (the entrance's,
	// which this game does not have); Reset Game Instance(False), which forgets the shards collected and resets the
	// lives; then the next level: TitleScreen when replaying, else 06_Cinematic, the next chapter's, which this game
	// does not have (its one level is done).
	if (StructSave)
	{
		StructSave->Hospital = FWasamiLevelProgress();
		WriteSave();
	}
	if (UWasamiGameInstance* Instance = GetWasamiGameInstance())
	{
		Instance->ForgetCollectedShards();
		Instance->ResetLives();
	}
	LevelToOpen = TitleLevelName;
	UGameplayStatics::OpenLevel(this, FName(LevelToOpen), true);
}

UWasamiGameInstance* AWasamiGameMode::GetWasamiGameInstance() const
{
	return GetGameInstance<UWasamiGameInstance>();
}

void AWasamiGameMode::RemoveShardsToBeRemoved()
{
	TotalShards = CountShards(GetWorld());
	const UWasamiGameInstance* Instance = GetWasamiGameInstance();
	if (!Instance || Instance->GetShardsToBeRemoved().IsEmpty())
	{
		return;
	}
	if (RemoveCollectedShards(GetWorld(), Instance->GetShardsToBeRemoved()) < 1)
	{
		OnAllShardsAlreadyCollected.Broadcast();
	}
}

void AWasamiGameMode::CheckShards()
{
	// @34491: Collect Shard, then a sequence: the Delay (a waiting one is not put back), and Check Streak each time.
	OnCollectShard.Broadcast();
	if (!GetWorldTimerManager().IsTimerActive(CheckShardsTimer))
	{
		GetWorldTimerManager().SetTimer(CheckShardsTimer, this, &AWasamiGameMode::CountShardsLeft, CheckShardsDelay, false);
	}
	CheckStreak();
}

uint8 AWasamiGameMode::CheckStreak()
{
	// @35674: the level's CurrentStreak + 1, and Shard Streak raised to it.
	if (!StructSave)
	{
		return 0;
	}
	FWasamiLevelProgress& Hospital = StructSave->Hospital;
	++Hospital.CurrentStreak;
	if (Hospital.CurrentStreak > ShardStreak)
	{
		ShardStreak = Hospital.CurrentStreak;
	}
	const uint8 Milestone = StreakMilestoneFor(Hospital.CurrentStreak);
	if (Milestone == 0)
	{
		return 0;
	}
	// The milestone's branch (@30741 for 20 …): Create(UMG_ShardStreak) with Streak, AddToPlayerScreen(2); then
	// (@6871 …) PlayCameraShake(BP_CameraShake_Streak, 1, CameraLocal), PlaySound2D(the milestone's sound, 1, 1), and the
	// level's Streak set to the milestone where its display name's number is below the milestone's. The 100's branch
	// also caches Steam's achievements for the ballroom (Level 0), which the hospital is not.
	UWasamiShardStreakWidget::Show(this, Milestone);
	if (LoadedStreakShake)
	{
		if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (Controller->PlayerCameraManager)
			{
				Controller->PlayerCameraManager->StartCameraShake(LoadedStreakShake, 1.f, ECameraShakePlaySpace::CameraLocal);
			}
		}
	}
	const int32 SoundIndex = StreakSoundIndex(Milestone);
	if (LoadedStreakSounds.IsValidIndex(SoundIndex) && LoadedStreakSounds[SoundIndex])
	{
		UGameplayStatics::PlaySound2D(this, LoadedStreakSounds[SoundIndex], 1.f, 1.f);
	}
	if (FWasamiLevelResults::StreakMilestone(Hospital.Streak) < FWasamiLevelResults::StreakMilestone(Milestone))
	{
		Hospital.Streak = Milestone;
	}
	return Milestone;
}

uint8 AWasamiGameMode::StreakMilestoneFor(int32 CurrentStreak)
{
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(StreakMilestones); ++Index)
	{
		if (StreakMilestones[Index] == CurrentStreak)
		{
			return static_cast<uint8>(Index + 1);
		}
	}
	return 0;
}

int32 AWasamiGameMode::StreakSoundIndex(uint8 Milestone)
{
	// V1A (@6972, @8946), V2 (@10925, @13210, @17692), V3A (@19984, @21958), V4 (@23933, @25907, @27881).
	if (Milestone >= 1 && Milestone <= 2)
	{
		return 0;
	}
	if (Milestone >= 3 && Milestone <= 5)
	{
		return 1;
	}
	if (Milestone >= 6 && Milestone <= 7)
	{
		return 2;
	}
	if (Milestone >= 8 && Milestone <= 10)
	{
		return 3;
	}
	return INDEX_NONE;
}

void AWasamiGameMode::CountShardsLeft()
{
	// After the Delay: Bierce's idle lines (item 20), and the level's shards counted. The hospital (Level 7) has no line
	// for half of them and none for all of them, and Event All Shards sends only BP_Monkey into a frenzy (no monkey here)
	// besides stopping the idle lines' timer, so none left comes down to All Shards Collected.
	if (CountShards(GetWorld()) < 1)
	{
		OnAllShardsCollected.Broadcast();
	}
}

int32 AWasamiGameMode::RemoveCollectedShards(UWorld* World, const TArray<FVector>& Collected)
{
	// Each shard's present place, truncated, against the list; destroyed shards drop out of the count that follows.
	TArray<AWasamiShard*> Found;
	for (TActorIterator<AWasamiShard> It(World); It; ++It)
	{
		if (Collected.Contains(UWasamiGameInstance::ShardKey(It->GetActorLocation())))
		{
			Found.Add(*It);
		}
	}
	for (AWasamiShard* Shard : Found)
	{
		Shard->Destroy();
	}
	return CountShards(World);
}

int32 AWasamiGameMode::ZoneOf(const FString& LevelName)
{
	if (LevelName.Contains(TEXT("Zone1")))
	{
		return 1;
	}
	if (LevelName.Contains(TEXT("Zone2")))
	{
		return 2;
	}
	return 0;
}

const TCHAR* AWasamiGameMode::LevelForCheckpoint(int32 Checkpoint)
{
	// 06_Hospital's Spawn (@81063): 4 to 6 open Zone 1, 7 to 10 Zone 2 (11 and 12 the boss, which this game does not
	// have); at 0 the entrance goes on to Zone 1 itself.
	return Checkpoint >= 7 && Checkpoint <= 10 ? Zone2LevelName : Zone1LevelName;
}

FName AWasamiGameMode::PlayerStartTagFor(int32 Zone, int32 Checkpoint)
{
	// Zone 1's Spawn (@13483): 4 → 04_Start, 5 → 05_Start, 6 → 06_Start. Zone 2's (@22328): 7 → PlayerStart_1,
	// where its Arrive Event moves the player for the ambulance's arrival (its cell's scene moves them on to
	// PlayerStart_Cell), 8 → PlayerStart_MiniBoss, 9 → PlayerStart_Maze, 10 → PlayerStart_PostMaze.
	if (Zone == 1)
	{
		switch (Checkpoint)
		{
		case 4: return TEXT("04_Start");
		case 5: return TEXT("05_Start");
		case 6: return TEXT("06_Start");
		default: break;
		}
	}
	else if (Zone == 2)
	{
		switch (Checkpoint)
		{
		case 7: return TEXT("PlayerStart_1");
		case 8: return TEXT("PlayerStart_MiniBoss");
		case 9: return TEXT("PlayerStart_Maze");
		case 10: return TEXT("PlayerStart_PostMaze");
		default: break;
		}
	}
	return NAME_None;
}

uint8 AWasamiGameMode::DeathScreenLevelFor(int32 Zone, bool bCausedByPlayer)
{
	// Zone 1 gives Traps when the cause is the player and Asylum otherwise; Zone 2 gives Asylum when the cause is the
	// player or BP_GremClown (not in this game) and Traps otherwise.
	if (Zone == 2)
	{
		return bCausedByPlayer ? UWasamiDeathScreenWidget::AsylumLevel : UWasamiDeathScreenWidget::TrapsLevel;
	}
	if (Zone == 1)
	{
		return bCausedByPlayer ? UWasamiDeathScreenWidget::TrapsLevel : UWasamiDeathScreenWidget::AsylumLevel;
	}
	return UWasamiDeathScreenWidget::AsylumLevel;
}
