#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WasamiWidgetProbe.generated.h"

class UWidget;

/**
 * Where a widget is on the game's viewport, for the scripts that click the screens (Tools/playthrough.py): Python
 * cannot read an FGeometry (its fields are not reflected, so the copy Python gets is empty).
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiWidgetProbe : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * The point at Fraction of Widget's box as it was last painted (with the render transforms), as fractions of the
	 * viewport's size; (-1, -1) for a widget that has not been painted.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Debug")
	static FVector2D ViewportFraction(UWidget* Widget, FVector2D Fraction);
};
