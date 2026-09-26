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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCameraAnimMoveTest, "Wasami.CameraAnim.Move",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCameraAnimMoveTest::RunTest(const FString& Parameters)
{
	UWasamiCameraAnim* Nurse = NewObject<UWasamiCameraAnim>();
	TestFalse(TEXT("no move track without curves"), Nurse->HasMoveTrack());
	TestTrue(TEXT("and no offset either"), Nurse->EvalMove(1.f).Equals(FTransform::Identity));

	// The capture scene's CameraAnim_Nurse_01, at four of its sixteen key times (the export's values, in the order
	// UInterpTrackMove makes the axes: the translations, then the rotation's X, Y and Z).
	const float Times[] = { 0.f, 1.7166667f, 1.8833333f, 2.f };
	const float Values[UWasamiCameraAnim::MoveCurveCount][4] = {
		{ 97.f, 215.83333f, 230.09137f, 230.09137f },
		{ 12.000021f, 18.318903f, 38.253197f, 38.253197f },
		{ 205.72832f, 116.87178f, 12.716431f, 9.7164307f },
		{ 0.f, -15.874989f, -83.19455f, -81.388962f },
		{ 0.f, -71.348885f, -11.385334f, -7.2126608f },
		{ 179.99995f, 196.14168f, 203.27153f, 202.9792f },
	};
	for (const float* Axis : Values)
	{
		FInterpCurveFloat& Curve = Nurse->MoveCurves.AddDefaulted_GetRef();
		for (int32 i = 0; i < UE_ARRAY_COUNT(Times); ++i)
		{
			AddFloatKey(Curve, Times[i], Axis[i], 0.f);
		}
	}
	TestTrue(TEXT("six axes make a move track"), Nurse->HasMoveTrack());

	// The camera starts 97 cm in front of where the sequence puts it, looking back over its shoulder (yaw 180).
	const FTransform Start = Nurse->EvalMove(0.f);
	TestEqual(TEXT("the first key's location"), Start.GetLocation(), FVector(97.0, 12.000021, 205.72832), 1e-2);
	TestEqual(TEXT("the first key's rotation"), Start.Rotator(), FRotator(0.0, 179.99995, 0.0), 1e-2);

	// 1.88 s in, the original rolls the camera onto its side: the fall to the floor the scene is about. The axes are
	// an euler UInterpTrackMove reads as FRotator(Y, Z, X), so the roll is the X axis, not the pitch.
	const FRotator Down = Nurse->EvalMove(1.8833333f).Rotator();
	TestEqual(TEXT("rolled onto its side"), Down.Roll, -83.19455, 1e-2);
	TestEqual(TEXT("pitched a little down"), Down.Pitch, -11.385334, 1e-2);
	// FRotator keeps the yaw within (-180, 180], so the anim's 203.27 comes out as its negative turn.
	TestEqual(TEXT("turned nearly right round"), Down.Yaw, 203.27153 - 360.0, 1e-2);
	TestEqual(TEXT("and down on the floor"), Nurse->EvalMove(1.8833333f).GetLocation().Z, 12.716431, 1e-2);

	// The section runs on to 25.27 s while the fade takes the screen, well past the last key at 2 s: the curves hold
	// their last value there, so the view stays on its side instead of snapping upright.
	TestEqual(TEXT("the last key"), Nurse->EvalMove(2.f).Rotator().Roll, -81.388962, 1e-2);
	TestEqual(TEXT("held past the end"), Nurse->EvalMove(4.f).Rotator().Roll, -81.388962, 1e-2);

	// What goes on the view is the change from the first key (bRelativeToInitialTransform): nothing at the start,
	// then by 2 s the camera has been knocked 1.3 m back and 2 m down to the floor, rolled onto its side. The first
	// key only turns yaw 180, so the change is the translation turned round and the yaw less 180.
	TestTrue(TEXT("no change at the start"), Nurse->EvalRelativeMove(0.f).Equals(FTransform::Identity, 1e-3));
	const FTransform Fallen = Nurse->EvalRelativeMove(2.f);
	TestEqual(TEXT("knocked back and down"), Fallen.GetLocation(),
		FVector(-(230.09137 - 97.0), -(38.253197 - 12.000021), 9.7164307 - 205.72832), 0.1);
	TestEqual(TEXT("its pitch"), Fallen.Rotator().Pitch, -7.2126608, 0.05);
	TestEqual(TEXT("its yaw, less the first key's"), Fallen.Rotator().Yaw, 202.9792 - 179.99995, 0.05);
	TestEqual(TEXT("its roll"), Fallen.Rotator().Roll, -81.388962, 0.05);

	// The offset goes on the view in the camera's own space: the location turned by the camera's rotation, the
	// rotation composed before it (FCameraAnimationHelper::ApplyOffset, where UE4 put a camera anim's).
	FMinimalViewInfo View;
	View.Location = FVector(-10900.0, -1020.0, 1017.0);
	View.Rotation = FRotator(0.f, 90.f, 0.f);
	UWasamiCameraAnimOffsetModifier::ApplyOffset(Fallen, View);
	TestEqual(TEXT("the offset's X goes along the camera's facing"), View.Location,
		FVector(-10900.0 + (38.253197 - 12.000021), -1020.0 - (230.09137 - 97.0), 1017.0 + 9.7164307 - 205.72832), 0.1);
	TestEqual(TEXT("and its yaw adds to the camera's"), View.Rotation.Yaw, 90.0 + 202.9792 - 179.99995, 0.05);
	TestEqual(TEXT("the roll comes through"), View.Rotation.Roll, -81.388962, 0.05);

	AWasamiCameraAnimOffset* Offset = NewObject<AWasamiCameraAnimOffset>();
	Offset->StartTime = 20.533333f;
	Offset->EndTime = 25.266666f;
	TestFalse(TEXT("before the section"), Offset->IsInSection(20.f));
	TestTrue(TEXT("its start"), Offset->IsInSection(20.533333f));
	TestTrue(TEXT("the fall"), Offset->IsInSection(22.416666f));
	TestFalse(TEXT("after it"), Offset->IsInSection(25.3f));
	return true;
}

#endif
