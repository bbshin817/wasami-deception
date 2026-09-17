#include "WasamiTelepathyTrackerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Curves/RichCurve.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "WasamiAssets.h"

namespace
{
	// UMG_TelepathyTracker: SizeBox_54's size overrides.
	constexpr float BoxSize = 256.f;
	// Construct: the material's Tiling and Speed, and the image's angle, each RandomFloatInRange.
	const FName TilingParameter(TEXT("Tiling"));
	const FName SpeedParameter(TEXT("Speed"));
	constexpr float MinParameter = 0.5f;
	constexpr float MaxParameter = 1.5f;
	constexpr float MinAngle = 0.5f;
	constexpr float MaxAngle = 360.f;

	// The animations' keys: ticks (60000 a second), values and the tangents saved per tick (here per second). All are
	// cubic. Appear plays [0, 30001) and ends at tick 30000; Disappear plays [0, 18001) and ends at tick 18000.
	constexpr double AnimTicksPerSecond = 60000.;
	struct FAnimKey
	{
		double Ticks;
		float Value;
		double TangentPerTick;
	};
	const FAnimKey AppearScaleKeys[] = {
		{0., 0.f, 0.},
		{15000., 1.f, 2.9999999242136255e-05},
		{30000., 0.949999988079071f, 0.},
	};
	const FAnimKey AppearOpacityKeys[] = {
		{0., 0.f, 0.},
		{30001., 1.f, 0.},
	};
	const FAnimKey DisappearScaleKeys[] = {
		{0., 1.f, 0.},
		{9000., 1.100000023841858f, -3.333333370392211e-05},
		{18000., 0.f, 0.},
	};
	const FAnimKey DisappearOpacityKeys[] = {
		{0., 1.f, 0.},
		{18000., 0.f, 0.},
	};
	// Disappear's transform section is [0, 18000): its last tick leaves the scale as it was.
	const float DisappearScaleSectionEnd = static_cast<float>(18000. / AnimTicksPerSecond);

	FRichCurve MakeAnimCurve(TConstArrayView<FAnimKey> Keys)
	{
		FRichCurve Curve;
		for (const FAnimKey& Each : Keys)
		{
			FRichCurveKey& Key = Curve.GetKey(Curve.AddKey(static_cast<float>(Each.Ticks / AnimTicksPerSecond), Each.Value));
			Key.InterpMode = RCIM_Cubic;
			Key.TangentMode = RCTM_User;
			Key.ArriveTangent = static_cast<float>(Each.TangentPerTick * AnimTicksPerSecond);
			Key.LeaveTangent = Key.ArriveTangent;
		}
		return Curve;
	}
}

UWasamiTelepathyTrackerWidget::UWasamiTelepathyTrackerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TrackerMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Blueprints/Main/Powers/Telepathy/MM_Telepathy_Inst")));
}

void UWasamiTelepathyTrackerWidget::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	Out.Add(GetDefault<UWasamiTelepathyTrackerWidget>()->TrackerMaterial.LoadSynchronous());
}

float UWasamiTelepathyTrackerWidget::EvaluateAppearScale(float Seconds)
{
	static const FRichCurve Curve = MakeAnimCurve(AppearScaleKeys);
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, AppearLength));
}

float UWasamiTelepathyTrackerWidget::EvaluateAppearOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeAnimCurve(AppearOpacityKeys);
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, AppearLength));
}

float UWasamiTelepathyTrackerWidget::EvaluateDisappearScale(float Seconds)
{
	static const FRichCurve Curve = MakeAnimCurve(DisappearScaleKeys);
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, DisappearLength));
}

float UWasamiTelepathyTrackerWidget::EvaluateDisappearOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeAnimCurve(DisappearOpacityKeys);
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, DisappearLength));
}

bool UWasamiTelepathyTrackerWidget::Initialize()
{
	const bool bResult = Super::Initialize();
	// The original's tree exists from the widget's creation (its tracker may set the size before Construct).
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_54"));
		SizeBox->SetWidthOverride(BoxSize);
		SizeBox->SetHeightOverride(BoxSize);
		WidgetTree->RootWidget = SizeBox;
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_90"));
		Image->SetBrushFromMaterial(TrackerMaterial.LoadSynchronous());
		SizeBox->AddChild(Image);
	}
	return bResult;
}

void UWasamiTelepathyTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// PlayAnimation(Appear) puts its first frame on at once; the angle set after it stays, as Appear only scales.
	AppearTime = 0.f;
	bAppearPlaying = true;
	ApplyAppear();
	if (!Image)
	{
		return;
	}
	if (UMaterialInstanceDynamic* Material = Image->GetDynamicMaterial())
	{
		Material->SetScalarParameterValue(TilingParameter, FMath::FRandRange(MinParameter, MaxParameter));
		Material->SetScalarParameterValue(SpeedParameter, FMath::FRandRange(MinParameter, MaxParameter));
	}
	Image->SetRenderTransformAngle(FMath::FRandRange(MinAngle, MaxAngle));
}

void UWasamiTelepathyTrackerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// The players tick in the order they started, so Disappear writes over Appear while both play.
	if (bAppearPlaying)
	{
		AppearTime += InDeltaTime;
		if (AppearTime >= AppearLength)
		{
			AppearTime = AppearLength;
			bAppearPlaying = false;
		}
		ApplyAppear();
	}
	if (bDisappearPlaying)
	{
		DisappearTime += InDeltaTime;
		if (DisappearTime >= DisappearLength)
		{
			DisappearTime = DisappearLength;
			bDisappearPlaying = false;
		}
		ApplyDisappear();
	}
}

void UWasamiTelepathyTrackerWidget::Remove()
{
	// PlayAnimation(Disappear): from its start again if it already plays, its first frame at once.
	DisappearTime = 0.f;
	bDisappearPlaying = true;
	ApplyDisappear();
}

void UWasamiTelepathyTrackerWidget::SetSize(float Size)
{
	if (SizeBox)
	{
		SizeBox->SetRenderScale(FVector2D(Size, Size));
	}
}

void UWasamiTelepathyTrackerWidget::ApplyAppear()
{
	if (Image)
	{
		Image->SetRenderScale(FVector2D(EvaluateAppearScale(AppearTime)));
		Image->SetRenderOpacity(EvaluateAppearOpacity(AppearTime));
	}
}

void UWasamiTelepathyTrackerWidget::ApplyDisappear()
{
	if (!Image)
	{
		return;
	}
	if (DisappearTime < DisappearScaleSectionEnd)
	{
		Image->SetRenderScale(FVector2D(EvaluateDisappearScale(DisappearTime)));
	}
	Image->SetRenderOpacity(EvaluateDisappearOpacity(DisappearTime));
}
