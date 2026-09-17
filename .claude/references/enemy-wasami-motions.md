# 通常の敵ワサミのモーション一式（作り直し用）

2026-09-18 にユーザーが「通常の敵ワサミは新しく作り直す。必要なモーションを一式」と依頼したので、Claude が本家のナース（`pak_reference_2`）の使い方から決めた一覧。ユーザーはこの一覧のとおりに用意する。**ここの名前を、取り込み・`UWasamiEnemyAnimInstance`・場面の組み立てで使う**（届くまでは今の `tmp/enemy_wasami.glb` を仮に使い、下の「仮の対応」で読み替える）。

Matron（Zone 2 の中ボス）は別のモデル（ユーザーが後で指定する）なので、この一覧に入れない（末尾の参考）。

## 作り方の決まり（ユーザーへのお願い）

- 全部のモーションを同じ骨組みで作る（1 つの glb にまとめるか、同じ骨組みの glb に分ける）。人型で、首と頭の骨があること（入口の立っているナースは、首と頭をプレイヤーへ向ける。これはこちらで動かす）。
- 足元（床の高さ、体の真下）に**ルートの骨**を置き、その子に骨盤を置く。今の glb は骨盤が根なので、前へ進む動きの扱いに手間がかかった。
- ループするものは**その場で**動かし（ルートも骨盤も前へ進まない）、最初と最後の姿勢をそろえる。
- 1 回だけのもので、前へ進む・跳ぶものは、移動をルートの骨に入れる（その場でもよい）。
- 待機へ戻るもの（`Stun_Recover`・`Stop`）は、終わりの姿勢を `Idle` の最初の姿勢にそろえる。
- 正面の向き・大きさ（メートル）・毎秒のコマ数（30 以上）を全部でそろえ、各モーションに下の名前を付ける（無名にしない）。
- 長さは目安。場面の演技は、台詞の長さに合わせてこちらでループ・切り詰める。

参考: 本家のナースの当たりは高さ約 236 cm・半径 34 cm（カプセルの半分の高さ 118.06。`BP_06_ReaperNurse`）。移動は巡回 350 cm/s・追跡 800 cm/s（`Normal Speed`・`Skate Speed`。ABP は 400 cm/s を超えると走りに切り替える）。本家のナースはローラースケートで滑るので、足の運びが速さに合わなくても不自然に見えない。本作で走らせるなら、この速さで足が滑って見えない動き（巡回は小走り、追跡は全力疾走）にする。

## A. ゲーム中の基本（作業一覧の項目 4・7）

| 名前 | 動き | 再生 | 長さの目安 | 本家（`Animation/Enemies/Nurse/Reaper/`） |
|---|---|---|---|---|
| `Idle` | 立ったまま軽く揺れる待機 | ループ | 4 秒 | `nurse_idle_01` 3.93 s（止まっている敵、入口に立つ 7 体、Zone 2 の独房） |
| `Idle_Alert` | 身構えて相手をにらむ待機 | ループ | 6 秒 | `ReaperNurse_Idle_Alert` 6.0 s（Zone 2 の見張り 6 体〈`bAggressiveIdle`〉、入口の脱出、Zone 2 への移り） |
| `Walk` | 巡回の移動（秒速 3.5 m に合う小走り） | ループ・その場 | 1 周期 | `nurse_skate_normal` 4.97 s |
| `Run` | 追跡の全力疾走（秒速 8 m） | ループ・その場 | 1 周期 | `nurse_skate_run` 4.97 s |
| `Run_Nightmare` | 全回収後の追跡（`Run` より荒々しく） | ループ・その場 | 1 周期 | 本作独自（ユーザーの決定。速さは項目 7 で本家から決める） |
| `Stun_Loop` | 気絶中（前かがみでふらつく。足は床から動かさない） | ループ | 3 秒 | `nurse_stunned` 2.63 s（気絶は 17 s） |
| `Stun_Recover` | 気絶から起き上がり、待機の姿勢で終わる | 1 回 | 5 秒以内 | 本作独自（ユーザーの「明けに起き上がる」。17 s の最後に入れる） |
| `Stun_Start`（任意） | よろめいて気絶の姿勢へ崩れる | 1 回 | 1 秒 | 本家は 0.25 s のブレンドだけ |
| `Stop`（任意） | 走りから急に止まり、待機の姿勢で終わる | 1 回 | 1 秒 | `nurse_skate_stop` 1.0 s（0.35 s から再生） |

## B. 捕獲（項目 9）

| 名前 | 動き | 再生 | 長さの目安 |
|---|---|---|---|
| `Capture_1`・`Capture_2`・`Capture_3` | 黒い部屋で、カメラへ飛びかかる・迫る決め技 3 種（本家ホテルの猿の捕獲の体。前へ進んでよい） | 1 回 | 3.5 秒前後（本家は捕獲から 3.5 s で死亡画面） |

今の glb の `Backflip`・`sliding_rool`・`Stylish_Walk`（2026-09-17 にユーザーが承認した割り当て）と同じ内容で作り直してもよい。本家のナースの捕獲（`nurse_kill_02`・`ReaperNurse_Needle_Attack`）と、Zone 1 の追跡役の突き（`BP_06_ReaperNurse_06_Chase` の `ReaperNurse_Needle_Attack_NoSound_Montage`）はこれに置き換わる。

## C. 場面の演技（項目 6・16）

本家は台詞 1 本ごとに専用の演技（入口の受付 13 本〈`Animation/06_Hospital/NurseAnims/`〉、案内 21 本〈`NurseIntro/Anims/StandingAnims/`〉、Zone 2 の独房 8 本）。本作は下の汎用の動きで代わりにし、台詞の間は話す身振りをループする（ユーザーが専用の演技を望めば、その分を足す）。

| 名前 | 動き | 再生 | 長さの目安 | 場面（本家） |
|---|---|---|---|---|
| `Phone_Talk` | 受付で受話器を耳に当てて話す | ループ | 5 秒 | 入口の受付（`06_Hospital_Entrance_Intro` の最初の 21 s） |
| `Talk` | 穏やかに話しかける身振り | ループ | 5 秒 | 入口の受付と案内、Zone 2 の独房 |
| `Talk_Angry` | 強い口調で叱る身振り | ループ | 5 秒 | 入口（「動くなと言ったでしょう」「手に負えない患者は受け入れない」） |
| `Laugh` | 笑う | 1 回 | 2〜3 秒 | 入口の受付（`Event_11`） |
| `Beckon` | 手招きし、行き先を指し示す | 1 回 | 1.5〜2 秒 | 入口の案内（「ついて来て」「こちらへ」） |
| `Threat` | 宣告の決め（顔を寄せる・指さすなど） | 1 回 | 2 秒 | 入口の最後（「あなたには見えないでしょうね」） |
| `Turn_Around` | その場で後ろへ振り向く | 1 回 | 0.8 秒 | 入口の案内役（`ReaperNurse_Turn_Around` 0.77 s） |
| `Walk_Back` | 後ずさりする | 1 回 | 1.7 秒 | 入口の案内役、Zone 2 の独房（`ReaperNurse_Walk_Back` 1.67 s） |
| `Dance_1`・`Dance_2` | 飾りの踊り 2 種 | ループ | 3〜4 秒 | 入口に 5 体ずつ（`nurse_dance` 4.3 s、`ReaperNurse_Sexy_Dance_Alt` 3.3 s） |
| `Spin_Attack` | 回転しながら突っ込む | 1 回 | 2.7 秒 | 入口の脱出の 2 体組（`ReaperNurse01/02_Duo_Spin` 2.67 s。2 体で同じものを使う） |
| `Attack_Pose` | 攻撃の決めポーズ | 1 回 | 2.3 秒 | 同上（`ReaperNurse01/02_Duo_Finish` 2.3 s） |
| `Knocked_Back` | 衝撃で吹き飛ばされて倒れ、倒れたまま終わる | 1 回 | 5 秒 | 同上、Primal Fear を受けたとき（`ReaperNurse_Pushed_Back` 4.97 s） |
| `Battle_Idle` | 戦う構え | ループ | 1.5 秒 | Zone 1 の途中の出来事の 2 体（`06_Hospital_Zone1_06Event`。`ReaperNurse_Boss_Idle_01` 1.42 s） |
| `Jump_Up` | 素早く真上へ跳ぶ（踏み切りから空中まで） | 1 回 | 1 秒 | 同上、入口（`ReaperNurse_Fast_Jump_Up` 0.5 s + `_Air` 0.43 s） |
| `Flip_Up` | 宙返りして上の足場へ上がる | 1 回 | 1.4 秒 | 同上（`ReaperNurse_Flip_Up` 1.43 s） |
| `Knockout_Hit` | 殴って相手を気絶させる | 1 回 | 1.6 秒 | Zone 1 から Zone 2 へ移る場面（`06_Hospital_Zone2_Capture`。`Event_39` 1.63 s） |

## 作らないもの

- 透明化（`nurse_cloak`・`nurse_decloak`）、薬投げ（`nurse_pilltoss`）、ボス戦（`Reaper/Bossfight/` の残り）。Zone 2 の独房でナースが透明になって去る所は、`Turn_Around` と `Walk` で去る形にする。

## 今の glb の仮の対応（新しいモデルが届くまで）

`tmp/enemy_wasami.glb`（2026-09-17、11 本）を仮に使うときの読み替え: `Walking` → `Walk`、`Running` → `Run`、`run_fast_2` → `Run_Nightmare`（骨盤の前進を消す）、無名のモーション（10.0 s）の前半 → `Stun_Loop`・後半 → `Stun_Recover`、`Backflip`・`sliding_rool`・`Stylish_Walk` → `Capture_1`〜`3`。`Idle`・`Idle_Alert` は無いので、無名のモーションの終わりの直立の姿勢で止める。C は無い。

## 参考: Matron（別のモデル。項目 11）

本家の Matron は歩かず、立ったまま視界で見張る（`Matron_MiniBoss_AnimBP`）。待機（ループ 13.3 s）、警戒の待機（ループ 8.0 s）、待機 → 警戒（0.8 s）、警戒 → 待機（0.8 s）、見つけた瞬間（1 回 1.33 s。`DD_Matron_Zone_02_Detected_Montage`）の 5 本。
