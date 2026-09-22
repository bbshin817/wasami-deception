#include "Misc/AutomationTest.h"

#include "../WasamiChapterPortalWidget.h"
#include "../WasamiPauseWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Widget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiChapterPortalTreeTest, "Wasami.ChapterPortal.Tree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiChapterPortalTreeTest::RunTest(const FString& Parameters)
{
	using W = UWasamiChapterPortalWidget;

	TestEqual(TEXT("added at Z 0"), W::ViewportZOrder, 0);

	// The tree as the designer shows it: Construct (which plays loop from 0.8 s) does not run at design time.
	W* Card = NewObject<W>();
	Card->Initialize();
#if WITH_EDITOR
	Card->SetDesignerFlags(EWidgetDesignFlags::Designing);
#endif
	Card->TakeWidget();
	UWidgetTree* Tree = Card->WidgetTree;
	if (!TestNotNull(TEXT("the tree"), Tree))
	{
		return false;
	}

	// The root's children in the original's order (the later drawn over the earlier), and the portal's.
	UCanvasPanel* Root = Cast<UCanvasPanel>(Tree->RootWidget);
	if (!TestNotNull(TEXT("CanvasPanel_0 is the root"), Root) || !TestEqual(TEXT("its name"), Root->GetFName(), FName(TEXT("CanvasPanel_0"))))
	{
		return false;
	}
	const TCHAR* const RootChildren[] = {TEXT("BackgroundBlur_0"), TEXT("Image_1"), TEXT("Image_48"), TEXT("Image_49"), TEXT("Icon"), TEXT("TitleCard")};
	if (TestEqual(TEXT("six under the root"), Root->GetChildrenCount(), 6))
	{
		for (int32 Index = 0; Index < 6; ++Index)
		{
			TestEqual(FString::Printf(TEXT("child %d"), Index), Root->GetChildAt(Index)->GetFName(), FName(RootChildren[Index]));
		}
	}
	UCanvasPanel* Icon = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Icon")));
	const TCHAR* const IconChildren[] = {TEXT("Image_40"), TEXT("Image_41"), TEXT("Logo")};
	if (TestNotNull(TEXT("Icon"), Icon) && TestEqual(TEXT("three in the portal"), Icon->GetChildrenCount(), 3))
	{
		for (int32 Index = 0; Index < 3; ++Index)
		{
			TestEqual(FString::Printf(TEXT("the portal's child %d"), Index), Icon->GetChildAt(Index)->GetFName(), FName(IconChildren[Index]));
		}
	}

	// The pictures and their sizes.
	struct FPicture
	{
		const TCHAR* Widget;
		const TCHAR* Texture;
		FVector2D Size;
	};
	const FPicture Pictures[] = {
		{TEXT("Image_40"), TEXT("chapter_ui_portal_outer"), FVector2D(470.f, 470.f)},
		{TEXT("Image_41"), TEXT("chapter_title_portal_inner"), FVector2D(470.f, 470.f)},
		{TEXT("Logo"), TEXT("T_PauseHead"), FVector2D(470.f, 470.f)},
		{TEXT("Image_48"), TEXT("chapter_ui_banner_bg_01"), FVector2D(4165.f, 872.f)},
		{TEXT("Image_49"), TEXT("chapter_ui_banner_bg_02"), FVector2D(4165.f, 872.f)},
		{TEXT("TitleCard"), TEXT("T_LevelTitle"), FVector2D(901.f, 180.f)},
	};
	for (const FPicture& Each : Pictures)
	{
		const UImage* Image = Cast<UImage>(Tree->FindWidget(Each.Widget));
		if (!TestNotNull(Each.Widget, Image))
		{
			continue;
		}
		const UObject* Texture = Image->GetBrush().GetResourceObject();
		if (Texture)
		{
			TestEqual(FString::Printf(TEXT("%s shows %s"), Each.Widget, Each.Texture), Texture->GetName(), FString(Each.Texture));
		}
		else
		{
			AddError(FString::Printf(TEXT("%s is missing: run WasamiDDTools.import_dd_ui"), Each.Texture));
		}
		TestEqual(FString::Printf(TEXT("%s's size"), Each.Widget), FVector2D(Image->GetBrush().ImageSize), Each.Size);
	}

	// The colours: the wash, the head tinted as on the pause menu, the title red; the banners tiled across.
	const UImage* Wash = Cast<UImage>(Tree->FindWidget(TEXT("Image_1")));
	TestTrue(TEXT("the wash's faint red"), Wash && Wash->GetBrush().TintColor.GetSpecifiedColor().Equals(FLinearColor(0.109375f, 0.f, 0.f, 0.2f)));
	TestNull(TEXT("the wash has no texture"), Wash ? Wash->GetBrush().GetResourceObject() : nullptr);
	const UImage* Logo = Cast<UImage>(Tree->FindWidget(TEXT("Logo")));
	TestTrue(TEXT("the head in the pause menu's red"), Logo && Logo->GetBrush().TintColor.GetSpecifiedColor().Equals(UWasamiPauseWidget::HeadTint()));
	TestEqual(TEXT("the head lifted into the middle of the ring"), Logo ? FVector2D(Logo->GetRenderTransform().Translation) : FVector2D::ZeroVector, UWasamiChapterPortalWidget::HeadOffset);
	const UImage* Title = Cast<UImage>(Tree->FindWidget(TEXT("TitleCard")));
	TestTrue(TEXT("the title red"), Title && Title->GetColorAndOpacity().Equals(FLinearColor(1.f, 0.f, 0.f, 1.f)));
	TestEqual(TEXT("the title moved (-72, 23)"), Title ? FVector2D(Title->GetRenderTransform().Translation) : FVector2D::ZeroVector, FVector2D(-72.f, 23.f));
	for (const TCHAR* Name : {TEXT("Image_48"), TEXT("Image_49")})
	{
		const UImage* Banner = Cast<UImage>(Tree->FindWidget(Name));
		TestTrue(FString::Printf(TEXT("%s tiles across"), Name), Banner && Banner->GetBrush().Tiling == ESlateBrushTileType::Horizontal);
	}
	const UImage* Ring = Cast<UImage>(Tree->FindWidget(TEXT("Image_40")));
	const UImage* Runes = Cast<UImage>(Tree->FindWidget(TEXT("Image_41")));
	TestEqual(TEXT("the ring's angle"), Ring ? Ring->GetRenderTransformAngle() : 0.f, 800.f);
	TestEqual(TEXT("the runes' angle"), Runes ? Runes->GetRenderTransformAngle() : 0.f, -359.f);
	const UBackgroundBlur* Blur = Cast<UBackgroundBlur>(Tree->FindWidget(TEXT("BackgroundBlur_0")));
	TestTrue(TEXT("the blur at 0 in the tree"), Blur && FMath::IsNearlyZero(Blur->GetBlurStrength()));

	// The slots: where each is anchored, its offsets, alignment and whether it takes its content's size.
	struct FPlace
	{
		const TCHAR* Widget;
		FAnchors Anchors;
		FVector2D Position;
		FVector2D Alignment;
		bool bAutoSize;
	};
	const FPlace Places[] = {
		{TEXT("BackgroundBlur_0"), FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D(-40.96381378173828f, -34.0782585144043f), FVector2D::ZeroVector, false},
		{TEXT("Image_1"), FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D(-13.51351261138916f, -15.0150146484375f), FVector2D::ZeroVector, false},
		{TEXT("Image_48"), FAnchors(0.5f, 0.5f), FVector2D(-1124.6058349609375f, 15.859466552734375f), FVector2D(0.5f, 0.5f), true},
		{TEXT("Image_49"), FAnchors(0.5f, 0.5f), FVector2D(1103.771484375f, 19.45947265625f), FVector2D(0.5f, 0.5f), true},
		{TEXT("Icon"), FAnchors(0.25178566575050354f, 0.47777774930000305f), FVector2D(-239.91238403320312f, -236.5164794921875f), FVector2D::ZeroVector, true},
		{TEXT("TitleCard"), FAnchors(0.36875003576278687f, 0.4888889193534851f), FVector2D(71.37957763671875f, -116.528564453125f), FVector2D::ZeroVector, true},
		{TEXT("Image_40"), FAnchors(0.5f, 0.5f), FVector2D::ZeroVector, FVector2D(0.5f, 0.5f), true},
		{TEXT("Image_41"), FAnchors(0.5f, 0.5f), FVector2D::ZeroVector, FVector2D(0.5f, 0.5f), true},
		{TEXT("Logo"), FAnchors(0.5f, 0.5f), FVector2D::ZeroVector, FVector2D(0.5f, 0.5f), true},
	};
	for (const FPlace& Each : Places)
	{
		const UWidget* Widget = Tree->FindWidget(Each.Widget);
		const UCanvasPanelSlot* Slot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
		if (!TestNotNull(FString::Printf(TEXT("%s's canvas slot"), Each.Widget), Slot))
		{
			continue;
		}
		const FAnchorData Layout = Slot->GetLayout();
		TestTrue(FString::Printf(TEXT("%s's anchors"), Each.Widget), Layout.Anchors.Minimum.Equals(Each.Anchors.Minimum) && Layout.Anchors.Maximum.Equals(Each.Anchors.Maximum));
		TestEqual(FString::Printf(TEXT("%s's position"), Each.Widget), FVector2D(Layout.Offsets.Left, Layout.Offsets.Top), Each.Position);
		TestEqual(FString::Printf(TEXT("%s's alignment"), Each.Widget), FVector2D(Layout.Alignment), Each.Alignment);
		TestEqual(FString::Printf(TEXT("%s's auto size"), Each.Widget), Slot->GetAutoSize(), Each.bAutoSize);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiChapterPortalLoopTest, "Wasami.ChapterPortal.Loop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiChapterPortalLoopTest::RunTest(const FString& Parameters)
{
	using W = UWasamiChapterPortalWidget;

	TestEqual(TEXT("loop is 10 s"), W::LoopLength, 600001.f / 60000.f);
	TestEqual(TEXT("played from 0.8 s"), W::LoopStart, 0.8f);
	TestEqual(TEXT("taken off 11 s after Construct"), W::RemoveDelay, 11.f);

	// The card: unseen at 0.8 s, in by 2 s and held, out by 10 s.
	TestEqual(TEXT("the card clear at 0.8 s"), W::EvaluateLoop(0.8f).CanvasOpacity, 0.f, 1e-4f);
	const float Appearing = W::EvaluateLoop(1.4f).CanvasOpacity;
	TestTrue(TEXT("coming in at 1.4 s"), Appearing > 0.f && Appearing < 1.f);
	TestEqual(TEXT("in at 2 s"), W::EvaluateLoop(2.f).CanvasOpacity, 1.f, 1e-4f);
	TestEqual(TEXT("in at 8.5 s"), W::EvaluateLoop(8.5f).CanvasOpacity, 1.f, 1e-4f);
	const float Leaving = W::EvaluateLoop(9.25f).CanvasOpacity;
	TestTrue(TEXT("going at 9.25 s"), Leaving > 0.f && Leaving < 1.f);
	TestEqual(TEXT("gone at 10 s"), W::EvaluateLoop(10.f).CanvasOpacity, 0.f, 1e-4f);

	// The blur: 10 from 2 s to 8.5 s, 0 from 9.75 s.
	const float Blurring = W::EvaluateLoop(1.f).BlurStrength;
	TestTrue(TEXT("blurring at 1 s"), Blurring > 0.f && Blurring < 10.f);
	TestEqual(TEXT("blur 10 at 2 s"), W::EvaluateLoop(2.f).BlurStrength, 10.f, 1e-3f);
	TestEqual(TEXT("blur 10 at 5 s"), W::EvaluateLoop(5.f).BlurStrength, 10.f, 1e-3f);
	TestEqual(TEXT("blur 10 at 8.5 s"), W::EvaluateLoop(8.5f).BlurStrength, 10.f, 1e-3f);
	TestEqual(TEXT("no blur at 9.75 s"), W::EvaluateLoop(9.75f).BlurStrength, 0.f, 1e-3f);

	// The banners slide left and the portal turns, at a steady rate.
	TestEqual(TEXT("banner 1 at 0 s"), W::EvaluateLoop(0.f).BannerLeft, 501.624755859375f, 1e-2f);
	TestEqual(TEXT("banner 1 at 10 s"), W::EvaluateLoop(10.f).BannerLeft, -1124.6058349609375f, 1e-2f);
	TestEqual(TEXT("banner 1 half way at 5 s"), W::EvaluateLoop(5.f).BannerLeft, (501.624755859375f - 1124.6058349609375f) / 2.f, 1e-2f);
	TestEqual(TEXT("banner 2 at 0 s"), W::EvaluateLoop(0.f).Banner2Left, 1103.771484375f, 1e-2f);
	TestEqual(TEXT("banner 2 at 10 s"), W::EvaluateLoop(10.f).Banner2Left, -1115.896240234375f, 1e-2f);
	TestEqual(TEXT("the ring at 0.8 s"), W::EvaluateLoop(0.8f).RingAngle, 64.f, 1e-2f);
	TestEqual(TEXT("the ring at 5 s"), W::EvaluateLoop(5.f).RingAngle, 400.f, 1e-2f);
	TestEqual(TEXT("the ring at 10 s"), W::EvaluateLoop(10.f).RingAngle, 800.f, 1e-2f);
	TestEqual(TEXT("the runes at 0.8 s"), W::EvaluateLoop(0.8f).RunesAngle, -28.72f, 1e-2f);
	TestEqual(TEXT("the runes at 10 s"), W::EvaluateLoop(10.f).RunesAngle, -359.f, 1e-2f);

	// The title lands at 2.2 s from half as big again, bounces, flashes white at 2.25 s, goes dark at 2.4 s, then red.
	TestTrue(TEXT("no title before 2 s"), W::EvaluateLoop(1.5f).TitleOpacity <= 0.f);
	TestEqual(TEXT("title clear at 2 s"), W::EvaluateLoop(2.f).TitleOpacity, 0.f, 1e-4f);
	TestEqual(TEXT("title in at 2.2 s"), W::EvaluateLoop(2.2f).TitleOpacity, 1.f, 1e-4f);
	TestEqual(TEXT("title still in at 5 s"), W::EvaluateLoop(5.f).TitleOpacity, 1.f, 1e-4f);
	TestEqual(TEXT("title 1.5 times at 2 s"), W::EvaluateLoop(2.f).TitleScale, 1.5f, 1e-4f);
	TestEqual(TEXT("title 1 at 2.2 s"), W::EvaluateLoop(2.2f).TitleScale, 1.f, 1e-4f);
	TestEqual(TEXT("title 1.15 at 2.25 s"), W::EvaluateLoop(2.25f).TitleScale, 1.15f, 1e-4f);
	TestEqual(TEXT("title 1 at 2.35 s"), W::EvaluateLoop(2.35f).TitleScale, 1.f, 1e-4f);
	TestEqual(TEXT("title 1 at 5 s"), W::EvaluateLoop(5.f).TitleScale, 1.f, 1e-4f);
	TestTrue(TEXT("title red at 2 s"), W::EvaluateLoop(2.f).TitleColor.Equals(FLinearColor(1.f, 0.f, 0.f, 1.f), 1e-4f));
	TestTrue(TEXT("still red at 2.24 s"), W::EvaluateLoop(2.24f).TitleColor.Equals(FLinearColor(1.f, 0.f, 0.f, 1.f), 1e-4f));
	TestTrue(TEXT("white at 2.25 s"), W::EvaluateLoop(2.25f).TitleColor.Equals(FLinearColor(1.f, 1.f, 1.f, 1.f), 1e-4f));
	TestTrue(TEXT("dark red at 2.4 s"), W::EvaluateLoop(2.4f).TitleColor.Equals(FLinearColor(0.265625f, 0.f, 0.f, 1.f), 1e-4f));
	TestTrue(TEXT("red at 3 s"), W::EvaluateLoop(3.f).TitleColor.Equals(FLinearColor(1.f, 0.f, 0.f, 1.f), 1e-4f));
	TestTrue(TEXT("red at 10 s"), W::EvaluateLoop(10.f).TitleColor.Equals(FLinearColor(1.f, 0.f, 0.f, 1.f), 1e-4f));

	// The card jolts as the title lands.
	TestTrue(TEXT("still at 2.2 s"), W::EvaluateLoop(2.2f).CanvasShake.Equals(FVector2D::ZeroVector, 1e-3));
	TestTrue(TEXT("(-10, 3) at 2.25 s"), W::EvaluateLoop(2.25f).CanvasShake.Equals(FVector2D(-10.f, 3.f), 1e-3));
	TestTrue(TEXT("(7, -4) at 2.3 s"), W::EvaluateLoop(137999.f / 60000.f).CanvasShake.Equals(FVector2D(7.f, -4.f), 1e-3));
	TestTrue(TEXT("still at 2.35 s"), W::EvaluateLoop(2.35f).CanvasShake.Equals(FVector2D::ZeroVector, 1e-3));
	TestTrue(TEXT("still at 5 s"), W::EvaluateLoop(5.f).CanvasShake.Equals(FVector2D::ZeroVector, 1e-3));

	// Constructed: loop's frame at 0.8 s at once, so nothing shows.
	W* Card = NewObject<W>();
	Card->Initialize();
	Card->TakeWidget();
	UWidgetTree* Tree = Card->WidgetTree;
	UCanvasPanel* Root = Tree ? Cast<UCanvasPanel>(Tree->RootWidget) : nullptr;
	UBackgroundBlur* Blur = Tree ? Cast<UBackgroundBlur>(Tree->FindWidget(TEXT("BackgroundBlur_0"))) : nullptr;
	UImage* Banner = Tree ? Cast<UImage>(Tree->FindWidget(TEXT("Image_48"))) : nullptr;
	UImage* Ring = Tree ? Cast<UImage>(Tree->FindWidget(TEXT("Image_40"))) : nullptr;
	UImage* Title = Tree ? Cast<UImage>(Tree->FindWidget(TEXT("TitleCard"))) : nullptr;
	if (!TestNotNull(TEXT("the root"), Root) || !TestNotNull(TEXT("the blur"), Blur) || !TestNotNull(TEXT("banner 1"), Banner)
		|| !TestNotNull(TEXT("the ring"), Ring) || !TestNotNull(TEXT("the title"), Title))
	{
		return false;
	}
	const UCanvasPanelSlot* BannerSlot = Cast<UCanvasPanelSlot>(Banner->Slot);
	TestEqual(TEXT("at 0.8 s once constructed"), Card->GetLoopTime(), 0.8f, 1e-5f);
	TestEqual(TEXT("the card clear"), Root->GetRenderOpacity(), 0.f, 1e-4f);
	TestEqual(TEXT("the ring's angle at 0.8 s"), Ring->GetRenderTransformAngle(), 64.f, 1e-2f);
	TestEqual(TEXT("banner 1 where it is at 0.8 s"), BannerSlot ? BannerSlot->GetOffsets().Left : 0.f, W::EvaluateLoop(0.8f).BannerLeft, 1e-2f);
	TestEqual(TEXT("banner 1 kept its top"), BannerSlot ? BannerSlot->GetOffsets().Top : 0.f, 15.859466552734375f, 1e-3f);
	TestEqual(TEXT("the title kept its move"), FVector2D(Title->GetRenderTransform().Translation), FVector2D(-72.f, 23.f));

	// Played by its own tick: loop runs out 9.2 s in and its last frame stays (the card faded out); 11 s in it is taken off.
	float Played = 0.f;
	bool bSawIn = false;
	bool bSawEnd = false;
	while (!Card->IsFinished() && Played < 12.f)
	{
		Card->Advance(1.f / 60.f);
		Played += 1.f / 60.f;
		if (!bSawIn && Played >= 1.2f + 0.5f / 60.f)
		{
			bSawIn = true;
			TestEqual(TEXT("in at 2 s into loop"), Root->GetRenderOpacity(), 1.f, 1e-2f);
			TestEqual(TEXT("blurred at 2 s into loop"), Blur->GetBlurStrength(), 10.f, 1e-1f);
			TestEqual(TEXT("the title's scale set"), static_cast<float>(Title->GetRenderTransform().Scale.X), W::EvaluateLoop(Card->GetLoopTime()).TitleScale, 1e-4f);
		}
		if (!bSawEnd && Played >= 10.f)
		{
			bSawEnd = true;
			TestEqual(TEXT("loop held at its end"), Card->GetLoopTime(), W::LoopLength, 1e-5f);
			TestEqual(TEXT("the card faded out"), Root->GetRenderOpacity(), 0.f, 1e-4f);
			TestFalse(TEXT("not yet taken off"), Card->IsFinished());
		}
	}
	TestTrue(TEXT("saw it in"), bSawIn);
	TestTrue(TEXT("saw loop's end"), bSawEnd);
	TestTrue(TEXT("taken off"), Card->IsFinished());
	TestEqual(TEXT("11 s after Construct"), Played, W::RemoveDelay, 1.f / 60.f);
	TestEqual(TEXT("elapsed"), Card->GetElapsed(), W::RemoveDelay, 1e-5f);
	return true;
}

#endif
