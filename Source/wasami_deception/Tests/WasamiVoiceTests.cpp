#include "Misc/AutomationTest.h"

#include "Sound/SoundClass.h"
#include "Sound/SoundWave.h"

#include "../WasamiVoice.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Every voice, in EWasamiVoice's order. */
	const EWasamiVoice AllVoices[] = {EWasamiVoice::Greeting, EWasamiVoice::Well, EWasamiVoice::Fast, EWasamiVoice::Best,
		EWasamiVoice::Found, EWasamiVoice::Calling, EWasamiVoice::Others, EWasamiVoice::Think, EWasamiVoice::Remember,
		EWasamiVoice::Fine, EWasamiVoice::Over};

	/** The five whose waves carry a subtitle (dd_voices.SUBTITLED). */
	bool IsSubtitled(EWasamiVoice Id)
	{
		return Id == EWasamiVoice::Greeting || Id == EWasamiVoice::Well || Id == EWasamiVoice::Fast
			|| Id == EWasamiVoice::Best || Id == EWasamiVoice::Found;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiVoiceTableTest, "Wasami.Voice.Table",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiVoiceTableTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the voices the WebGL version plays"), WasamiVoice::Num(), static_cast<int32>(UE_ARRAY_COUNT(AllVoices)));

	// The packages the import makes, and manifest.json's lengths.
	TestEqual(TEXT("Greeting's wave"), WasamiVoice::Path(EWasamiVoice::Greeting).ToString(),
		FString(TEXT("/Game/Wasami/Voices/Wasami_Greeting.Wasami_Greeting")));
	TestEqual(TEXT("Over's wave"), WasamiVoice::Path(EWasamiVoice::Over).ToString(),
		FString(TEXT("/Game/Wasami/Voices/Wasami_Over.Wasami_Over")));
	TestEqual(TEXT("Greeting is the long one"), WasamiVoice::Seconds(EWasamiVoice::Greeting), 3.878f, 1e-4f);
	TestEqual(TEXT("Found is half a second"), WasamiVoice::Seconds(EWasamiVoice::Found), 0.622f, 1e-4f);

	// The subtitle's time: max(2.2, the clip + 1.2), and Found's flat 2.4.
	TestEqual(TEXT("Greeting's subtitle lasts the clip and 1.2"), WasamiVoice::SubtitleSeconds(EWasamiVoice::Greeting),
		3.878f + 1.2f, 1e-4f);
	TestEqual(TEXT("a short one is held to 2.2"), WasamiVoice::SubtitleSeconds(EWasamiVoice::Well), 2.2f, 1e-4f);
	TestEqual(TEXT("Found's is 2.4"), WasamiVoice::SubtitleSeconds(EWasamiVoice::Found), 2.4f, 1e-4f);
	for (EWasamiVoice Id : AllVoices)
	{
		TestTrue(TEXT("no subtitle is shorter than its clip"), WasamiVoice::SubtitleSeconds(Id) >= WasamiVoice::Seconds(Id));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiVoiceWavesTest, "Wasami.Voice.Waves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiVoiceWavesTest::RunTest(const FString& Parameters)
{
	// The waves WasamiDDTools.import_wasami_voices makes; run it first where /Game/Wasami is empty.
	for (EWasamiVoice Id : AllVoices)
	{
		const FString Name = WasamiVoice::Path(Id).GetAssetName();
		const USoundWave* Wave = Cast<USoundWave>(WasamiVoice::Load(Id));
		if (!Wave)
		{
			AddError(FString::Printf(TEXT("%s is not imported"), *Name));
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s lasts what the manifest says"), *Name), static_cast<float>(Wave->Duration),
			WasamiVoice::Seconds(Id), 0.01f);
		TestTrue(*FString::Printf(TEXT("%s is a dialogue sound"), *Name),
			Wave->SoundClassObject && Wave->SoundClassObject->GetName() == TEXT("DD_SoundClass_Dialogue"));
		TestEqual(*FString::Printf(TEXT("%s's subtitle"), *Name), Wave->Subtitles.Num(), IsSubtitled(Id) ? 1 : 0);
	}
	return true;
}

#endif
