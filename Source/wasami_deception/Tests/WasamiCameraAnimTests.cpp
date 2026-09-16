#include "Misc/AutomationTest.h"
#include "../WasamiCameraAnim.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	void AddFloatKey(FInterpCurveFloat& Curve, float Time, float Value, float Tangent)
	{
		FInterpCurvePoint<float>& Point = Curve.Points[Curve.AddPoint(Time, Value)];
		Point.ArriveTangent = Tangent;
		Point.LeaveTangent = Tangent;
		Point.InterpMode = CIM_CurveAutoClamped;
	}

	void AddColorKey(FInterpCurveLinearColor& Curve, float Time, const FLinearColor& Value, const FLinearColor& Tangent)
	{
		FInterpCurvePoint<FLinearColor>& Point = Curve.Points[Curve.AddPoint(Time, Value)];
		Point.ArriveTangent = Tangent;
		Point.LeaveTangent = Tangent;
		Point.InterpMode = CIM_CurveAutoClamped;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCameraAnimPlaybackTest, "Wasami.CameraAnim.Playback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCameraAnimPlaybackTest::RunTest(const FString& Parameters)
{
	// The speed boost's: CameraAnim_SpeedBoost (24.5 s) with 0.5 s blends, over the boost's 9.75 s.
	FWasamiCameraAnimPlayback Boost;
	Boost.Start(24.502966f, 1.f, 1.f, 0.5f, 0.5f, false, 9.75f);
	Boost.Advance(0.25f);
	TestEqual(TEXT("halfway into the blend in"), Boost.Weight, 0.5f);
	Boost.Advance(0.25f);
	TestEqual(TEXT("blended in"), Boost.Weight, 1.f);
	for (int32 Step = 0; Step < 35; ++Step)
	{
		Boost.Advance(0.25f);
	}
	TestEqual(TEXT("still full at 9.25 s"), Boost.Weight, 1.f);
	TestTrue(TEXT("the duration's blend out has started"), Boost.bBlendingOut);
	Boost.Advance(0.25f);
	TestEqual(TEXT("halfway out at 9.5 s"), Boost.Weight, 0.5f);
	Boost.Advance(0.25f);
	TestEqual(TEXT("gone at 9.75 s"), Boost.Weight, 0.f);
	Boost.Advance(0.25f);
	TestTrue(TEXT("finished after its duration"), Boost.bFinished);

	// The reset's Stop(true).
	FWasamiCameraAnimPlayback Reset;
	Reset.Start(24.502966f, 1.f, 1.f, 0.5f, 0.5f, false, 9.75f);
	Reset.Advance(1.f);
	Reset.Stop(true);
	TestTrue(TEXT("an immediate stop ends it"), Reset.bFinished);
	TestEqual(TEXT("an immediate stop leaves no weight"), Reset.Weight, 0.f);

	// The teleport's: no blends, no duration, it ends with its length.
	FWasamiCameraAnimPlayback Teleport;
	Teleport.Start(0.5f, 1.f, 1.f, 0.f, 0.f, false, 0.f);
	Teleport.Advance(0.25f);
	TestEqual(TEXT("no blend in means full at once"), Teleport.Weight, 1.f);
	TestEqual(TEXT("the time in the anim"), Teleport.CurTime, 0.25f);
	Teleport.Advance(0.3f);
	TestTrue(TEXT("past its length it is finished"), Teleport.bFinished);

	// A blend out asked for during the blend in carries on from the lower weight.
	FWasamiCameraAnimPlayback Early;
	Early.Start(24.502966f, 1.f, 1.f, 0.5f, 0.5f, false, 0.f);
	Early.Advance(0.1f);
	Early.Stop(false);
	Early.Advance(0.1f);
	TestEqual(TEXT("the smaller of the two blends"), Early.Weight, 0.4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCameraAnimTracksTest, "Wasami.CameraAnim.Tracks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCameraAnimTracksTest::RunTest(const FString& Parameters)
{
	// CameraAnim_SpeedBoost: one key before the start holds its tint for the whole anim.
	UWasamiCameraAnim* Boost = NewObject<UWasamiCameraAnim>();
	FWasamiCameraAnimColorTrack& BoostTint = Boost->ColorTracks.AddDefaulted_GetRef();
	BoostTint.PropertyName = TEXT("CameraComponent.PostProcessSettings.SceneColorTint");
	const FLinearColor Red(2.f, 0.583955f, 0.498f, 1.f);
	AddColorKey(BoostTint.Curve, -0.0017948f, Red, FLinearColor(0.f, 0.f, 0.f, 0.f));
	FPostProcessSettings Settings;
	Boost->ApplyPostProcessTracks(5.f, Settings);
	TestTrue(TEXT("the boost's tint holds"), Settings.SceneColorTint.Equals(Red));
	TestFalse(TEXT("the tracks leave the override flags alone"), Settings.bOverride_SceneColorTint != 0);

	// CameraAnim_Teleport (pak_reference): the saved tangents give the export's own samples (CameraAnim_Teleport.csv).
	UWasamiCameraAnim* Teleport = NewObject<UWasamiCameraAnim>();
	FWasamiCameraAnimFloatTrack& Exposure = Teleport->FloatTracks.AddDefaulted_GetRef();
	Exposure.PropertyName = TEXT("CameraComponent.PostProcessSettings.AutoExposureBias");
	AddFloatKey(Exposure.Curve, 0.f, 0.f, 0.f);
	AddFloatKey(Exposure.Curve, 0.07f, 1.f, 15.384616f);
	AddFloatKey(Exposure.Curve, 0.13f, 2.f, 62.066772f);
	AddFloatKey(Exposure.Curve, 0.13541664f, 100.f, 0.f);
	AddFloatKey(Exposure.Curve, 0.13982369f, 2.f, -107.31531f);
	AddFloatKey(Exposure.Curve, 0.21f, 0.f, 0.f);
	FWasamiCameraAnimFloatTrack& FieldOfView = Teleport->FloatTracks.AddDefaulted_GetRef();
	FieldOfView.PropertyName = TEXT("CameraComponent.FieldOfView");
	AddFloatKey(FieldOfView.Curve, 0.f, 90.f, 0.f);
	FWasamiCameraAnimColorTrack& Tint = Teleport->ColorTracks.AddDefaulted_GetRef();
	Tint.PropertyName = TEXT("CameraComponent.PostProcessSettings.SceneColorTint");
	const FLinearColor White(1.f, 1.f, 1.f, 1.f);
	const FLinearColor Deep(2.f, 0.11445439f, 0.f, 1.f);
	AddColorKey(Tint.Curve, 0.f, White, FLinearColor(0.f, 0.f, 0.f, 0.f));
	AddColorKey(Tint.Curve, 0.069711536f, White, FLinearColor(7.8f, -6.9072556f, -7.8f, 0.f));
	AddColorKey(Tint.Curve, 0.12820514f, Deep, FLinearColor(7.0756302f, -6.2657933f, -7.0756302f, 0.f));
	AddColorKey(Tint.Curve, 0.2110417f, Deep, FLinearColor(-3.9788322f, 3.5234375f, 3.9788322f, 0.f));
	AddColorKey(Tint.Curve, 0.37953517f, White, FLinearColor(0.f, 0.f, 0.f, 0.f));

	FPostProcessSettings AtTenth;
	Teleport->ApplyPostProcessTracks(0.1f, AtTenth);
	TestEqual(TEXT("exposure at 0.1 s"), AtTenth.AutoExposureBias, 1.149884f, 1e-4f);
	TestEqual(TEXT("tint R at 0.1 s"), AtTenth.SceneColorTint.R, 1.528122f, 1e-4f);
	TestEqual(TEXT("tint G at 0.1 s"), AtTenth.SceneColorTint.G, 0.532324f, 1e-4f);
	TestEqual(TEXT("tint B at 0.1 s"), AtTenth.SceneColorTint.B, 0.471878f, 1e-4f);
	FPostProcessSettings AtTwentieth;
	Teleport->ApplyPostProcessTracks(0.05f, AtTwentieth);
	TestEqual(TEXT("exposure at 0.05 s"), AtTwentieth.AutoExposureBias, 0.644763f, 1e-4f);
	TestEqual(TEXT("tint G at 0.05 s"), AtTwentieth.SceneColorTint.G, 1.070042f, 1e-4f);
	// The CSV's 0.13333 is its eighth 60 fps frame; the exposure climbs steeply there.
	FPostProcessSettings AtFlash;
	Teleport->ApplyPostProcessTracks(8.f / 60.f, AtFlash);
	TestEqual(TEXT("the exposure's flash"), AtFlash.AutoExposureBias, 67.69149f, 1e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCameraAnimFieldOfViewTest, "Wasami.CameraAnim.FieldOfView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCameraAnimFieldOfViewTest::RunTest(const FString& Parameters)
{
	// CameraAnim_Teleport's field of view (pak_reference), played as the teleport's click plays it.
	UWasamiCameraAnim* Teleport = NewObject<UWasamiCameraAnim>();
	Teleport->AnimLength = 0.5f;
	Teleport->BaseFOV = 137.24078f;
	FWasamiCameraAnimFloatTrack& FieldOfView = Teleport->FloatTracks.AddDefaulted_GetRef();
	FieldOfView.PropertyName = TEXT("CameraComponent.FieldOfView");
	AddFloatKey(FieldOfView.Curve, 0.f, 90.f, 0.f);
	AddFloatKey(FieldOfView.Curve, 0.13f, 150.f, 0.f);
	AddFloatKey(FieldOfView.Curve, 0.18f, 80.f, 0.f);
	AddFloatKey(FieldOfView.Curve, 0.25f, 100.f, 0.f);
	AddFloatKey(FieldOfView.Curve, 0.4f, 90.f, 0.f);
	TestNotNull(TEXT("the field of view track is found"), Teleport->FindFieldOfViewTrack());
	FPostProcessSettings Untouched;
	Teleport->ApplyPostProcessTracks(0.13f, Untouched);
	TestEqual(TEXT("the field of view is no post-process setting"), Untouched.AutoExposureBias, FPostProcessSettings().AutoExposureBias);

	UWasamiCameraAnimModifier* Anims = NewObject<UWasamiCameraAnimModifier>();
	const int32 Handle = Anims->Play(Teleport, 1.f, 1.f, 0.f, 0.f, false, 0.f);
	// The track counts from its value at the start (90), not from BaseFOV: the classic build widens at once.
	FMinimalViewInfo View;
	View.FOV = 90.f;
	Anims->ModifyCamera(1.f / 60.f, View);
	TestEqual(TEXT("the export's first 60 fps sample"), View.FOV, 92.706f, 1e-2f);
	// A view whose FOV is not 90 gets the same change.
	View.FOV = 100.f;
	Anims->ModifyCamera(0.05f - 1.f / 60.f, View);
	TestEqual(TEXT("the change at 0.05 s on a 100° view"), View.FOV, 119.7997f, 1e-2f);
	View.FOV = 90.f;
	Anims->ModifyCamera(0.08f, View);
	TestEqual(TEXT("the widest at 0.13 s"), View.FOV, 150.f, 1e-2f);
	View.FOV = 90.f;
	Anims->ModifyCamera(0.4f, View);
	TestFalse(TEXT("over after 0.5 s"), Anims->IsPlaying(Handle));
	TestEqual(TEXT("an ended anim leaves the view alone"), View.FOV, 90.f);

	TestEqual(TEXT("kept under 170°"), UWasamiCameraAnimModifier::AddFieldOfView(165.f, 150.f, 90.f, 1.f), 170.f);
	TestEqual(TEXT("kept over 5°"), UWasamiCameraAnimModifier::AddFieldOfView(10.f, 80.f, 90.f, 1.f), 5.f);
	TestEqual(TEXT("scaled by the weight"), UWasamiCameraAnimModifier::AddFieldOfView(90.f, 150.f, 90.f, 0.5f), 120.f);
	return true;
}

#endif
