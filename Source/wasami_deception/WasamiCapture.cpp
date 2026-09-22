#include "WasamiCapture.h"

#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Math/InterpCurve.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiEnemy.h"
#include "WasamiEnemyAnimInstance.h"
#include "WasamiGameInstance.h"
#include "WasamiGameMode.h"
#include "WasamiPlayerCharacter.h"

namespace
{
	// The monkey's feet in 01_Hotel (MonkeyJumpscare's InterpTrackMove_1 at t = 0), its camera (InterpTrackMove_2 at
	// t = 0) and the ceiling light above it.
	const FVector HotelMonkey(4737.9833984375, 1072.7213134765625, 6917.716796875);
	const FVector HotelCamera(4832.646484375, 1075.6009521484375, 7107.3759765625);
	const FVector HotelLight(4777.72900390625, 1072.5234375, 7202.3505859375);

	// Each Matinee's length and its InterpTrackFade's keys (0 → 1, bPersistFade): MonkeyJumpscare, MonkeyJumpscare2,
	// MonkeyJumpscare3.
	struct FMatineeFade
	{
		float Length;
		float Start;
		float End;
	};
	constexpr FMatineeFade MatineeFades[AWasamiCapture::NumHotelChoices] = {
		{2.121222972869873f, 1.7005259990692139f, 1.769968032836914f},
		{2.464282989501953f, 1.920689582824707f, 2.051270008087158f},
		{3.068389654159546f, 2.6005260944366455f, 2.769968032836914f},
	};

	// A Matinee key as saved: its time, value and tangents (the auto-clamped ones already worked out, so every key is
	// evaluated with them as a user curve).
	struct FTrackKey
	{
		float Time;
		FVector Value;
		FVector Arrive;
		FVector Leave;
	};

	// The Matinees' NewCameraGroup (JumpscareCam's InterpTrackMove_2, pak_reference/_levels/01_Hotel.full.json): its
	// PosTrack and its EulerTrack (roll, pitch, yaw), world. MonkeyJumpscare3's keys from 0.204 s to 2.572 s are
	// MonkeyJumpscare's from 0.204 s to 1.522 s, stretched 1.7966 times (their tangents shrunk as much); its own hold
	// there has no keys.
	const FTrackKey Hotel1Position[] = {
		{0.0f, FVector(4832.646484375, 1075.6009521484375, 7107.3759765625), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.20432691276073456f, FVector(4832.646484375, 1075.6009521484375, 7107.3759765625), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.2684294879436493f, FVector(4833.3798828125, 1075.6009521484375, 7133.28369140625), FVector(24.71471405029297, 0., 0.), FVector(24.71471405029297, 0., 0.)},
		{0.35256409645080566f, FVector(4848.55859375, 1075.6009521484375, 7133.28369140625), FVector(121.82028198242188, 0., 0.), FVector(121.82028198242188, 0., 0.)},
		{0.46474358439445496f, FVector(4857.294921875, 1075.6009521484375, 7127.689453125), FVector(0., 0., -34.01874542236328), FVector(0., 0., -34.01874542236328)},
		{0.552884578704834f, FVector(4849.6103515625, 1072.6243896484375, 7125.75439453125), FVector(-94.05863952636719, 0., 0.), FVector(-94.05863952636719, 0., 0.)},
		{0.6169871687889099f, FVector(4842.97509765625, 1075.6009521484375, 7127.689453125), FVector(-71.71471405029297, 49.89038848876953, 58.1912841796875), FVector(-71.71471405029297, 49.89038848876953, 58.1912841796875)},
		{0.7211538553237915f, FVector(4837.54296875, 1081.0194091796875, 7152.61962890625), FVector(-5.968288421630859, 0., 0.), FVector(-5.968288421630859, 0., 0.)},
		{0.8573717474937439f, FVector(4837.22216796875, 1075.6009521484375, 7140.81884765625), FVector(0., 0., -80.04500579833984), FVector(0., 0., -80.04500579833984)},
		{0.9935896992683411f, FVector(4839.69482421875, 1078.3787841796875, 7130.8125), FVector(38.93999481201172, 0., -94.73448944091797), FVector(38.93999481201172, 0., -94.73448944091797)},
		{1.129807710647583f, FVector(4858.06201171875, 1073.761962890625, 7115.009765625), FVector(0., 0., -20.342525482177734), FVector(0., 0., -20.342525482177734)},
		{1.2259615659713745f, FVector(4849.041015625, 1074.1827392578125, 7114.06640625), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{1.3221153020858765f, FVector(4849.041015625, 1074.1827392578125, 7114.06640625), FVector(-1.9708106517791748, -0.3404487669467926, 0.), FVector(-1.9708106517791748, -0.3404487669467926, 0.)},
		{1.4342948198318481f, FVector(4844.32373046875, 1082.92333984375, 7112.79248046875), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{1.5224359035491943f, FVector(4844.50830078125, 1068.57275390625, 7113.30322265625), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{1.6625638008117676f, FVector(4648.45361328125, 1070.0736083984375, 7080.06494140625), FVector(-1078.701171875, 17.355913162231445, 0.), FVector(-1078.701171875, 17.355913162231445, 0.)},
		{1.7387819290161133f, FVector(4587.26416015625, 1072.32763671875, 7085.09912109375), FVector(0., 0., 0.), FVector(0., 0., 0.)},
	};

	const FTrackKey Hotel1Euler[] = {
		{0.0f, FVector(0., 0., -179.9999542236328), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.20432691276073456f, FVector(0., 0., -179.9999542236328), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.2684294879436493f, FVector(2.0406732673030475e-13, -4.05889892578125, -179.9999542236328), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.35256409645080566f, FVector(-5.014440536499023, -4.043396472930908, -179.64544677734375), FVector(0., 0.24050351977348328, 0.), FVector(0., 0.24050351977348328, 0.)},
		{0.46474358439445496f, FVector(8.773558616638184, -4.011479377746582, -180.61866760253906), FVector(54.69683837890625, 0., 0.), FVector(54.69683837890625, 0., 0.)},
		{0.552884578704834f, FVector(11.39043140411377, -4.015960693359375, -179.9176025390625), FVector(0., -0.09148796647787094, 7.458028793334961), FVector(0., -0.09148796647787094, 7.458028793334961)},
		{0.6169871687889099f, FVector(-7.321878910064697, -4.025808811187744, -179.4832305908203), FVector(-105.8553466796875, -0.32909393310546875, 10.753969192504883), FVector(-105.8553466796875, -0.32909393310546875, 10.753969192504883)},
		{0.7211538553237915f, FVector(-12.26275634765625, -19.439300537109375, -177.57565307617188), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.8573717474937439f, FVector(15.319572448730469, -13.61390495300293, -181.00454711914062), FVector(0., 0.0017528533935546875, 0.), FVector(0., 0.0017528533935546875, 0.)},
		{0.9935896992683411f, FVector(-12.296598434448242, -13.613809585571289, -181.00453186035156), FVector(0., 0.001750946044921875, 0.), FVector(0., 0.001750946044921875, 0.)},
		{1.129807710647583f, FVector(4.826965808868408, -5.962024211883545, -182.67547607421875), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{1.2259615659713745f, FVector(-9.967408180236816, -5.962024211883545, -182.67547607421875), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{1.3221153020858765f, FVector(1.3622221946716309, -5.962010383605957, -182.67547607421875), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{1.4342948198318481f, FVector(-5.5430908203125, -5.412689208984375, -176.66770935058594), FVector(0., 5.772935390472412, 0.), FVector(0., 5.772935390472412, 0.)},
		{1.5224359035491943f, FVector(5.723729610443115, -4.805572509765625, -183.06292724609375), FVector(8.392333984375e-05, 0., 0.), FVector(8.392333984375e-05, 0., 0.)},
		{1.6625638008117676f, FVector(5.723733901977539, -4.805572509765625, -183.06292724609375), FVector(9.021162986755371e-05, 0., 0.), FVector(9.021162986755371e-05, 0., 0.)},
		{1.7387819290161133f, FVector(5.784770965576172, 9.593901634216309, -181.61453247070312), FVector(0., 0., 0.), FVector(0., 0., 0.)},
	};

	const FTrackKey Hotel2Position[] = {
		{0.0f, FVector(4832.646484375, 1075.6009521484375, 7107.3759765625), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.20432700216770172f, FVector(4832.646484375, 1075.6009521484375, 7107.3759765625), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.2684290111064911f, FVector(4833.3798828125, 1075.6009521484375, 7133.28369140625), FVector(24.714820861816406, 0., 0.), FVector(24.714820861816406, 0., 0.)},
		{0.3525640070438385f, FVector(4848.55859375, 1075.6009521484375, 7133.28369140625), FVector(121.81973266601562, 0., 0.), FVector(121.81973266601562, 0., 0.)},
		{0.4647440016269684f, FVector(4857.294921875, 1075.6009521484375, 7127.689453125), FVector(195.52395629882812, 0., 0.), FVector(195.52395629882812, 0., 0.)},
		{0.5031899213790894f, FVector(4884.7021484375, 1074.186767578125, 7128.56982421875), FVector(0., -33.77046585083008, 0.), FVector(0., -33.77046585083008, 0.)},
		{0.5528849959373474f, FVector(4849.6103515625, 1072.6243896484375, 7125.75439453125), FVector(-229.1803436279297, 0., 0.), FVector(-229.1803436279297, 0., 0.)},
		{0.6169869899749756f, FVector(4842.97509765625, 1075.6009521484375, 7127.689453125), FVector(-71.71483612060547, 49.89047622680664, 58.191566467285156), FVector(-71.71483612060547, 49.89047622680664, 58.191566467285156)},
		{0.721153974533081f, FVector(4837.54296875, 1081.0194091796875, 7152.61962890625), FVector(-5.968278884887695, 0., 0.), FVector(-5.968278884887695, 0., 0.)},
		{0.857371985912323f, FVector(4837.22216796875, 1075.6009521484375, 7140.81884765625), FVector(0., 0., -186.09835815429688), FVector(0., 0., -186.09835815429688)},
		{0.9542563557624817f, FVector(4842.79443359375, 1075.6009521484375, 7094.20947265625), FVector(108.62866973876953, 0., -870.5941162109375), FVector(108.62866973876953, 0., -870.5941162109375)},
		{1.0455955266952515f, FVector(4862.69091796875, 1075.6009521484375, 6950.66552734375), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{1.151400089263916f, FVector(4862.69091796875, 1075.6009521484375, 6982.4951171875), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{1.2561516761779785f, FVector(4862.69091796875, 1075.6009521484375, 6959.65234375), FVector(0., 0., -60.932315826416016), FVector(0., 0., -60.932315826416016)},
		{1.4493842124938965f, FVector(4862.69091796875, 1075.6009521484375, 6954.787109375), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{2.044156551361084f, FVector(4862.69091796875, 1075.6009521484375, 6954.787109375), FVector(0., 0., 0.), FVector(0., 0., 0.)},
	};

	const FTrackKey Hotel2Euler[] = {
		{0.0f, FVector(0., 0., -179.9999542236328), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.20432700216770172f, FVector(0., 0., -179.9999542236328), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.2684290111064911f, FVector(0., -4.05889892578125, -179.9999542236328), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.3525640070438385f, FVector(-5.014441013336182, -4.04339599609375, -179.64544677734375), FVector(0., 0.24050608277320862, 0.), FVector(0., 0.24050608277320862, 0.)},
		{0.4647440016269684f, FVector(8.773558616638184, -4.011478900909424, -180.61866760253906), FVector(71.42028045654297, 0., -16.18885612487793), FVector(71.42028045654297, 0., -16.18885612487793)},
		{0.5031899213790894f, FVector(10.74866008758545, -12.872051239013672, -182.0839080810547), FVector(25.26874542236328, 0., 0.), FVector(25.26874542236328, 0., 0.)},
		{0.5528849959373474f, FVector(11.39043140411377, -4.015961170196533, -179.9176025390625), FVector(0., 0., 14.840182304382324), FVector(0., 0., 14.840182304382324)},
		{0.6169869899749756f, FVector(-7.321878910064697, -4.025808811187744, -179.4832305908203), FVector(-105.8554458618164, -0.329071044921875, 10.754011154174805), FVector(-105.8554458618164, -0.329071044921875, 10.754011154174805)},
		{0.721153974533081f, FVector(-12.26275634765625, -19.439300537109375, -177.57565307617188), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.857371985912323f, FVector(15.319572448730469, -13.61390495300293, -181.00454711914062), FVector(115.03346252441406, 102.2680435180664, 0.), FVector(115.03346252441406, 102.2680435180664, 0.)},
		{0.9542563557624817f, FVector(22.03186798095703, 28.68880844116211, -175.3293914794922), FVector(44.20232391357422, 346.90765380859375, 106.34553527832031), FVector(44.20232391357422, 346.90765380859375, 106.34553527832031)},
		{1.0455955266952515f, FVector(24.46732521057129, 51.682281494140625, -157.66635131835938), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{1.151400089263916f, FVector(13.064330101013184, 38.19944763183594, -167.06153869628906), FVector(-80.71808624267578, 0., -70.59036254882812), FVector(-80.71808624267578, 0., -70.59036254882812)},
		{1.2561516761779785f, FVector(7.431351184844971, 41.6552619934082, -172.5295867919922), FVector(0., 29.13799285888672, 0.), FVector(0., 29.13799285888672, 0.)},
		{1.4493842124938965f, FVector(25.153860092163086, 46.88210678100586, -167.90184020996094), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{2.044156551361084f, FVector(9.663382530212402, 38.5451545715332, -170.68177795410156), FVector(0., 0., 0.), FVector(0., 0., 0.)},
	};

	const FTrackKey Hotel3Position[] = {
		{0.0f, FVector(4832.646484375, 1075.6009521484375, 7107.3759765625), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.20432700216770172f, FVector(4832.646484375, 1075.6009521484375, 7107.3759765625), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.319493267439243f, FVector(4833.3798828125, 1075.6009521484375, 7133.28369140625), FVector(13.756412106705753, 0., 0.), FVector(13.756412106705753, 0., 0.)},
		{0.470649162948807f, FVector(4848.55859375, 1075.6009521484375, 7133.28369140625), FVector(67.80616593399073, 0., 0.), FVector(67.80616593399073, 0., 0.)},
		{0.672190374809136f, FVector(4857.294921875, 1075.6009521484375, 7127.689453125), FVector(0., 0., -18.935112113004266), FVector(0., 0., -18.935112113004266)},
		{0.8305441267607541f, FVector(4849.6103515625, 1072.6243896484375, 7125.75439453125), FVector(-52.353808540441186, 0., 0.), FVector(-52.353808540441186, 0., 0.)},
		{0.9457105794318545f, FVector(4842.97509765625, 1075.6009521484375, 7127.689453125), FVector(-39.916996756784116, 27.769398537994693, 32.38974501445704), FVector(-39.916996756784116, 27.769398537994693, 32.38974501445704)},
		{1.1328560248653445f, FVector(4837.54296875, 1081.0194091796875, 7152.61962890625), FVector(-3.3219981802161, 0., 0.), FVector(-3.3219981802161, 0., 0.)},
		{1.3775845360061911f, FVector(4837.22216796875, 1075.6009521484375, 7140.81884765625), FVector(0., 0., -44.55370531922304), FVector(0., 0., -44.55370531922304)},
		{1.6223131542325f, FVector(4839.69482421875, 1078.3787841796875, 7130.8125), FVector(21.67431979900521, 0., -52.72999213406574), FVector(21.67431979900521, 0., -52.72999213406574)},
		{1.8670418795442714f, FVector(4858.06201171875, 1073.761962890625, 7115.009765625), FVector(0., 0., -11.32281616750823), FVector(0., 0., -11.32281616750823)},
		{2.039791505008191f, FVector(4849.041015625, 1074.1827392578125, 7114.06640625), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{2.2125409163011858f, FVector(4849.041015625, 1074.1827392578125, 7114.06640625), FVector(-1.0969693379818133, -0.18949656993034827, 0.), FVector(-1.0969693379818133, -0.18949656993034827, 0.)},
		{2.414082181704246f, FVector(4844.32373046875, 1082.92333984375, 7112.79248046875), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{2.5724360942840576f, FVector(4844.50830078125, 1068.57275390625, 7113.30322265625), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{2.762563943862915f, FVector(4648.45361328125, 1070.0736083984375, 7080.06494140625), FVector(-600.7835083007812, 10.249563217163086, 0.), FVector(-600.7835083007812, 10.249563217163086, 0.)},
		{2.93878173828125f, FVector(4587.26416015625, 1072.32763671875, 7085.09912109375), FVector(0., 0., 0.), FVector(0., 0., 0.)},
	};

	const FTrackKey Hotel3Euler[] = {
		{0.0f, FVector(0., 0., -179.9999542236328), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.20432700216770172f, FVector(0., 0., -179.9999542236328), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.319493267439243f, FVector(2.0406732673030475e-13, -4.05889892578125, -179.9999542236328), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.470649162948807f, FVector(-5.014440536499023, -4.043396472930908, -179.64544677734375), FVector(0., 0.13386622739736181, 0.), FVector(0., 0.13386622739736181, 0.)},
		{0.672190374809136f, FVector(8.773558616638184, -4.011479377746582, -180.61866760253906), FVector(30.444707883043236, 0., 0.), FVector(30.444707883043236, 0., 0.)},
		{0.8305441267607541f, FVector(11.39043140411377, -4.015960693359375, -179.9176025390625), FVector(0., -0.050922950883146266, 4.151199863207682), FVector(0., -0.050922950883146266, 4.151199863207682)},
		{0.9457105794318545f, FVector(-7.321878910064697, -4.025808811187744, -179.4832305908203), FVector(-58.91995228675227, -0.18317637648580515, 5.985747263507636), FVector(-58.91995228675227, -0.18317637648580515, 5.985747263507636)},
		{1.1328560248653445f, FVector(-12.26275634765625, -19.439300537109375, -177.57565307617188), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{1.3775845360061911f, FVector(15.319572448730469, -13.61390495300293, -181.00454711914062), FVector(0., 0.000975652544282224, 0.), FVector(0., 0.000975652544282224, 0.)},
		{1.6223131542325f, FVector(-12.296598434448242, -13.613809585571289, -181.00453186035156), FVector(0., 0.0009745908984233751, 0.), FVector(0., 0.0009745908984233751, 0.)},
		{1.8670418795442714f, FVector(4.826965808868408, -5.962024211883545, -182.67547607421875), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{2.039791505008191f, FVector(-9.967408180236816, -5.962024211883545, -182.67547607421875), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{2.2125409163011858f, FVector(1.3622221946716309, -5.962010383605957, -182.67547607421875), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{2.414082181704246f, FVector(-5.5430908203125, -5.412689208984375, -176.66770935058594), FVector(0., 3.2132630842954626, 0.), FVector(0., 3.2132630842954626, 0.)},
		{2.5724360942840576f, FVector(5.723730087280273, -4.805572986602783, -183.06292724609375), FVector(2.4318695068359375e-05, 0., 0.), FVector(2.4318695068359375e-05, 0., 0.)},
		{2.762563943862915f, FVector(5.723733901977539, -4.805572986602783, -183.06292724609375), FVector(5.1334500312805176e-05, 0., 0.), FVector(5.1334500312805176e-05, 0., 0.)},
		{2.93878173828125f, FVector(5.784770965576172, 9.593901634216309, -181.61453247070312), FVector(0., 0., 0.), FVector(0., 0., 0.)},
	};

	// 03_Watcher_Kill3 (pak_reference/_camera/_camera_anims.json): its camera's PosTrack and EulerTrack, in its own
	// frame (it starts at rest, looking along +X).
	const FTrackKey WatcherKill3Position[] = {
		{0.0f, FVector(-1020.0, 149.0, 1007.0), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.03927004337310791f, FVector(-1013.50537109375, 149.1998291015625, 1008.8984375), FVector(212.95999145507812, 6.552473545074463, 62.25003433227539), FVector(212.95999145507812, 6.552473545074463, 62.25003433227539)},
		{0.20065224170684814f, FVector(-955.0, 151.0, 1026.0), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.29846394062042236f, FVector(-955.0, 151.0, 1026.0), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.4280608892440796f, FVector(-970.8790283203125, 153.55076599121094, 1029.9088134765625), FVector(0., 0., 55.4953727722168), FVector(0., 0., 55.4953727722168)},
		{0.48779428005218506f, FVector(-962.5465698242188, 153.36053466796875, 1036.5069580078125), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.5355404615402222f, FVector(-962.5465698242188, 153.36053466796875, 1036.5069580078125), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.5832867622375488f, FVector(-962.5465698242188, 153.36053466796875, 1036.5069580078125), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.6105703115463257f, FVector(-953.7954711914062, 150.56942749023438, 1034.1336669921875), FVector(27.050140380859375, 0., 0.), FVector(27.050140380859375, 0., 0.)},
		{0.6787792444229126f, FVector(-953.1688232421875, 151.21319580078125, 1036.181396484375), FVector(16.470745086669922, 0., 49.57331848144531), FVector(16.470745086669922, 0., 49.57331848144531)},
		{0.7401672601699829f, FVector(-951.4065551757812, 147.08853149414062, 1040.7510986328125), FVector(0., 0., 4.757499694824219), FVector(0., 0., 4.757499694824219)},
		{0.7913239002227783f, FVector(-951.4207153320312, 151.08712768554688, 1040.8570556640625), FVector(0., 0., 3.945748805999756), FVector(0., 0., 3.945748805999756)},
		{0.8390700817108154f, FVector(-944.7265625, 150.88658142089844, 1041.241943359375), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.8765851259231567f, FVector(-944.7265625, 150.88658142089844, 1041.241943359375), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.920920729637146f, FVector(-945.57568359375, 150.9046173095703, 1039.43115234375), FVector(0., 0., -49.246734619140625), FVector(0., 0., -49.246734619140625)},
		{0.9823087453842163f, FVector(-938.3321533203125, 150.8765869140625, 1036.035400390625), FVector(0., 0., 0.), FVector(0., 0., 0.)},
	};

	const FTrackKey WatcherKill3Euler[] = {
		{0.0f, FVector(0., 0., 0.), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.03927004337310791f, FVector(0.6057783365249634, -9.531508445739746, -0.10035117715597153), FVector(19.79886245727539, 0., 0.), FVector(19.79886245727539, 0., 0.)},
		{0.20065224170684814f, FVector(5.979196548461914, 0., 0.), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.29846394062042236f, FVector(-6.402787685394287, 0., 0.), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.4280608892440796f, FVector(1.2880061864852905, -10.433440208435059, -2.544062614440918), FVector(0., -86.87409973144531, -4.216819763183594), FVector(0., -86.87409973144531, -4.216819763183594)},
		{0.48779428005218506f, FVector(-8.68997573852539, -16.44790267944336, -2.684776544570923), FVector(0., -3.3130340576171875, 0.), FVector(0., -3.3130340576171875, 0.)},
		{0.5355404615402222f, FVector(2.7386951446533203, -16.51667594909668, -1.2852767705917358), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.5832867622375488f, FVector(-6.4652228355407715, -16.281715393066406, -4.270352363586426), FVector(0., 8.10506534576416, 0.), FVector(0., 8.10506534576416, 0.)},
		{0.6105703115463257f, FVector(4.467431545257568, -15.908554077148438, 1.3834638595581055), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.6787792444229126f, FVector(-9.280336380004883, -16.452972412109375, -1.9189913272857666), FVector(0., -16.3199520111084, 0.), FVector(0., -16.3199520111084, 0.)},
		{0.7401672601699829f, FVector(-4.034805774688721, -18.959640502929688, 8.54151439666748), FVector(77.70449829101562, -60.59701156616211, 0.), FVector(77.70449829101562, -60.59701156616211, 0.)},
		{0.7913239002227783f, FVector(-0.5351104736328125, -23.272842407226562, -1.4765031337738037), FVector(0., -107.37998962402344, 0.), FVector(0., -107.37998962402344, 0.)},
		{0.8390700817108154f, FVector(-4.741378307342529, -29.579824447631836, -1.4089213609695435), FVector(0., 0., 3.5737123489379883), FVector(0., 0., 3.5737123489379883)},
		{0.8765851259231567f, FVector(-0.44045716524124146, -29.577259063720703, -0.18275341391563416), FVector(1.0235595703125, 0.162384033203125, 0.), FVector(1.0235595703125, 0.162384033203125, 0.)},
		{0.920920729637146f, FVector(-0.42310044169425964, -25.1270694732666, -0.22053417563438416), FVector(0., 0., 0.), FVector(0., 0., 0.)},
		{0.9823087453842163f, FVector(-0.42310044169425964, -25.1270694732666, -0.22053414583206177), FVector(0., 0., 0.), FVector(0., 0., 0.)},
	};

	struct FCameraCurves
	{
		FInterpCurveVector Location;
		FInterpCurveVector Euler;
	};

	template <int32 N>
	FInterpCurveVector MakeCurve(const FTrackKey (&Keys)[N])
	{
		FInterpCurveVector Curve;
		for (const FTrackKey& Key : Keys)
		{
			Curve.Points.Emplace(Key.Time, Key.Value, Key.Arrive, Key.Leave, CIM_CurveUser);
		}
		return Curve;
	}

	const FCameraCurves& HotelCurves(int32 Choice)
	{
		static const FCameraCurves Curves[AWasamiCapture::NumHotelChoices] = {
			{MakeCurve(Hotel1Position), MakeCurve(Hotel1Euler)},
			{MakeCurve(Hotel2Position), MakeCurve(Hotel2Euler)},
			{MakeCurve(Hotel3Position), MakeCurve(Hotel3Euler)},
		};
		return Curves[FMath::Clamp(Choice, 0, AWasamiCapture::NumHotelChoices - 1)];
	}

	const FCameraCurves& WatcherCurves()
	{
		static const FCameraCurves Curves = {MakeCurve(WatcherKill3Position), MakeCurve(WatcherKill3Euler)};
		return Curves;
	}

	// A Matinee Euler key (roll, pitch, yaw) as a rotator.
	FRotator EulerRotator(const FVector& Euler)
	{
		return FRotator::MakeFromEuler(Euler);
	}

	// The room: a 20 m cube of black planes around the mark, its floor at the Wasami's feet, reaching 15 m behind it
	// (Capture_2 starts some 7.75 m back).
	const FVector RoomCentre(-500., 0., 1000.);
	constexpr double RoomHalfSize = 1000.;
	// Engine's Plane is 100 cm across.
	constexpr double PlaneSize = 100.;
	const FVector WallNormals[] = {
		FVector(0., 0., 1.), FVector(0., 0., -1.), FVector(1., 0., 0.), FVector(-1., 0., 0.), FVector(0., 1., 0.),
		FVector(0., -1., 0.),
	};

	// The enemy's mesh as the room's Wasami wears it: facing +X, grown as the enemy's.
	FTransform BodyPlacement()
	{
		return FTransform(FRotator(0., AWasamiEnemy::MeshYaw, 0.), FVector::ZeroVector, FVector(AWasamiEnemy::MeshScale));
	}

	const FName HeadBone(TEXT("head"));

	// A bone where the mesh's bind pose puts it, in the mesh's own frame.
	FTransform RefBoneTransform(const USkeletalMesh* Mesh, const FName& Bone)
	{
		FTransform Pose = FTransform::Identity;
		if (!Mesh)
		{
			return Pose;
		}
		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		const TArray<FTransform>& Bones = Ref.GetRefBonePose();
		for (int32 Index = Ref.FindBoneIndex(Bone); Index != INDEX_NONE; Index = Ref.GetParentIndex(Index))
		{
			Pose = Pose * Bones[Index];
		}
		return Pose;
	}
}

const FVector AWasamiCapture::RoomLocation(0., 0., 50000.);
const double AWasamiCapture::MonkeyTop = 65.78780364990234 * 4.;
const double AWasamiCapture::WasamiTop = 170. * AWasamiEnemy::MeshScale;
const double AWasamiCapture::SceneScale = AWasamiCapture::WasamiTop / AWasamiCapture::MonkeyTop;
const FVector AWasamiCapture::HotelCameraOffset = HotelCamera - HotelMonkey;
const FVector AWasamiCapture::HotelLightOffset = HotelLight - HotelMonkey;
const double AWasamiCapture::MonkeyHeadBase = 34.261745931581764 * 4.;
const double AWasamiCapture::MonkeyHeadTop = 58.8946292245342 * 4.;
const double AWasamiCapture::FrameScale = AWasamiCapture::WasamiTop / 2. / (AWasamiCapture::MonkeyHeadTop - AWasamiCapture::MonkeyHeadBase);
const FVector AWasamiCapture::CameraOffset(
	AWasamiCapture::HotelCameraOffset.X * AWasamiCapture::FrameScale,
	AWasamiCapture::HotelCameraOffset.Y * AWasamiCapture::FrameScale,
	AWasamiCapture::WasamiTop / 2. + (AWasamiCapture::HotelCameraOffset.Z - AWasamiCapture::MonkeyHeadBase) * AWasamiCapture::FrameScale);
const FColor AWasamiCapture::LightColor(255, 236, 142, 255);
const double AWasamiCapture::WasamiHeadBase = 144.2648884628 * AWasamiEnemy::MeshScale;
const double AWasamiCapture::WasamiHeadTop = AWasamiEnemy::WasamiHeadTop * AWasamiEnemy::MeshScale;
const double AWasamiCapture::FaceScale =
	(AWasamiCapture::WasamiHeadTop - AWasamiCapture::WasamiHeadBase) / (AWasamiCapture::MonkeyHeadTop - AWasamiCapture::MonkeyHeadBase);
const FVector AWasamiCapture::FaceLightPlaces[AWasamiCapture::NumFaceLights] = {
	FVector(2.752, 11.505, 157.419),
	FVector(-2.589, 11.505, 157.576),
};
const float AWasamiCapture::WatcherKill3Length = 1.0187135934829712f;

AWasamiCapture::AWasamiCapture()
{
	// Ticks from Start: the clip, the Wasami's rush and the camera each frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetRelativeTransform(BodyPlacement());
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// JumpscareCam, where the Matinee's camera is at t = 0, looking back at the Wasami.
	CameraStart = CameraOffset;
	ClipStarts = {0.f, 0.45f, 1.2f};
	View = CreateDefaultSubobject<UCameraComponent>(TEXT("View"));
	View->SetupAttachment(Root);
	View->SetRelativeLocationAndRotation(CameraOffset, FRotator(0., 180., 0.));
	View->SetFieldOfView(FieldOfView);

	// ceilinglights_80 (a stationary light in the hotel; this room is spawned, so it is movable).
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetRelativeLocation(HotelLightOffset * SceneScale);
	Light->SetIntensityUnits(ELightUnits::Unitless);
	Light->SetIntensity(LightIntensity * SceneScale * SceneScale);
	Light->SetAttenuationRadius(LightRadius * SceneScale);
	Light->SetSourceRadius(LightSourceRadius * SceneScale);
	Light->SetLightFColor(LightColor);

	// jumpscarelight and _5 on the eyes, riding the head bone as the monkey's ride its Head_Top socket. Their places
	// come with the mesh (OnConstruction); their reach goes by the head's size and their strength by the square of it,
	// and their own scale undoes the mesh's so they stand at world scale, as the original's 0.25 does against its 4.
	for (int32 Index = 0; Index < NumFaceLights; ++Index)
	{
		UPointLightComponent* Eye = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("FaceLight%d"), Index));
		Eye->SetupAttachment(Body, HeadBone);
		Eye->SetMobility(EComponentMobility::Movable);
		Eye->SetRelativeScale3D(FVector(1. / AWasamiEnemy::MeshScale));
		Eye->SetIntensityUnits(ELightUnits::Unitless);
		Eye->SetIntensity(FaceLightIntensity * FaceScale * FaceScale);
		Eye->SetAttenuationRadius(FaceLightRadius * FaceScale);
		Eye->SetCastShadows(false);
		Eye->SetLightFColor(LightColor);
		FaceLights.Add(Eye);
	}

	// jumpscareblock and its fellows: black unlit planes without shadows, here facing in on every side.
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(WallNormals); ++Index)
	{
		UStaticMeshComponent* Wall = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Wall%d"), Index));
		Wall->SetupAttachment(Root);
		const FVector& Normal = WallNormals[Index];
		Wall->SetRelativeLocationAndRotation(RoomCentre - Normal * RoomHalfSize, FRotationMatrix::MakeFromZ(Normal).Rotator());
		Wall->SetRelativeScale3D(FVector(2. * RoomHalfSize / PlaneSize, 2. * RoomHalfSize / PlaneSize, 1.));
		Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Wall->SetCastShadow(false);
		Walls.Add(Wall);
	}

	ShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/Main/JumpscareShake")));
	BodyMesh = TSoftObjectPtr<USkeletalMesh>(WasamiAssets::Path(TEXT("/Game/Wasami/Enemy/SK_WasamiEnemy")));
	WallMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Engine/BasicShapes/Plane")));
	WallMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/Wasami/Enemy/M_WasamiCaptureBlack")));
	ScreamSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/01_Hotel/Evil_Monkey_Scream")));
	LaughSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/03_Manor/LIVING_STATUE_Laughter_05")));
	HitSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/03_Manor/Axe_Hit_03")));
}

void AWasamiCapture::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Body->SetSkeletalMeshAsset(BodyMesh.LoadSynchronous());
	PlaceFaceLights();
	UStaticMesh* Plane = WallMesh.LoadSynchronous();
	UMaterialInterface* Black = WallMaterial.LoadSynchronous();
	for (UStaticMeshComponent* Wall : Walls)
	{
		Wall->SetStaticMesh(Plane);
		Wall->SetMaterial(0, Black);
	}
}

int32 AWasamiCapture::MatineeFor(int32 InChoice)
{
	// Capture_1 (the back flip) with MonkeyJumpscare, Capture_2 (the slide, turned away from the camera once up) with
	// MonkeyJumpscare3 (the camera stands), Capture_3 (the walk at the camera) with MonkeyJumpscare2 (the camera falls
	// to the floor and looks up).
	constexpr int32 Matinees[NumHotelChoices] = {0, 2, 1};
	return Matinees[FMath::Clamp(InChoice, 0, NumHotelChoices - 1)];
}

float AWasamiCapture::FadeStartShare(int32 InChoice)
{
	const FMatineeFade& Fade = MatineeFades[MatineeFor(InChoice)];
	return Fade.Start / Fade.Length;
}

float AWasamiCapture::FadeEndShare(int32 InChoice)
{
	const FMatineeFade& Fade = MatineeFades[MatineeFor(InChoice)];
	return Fade.End / Fade.Length;
}

float AWasamiCapture::MatineeLength(int32 InChoice)
{
	return MatineeFades[MatineeFor(InChoice)].Length;
}

void AWasamiCapture::EvaluateHotelCamera(int32 InChoice, float MatineeTime, FVector& OutLocation, FRotator& OutRotation)
{
	const FCameraCurves& Curves = HotelCurves(MatineeFor(InChoice));
	OutLocation = Curves.Location.Eval(MatineeTime, FVector::ZeroVector);
	OutRotation = EulerRotator(Curves.Euler.Eval(MatineeTime, FVector::ZeroVector));
}

void AWasamiCapture::EvaluateWatcherCamera(float Time, FVector& OutMove, FRotator& OutTurn)
{
	const FCameraCurves& Curves = WatcherCurves();
	const float At = FMath::Clamp(Time, 0.f, WatcherKill3Length);
	OutMove = Curves.Location.Eval(At, FVector::ZeroVector) - Curves.Location.Eval(0.f, FVector::ZeroVector);
	OutTurn = EulerRotator(Curves.Euler.Eval(At, FVector::ZeroVector));
}

float AWasamiCapture::SceneTime(float Time) const
{
	if (RateEaseTime <= 0.f)
	{
		return Time;
	}
	return Time + (StartRate - 1.f) * RateEaseTime * (1.f - FMath::Exp(-Time / RateEaseTime));
}

float AWasamiCapture::RealTime(float Scene) const
{
	// The scene's time rises at a rate between StartRate and 1: halve the bracket until it is tight.
	float Low = 0.f;
	float High = FMath::Max(Scene, 0.f) / FMath::Max(FMath::Min(StartRate, 1.f), KINDA_SMALL_NUMBER);
	for (int32 Step = 0; Step < 40; ++Step)
	{
		const float Mid = 0.5f * (Low + High);
		(SceneTime(Mid) < Scene ? Low : High) = Mid;
	}
	return 0.5f * (Low + High);
}

AWasamiCapture* AWasamiCapture::StartCapture(const UObject* WorldContextObject, AActor* Cause, int32 InChoice)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	AWasamiGameMode* GameMode = World ? World->GetAuthGameMode<AWasamiGameMode>() : nullptr;
	if (!GameMode || !GameMode->IsDeathOpen() || TActorIterator<AWasamiCapture>(World))
	{
		return nullptr;
	}
	if (InChoice < 0 || InChoice >= NumChoices)
	{
		UWasamiGameInstance* Instance = GameMode->GetWasamiGameInstance();
		InChoice = Instance ? Instance->TakeCaptureChoice() : FMath::RandRange(0, NumChoices - 1);
	}
	AWasamiCapture* Room = World->SpawnActor<AWasamiCapture>(StaticClass(), FTransform(RoomLocation));
	if (Room)
	{
		Room->Start(GameMode, Cause, InChoice);
	}
	return Room;
}

int32 AWasamiCapture::NumSounds(int32 InChoice)
{
	return InChoice == FaceChoice ? 2 : 1;
}

float AWasamiCapture::SoundTime(int32 InChoice, int32 Index)
{
	if (InChoice != FaceChoice)
	{
		// Every hotel Matinee opens with its scream (MonkeyJumpscare, 2 and 3's InterpTrackSound_0 at t = 0).
		return 0.f;
	}
	return Index == 0 ? WatcherAnimDelay : WatcherAnimDelay + WatcherHitDelay;
}

USoundBase* AWasamiCapture::GetSound(int32 InChoice, int32 Index) const
{
	if (InChoice == FaceChoice)
	{
		return Index == 0 ? LaughSound.LoadSynchronous() : Index == 1 ? HitSound.LoadSynchronous() : nullptr;
	}
	return Index == 0 ? ScreamSound.LoadSynchronous() : nullptr;
}

float AWasamiCapture::GetSoundDelay(int32 Index) const
{
	return SoundTimers.IsValidIndex(Index) ? FMath::Max(GetWorldTimerManager().GetTimerRemaining(SoundTimers[Index]), 0.f) : 0.f;
}

void AWasamiCapture::PlayCaptureSound(int32 Index)
{
	if (USoundBase* Sound = GetSound(Choice, Index))
	{
		// The Matinee key's and the watcher's own multipliers, both 1. A UI sound, as PlaySound2D makes it: the death
		// screen pauses the game while the axe's hit is still going.
		UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f);
	}
}

void AWasamiCapture::Start(AWasamiGameMode* InMode, AActor* Cause, int32 InChoice)
{
	Mode = InMode;
	CauseActor = Cause;
	Choice = FMath::Clamp(InChoice, 0, NumChoices - 1);
	const bool bFace = Choice == FaceChoice;

	// The clip: the hotel's Capture_1 to Capture_3 once, the face's Run over and over.
	Clip = Cast<UAnimSequence>(WasamiEnemyAnim::ClipPath(bFace ? int32(WasamiEnemyClip::Run) : WasamiEnemyClip::Capture1 + Choice).TryLoad());
	ClipLength = Clip ? Clip->GetPlayLength() : 0.f;
	if (bFace)
	{
		// The Gold Watcher's kill: black at once, and DeathEvent with it.
		FadeStart = FaceDeathDelay;
		FadeDuration = 0.f;
		ClipStart = 0.f;
		MatineeRate = 0.f;
		BodyStart = FVector(-FaceRushDistance, 0., 0.);
		View->SetFieldOfView(WatcherFieldOfView);
		// 03_Watcher_Kill3 is a camera anim on the player's own camera, with no depth of field of its own.
		View->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = false;
		View->PostProcessSettings.bOverride_DepthOfFieldFstop = false;
	}
	else
	{
		// The clip from ClipStarts on, its fade at the paired Matinee's share of it: the Matinee from its start to its
		// fade spans the clip from ClipStart to there, on the scene's time. A clip that carries the Wasami forward is
		// set as far back as it goes from its own start by the fade, so that the pelvis is then at the mark.
		const float Length = Clip ? ClipLength : DeathDelay;
		const float ClipFade = Length * FadeStartShare(Choice);
		ClipStart = ClipStarts.IsValidIndex(Choice) ? FMath::Clamp(ClipStarts[Choice], 0.f, ClipFade * 0.9f) : 0.f;
		MatineeRate = FadeStartShare(Choice) * MatineeLength(Choice) / (ClipFade - ClipStart);
		FadeStart = RealTime(ClipFade - ClipStart);
		FadeDuration = RealTime(FadeEndShare(Choice) * MatineeLength(Choice) / MatineeRate) - FadeStart;
		BodyStart = FVector::ZeroVector;
		if (Clip)
		{
			FVector Travel = WasamiEnemyAnim::GetRootTransform(*Clip, ClipFade).GetLocation()
				- WasamiEnemyAnim::GetRootTransform(*Clip, 0.).GetLocation();
			Travel.Z = 0.;
			BodyStart = -BodyPlacement().TransformVector(Travel);
		}
		View->SetFieldOfView(FieldOfView);
		// PostProcessVolume_1's Fstop under the Matinee's focal distance, which UpdateScene pulls each frame.
		View->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
		View->PostProcessSettings.bOverride_DepthOfFieldFstop = true;
		View->PostProcessSettings.DepthOfFieldFstop = DofFstop;
	}
	if (Clip)
	{
		// Played at a standstill and moved on by the scene's time each frame.
		Body->PlayAnimation(Clip, bFace);
		Body->SetPlayRate(0.f);
	}
	// The first frame as it is at t = 0, the pose worked out now for the camera to look at.
	Elapsed = 0.f;
	bAimSet = false;
	Body->SetRelativeLocation(BodyStart);
	if (Clip)
	{
		Body->SetPosition(ClipStart, false);
		Body->TickAnimation(0.f, false);
		Body->RefreshBoneTransforms();
	}
	UpdateScene(0.f, 0.f);
	Body->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);
	SetActorTickEnabled(true);

	// DisableInput(the player's controller) on the player character, Put Down Tablet, and the Matinee's director cut
	// to its camera.
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		Player->DisableInput(Controller);
		if (AWasamiPlayerCharacter* WasamiPlayer = Cast<AWasamiPlayerCharacter>(Player))
		{
			// Beyond the original: what the player the level makes when it opens again gets back, written before Put Down
			// Tablet lowers the tablet (the reviewer's call of 2026-09-22, the roadmap's 38).
			if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
			{
				Instance->RememberPlayerState(WasamiPlayer->IsTabletUp(), WasamiPlayer->IsSprintOn());
			}
			WasamiPlayer->PutDownTablet();
		}
	}
	if (Controller)
	{
		Controller->SetViewTarget(this);
		// The hotel's shake; the Gold Watcher's kill has its camera anim alone.
		const TSubclassOf<UCameraShakeBase> Shake = bFace ? nullptr : ShakeClass.LoadSynchronous();
		if (Shake)
		{
			Controller->ClientStartCameraShake(Shake, ShakeScale);
		}
	}

	FTimerManager& Timers = GetWorldTimerManager();
	Timers.SetTimer(FadeTimer, this, &AWasamiCapture::StartFade, FMath::Max(FadeStart, KINDA_SMALL_NUMBER), false);
	Timers.SetTimer(DeathTimer, this, &AWasamiCapture::EndCapture, GetDeathDelay(), false);

	// The Matinee's sound track (the hotel) and the Gold Watcher's PlaySound2D calls (the face). Their times are the
	// original's own, which the scene's rate does not touch: the hotel's is at t = 0 and the watcher's two come of
	// Delays, as this room's fade and death do.
	SoundTimers.Reset();
	SoundTimers.SetNum(NumSounds(Choice));
	for (int32 Index = 0; Index < SoundTimers.Num(); ++Index)
	{
		const float At = SoundTime(Choice, Index);
		if (At <= 0.f)
		{
			PlayCaptureSound(Index);
		}
		else
		{
			Timers.SetTimer(SoundTimers[Index], FTimerDelegate::CreateUObject(this, &AWasamiCapture::PlayCaptureSound, Index), At, false);
		}
	}
}

void AWasamiCapture::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	UpdateScene(Elapsed, DeltaSeconds);
	if (bFading)
	{
		FadeTime += DeltaSeconds;
		const float Alpha = FadeTime / FMath::Max(FadeDuration, KINDA_SMALL_NUMBER);
		ApplyFade(FadeCurve(Alpha));
		bFading = Alpha < 1.f;
	}
}

void AWasamiCapture::UpdateScene(float Time, float DeltaSeconds)
{
	const bool bFace = Choice == FaceChoice;
	const float Scene = SceneTime(Time);
	if (Clip && ClipLength > 0.f)
	{
		Body->SetPosition(bFace ? FMath::Fmod(Scene, ClipLength) : FMath::Min(ClipStart + Scene, ClipLength), false);
	}
	FVector BodyAt = BodyStart;
	if (bFace)
	{
		BodyAt.X = -FaceRushDistance * FMath::Exp(-Time / FMath::Max(FaceRushTime, KINDA_SMALL_NUMBER));
	}
	Body->SetRelativeLocation(BodyAt);

	// The room's frame, which the bones' places come back into.
	const FTransform& RoomTransform = GetActorTransform();

	FVector Location;
	FQuat Rotation;
	if (bFace)
	{
		// The watcher's camera looks back along -X from the Delay on; its camera anim moves it in its own frame (UE4's
		// CameraLocal play space).
		const FQuat Base(FRotator(0., 180., 0.));
		FVector Move = FVector::ZeroVector;
		FRotator Turn = FRotator::ZeroRotator;
		if (Time > WatcherAnimDelay)
		{
			EvaluateWatcherCamera(Time - WatcherAnimDelay, Move, Turn);
		}
		Location = FaceCameraOffset + Base.RotateVector(Move);
		const FVector Head = RoomTransform.InverseTransformPosition(Body->GetSocketLocation(HeadBone));
		Location.X = FMath::Max(Location.X, Head.X + FaceGap);
		Rotation = Base * Turn.Quaternion();
	}
	else
	{
		// The Matinee's camera on the scene's time: its moves from t = 0, grown by the room's camera height over the
		// hotel's, and its turns as they are (they look at where the monkey stands, from the floor too). The eyes'
		// following the Wasami goes on top as the turn, seen from where the camera starts, from the Wasami standing at
		// the mark (AimRest) to where it is.
		const float Matinee = FMath::Min(Scene * MatineeRate, MatineeLength(Choice));
		View->PostProcessSettings.DepthOfFieldFocalDistance = HotelFocalDistance(Matinee);
		FVector HotelAt, HotelFrom;
		FRotator HotelTurn, HotelTurnFrom;
		EvaluateHotelCamera(Choice, Matinee, HotelAt, HotelTurn);
		EvaluateHotelCamera(Choice, 0.f, HotelFrom, HotelTurnFrom);
		Location = CameraStart + (HotelAt - HotelFrom) * (CameraStart.Z / HotelCameraOffset.Z);
		const FVector Target = RoomTransform.InverseTransformPosition(Body->GetSocketLocation(AimBone));
		Location.X = FMath::Max(Location.X, Target.X + AimGap);
		const FRotator Desired = (Target - CameraStart).Rotation();
		if (!bAimSet)
		{
			Aim = Desired;
			bAimSet = true;
		}
		else if (DeltaSeconds > 0.f)
		{
			Aim = FMath::RInterpTo(Aim, Desired, DeltaSeconds, AimSpeed);
		}
		const FRotator Rest = (AimRest - CameraStart).Rotation();
		Rotation = Aim.Quaternion() * Rest.Quaternion().Inverse() * HotelTurn.Quaternion();
	}
	View->SetRelativeLocationAndRotation(Location, Rotation);
}

void AWasamiCapture::PlaceFaceLights()
{
	// FaceLightPlaces are in the mesh's frame and the lights ride the head bone, so they go through its bind pose.
	const FTransform Bind = RefBoneTransform(Body->GetSkeletalMeshAsset(), HeadBone);
	for (int32 Index = 0; Index < FaceLights.Num(); ++Index)
	{
		FaceLights[Index]->SetRelativeLocation(Bind.InverseTransformPosition(FaceLightPlaces[Index]));
	}
}

float AWasamiCapture::FadeCurve(float Alpha)
{
	return FMath::CubicInterp(0.f, 0.f, 1.f, 0.f, FMath::Clamp(Alpha, 0.f, 1.f));
}

float AWasamiCapture::HotelFocalDistance(float MatineeTime)
{
	// Auto-clamped keys whose tangents all come out 0 — the two tracks' ends are stationary, and the focal distance's
	// middle key sits on its hold, which clamps it — so each pair eases along FadeCurve's cubic, held at either end.
	const float Near = FMath::Lerp(DofFocalNear, DofFocalNearEnd,
		FadeCurve((MatineeTime - DofFocalNearHold) / (DofFocalNearTime - DofFocalNearHold)));
	const float Depth = FMath::Lerp(DofFocalDepth, DofFocalDepthEnd, FadeCurve(MatineeTime / DofFocalDepthTime));
	return float((Near + Depth * 0.5f) * FrameScale);
}

void AWasamiCapture::StartFade()
{
	// The Matinee's fade eases in and out of its two keys, which the camera manager's own fade cannot do: black by
	// hand each frame instead, on the Matinee's curve. The Gold Watcher's UMG_BlackFade_3 is black at once (0 long).
	// Held black from there until the level opens again, as bPersistFade does.
	FadeTime = 0.f;
	bFading = FadeDuration > 0.f;
	ApplyFade(bFading ? 0.f : 1.f);
}

void AWasamiCapture::ApplyFade(float Amount)
{
	FadeAmount = Amount;
	if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APlayerCameraManager* Camera = Controller->PlayerCameraManager)
		{
			Camera->SetManualCameraFade(Amount, FLinearColor::Black, false);
		}
	}
}

void AWasamiCapture::EndCapture()
{
	// The face's black comes with DeathEvent: put it up first when both are due on the same frame.
	FTimerManager& Timers = GetWorldTimerManager();
	if (Timers.IsTimerActive(FadeTimer))
	{
		Timers.ClearTimer(FadeTimer);
		StartFade();
	}
	// The Matinees are all black by the death screen; nothing ticks the last frames of the fade in after this.
	if (bFading)
	{
		bFading = false;
		ApplyFade(1.f);
	}
	SetActorTickEnabled(false);

	// Delay 3.5 (the face: 1.15) → the zone's DeathEvent (the death screen, the pause). The enemy that caught the
	// player is gone by now (the capture removes every enemy), which reads as no cause: not the player's doing either
	// way.
	if (AWasamiGameMode* GameMode = Mode.Get())
	{
		GameMode->DeathEvent(CauseActor.Get());
	}
}
