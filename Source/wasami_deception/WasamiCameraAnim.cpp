#include "WasamiCameraAnim.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraAnimationHelper.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

DEFINE_LOG_CATEGORY_STATIC(LogWasamiCameraAnim, Log, All);

namespace
{
	// The tracks' property paths start with this; what follows is a member of FPostProcessSettings.
	const TCHAR* const PostProcessPrefix = TEXT("CameraComponent.PostProcessSettings.");
	const TCHAR* const FieldOfViewProperty = TEXT("CameraComponent.FieldOfView");
	// UE4's CameraAnimInst kept the animated FOV within these.
	constexpr float MinFOV = 5.f;
	constexpr float MaxFOV = 170.f;

	/** The FPostProcessSettings member a track animates, or null for a track that is not a post-process one. */
	FProperty* FindPostProcessProperty(const FString& PropertyName)
	{
		if (!PropertyName.StartsWith(PostProcessPrefix))
		{
			return nullptr;
		}
		const FString Member = PropertyName.RightChop(FCString::Strlen(PostProcessPrefix));
		return FPostProcessSettings::StaticStruct()->FindPropertyByName(FName(*Member));
	}
}

void UWasamiCameraAnim::ApplyPostProcessTracks(float Time, FPostProcessSettings& Settings) const
{
	for (const FWasamiCameraAnimFloatTrack& Track : FloatTracks)
	{
		if (FFloatProperty* Property = CastField<FFloatProperty>(FindPostProcessProperty(Track.PropertyName)))
		{
			const float Current = Property->GetPropertyValue_InContainer(&Settings);
			Property->SetPropertyValue_InContainer(&Settings, Track.Curve.Eval(Time, Current));
		}
	}
	for (const FWasamiCameraAnimColorTrack& Track : ColorTracks)
	{
		const FStructProperty* Property = CastField<FStructProperty>(FindPostProcessProperty(Track.PropertyName));
		if (Property && Property->Struct == TBaseStructure<FLinearColor>::Get())
		{
			FLinearColor* Value = Property->ContainerPtrToValuePtr<FLinearColor>(&Settings);
			*Value = Track.Curve.Eval(Time, *Value);
		}
	}
}

FTransform UWasamiCameraAnim::EvalMove(float Time) const
{
	if (!HasMoveTrack())
	{
		return FTransform::Identity;
	}
	const FVector Location(MoveCurves[0].Eval(Time, 0.f), MoveCurves[1].Eval(Time, 0.f), MoveCurves[2].Eval(Time, 0.f));
	// UInterpTrackMove makes an euler of its three rotation axes and reads it as FRotator(Y, Z, X).
	const FRotator Rotation(MoveCurves[4].Eval(Time, 0.f), MoveCurves[5].Eval(Time, 0.f), MoveCurves[3].Eval(Time, 0.f));
	return FTransform(Rotation, Location);
}

const FWasamiCameraAnimFloatTrack* UWasamiCameraAnim::FindFieldOfViewTrack() const
{
	return FloatTracks.FindByPredicate([](const FWasamiCameraAnimFloatTrack& Track) { return Track.PropertyName == FieldOfViewProperty; });
}

void FWasamiCameraAnimPlayback::Start(float InAnimLength, float InRate, float InScale, float InBlendInTime, float InBlendOutTime, bool bInLoop, float Duration)
{
	AnimLength = InAnimLength;
	PlayRate = InRate;
	Scale = InScale;
	BlendInTime = InBlendInTime;
	BlendOutTime = InBlendOutTime;
	bLoop = bInLoop;
	CurTime = 0.f;
	CurBlendInTime = 0.f;
	CurBlendOutTime = 0.f;
	bBlendingIn = true;
	bBlendingOut = false;
	bFinished = false;
	Weight = 0.f;
	// A Duration is the whole play time, the blend out included.
	RemainingTime = Duration > 0.f ? Duration - BlendOutTime : 0.f;
}

void FWasamiCameraAnimPlayback::Advance(float DeltaTime)
{
	if (bFinished)
	{
		return;
	}
	bool bJustFinished = false;
	CurTime += DeltaTime * PlayRate;
	if (bBlendingIn)
	{
		CurBlendInTime += DeltaTime;
	}
	if (bBlendingOut)
	{
		CurBlendOutTime += DeltaTime;
	}

	if (CurTime > AnimLength)
	{
		if (bLoop && AnimLength > 0.f)
		{
			CurTime = FMath::Fmod(CurTime, AnimLength);
		}
		else
		{
			bJustFinished = true;
		}
	}
	else if (!bLoop && !bBlendingOut && CurTime > AnimLength - BlendOutTime * PlayRate)
	{
		// A non-looping anim blends out so that it is gone at its end.
		bBlendingOut = true;
		CurBlendOutTime = CurTime - (AnimLength - BlendOutTime * PlayRate);
	}

	if (bBlendingIn && (CurBlendInTime > BlendInTime || BlendInTime == 0.f))
	{
		bBlendingIn = false;
	}
	if (bBlendingOut && CurBlendOutTime > BlendOutTime)
	{
		CurBlendOutTime = BlendOutTime;
		bJustFinished = true;
	}

	// The blends are weighed apart and the smaller wins, so a blend out during the blend in carries on from where it is.
	const float BlendInWeight = bBlendingIn ? CurBlendInTime / BlendInTime : 1.f;
	const float BlendOutWeight = bBlendingOut ? 1.f - CurBlendOutTime / BlendOutTime : 1.f;
	Weight = FMath::Min(BlendInWeight, BlendOutWeight) * Scale;

	if (bJustFinished)
	{
		Stop(true);
	}
	else if (RemainingTime > 0.f)
	{
		RemainingTime -= DeltaTime;
		if (RemainingTime <= 0.f)
		{
			Stop(false);
		}
	}
}

void FWasamiCameraAnimPlayback::Stop(bool bImmediate)
{
	if (bImmediate || BlendOutTime <= 0.f)
	{
		bFinished = true;
		Weight = 0.f;
	}
	else if (!bBlendingOut)
	{
		bBlendingOut = true;
		CurBlendOutTime = 0.f;
	}
}

UWasamiCameraAnimModifier* UWasamiCameraAnimModifier::Get(APlayerCameraManager* CameraManager)
{
	if (!CameraManager)
	{
		return nullptr;
	}
	if (UCameraModifier* Found = CameraManager->FindCameraModifierByClass(StaticClass()))
	{
		return CastChecked<UWasamiCameraAnimModifier>(Found);
	}
	return Cast<UWasamiCameraAnimModifier>(CameraManager->AddNewCameraModifier(StaticClass()));
}

int32 UWasamiCameraAnimModifier::Play(UWasamiCameraAnim* Anim, float Rate, float Scale, float BlendInTime, float BlendOutTime, bool bLoop, float Duration)
{
	if (!Anim)
	{
		return 0;
	}
	for (const FWasamiCameraAnimFloatTrack& Track : Anim->FloatTracks)
	{
		if (Track.PropertyName != FieldOfViewProperty && !FindPostProcessProperty(Track.PropertyName))
		{
			UE_LOG(LogWasamiCameraAnim, Warning, TEXT("%s: the track of %s is not played (only post-process and field of view tracks are)."), *Anim->GetName(), *Track.PropertyName);
		}
	}

	FWasamiCameraAnimInstance& Instance = Instances.AddDefaulted_GetRef();
	Instance.Anim = Anim;
	Instance.Handle = NextHandle++;
	Instance.Playback.Start(Anim->AnimLength, Rate, Scale, BlendInTime, BlendOutTime, bLoop, Duration);
	if (const FWasamiCameraAnimFloatTrack* FieldOfView = Anim->FindFieldOfViewTrack())
	{
		Instance.InitialFOV = FieldOfView->Curve.Eval(Instance.Playback.CurTime, Anim->BaseFOV);
	}
	return Instance.Handle;
}

void UWasamiCameraAnimModifier::Stop(int32 Handle, bool bImmediate)
{
	for (FWasamiCameraAnimInstance& Instance : Instances)
	{
		if (Instance.Handle == Handle)
		{
			Instance.Playback.Stop(bImmediate);
		}
	}
	Instances.RemoveAll([](const FWasamiCameraAnimInstance& Each) { return Each.Playback.bFinished; });
}

bool UWasamiCameraAnimModifier::IsPlaying(int32 Handle) const
{
	return Instances.ContainsByPredicate([Handle](const FWasamiCameraAnimInstance& Each) { return Each.Handle == Handle && !Each.Playback.bFinished; });
}

float UWasamiCameraAnimModifier::AddFieldOfView(float ViewFOV, float TrackFOV, float InitialFOV, float Weight)
{
	return FMath::Clamp(ViewFOV + (TrackFOV - InitialFOV) * Weight, MinFOV, MaxFOV);
}

bool UWasamiCameraAnimModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);
	for (FWasamiCameraAnimInstance& Instance : Instances)
	{
		FWasamiCameraAnimPlayback& Playback = Instance.Playback;
		Playback.Advance(DeltaTime);
		const UWasamiCameraAnim* Anim = Instance.Anim;
		if (Playback.bFinished || Playback.Weight <= 0.f || !Anim)
		{
			continue;
		}
		if (const FWasamiCameraAnimFloatTrack* FieldOfView = Anim->FindFieldOfViewTrack())
		{
			InOutPOV.FOV = AddFieldOfView(InOutPOV.FOV, FieldOfView->Curve.Eval(Playback.CurTime, Instance.InitialFOV), Instance.InitialFOV, Playback.Weight);
		}
		const float PostProcessWeight = Anim->BasePostProcessBlendWeight * Playback.Weight;
		if (PostProcessWeight > 0.f && CameraOwner)
		{
			FPostProcessSettings Settings = Anim->BasePostProcessSettings;
			Anim->ApplyPostProcessTracks(Playback.CurTime, Settings);
			CameraOwner->AddCachedPPBlend(Settings, PostProcessWeight, VTBlendOrder_Base);
		}
	}
	Instances.RemoveAll([](const FWasamiCameraAnimInstance& Each) { return Each.Playback.bFinished; });
	return false;
}

void AWasamiCameraAnimOffset::BeginPlay()
{
	Super::BeginPlay();
	APlayerCameraManager* Manager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (UWasamiCameraAnimOffsetModifier* Modifier = UWasamiCameraAnimOffsetModifier::Get(Manager))
	{
		Modifier->Add(this);
	}
	else
	{
		UE_LOG(LogWasamiCameraAnim, Warning, TEXT("%s: no player camera manager, so %s is not played."),
			*GetName(), Anim ? *Anim->GetName() : TEXT("its anim"));
	}
}

void AWasamiCameraAnimOffset::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWasamiCameraAnimOffsetModifier* Modifier = UWasamiCameraAnimOffsetModifier::Get(UGameplayStatics::GetPlayerCameraManager(this, 0)))
	{
		Modifier->Remove(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool AWasamiCameraAnimOffset::CurrentOffset(FTransform& OutOffset) const
{
	ULevelSequencePlayer* Player = Sequence ? Sequence->GetSequencePlayer() : nullptr;
	if (!Anim || !Anim->HasMoveTrack() || !Player || !Player->IsPlaying())
	{
		return false;
	}
	const float Time = static_cast<float>(Player->GetCurrentTime().AsSeconds());
	if (!IsInSection(Time))
	{
		return false;
	}
	OutOffset = Anim->EvalMove(Time - StartTime);
	return true;
}

UWasamiCameraAnimOffsetModifier* UWasamiCameraAnimOffsetModifier::Get(APlayerCameraManager* CameraManager)
{
	if (!CameraManager)
	{
		return nullptr;
	}
	if (UCameraModifier* Found = CameraManager->FindCameraModifierByClass(StaticClass()))
	{
		return CastChecked<UWasamiCameraAnimOffsetModifier>(Found);
	}
	return Cast<UWasamiCameraAnimOffsetModifier>(CameraManager->AddNewCameraModifier(StaticClass()));
}

void UWasamiCameraAnimOffsetModifier::Add(AWasamiCameraAnimOffset* Offset)
{
	Offsets.AddUnique(Offset);
}

void UWasamiCameraAnimOffsetModifier::Remove(AWasamiCameraAnimOffset* Offset)
{
	Offsets.Remove(Offset);
}

void UWasamiCameraAnimOffsetModifier::ApplyOffset(const FTransform& Offset, FMinimalViewInfo& InOutPOV)
{
	const FCameraAnimationHelperOffset Move{ Offset.GetLocation(), Offset.Rotator() };
	FVector Location;
	FRotator Rotation;
	FCameraAnimationHelper::ApplyOffset(InOutPOV, Move, Location, Rotation);
	InOutPOV.Location = Location;
	InOutPOV.Rotation = Rotation;
}

bool UWasamiCameraAnimOffsetModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);
	const AActor* ViewTarget = CameraOwner ? CameraOwner->GetViewTarget() : nullptr;
	for (const TObjectPtr<AWasamiCameraAnimOffset>& Offset : Offsets)
	{
		FTransform Move;
		// Only what the view is looking through: the offset is in that camera's own space.
		if (Offset && Offset->Camera == ViewTarget && Offset->CurrentOffset(Move))
		{
			ApplyOffset(Move, InOutPOV);
		}
	}
	return false;
}
