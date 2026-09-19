#include "WasamiBossAnimInstance.h"

#include "AnimationCoreLibrary.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "BonePose.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "WasamiAssets.h"
#include "WasamiMatron.h"

DEFINE_LOG_CATEGORY_STATIC(LogWasamiBossAnim, Log, All);

namespace WasamiBossAnim
{
	const TCHAR* const ClipNames[WasamiBossClip::Num] = {
		TEXT("Idle"),
		TEXT("Alert"),
		TEXT("Detected"),
	};

	FSoftObjectPath ClipPath(int32 Clip)
	{
		check(Clip >= 0 && Clip < WasamiBossClip::Num);
		return WasamiAssets::Path(*FString::Printf(TEXT("/Game/Wasami/Boss/A_WasamiBoss_%s"), ClipNames[Clip]));
	}
}

void FWasamiBossAnimState::Init(TArrayView<const float> InLengths)
{
	*this = FWasamiBossAnimState();
	Lengths.Append(InLengths.GetData(), InLengths.Num());
	Lengths.SetNumZeroed(FMath::Max(Lengths.Num(), static_cast<int32>(WasamiBossClip::Num)));
	ClipTimes.SetNumZeroed(Lengths.Num());
}

bool FWasamiBossAnimState::PlayDetected()
{
	if (GetLength(WasamiBossClip::Detected) <= 0.f)
	{
		return false;
	}
	bDetected = true;
	DetectedElapsed = 0.f;
	ClipTimes[WasamiBossClip::Detected] = 0.f;
	return true;
}

void FWasamiBossAnimState::Update(bool bAlert, bool bSpotted, float DeltaSeconds)
{
	using namespace WasamiBossAnim;
	if (Lengths.Num() < WasamiBossClip::Num)
	{
		Init({});
	}
	const bool bFirst = !bStarted;
	bStarted = true;

	// The slot: Detected blends in (the weight first, as UAnimInstance::UpdateMontage), moves on and stays at its end.
	if (bDetected)
	{
		DetectedElapsed += DeltaSeconds;
		ClipTimes[WasamiBossClip::Detected] = FMath::Min(ClipTimes[WasamiBossClip::Detected] + DeltaSeconds, Lengths[WasamiBossClip::Detected]);
	}

	// The state machine: a change starts once the last one has ended, so the state it enters has no weight and starts
	// its clip over.
	if (!bFirst && bAlert != Change.bInB && Change.Elapsed >= Change.Duration)
	{
		ClipTimes[bAlert ? WasamiBossClip::Alert : WasamiBossClip::Idle] = 0.f;
		Change.Enter(bAlert, ChangeTime, ChangeBlend);
	}
	Change.Advance(DeltaSeconds);
	for (const int32 Clip : {static_cast<int32>(WasamiBossClip::Idle), static_cast<int32>(WasamiBossClip::Alert)})
	{
		const float Weight = Clip == WasamiBossClip::Alert ? Change.WeightB : 1.f - Change.WeightB;
		if (Weight > ZERO_ANIMWEIGHT_THRESH && Lengths[Clip] > 0.f)
		{
			ClipTimes[Clip] = FMath::Fmod(ClipTimes[Clip] + DeltaSeconds, Lengths[Clip]);
		}
	}

	// The LookAt's FInputAlphaBoolBlend: its first update snaps, then in over 0.5 s and out at once.
	LookAt.Update(bSpotted, bSpotted ? LookAtBlendIn : 0.f, DeltaSeconds);
}

float FWasamiBossAnimState::GetLookAtAlpha() const
{
	return FAlphaBlend::AlphaToBlendOption(LookAt.Weight, WasamiBossAnim::LookAtBlend);
}

void FWasamiBossAnimState::GetSamples(TArray<FWasamiEnemyAnimSample>& OutSamples) const
{
	OutSamples.Reset();
	if (!bStarted)
	{
		return;
	}
	auto Add = [this, &OutSamples](int32 Clip, float Weight, bool bLoop)
	{
		if (Weight > ZERO_ANIMWEIGHT_THRESH && GetLength(Clip) > 0.f)
		{
			OutSamples.Add({Clip, ClipTimes[Clip], Weight, bLoop});
		}
	};

	const float Slot = bDetected && GetLength(WasamiBossClip::Detected) > 0.f
		? FAlphaBlend::AlphaToBlendOption(FMath::Clamp(DetectedElapsed / WasamiBossAnim::DetectedBlendIn, 0.f, 1.f), WasamiBossAnim::DetectedBlend)
		: 0.f;
	const float Base = 1.f - Slot;
	Add(WasamiBossClip::Idle, Base * (1.f - Change.WeightB), true);
	Add(WasamiBossClip::Alert, Base * Change.WeightB, true);
	Add(WasamiBossClip::Detected, Slot, false);

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

float FWasamiBossAnimState::GetClipWeight(int32 Clip) const
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

void FWasamiBossAnimInstanceProxy::PreEvaluateAnimation(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::PreEvaluateAnimation(InAnimInstance);

	const UWasamiBossAnimInstance* Instance = CastChecked<UWasamiBossAnimInstance>(InAnimInstance);
	Samples.Reset();
	for (const FWasamiEnemyAnimSample& Sample : Instance->FrameSamples)
	{
		if (Instance->Clips.IsValidIndex(Sample.Clip) && Instance->Clips[Sample.Clip])
		{
			Samples.Add({Instance->Clips[Sample.Clip].Get(), Sample.Time, Sample.Weight, Sample.bLoop});
		}
	}
	LookAtAlpha = Instance->GetLookAtAlpha();
	LookAtLocation = Instance->Location;
}

bool FWasamiBossAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	WasamiEnemyAnim::BlendPoses(Samples, Output);
	if (LookAtAlpha > ZERO_ANIMWEIGHT_THRESH)
	{
		LookAt(Output);
	}
	return true;
}

void FWasamiBossAnimInstanceProxy::LookAt(FPoseContext& Output) const
{
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const int32 MeshBone = Bones.GetPoseBoneIndexForBoneName(FName(WasamiBossAnim::LookAtBone));
	if (MeshBone == INDEX_NONE)
	{
		return;
	}
	const FCompactPoseBoneIndex Bone = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshBone));
	if (!Bone.IsValid())
	{
		return;
	}
	// LookAt_Axis (0, 1, 0) not in the bone's space, no look up axis, no interpolation.
	FCSPose<FCompactPose> Pose;
	Pose.InitPose(Output.Pose);
	FTransform Transform = Pose.GetComponentSpaceTransform(Bone);
	const FVector Target = GetComponentTransform().InverseTransformPosition(LookAtLocation);
	const FQuat Delta = AnimationCore::SolveAim(Transform, Target, FVector::YAxisVector, false, FVector::UpVector, WasamiBossAnim::LookAtClamp);
	Transform.SetRotation(Delta * Transform.GetRotation());
	const FBoneTransform Aimed(Bone, Transform);
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(&Aimed, 1), LookAtAlpha);
	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Pose), Output.Pose);
}

float UWasamiBossAnimInstance::PlayDetected()
{
	if (!AnimState.PlayDetected())
	{
		UE_LOG(LogWasamiBossAnim, Warning, TEXT("%s: no clip Detected to play"), *GetPathName());
		return 0.f;
	}
	return AnimState.GetLength(WasamiBossClip::Detected);
}

FName UWasamiBossAnimInstance::GetMainClip(float& OutTime, float& OutWeight) const
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
	return Main ? FName(WasamiBossAnim::ClipNames[Main->Clip]) : NAME_None;
}

void UWasamiBossAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	TArray<float> Lengths;
	Lengths.SetNumZeroed(WasamiBossClip::Num);
	Clips.Reset();
	Clips.SetNum(WasamiBossClip::Num);
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		for (int32 Clip = 0; Clip < WasamiBossClip::Num; ++Clip)
		{
			const FSoftObjectPath Path = WasamiBossAnim::ClipPath(Clip);
			UAnimSequence* Sequence = Cast<UAnimSequence>(Path.TryLoad());
			if (!Sequence)
			{
				UE_LOG(LogWasamiBossAnim, Warning, TEXT("No animation %s (WasamiDDTools.import_wasami_boss makes it)"), *Path.ToString());
				continue;
			}
			Clips[Clip] = Sequence;
			Lengths[Clip] = Sequence->GetPlayLength();
		}
	}
	AnimState.Init(Lengths);
	FrameSamples.Reset();
}

void UWasamiBossAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// The ABP's event graph: its Matron's bMode and bSpotted, and the player's location while there is a player.
	if (const AWasamiMatron* Matron = Cast<AWasamiMatron>(GetOwningActor()))
	{
		bAlert = Matron->bMode;
		bSpotted = Matron->bSpotted;
	}
	if (const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		Location = Player->GetActorLocation();
	}
	AnimState.Update(bAlert, bSpotted, DeltaSeconds);
	AnimState.GetSamples(FrameSamples);
}

FAnimInstanceProxy* UWasamiBossAnimInstance::CreateAnimInstanceProxy()
{
	return new FWasamiBossAnimInstanceProxy(this);
}
