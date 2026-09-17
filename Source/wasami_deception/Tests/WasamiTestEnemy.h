#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../WasamiEnemyInterface.h"
#include "WasamiTestEnemy.generated.h"

class UCapsuleComponent;

/**
 * A stand-in for the enemies (not made yet), for the powers' tests: a pawn-type capsule of a character's size with the
 * Enemy tag that implements the enemy interface and counts what it is sent. Its capsule shows in game.
 */
UCLASS(NotBlueprintable)
class WASAMI_DECEPTION_API AWasamiTestEnemy : public AActor, public IWasamiEnemyInterface
{
	GENERATED_BODY()

public:
	AWasamiTestEnemy();

	/** Spawns one at Location in the world of WorldContextObject (a PIE check can call it from Python). */
	UFUNCTION(BlueprintCallable, Category = "Test", meta = (WorldContext = "WorldContextObject"))
	static AWasamiTestEnemy* SpawnTestEnemy(const UObject* WorldContextObject, FVector Location);

	/** How many Set State calls came, and the last one's values. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Test")
	int32 SetStateCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Test")
	EWasamiEnemyState State = EWasamiEnemyState::Patrol;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Test")
	bool bLastByOrb = false;

	/** How many Player Vanish calls came. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Test")
	int32 PlayerVanishCount = 0;

	/** No Telepathy's answer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	bool bNoTelepathy = false;

protected:
	virtual void SetState_Implementation(EWasamiEnemyState NewState, bool bByOrb) override;
	virtual EWasamiEnemyState GetState_Implementation() const override { return State; }
	virtual void PlayerVanish_Implementation() override { ++PlayerVanishCount; }
	virtual bool NoTelepathy_Implementation() const override { return bNoTelepathy; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test")
	TObjectPtr<UCapsuleComponent> Capsule;
};
