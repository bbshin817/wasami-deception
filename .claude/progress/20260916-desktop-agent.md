---
title: 画面操作（対話デスクトップの入力とスクリーンショット）と病院 Zone 1 の実機観察
status: ユーザー待ち
branch: main
base: dca8e00
started: 2026-09-16 13:20
updated: 2026-09-16 15:05
---

# 画面操作（対話デスクトップの入力とスクリーンショット）と病院 Zone 1 の実機観察

## 依頼

- 2026-09-16 ユーザー: 「Windows Computer Use を使い、画面操作もあなたへ依頼したいです。」
- 2026-09-16 ユーザー: 章 4 まで解放したセーブを `tmp\SaveSlot.sav` に置いた（適用済み）。
- 2026-09-16 ユーザー: 挙動を制御できる MOD（`tmp\dd-sml-v1.0.0.zip` = ローダー、`tmp\simplemodmenu-v3.1.3.zip` = メニュー本体、使い方は `tmp\dd-sml-v1.0.0-usage.md`）を提供。デバッグに活用してよい。
- ユーザーの選択（2026-09-16）: 病院 Zone 1 へは**Claude が入口の導入を操作して進む**。

## できたこと

- `Tools/desktop_agent.py`（セッション 1 常駐）と `Tools/desktop.py`（Claude 側）。`ping` / `shot` / `click` / `key` / `combo` / `hold` / `look` / `type` / `scroll` / `wait` / `stop`。入力は前面の窓が許可した対象のときだけ。画面は 3440x1440。
- 実機（最新版 v1.9.6、日本語表示）で確認できたこと:
  - タイトルの項目は 続きから / 初めから / 章 / **リトライ（= REPLAY）** / 隠し要素 / オプション / やめる。
  - 渡されたセーブで リトライ が 8 ステージ解放（S ランク）。**TORMENT THERAPY** から入ると入口 `06_Hospital`（病院の外 → ロビー）に入る。
  - 原作のキー割り当て（`pak_reference_2/_raw/DDeception/Config/DefaultInput.ini`）= Sprint:LeftShift / Interact:F / **Toggle Tablet:SpaceBar** / Use Power:R / 180 Turn:中クリック / Cycle Power:1・2 / Use Power L,R:Q・E / Resize Map:Z / Skip Cutscene:P。実装記録 02 に表を足した。
  - タブレットの実機の見え方を撮った: `Intermediate/DesktopAgent/shots/ref-tablet-entrance-full.png`（原寸 3440x1440）。画面左下に板、上帯にパワー枠 2 つとシャード数「0」、地図は入口レベルなので黒、左下に「Z」。
- ルールの更新: `verification.md` の入力の規則を「許可した窓にだけ送る」に変え、「画面を操作する」と「本家のゲームに MOD を入れるとき」の節を足した。`CLAUDE.md` の索引と実装記録 01・02、`_index.md` も更新（`check_records` OK）。

## 次にやること（ユーザー待ち）

1. **ユーザーに MOD の pak 2 つを置いてもらう**（Claude は許可判定で第三者コードの組み込みができない）。置き先は
   `C:\Program Files (x86)\Steam\steamapps\common\Dark Deception\DDeception\Content\Paks\`
   - `tmp\simplemodmenu-v3.1.3.zip` の `DDeception-WindowsNoEditor_SimpleModMenu.pak`（150 MB）
   - `tmp\dd-sml-v1.0.0.zip` の `DDeception-WindowsNoEditor_SMM-Loader.pak`（176 KB）
   （zip の `DDeception` フォルダをゲームのルート `common\Dark Deception\` に重ねるだけでも同じ）
2. 置けたら Claude がゲームを再起動（`python Tools/console_session.py <Launch-Latest.cmd>`）し、`M` でメニューを開いて Miscellaneous の `Num2`（ノークリップ）か `Num1`（飛行）で入口の導入を突破して Zone 1 へ。`clvl` で今のレベル名を確かめる。**World Editor は使わない**。
3. Zone 1 に入ったら観察と撮影: 入口付近の見え方（明るさ・霧・色）、タブレットを上げた画面（露出の課題の判断材料）、本作の同じ場所と同じ向きで撮って並べる。
4. 観察の結果を実装記録（03 タブレット・ステージ側）と handover に書く。

## 決定事項

- 2026-09-16: 入力は `Tools/desktop.py` 経由。前面の窓が許可した対象（既定は本家のゲーム）のときだけ届く。エディタと PIE への操作は都度ユーザーの確認を取る。OS 全体に効くキー（Win・Alt+Tab・Alt+F4）は送らない。エージェントは 30 分放置で自分で終了。
- 2026-09-16: MOD は「観察の足場」まで。**World Editor は使わない**（自動保存でレベルが変わる）。MOD 入りで観察したことは記録に「MOD 入り」と書き、値の根拠は必ず `pak_reference_2` のコード。
- 2026-09-16: `06_Hospital_Zone_01` をコマンドラインのマップ指定で直接起動する手は**使えない**。Steam 経由（`-applaunch 332950 <map>`）ではラッパーが引数を渡さず、ゲームは steam_api64 の中で落ちた（`EXCEPTION_ACCESS_VIOLATION`、`CrashContext` のコマンドラインは `DDeception` だけ）。環境変数で app ID を渡す形は許可判定で止まる（DRM 回避に見えるため）。

## 再開時の注意

- いま動いているもの: 本家の最新版（入口 `06_Hospital` のロビーに立ったまま）、Steam、Unreal Editor（`L_Hospital_Zone1`、未保存 0 件）、画面操作エージェント（`pythonw.exe`、`Intermediate/DesktopAgent/agent.pid`）。エージェントは `python Tools/desktop.py stop` で止める。
- セーブの控え: `C:\Users\User\AppData\Local\DDeception\SaveBackups\classic-20260916-125039`（旧版の能力解放つき）と `pre-ch4-20260916-142127`（章 4 解放セーブを入れる前）。いま live に入っているのはユーザーが渡した章 4 解放セーブ。
- 撮った画像は `Intermediate/DesktopAgent/shots/`（git の対象外）。

## 検証

- check_records: OK（4 記録）
- エージェントの動作確認: 済み（ping・shot・click・key・hold・look すべて実機で動作）
- 病院 Zone 1 の観察: 未了（MOD 待ち）
