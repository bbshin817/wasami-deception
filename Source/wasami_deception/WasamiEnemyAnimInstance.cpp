#include "WasamiEnemyAnimInstance.h"

#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "WasamiAssets.h"
#include "WasamiEnemy.h"

DEFINE_LOG_CATEGORY_STATIC(LogWasamiEnemyAnim, Log, All);

namespace
{
	// The clips' strides grow with the size the enemy draws its mesh at.
	constexpr float StrideScale = static_cast<float>(AWasamiEnemy::MeshScale);
}

namespace WasamiEnemyAnim
{
	const TCHAR* const ClipNames[WasamiEnemyClip::Num] = {
		TEXT("Idle"),
		TEXT("Idle_Alert"),
		TEXT("Walk"),
		TEXT("Run"),
		TEXT("Run_Nightmare"),
		TEXT("Stun_Loop"),
		TEXT("Stun_Recover"),
		TEXT("Capture_1"),
		TEXT("Capture_2"),
		TEXT("Capture_3"),
		TEXT("Chase_PickUp"),
		TEXT("Chase_Charge"),
		TEXT("Chase_VaultRoll"),
		TEXT("Chase_VaultLand"),
		TEXT("Chase_RunFast"),
		TEXT("Chase_Slide"),
		TEXT("BeHit_FlyUp"),
		TEXT("Knock_Down"),
		TEXT("Push_Up_To_Idle"),
	};

	int32 FindClip(FName Name)
	{
		for (int32 Clip = 0; Clip < WasamiEnemyClip::Num; ++Clip)
		{
			if (Name == FName(ClipNames[Clip]))
			{
				return Clip;
			}
		}
		return INDEX_NONE;
	}

	FSoftObjectPath ClipPath(int32 Clip)
	{
		check(Clip >= 0 && Clip < WasamiEnemyClip::Num);
		return WasamiAssets::Path(*FString::Printf(TEXT("/Game/Wasami/Enemy/A_WasamiEnemy_%s"), ClipNames[Clip]));
	}
}

void FWasamiBoolBlend::Update(bool bNewValue, float BlendTime, float DeltaSeconds)
{
	bValue = bNewValue;
	const float Target = bNewValue ? 1.f : 0.f;
	if (!bStarted || BlendTime <= 0.f)
	{
		bStarted = true;
		Weight = Target;
		return;
	}
	const float Step = DeltaSeconds / BlendTime;
	Weight = Target > Weight ? FMath::Min(Target, Weight + Step) : FMath::Max(Target, Weight - Step);
}

void FWasamiStateBlend::Enter(bool bToB, float InDuration, EAlphaBlendOption InOption)
{
	if (bToB == bInB)
	{
		return;
	}
	bInB = bToB;
	StartWeight = bToB ? WeightB : 1.f - WeightB;
	Elapsed = 0.f;
	Duration = InDuration;
	Option = InOption;
}

void FWasamiStateBlend::Advance(float DeltaSeconds)
{
	Elapsed += DeltaSeconds;
	const float Alpha = Duration > 0.f ? FMath::Clamp(Elapsed / Duration, 0.f, 1.f) : 1.f;
	const float Entered = Alpha >= 1.f ? 1.f : FMath::Lerp(StartWeight, 1.f, FAlphaBlend::AlphaToBlendOption(Alpha, Option));
	WeightB = bInB ? Entered : 1.f - Entered;
}

void FWasamiStunPlayback::Start(float Duration, float InLoopLength, float InRecoverLength)
{
	LoopLength = InLoopLength;
	RecoverLength = InRecoverLength;
	Elapsed = 0.f;
	RecoverStart = FMath::Max(0.f, Duration - RecoverLength);
	RecoverTimeStart = FMath::Max(0.f, RecoverLength - Duration);
	// The loop's time is 0 (its first key, which is the recovery's) when the recovery starts.
	LoopStart = LoopLength > 0.f ? FMath::Fmod(LoopLength - FMath::Fmod(RecoverStart, LoopLength), LoopLength) : 0.f;
}

float FWasamiStunPlayback::GetClipTime() const
{
	if (!IsRecovering())
	{
		return LoopLength > 0.f ? FMath::Fmod(LoopStart + Elapsed, LoopLength) : 0.f;
	}
	return FMath::Min(RecoverTimeStart + Elapsed - RecoverStart, RecoverLength);
}

void FWasamiOncePlayback::Advance(float DeltaSeconds)
{
	// As UAnimInstance::UpdateMontage: the weight first, then the position and the automatic blend out.
	if (bStopping)
	{
		Weight = BlendOut > 0.f ? FMath::Max(0.f, Weight - DeltaSeconds / BlendOut) : 0.f;
	}
	else
	{
		Weight = BlendIn > 0.f ? FMath::Min(1.f, Weight + DeltaSeconds / BlendIn) : 1.f;
	}
	Time = FMath::Min(Time + DeltaSeconds * PlayRate, Length);
	if (!bStopping && Length - Time <= BlendOut * PlayRate + UE_KINDA_SMALL_NUMBER)
	{
		Stop(BlendOut);
	}
}

void FWasamiOncePlayback::Stop(float InBlendOut)
{
	bStopping = true;
	BlendOut = InBlendOut;
	if (BlendOut <= 0.f)
	{
		Weight = 0.f;
	}
}

void FWasamiEnemyAnimState::Init(TArrayView<const float> InLengths)
{
	*this = FWasamiEnemyAnimState();
	Lengths.Append(InLengths.GetData(), InLengths.Num());
	Lengths.SetNumZeroed(FMath::Max(Lengths.Num(), static_cast<int32>(WasamiEnemyClip::Num)));
	ClipTimes.SetNumZeroed(Lengths.Num());
	ClipRates.SetNumZeroed(Lengths.Num());
}

void FWasamiEnemyAnimState::AdvanceClip(int32 Clip, float Rate, float DeltaSeconds)
{
	ClipRates[Clip] = Rate;
	if (Lengths[Clip] > 0.f)
	{
		ClipTimes[Clip] = FMath::Fmod(ClipTimes[Clip] + Rate * DeltaSeconds, Lengths[Clip]);
	}
}

void FWasamiEnemyAnimState::Update(const FWasamiEnemyAnimInputs& Inputs, float DeltaSeconds)
{
	using namespace WasamiEnemyAnim;
	if (Lengths.Num() < WasamiEnemyClip::Num)
	{
		Init({});
	}
	const bool bFirst = !bStarted;
	bStarted = true;

	// What plays once.
	for (FWasamiOncePlayback& Playback : Once)
	{
		Playback.Advance(DeltaSeconds);
	}
	Once.RemoveAll([](const FWasamiOncePlayback& Playback) { return Playback.IsFinished(); });

	// The root: the stun.
	const bool bStunStarts = Inputs.bStunned && (!Stun.bStarted || !Stun.bValue);
	if (bStunStarts)
	{
		StunPlayback.Start(Inputs.StunDuration, Lengths[WasamiEnemyClip::StunLoop], Lengths[WasamiEnemyClip::StunRecover]);
		StopOnce(StunBlendTime);
	}
	Stun.Update(Inputs.bStunned, StunBlendTime, DeltaSeconds);
	if (Stun.Weight > ZERO_ANIMWEIGHT_THRESH)
	{
		StunPlayback.Advance(DeltaSeconds);
		const int32 StunClip = StunPlayback.IsRecovering() ? WasamiEnemyClip::StunRecover : WasamiEnemyClip::StunLoop;
		ClipTimes[StunClip] = StunPlayback.GetClipTime();
		ClipRates[StunClip] = 1.f;
	}

	// The locomotion: Idle ↔ Moving. A state entered from nothing starts over.
	const float MovingBefore = bFirst ? 0.f : Moving.WeightB;
	const bool bWasMoving = Moving.bInB;
	if (Inputs.Speed > MoveSpeed)
	{
		Moving.Enter(true, StartMovingTime, StartMovingBlend);
	}
	else if (Inputs.Speed < MoveSpeed)
	{
		Moving.Enter(false, StopMovingTime, StopMovingBlend);
	}
	if (bFirst || (!Moving.bInB && bWasMoving && 1.f - MovingBefore <= ZERO_ANIMWEIGHT_THRESH))
	{
		Alert.Reset();
		ClipTimes[WasamiEnemyClip::Idle] = 0.f;
		ClipTimes[WasamiEnemyClip::IdleAlert] = 0.f;
	}
	if (Moving.bInB && !bWasMoving && MovingBefore <= ZERO_ANIMWEIGHT_THRESH)
	{
		Running.Reset();
		Nightmare.Reset();
		ClipTimes[WasamiEnemyClip::Walk] = 0.f;
		ClipTimes[WasamiEnemyClip::Run] = 0.f;
		ClipTimes[WasamiEnemyClip::RunNightmare] = 0.f;
	}
	Moving.Advance(DeltaSeconds);

	if (1.f - Moving.WeightB > ZERO_ANIMWEIGHT_THRESH)
	{
		Alert.Update(Inputs.bAggressiveIdle, AlertBlendTime, DeltaSeconds);
		if (1.f - Alert.Weight > ZERO_ANIMWEIGHT_THRESH)
		{
			AdvanceClip(WasamiEnemyClip::Idle, 1.f, DeltaSeconds);
		}
		if (Alert.Weight > ZERO_ANIMWEIGHT_THRESH)
		{
			AdvanceClip(WasamiEnemyClip::IdleAlert, 1.f, DeltaSeconds);
		}
	}
	if (Moving.WeightB > ZERO_ANIMWEIGHT_THRESH)
	{
		Running.Update(Inputs.Speed > RunSpeed, RunBlendTime, DeltaSeconds);
		Nightmare.Update(Inputs.bNightmare, NightmareBlendTime, DeltaSeconds);
		if (1.f - Running.Weight > ZERO_ANIMWEIGHT_THRESH)
		{
			AdvanceClip(WasamiEnemyClip::Walk, FMath::Clamp(Inputs.Speed / (WalkStrideSpeed * StrideScale), WalkRateMin, WalkRateMax), DeltaSeconds);
		}
		if (Running.Weight > ZERO_ANIMWEIGHT_THRESH && 1.f - Nightmare.Weight > ZERO_ANIMWEIGHT_THRESH)
		{
			AdvanceClip(WasamiEnemyClip::Run, FMath::Clamp(Inputs.Speed / (RunStrideSpeed * StrideScale), RunRateMin, RunRateMax), DeltaSeconds);
		}
		if (Running.Weight > ZERO_ANIMWEIGHT_THRESH && Nightmare.Weight > ZERO_ANIMWEIGHT_THRESH)
		{
			AdvanceClip(WasamiEnemyClip::RunNightmare, FMath::Clamp(Inputs.Speed / (NightmareStrideSpeed * StrideScale), RunRateMin, RunRateMax), DeltaSeconds);
		}
	}
}

bool FWasamiEnemyAnimState::PlayOnce(int32 Clip, float PlayRate, float BlendIn, float BlendOut)
{
	if (GetLength(Clip) <= 0.f || PlayRate <= 0.f)
	{
		return false;
	}
	StopOnce(BlendIn);
	FWasamiOncePlayback& Playback = Once.AddDefaulted_GetRef();
	Playback.Clip = Clip;
	Playback.Length = Lengths[Clip];
	Playback.PlayRate = PlayRate;
	Playback.BlendIn = BlendIn;
	Playback.BlendOut = BlendOut;
	return true;
}

void FWasamiEnemyAnimState::StopOnce(float BlendOut)
{
	for (FWasamiOncePlayback& Playback : Once)
	{
		Playback.Stop(Playback.bStopping ? FMath::Min(Playback.BlendOut, BlendOut) : BlendOut);
	}
}

bool FWasamiEnemyAnimState::IsPlayingOnce() const
{
	return Once.ContainsByPredicate([](const FWasamiOncePlayback& Playback) { return !Playback.bStopping; });
}

float FWasamiEnemyAnimState::GetOnceWeight(float* OutSum) const
{
	float Sum = 0.f;
	for (const FWasamiOncePlayback& Playback : Once)
	{
		Sum += Playback.Weight;
	}
	if (OutSum)
	{
		*OutSum = Sum;
	}
	return FMath::Min(1.f, Sum);
}

void FWasamiEnemyAnimState::GetSamples(TArray<FWasamiEnemyAnimSample>& OutSamples) const
{
	OutSamples.Reset();
	if (!bStarted)
	{
		return;
	}
	auto Add = [this, &OutSamples](int32 Clip, float Time, float Weight, bool bLoop)
	{
		if (Weight > ZERO_ANIMWEIGHT_THRESH && GetLength(Clip) > 0.f)
		{
			OutSamples.Add({Clip, Time, Weight, bLoop});
		}
	};

	float OnceSum = 0.f;
	const float OnceWeight = GetOnceWeight(&OnceSum);
	const float Base = 1.f - OnceWeight;

	const float StunWeight = Base * Stun.Weight;
	if (StunPlayback.IsRecovering())
	{
		Add(WasamiEnemyClip::StunRecover, StunPlayback.GetClipTime(), StunWeight, false);
	}
	else
	{
		Add(WasamiEnemyClip::StunLoop, StunPlayback.GetClipTime(), StunWeight, true);
	}

	const float Locomotion = Base * (1.f - Stun.Weight);
	const float IdleWeight = Locomotion * (1.f - Moving.WeightB);
	const float MovingWeight = Locomotion * Moving.WeightB;
	Add(WasamiEnemyClip::Idle, ClipTimes[WasamiEnemyClip::Idle], IdleWeight * (1.f - Alert.Weight), true);
	Add(WasamiEnemyClip::IdleAlert, ClipTimes[WasamiEnemyClip::IdleAlert], IdleWeight * Alert.Weight, true);
	Add(WasamiEnemyClip::Walk, ClipTimes[WasamiEnemyClip::Walk], MovingWeight * (1.f - Running.Weight), true);
	const float RunWeight = MovingWeight * Running.Weight;
	Add(WasamiEnemyClip::Run, ClipTimes[WasamiEnemyClip::Run], RunWeight * (1.f - Nightmare.Weight), true);
	Add(WasamiEnemyClip::RunNightmare, ClipTimes[WasamiEnemyClip::RunNightmare], RunWeight * Nightmare.Weight, true);

	for (const FWasamiOncePlayback& Playback : Once)
	{
		Add(Playback.Clip, Playback.Time, OnceSum > 0.f ? Playback.Weight * OnceWeight / OnceSum : 0.f, false);
	}

	// A missing clip leaves the rest short of 1.
	float Sum = 0.f;
	for (const FWasamiEnemyAnimSample& Sample : OutSamples)
	{
		Sum += Sample.Weight;
	}
	if (Sum > 0.f && !FMath::IsNearlyEqual(Sum, 1.f))
	{
		for (FWasamiEnemyAnimSample& Sample : OutSamples)
		{
			Sample.Weight /= Sum;
		}
	}
}

float FWasamiEnemyAnimState::GetClipWeight(int32 Clip) const
{
	TArray<FWasamiEnemyAnimSample> Samples;
	GetSamples(Samples);
	float Weight = 0.f;
	for (const FWasamiEnemyAnimSample& Sample : Samples)
	{
		if (Sample.Clip == Clip)
		{
			Weight += Sample.Weight;
		}
	}
	return Weight;
}

void FWasamiEnemyAnimInstanceProxy::PreEvaluateAnimation(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::PreEvaluateAnimation(InAnimInstance);

	const UWasamiEnemyAnimInstance* Instance = CastChecked<UWasamiEnemyAnimInstance>(InAnimInstance);
	Samples.Reset();
	for (const FWasamiEnemyAnimSample& Sample : Instance->FrameSamples)
	{
		if (Instance->Clips.IsValidIndex(Sample.Clip) && Instance->Clips[Sample.Clip])
		{
			Samples.Add({Instance->Clips[Sample.Clip].Get(), Sample.Time, Sample.Weight, Sample.bLoop});
		}
	}
}

bool FWasamiEnemyAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	auto Extract = [](const FSample& Sample, FPoseContext& Pose)
	{
		FAnimationPoseData PoseData(Pose);
		Sample.Sequence->GetAnimationPose(PoseData, FAnimExtractContext(static_cast<double>(Sample.Time), false, FDeltaTimeRecord(), Sample.bLoop));
	};

	TArray<const FSample*, TInlineAllocator<8>> Usable;
	for (const FSample& Sample : Samples)
	{
		if (Sample.Sequence->GetSkeleton())
		{
			Usable.Add(&Sample);
		}
	}

	if (Usable.IsEmpty())
	{
		Output.ResetToRefPose();
		return true;
	}
	if (Usable.Num() == 1)
	{
		Extract(*Usable[0], Output);
		return true;
	}

	TArray<FCompactPose, TInlineAllocator<8>> Poses;
	TArray<FBlendedCurve, TInlineAllocator<8>> Curves;
	TArray<UE::Anim::FStackAttributeContainer, TInlineAllocator<8>> Attributes;
	TArray<float, TInlineAllocator<8>> Weights;
	Poses.SetNum(Usable.Num());
	Curves.SetNum(Usable.Num());
	Attributes.SetNum(Usable.Num());
	float WeightSum = 0.f;
	for (int32 Index = 0; Index < Usable.Num(); ++Index)
	{
		FPoseContext Pose(Output);
		Extract(*Usable[Index], Pose);
		Poses[Index].MoveBonesFrom(Pose.Pose);
		Curves[Index].MoveFrom(Pose.Curve);
		Attributes[Index].MoveFrom(Pose.CustomAttributes);
		Weights.Add(Usable[Index]->Weight);
		WeightSum += Usable[Index]->Weight;
	}
	for (float& Weight : Weights)
	{
		Weight /= WeightSum;
	}

	FAnimationPoseData OutPoseData(Output);
	FAnimationRuntime::BlendPosesTogether(Poses, Curves, Attributes, Weights, OutPoseData);
	return true;
}

float UWasamiEnemyAnimInstance::PlayOnce(FName Clip, float PlayRate, float BlendIn, float BlendOut)
{
	const int32 Index = WasamiEnemyAnim::FindClip(Clip);
	if (Index == INDEX_NONE || !AnimState.PlayOnce(Index, PlayRate, BlendIn, BlendOut))
	{
		UE_LOG(LogWasamiEnemyAnim, Warning, TEXT("%s: no clip %s to play (or a play rate of %f)"), *GetPathName(), *Clip.ToString(), PlayRate);
		return 0.f;
	}
	return AnimState.GetLength(Index) / PlayRate;
}

void UWasamiEnemyAnimInstance::StopOnce(float BlendOut)
{
	AnimState.StopOnce(BlendOut);
}

FName UWasamiEnemyAnimInstance::GetMainClip(float& OutTime, float& OutWeight) const
{
	const FWasamiEnemyAnimSample* Main = nullptr;
	for (const FWasamiEnemyAnimSample& Sample : FrameSamples)
	{
		if (!Main || Sample.Weight > Main->Weight)
		{
			Main = &Sample;
		}
	}
	OutTime = Main ? Main->Time : 0.f;
	OutWeight = Main ? Main->Weight : 0.f;
	return Main ? FName(WasamiEnemyAnim::ClipNames[Main->Clip]) : NAME_None;
}

void UWasamiEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	TArray<float> Lengths;
	Lengths.SetNumZeroed(WasamiEnemyClip::Num);
	Clips.Reset();
	Clips.SetNum(WasamiEnemyClip::Num);
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		for (int32 Clip = 0; Clip < WasamiEnemyClip::Num; ++Clip)
		{
			const FSoftObjectPath Path = WasamiEnemyAnim::ClipPath(Clip);
			UAnimSequence* Sequence = Cast<UAnimSequence>(Path.TryLoad());
			if (!Sequence)
			{
				UE_LOG(LogWasamiEnemyAnim, Warning, TEXT("No animation %s (WasamiDDTools.import_wasami_enemy makes it)"), *Path.ToString());
				continue;
			}
			Clips[Clip] = Sequence;
			Lengths[Clip] = Sequence->GetPlayLength();
		}
	}
	AnimState.Init(Lengths);
	FrameSamples.Reset();
}

void UWasamiEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const AActor* Owner = GetOwningActor();
	Speed = Owner ? static_cast<float>(Owner->GetVelocity().Size()) : 0.f;
	// The ABP's event graph: its nurse's State == Stun and bAggressiveIdle. What is left of the stun is read with it, so
	// that the recovery ends as the enemy goes back to Patrol. Another owner sets the flags itself.
	if (const AWasamiEnemy* Enemy = Cast<AWasamiEnemy>(Owner))
	{
		bStunned = Enemy->IsStunned();
		StunDuration = Enemy->GetStunTimeLeft();
		bAggressiveIdle = Enemy->bAggressiveIdle;
		bNightmare = Enemy->bNightmare;
	}

	FWasamiEnemyAnimInputs Inputs;
	Inputs.Speed = Speed;
	Inputs.bStunned = bStunned;
	Inputs.StunDuration = StunDuration;
	Inputs.bAggressiveIdle = bAggressiveIdle;
	Inputs.bNightmare = bNightmare;
	AnimState.Update(Inputs, DeltaSeconds);
	AnimState.GetSamples(FrameSamples);
}

FAnimInstanceProxy* UWasamiEnemyAnimInstance::CreateAnimInstanceProxy()
{
	return new FWasamiEnemyAnimInstanceProxy(this);
}
