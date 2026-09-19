#include "WasamiWidgetProbe.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Widget.h"

FVector2D UWasamiWidgetProbe::ViewportFraction(UWidget* Widget, FVector2D Fraction)
{
	const FVector2D None(-1.f, -1.f);
	if (!Widget)
	{
		return None;
	}
	const FGeometry& Geometry = Widget->GetCachedGeometry();
	const FVector2D Size = Geometry.GetLocalSize();
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(Widget);
	if (Size.X <= 0.f || Size.Y <= 0.f || ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f)
	{
		return None;
	}
	FVector2D Pixel;
	FVector2D Viewport;
	USlateBlueprintLibrary::LocalToViewport(Widget, Geometry, Size * Fraction, Pixel, Viewport);
	return Pixel / ViewportSize;
}
