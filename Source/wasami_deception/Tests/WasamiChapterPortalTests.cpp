#include "Misc/AutomationTest.h"

#include "../WasamiChapterPortalWidget.h"
#include "../WasamiPauseWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiChapterPortalTreeTest, "Wasami.ChapterPortal.Tree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiChapterPortalTreeTest::RunTest(const FString& Parameters)
{
	using W = UWasamiChapterPortalWidget;

	TestEqual(TEXT("added at Z 0"), W::ViewportZOrder, 0);

	W* Card = NewObject<W>();
	Card->Initialize();
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
	TestTrue(TEXT("the blur starts at 0"), Blur && FMath::IsNearlyZero(Blur->GetBlurStrength()));

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

#endif
