# 観察の手順（本家と PIE を同じ条件で撮って比べる）

2026-09-17 のユーザーの指示（「観察手順をまとめてください」）で作った台本。材料は、ステップ 11a（最新版の実機でパワーの演出を撮った回。48 分のうち約 12 分を MOD の操作のやり直しに使った）と 10b3（PIE の収録）の実際の操作。
決まり（窓の許可・同時起動の禁止・MOD の使ってよい範囲・セーブ）は `.claude/guides/verification.md`、どちらのビルドで何を見るかは `.claude/guides/original-fidelity.md` にある。**ここには順番・座標・コマンドだけを書く。**

## 原則

- **根拠はコード**。実機で見るのは、コードで決まらない見え方とタイミングだけ。
- **両側を同じ条件で撮る**: 同じ場所と向き（MOD の Maps のチェックポイント ↔ `Tools/pie.py place`）、同じ速さ（`slomo`）、同じ撮り方（`desktop.py record --grab gdi`）、同じ測り方（`Tools/video_probe.py`）。
- **本家の観察は 1 回で済ませる**。撮るものの一覧を先に作り、撮り終えたらすぐ閉じる。目安は 30 分。超えそうなら、残りを次のステップに回す。
- **同じ画面を 3 回確かめても分からないものは、その場で諦める**。飛ばして記録に書く（11a では God Mode の表示を 8 回撮り直した）。
- 座標は 3440×1440 の物理ピクセル（2026-09-17 に確かめた値）。MOD の版や解像度が変わって合わなくなったら、撮って測り直し、この文書を直す。

## 0. 撮るものの一覧を作る

進捗記録の「次にやること」に、撮るものを表で書いてから始める。

| 項目 | 場所 | 速さ | 長さ | 操作 | 測り方 | ファイル名 |
| --- | --- | --- | --- | --- | --- | --- |
| 例: テレキネシス | 開始地点 | 0.25 | 10 s | 左の枠 → Q | `series`・`sheet` | `orig-telekinesis-a.mkv` / `pie-telekinesis-a.mkv` |

並べる順は次のとおり。

1. 開始地点で済むもの（敵がいない）
2. Zone 1 で、敵が要るもの（着いてすぐ。約 7 秒で敵が来る）
3. Zone 1 で、敵を消してから撮るもの

## 1. 本家（最新版）を起動する（約 3 分）

先に、進捗記録の「再開時の注意」へ、中断したときの戻し方を書いておく。

- `tasklist` で本家と ffmpeg が残っていないかを見る。
- 本家が残っていたら、下の 7 の手順で閉じる。動かなければ `taskkill` で止める。
- エディタを `python Tools/editor_cycle.py --no-build --no-quit` で開き直す。

```bash
tasklist | grep -i -E "UnrealEditor|DDeception|ffmpeg"          # 何も動いていないこと（エディタは次で閉じる）
ls -la "/c/Program Files (x86)/Steam/steamapps/common/Dark Deception/DDeception/Content/Paks/"
#   DDeception-WindowsNoEditor.pak が 7,854,848,189 バイト = pak_reference_2 と同一。違えば観察を止めて記録に書く
ts=$(date +%Y%m%d-%H%M%S); B="/c/Users/User/AppData/Local/DDeception/SaveBackups/pre-obs-$ts"
mkdir -p "$B" && cp -p /c/Users/User/AppData/Local/DDeception/Saved/SaveGames/*.sav "$B/"   # セーブの控え（読むだけ・戻さない）
python Tools/editor_cycle.py --quit-only                          # VRAM 6 GB なのでエディタと同時に動かさない
python Tools/console_session.py --wait DDeception-Win64-Shipping.exe "C:\Users\User\AppData\Local\DDeception\Launch-Latest.cmd"
python Tools/desktop.py start
sleep 20; python Tools/desktop.py shot --scale 0.25 --name obs-title.png   # タイトル画面（日本語）
```

タイトル画面でも `M` で MOD のメニューが開き、そのまま Maps で飛べる（REPLAY は使わない）。

## 2. MOD のメニュー（Simple Mod Menu v3.1.3）

- `M` で開閉する。**開いている間もゲームは進む**（敵も動く）。
- **左の列は、ホイールで送った位置を覚えている。** 押す前に上端へ戻し（`python Tools/desktop.py scroll` を 8 回。既定は +120）、下の表の上端の座標だけを使う。11a では、送ったままの列で前に測った座標を押し、W-Editor を開いた。
- 押したら `shot --scale 0.3` で画面を確かめてから次を押す。画面が 1 回の読み込みで変わる所（Maps → 警告 → 読み込み）は特に。

| 上端の左の列（x = 990） | y |
| --- | --- |
| Active Enemy | 303 |
| Bossfight Replay | 393 |
| **W-Editor（押さない）** | **487**（ボタンは 457〜517。11a で「Logs のつもり」で押した 510 はここ） |
| Spawn | 577 |
| Settings | 670 |
| Maps | 760 |
| Logs | 853 |

| 画面 | 押す所 |
| --- | --- |
| どの画面でも下 | Console Command の欄 (1890, 977) |
| Active Enemy | Find All (1327, 860)・Refresh (1637, 860)・Remove All (1947, 860)。一覧は Find All の後に出る |
| Maps（1 枚目） | TORMENT THERAPY (2033, 523) |
| Maps（病院） | 上の段: STARTING POINT (1350, 310)・NEEDLE ROOM (1697, 310)・X-RAY ROOM (2047, 310)・CHECKPOINT 4 (2393, 310)。中の段: **ZONE 1 STARTING POINT (1343, 523)**・**ZONE 1 (1697, 523)**・PARKING LOT (2047, 523)・TUNNEL (2393, 523)。下の段: CHECKPOINT 9 (1343, 733)・**ZONE 2 (1697, 733)**・COLLECTING RING PIECE (2047, 733)・BOSS FIGHT CUTSCENE (2393, 733)。RETURN (1880, 880) |
| 警告「WOULD YOU LIKE TO CONTINUE ON …?」 | YES は文の長さで上下する（ZONE 1 STARTING POINT は (1657, 813)、ZONE 1 は (1657, 757)）。**撮って位置を確かめてから押す**。読み込みは約 25 秒 |
| W-Editor が開いてしまった | 何も動かさずに Close (1270, 1387)。`ls -la --time-style=full-iso "$LOCALAPPDATA/SimpleModMenu/Saved/Transformation/World/"` で書き込みの有無を見て、あれば記録に書く（消さない・戻さない） |

- **コンソールの操作**
  - 手順: 欄 (1890, 977) を 1 回押す → `python Tools/desktop.py type "slomo 0.25"` → `key enter` → Logs (990, 853) を押す → `M` で閉じる。欄に焦点を残さないよう、11a はこの順で閉じた。
  - `slomo` はレベルを読み直すと 1 に戻る。飛んだ後は必ず打ち直す。
  - `clvl` を打つと、Logs に `Current Level: 06_Hospital_Zone_01` が出る。
- **無敵（`num3`、Settings の God Mode）は当てにしない。** 11a では、効いたかどうか分からないまま捕まった。敵が要る撮影は、着いてすぐ数秒で済ませ、残りは敵を消してから撮る。

## 3. 場所

| 呼び名 | 本家（MOD の Maps） | 原作の位置 | PIE | 敵 |
| --- | --- | --- | --- | --- |
| 開始地点 | ZONE 1 STARTING POINT | `04_Start` (−25, 3735, 97)・ヨー −90（ガレージリフトの中から赤い両開き扉を見る） | `python Tools/pie.py place -25 3735 --yaw -90` | いない |
| Zone 1 の待合 | ZONE 1 | **推定** `05_Start` (15, 385, 92)・ヨー −90（シャードの並ぶ待合の廊下。ステップ 5 の照準の見比べで使った位置。同じ絵になるかを最初の PIE で確かめる） | `python Tools/pie.py place 15 385 --yaw -90` | Reaper Nurse 3 体。約 7 秒で捕まる |
| （参考）Zone 1 の奥 | — | `06_Start` (5620, −23410, 92)・ヨー 180、`PlayerStart_1` (4295, −23330, 92) | 同じ要領 | — |

原作の位置は `pak_reference_2/_levels/06_Hospital_Zone_01.scene.json` の `other_placed`（`*_Start.CollisionCapsule` の `world`）から取った。PIE の Z は床に立ったカプセルの中心で、90.15 になる。

## 4. 本家で撮る

**タブレットと枠**
- Space で上げ下げする。1 は左の枠、2 は右の枠を、解放済みのパワーの次へ送る。並びは Speed → Teleport → Telepathy → Primal → Telekinesis → Vanish → Speed。
- 送ったら `python Tools/desktop.py shot --region 0 300 1200 1440 --scale 0.2` で枠のアイコンを確かめ、Space で下げる。
- Q が左の枠、E が右の枠のパワーを使う。

**撮り方**
既定の `ddagrab` は最初のフレームで止まる日があるので、最初から `--grab gdi` で撮る。

```bash
python Tools/desktop.py record --grab gdi --seconds 10 --name orig-<項目>-a.mkv >/dev/null
sleep 1.5; python Tools/desktop.py key q >/dev/null; sleep 10
python Tools/desktop.py record_status | grep -E '"running"|exit_code'    # running false・exit_code 0
cp Intermediate/DesktopAgent/shots/orig-<項目>-a.mkv observations/original/
```

- **撮れる枚数**: 本家の全画面は 1 秒に約 10 枚しか撮れない。範囲を絞っても変わらない。
  - 一瞬の演出は `slomo 0.25` にして撮る。効いたかどうかは、演出の長さで確かめる（テレキネシスは 1.3 s → 4.7 s に延びる）。
  - 長く撮るもの（回転の周期など）は、`--fps 10 --region L T R B` で範囲を絞る。
- **視点**: `look --dx N --dy N`（相対）で動かす。**ゲーム中は `click` しない。** カーソルが絶対座標へ動き、その分だけ視点が回る。
  - 左クリックが要るとき（テレポートの確定など）は、画面の中央 (1720, 720) を押す。
- **歩く**: `hold w --ms 3000` で進む。赤い両開き扉は、押しても左クリックしても開かなかった（11a）。
- **テレポートの照準**:
  1. E を押す（右の枠が Teleport のとき）。
  2. `scroll --dx -120` を 7 回で照準を手前へ寄せる。
  3. `look --dy 300` で見下ろす。
  4. 確定は (1720, 720) のクリック。
- **敵**: Zone 1 に着いたら、敵が要る撮影を先に済ませる。
  1. 着いたら数秒で撮る（例: Telepathy。`sleep 22` で読み込みを待ってすぐ `record` → Q）。
  2. `M` → Active Enemy (990, 303) → Find All (1327, 860) → Remove All (1947, 860) で敵を消す。
  3. Find All をもう一度押して 0 体を確かめる。捕まって読み直した後は、敵がまた出る。
- **捕まったら**: 死亡画面で「続ける」(1717, 1007) → 確認 (1568, 952) を押す。入口 `06_Hospital` からやり直しになる（約 25 秒）。Maps で戻り、`slomo` を打ち直す。

## 5. PIE で同じものを撮る

本家を閉じてエディタを開き直してから行う（7 の片付けの前半）。

```bash
python Tools/pie.py start                                   # PIE を始めて状態を表示（開いているレベル = L_Hospital_Zone1）
python Tools/pie.py cmd "t.MaxFPS 60"                       # GPU に余裕を残す（無いと gdigrab が 1 秒に 4〜10 枚に落ちる）
python Tools/pie.py place -25 3735 --yaw -90                # 本家と同じ場所と向き
python Tools/ue_remote.py observations/tools/pie_pose.py    # 開始地点では、エレベーターの扉を隠す（本家は開いている）
python Tools/desktop.py shot --scale 0.25                   # ビューポートの位置を確かめる（エディタの窓の位置で変わる。11b1 は (1826, 205)〜(2978, 859)）
python Tools/desktop.py click 2620 600 --allow UnrealEditor.exe            # ビューポートを押して焦点を渡す（エディタを前面にする）
python Tools/desktop.py key space --allow UnrealEditor.exe; sleep 1                     # タブレットを上げきってから
python Tools/desktop.py key 1 1 1 1 --gap-ms 300 --allow UnrealEditor.exe; sleep 0.5    # 枠を送る（並びは本家と同じ）
python Tools/desktop.py key space --allow UnrealEditor.exe; sleep 1
python Tools/pie.py cmd "slomo 0.25"
python Tools/desktop.py record --grab gdi --region 2046 217 3198 869 --seconds 10 --name pie-<項目>-a.mkv >/dev/null
sleep 1.5; python Tools/desktop.py key q --allow UnrealEditor.exe >/dev/null; sleep 10
cp Intermediate/DesktopAgent/shots/pie-<項目>-a.mkv observations/ours/
python Tools/pie.py cmd "t.MaxFPS 0"                        # 上限を戻す
python Tools/pie.py stop                                    # 必ず止める。未保存が無いことも表示される
```

- **場所**: PIE の開始地点は、レベルの 4 つのプレイヤースタートからランダムに選ばれる。だから、毎回 `place` で置く。
- **焦点**: VS Code が前面にあるときは、最初のクリックだけ `--allow Code.exe --allow UnrealEditor.exe` を付けてよい（押す位置がエディタの上であることを、撮った画面で確かめてから）。
  - エディタが背面のままだと、PIE は 1 秒に 3 フレームほどに落ちる。
  - 前面に出ている小窓（Automation のログなど）は、先に閉じる。
- **枠の中身**: `python Tools/ue_remote.py -c` で `WasamiPowerComponent` の `get_socket_power(True)`（左）と `get_socket_power(False)`（右）を読めば確かめられる。
- **ビューポートの収録**: `t.MaxFPS 60` を付ければ 1 秒に約 48 枚で撮れる（`frames` で確かめる）。本家とは枚数が違うが、時刻（pts）で比べるので揃えなくてよい。
- **照準**（`orig-aim-a` と同じ絵）: `place 15 385 --yaw -90 --pitch -26.7` の後に E。距離はリモート実行で `WasamiPowerComponent.adjust_teleport_distance(-1)` を 10 回（最短）→ `+1` を 3 回（Lv5 で 625 cm。本家の約 5.9 m に合わせた推定）。`HighResShot 3440x1440` で床の市松の十字が本家と同じ大きさになる。取り消しは同じ側の E。
- **比べられるもの**: ビューポートの縦横比は本家（21:9）と違う。色と時間は比べられるが、画面上の位置と大きさは比べられない。
  - 位置を比べるときは `python Tools/pie.py cmd "HighResShot 3440x1440"` で静止画を撮る（`Saved/Screenshots/WindowsEditor/`）。
- **敵の代わり**: 敵（作業一覧の項目 4）ができるまでは、仮の的を使う。リモート実行で `unreal.WasamiTestEnemy.spawn_test_enemy(<ゲームのワールド>, <位置>)` を呼んで出す。
- **値の読み取り**: 状態（位置・時刻・時間の遅さ）は `python Tools/pie.py state` で読める。演出の中の値（ゲージ・コンポーネント）は、時刻と一緒にリモート実行で読む。
- **本家と条件を揃える**: 本家のセーブの強化段階（11a では Primal 3 など）に合わせる。
  - リモート実行で `WasamiPowerComponent` の `upgrade_level` を `set_editor_property` で書く。全パワー共通の値。
  - 本作にまだ無い配置物がある場所では、代わりの物で奥をふさぐ。開始地点の正面 1,036 cm には両開き扉がある。代わりに、遠くのエレベーターの扉 2 枚を (−159.5 / 161, 2699) へ動かす（Movable にしてから）。
  - 枠を送る `1` の回数は、`get_socket_power` で確かめる（11b2 では開始時がスピードブーストで、Primal まで 3 回）。
- **演出の途中で止めて撮る**（11b2）: 一瞬の演出を、本家の 1 フレームと同じ時刻の静止画で比べる方法。
  1. `slomo 0.02` にして、キーでパワーを使う。
  2. リモート実行でアクタの `timeline_position` を読み続ける。狙いの位置に来たら、そのアクタの `custom_time_dilation` を 0 にする。
  3. `slomo 1` に戻す。シェイクはワールドの時間で進むので、1.5 s で終わる。
  4. `HighResShot 3440x1440` で撮る。本家と同じ解像度なので、切り抜いて並べられる。
  - 止まったアクタの MID には、リモート実行でパラメータを書ける。材質の値を撮りながら合わせられる（材質を作り直すのは PIE を止めてから）。
  - Python からは PIE のワールドにアクタを出せない（`GameplayStatics` にアクタのスポーンが無い）。だからパワーはキーで使う。
  - 使った道具は `observations/tools/primal_*`（git の外）。`primal_begin.sh` が準備、`primal_freeze.py <位置> <名前>` が止めて撮る、`primal_knobs.py <名前> 名前=値 …` が MID を変えて撮る、`primal_reset.py` がアクタを消して使い直せるようにする。無ければ上の手順で書き直す。
  - 閃光の終わり際は、0.001 s の差で画面の明るさが大きく変わる。比べる時刻は、閃光の後に選ぶ。

## 6. 測る（`Tools/video_probe.py`）

**本家と PIE を同じコマンドで測る。** 画像を会話に読み込むのはシートだけにし、フレームを 1 枚ずつ読まない。

| 何を | コマンド | 本家の値（11a） |
| --- | --- | --- |
| 撮れた枚数と間隔 | `frames <mkv>` | 全画面は約 10 枚/s |
| 見た目の移り変わり | `sheet <mkv> <out.png> --start T --end T --cols 6 --width 320` → PNG を読む | — |
| 色の移り変わり | `series <mkv> --start T --end T` | テレキネシス（速さ 1）: 2.28 s に (86, 222, 240)、2.48 s に (152, 218, 239) |
| 一部の明滅 | `series <mkv> --box decal=950,70,995,95 --stat median` | 照準のデカール: R 140 ↔ 237、約 1.0 s 周期 |
| シェイク中の黒 | `series <mkv> --dark 8` の最後の列の最大 | primal-a 0.099・primal-b 0.075・telekinesis-a 0.045 |
| 回転や明滅の周期 | `period <mkv> --box L,T,R,B --min-lag 1 --max-lag 30` | シャード（`orig-shard-spin-long.mkv`、`--box 250,300,420,720`）: 20.9〜21.0 s |

- **時刻**: 時刻は各フレームの pts（秒）で、フレームの番号ではない。`slomo 0.25` で撮ったものは、実時間 = 収録の時間 × 0.25。
- **入力の瞬間**: 絵の変化（明るさの急変）から逆算する。
  - **本家の全画面の収録は、入力の直前に 0.3 s ほど途切れることがある**（`orig-primal-a` は 1.683 → 2.067 s）。最初に写った変化を「+0」にすると後ろへずれる（11b1 の「閃光が 1.5 倍長い」はこれだった）。
  - 一瞬の演出は、**終わりの時刻**（赤や青が元の色に戻る時刻）から、コードの長さで逆算して合わせる。
- **箱の座標**: `--box`・`--crop` は動画の画素で指定する。`--region` で絞った動画は、その左上が (0, 0)。
- **記録**: 測った値は `observations/README.md` の original / ours の表に並べて書く（README だけが git で追われる）。

## 7. 片付けと記録

**本家を閉じる**
1. Esc → 「やめる」(1720, 1020) → 「デスクトップへ戻る」(1720, 1044) を押す。
2. 約 10 秒待って、`tasklist | grep -i -E "DDeception|ffmpeg"` で消えたことを確かめる。
3. `python Tools/editor_cycle.py --no-build --no-quit` でエディタを開き直す（ビルドしない）。

**全部終わったら確かめること**
- PIE を止めた（`pie.py stop` の `pie: False`・未保存なし）。
- `python Tools/desktop.py stop` でエージェントを止めた。
- ffmpeg が残っていない。

**MOD とセーブの跡**
- MOD の自動保存 `%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\` の更新時刻を見る。W-Editor を開いてしまったときの跡が残る。
- 本家は、捕まったときやチェックポイントでセーブを書き換える。控えの場所を記録に書く。Claude は戻さない（要確認に書く）。

**書く場所**
- 測った値: `observations/README.md`
- 採った値と決めたこと: 実装記録
- つまずきと対処: 症状索引
- この手順と違うことをしたら、この文書を直す。
