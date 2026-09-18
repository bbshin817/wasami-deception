#include "WasamiGameMode.h"

#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "WasamiBlackFadeWidget.h"
#include "WasamiDeathScreenWidget.h"
#include "WasamiGameInstance.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiSaveGame.h"
#include "WasamiSavingWidget.h"
#include "WasamiShard.h"

namespace
{
	// ReceiveBeginPlay's Delay before the collected shards are taken out (@34280 → @4551).
	constexpr float ShardRemovalDelay = 0.2f;

	// Zone 1's first checkpoint, the lift's arrival (the entrance's 03_ElevatorEnter saves it).
	constexpr int32 Zone1Arrival = 4;

	int32 CountShards(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<AWasamiShard> It(World); It; ++It)
		{
			++Count;
		}
		return Count;
	}

	AWasamiGameMode* GameModeOf(UWorld* World)
	{
		return World ? World->GetAuthGameMode<AWasamiGameMode>() : nullptr;
	}

	// Debug commands. The original's development shortcuts (the Fake checkpoints of an unpackaged game, the J key) are
	// not copied.
	FAutoConsoleCommandWithWorldAndArgs KillCommand(TEXT("Wasami.Kill"),
		TEXT("Kills the player: the game mode's DeathEvent with the player as the cause."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AWasamiGameMode* Mode = GameModeOf(World))
			{
				Mode->DeathEvent(UGameplayStatics::GetPlayerCharacter(World, 0));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CheckpointCommand(TEXT("Wasami.Checkpoint"),
		TEXT("Wasami.Checkpoint N: saves checkpoint N as the levels do (SAVING PROGRESS, the time, the slot)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AWasamiGameMode* Mode = GameModeOf(World);
			if (Mode && Args.Num() > 0)
			{
				Mode->SaveCheckpoint(FCString::Atoi(*Args[0]));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ResetSaveCommand(TEXT("Wasami.ResetSave"),
		TEXT("Starts the save over (no checkpoint, deaths, time or streaks, no warning) with 3 lives and no shards remembered; open the level again to begin anew."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AWasamiGameMode* Mode = GameModeOf(World);
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

	FAutoConsoleCommandWithWorldAndArgs LivesCommand(TEXT("Wasami.Lives"),
		TEXT("Wasami.Lives N: sets the lives to N (0 to 6)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AWasamiGameMode* Mode = GameModeOf(World);
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
}

const TCHAR* AWasamiGameMode::Zone1LevelName = TEXT("L_Hospital_Zone1");

AWasamiGameMode::AWasamiGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AWasamiPlayerCharacter::StaticClass();
	CurrentObjective = NSLOCTEXT("Wasami", "ObjectiveCollectAllShards", "Collect all shards");
	SaveSlotName = UWasamiSaveGame::SlotName;
}

void AWasamiGameMode::BeginPlay()
{
	Super::BeginPlay();
	PrepareStart();
	// The original also sets the tablet's count to the level's shards here; the player's tablet refresh (every 0.1 s)
	// counts what is left.
	GetWorldTimerManager().SetTimer(ShardRemovalTimer, this, &AWasamiGameMode::RemoveShardsToBeRemoved, ShardRemovalDelay, false);
	// @6710: the level opens out of black (UMG_BlackFade_2, Fade in? false, Speed 10, Z 10).
	UWasamiBlackFadeWidget::Show(this, false, OpeningFadeSpeed, OpeningFadeZOrder);

	// A zone's Spawn opens the entrance when the save has no checkpoint; here Zone 2 opens Zone 1 (whose 0 reads as 4).
	if (ZoneOf(UGameplayStatics::GetCurrentLevelName(this, true)) == 2 && StartCheckpoint == 0)
	{
		UGameplayStatics::OpenLevel(this, Zone1LevelName, true);
	}
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
	if (ZoneOf(UGameplayStatics::GetCurrentLevelName(this, true)) == 1 && StructSave->Hospital.LevelCheckpoint == 0)
	{
		StructSave->Hospital.LevelCheckpoint = Zone1Arrival;
		WriteSave();
	}
	StartCheckpoint = StructSave->Hospital.LevelCheckpoint;
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
	const FName Tag = PlayerStartTagFor(ZoneOf(UGameplayStatics::GetCurrentLevelName(this, true)), StartCheckpoint);
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
	UWasamiDeathScreenWidget::Show(this, DeathScreenLevelFor(ZoneOf(UGameplayStatics::GetCurrentLevelName(this, true)), bCausedByPlayer));
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

FName AWasamiGameMode::PlayerStartTagFor(int32 Zone, int32 Checkpoint)
{
	// Zone 1's Spawn (@13483): 4 → 04_Start, 5 → 05_Start, 6 → 06_Start. Zone 2's (@22328): 7 → PlayerStart_1,
	// 8 → PlayerStart_MiniBoss, 9 → PlayerStart_Maze, 10 → PlayerStart_PostMaze.
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
