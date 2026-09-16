#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WasamiEnemyInterface.generated.h"

/** An enemy's state, as Dark Deception's Enum_EnemyStates (its value 2 is the stun Primal Fear and the power orb send). */
UENUM(BlueprintType)
enum class EWasamiEnemyState : uint8
{
	Patrol,
	Pursue,
	Stun,
	Teleport
};

UINTERFACE(BlueprintType)
class UWasamiEnemyInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * What the powers ask of an enemy, after the original's DD_EnemyInterface (pak_reference_2). The defaults are those of
 * its BP_DD_Character_Base: the events do nothing, the state reads Patrol and the enemy shows up in the telepathy.
 */
class WASAMI_DECEPTION_API IWasamiEnemyInterface
{
	GENERATED_BODY()

public:
	/** Set State: Primal Fear sends Stun with bByOrb false, the power orb Stun with true. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy")
	void SetState(EWasamiEnemyState State, bool bByOrb);

	/** Get State. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy")
	EWasamiEnemyState GetState() const;

	/** Player Vanish: sent once to every enemy when the player vanishes. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy")
	void PlayerVanish();

	/** No Telepathy: true keeps the enemy off the telepathy's markers. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy")
	bool NoTelepathy() const;

protected:
	virtual void SetState_Implementation(EWasamiEnemyState State, bool bByOrb) {}
	virtual EWasamiEnemyState GetState_Implementation() const { return EWasamiEnemyState::Patrol; }
	virtual void PlayerVanish_Implementation() {}
	virtual bool NoTelepathy_Implementation() const { return false; }
};
