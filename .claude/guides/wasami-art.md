# ワサミの新しい 2D 素材を画像生成で作る

2026-09-26 のユーザーの依頼「tmp\codex-gpt-image を活用して、参照画像としてワサミアセットを渡し、wasami deception に合う新しいアセットを用意するための仕組みを整えて」と、同日の指示「ワサミ素材は `C:\Users\User\Pictures\Screenshots` を参照してください」による仕組み。道具は `Tools/wasami_art/`（中身は実装記録 01 の「ワサミの 2D 素材の画像生成」）。

## いつ使うか

- 本作独自の 2D 素材（UI の絵・紋章・アイコン、場内のポスターや掲示、材質の模様など）が新しく要るとき。
- 使わない: 本家の素材で済むもの（原作データから作る。`original-fidelity.md`）、本家のロゴやキャラクターを似せたもの、3D のモデル。

## 流れ

1. **依頼書を書く**: `Tools/wasami_art/briefs/<名前>.json`。項目は下の表。例は `example_ward_poster.json`。短い試しなら依頼書なしで `gen --name <名前> --style <型> --prompt "..."`。
2. **候補を作る**: `python Tools/wasami_art/wasami_art.py gen Tools/wasami_art/briefs/<名前>.json`。1 枚 40〜60 秒。出力は `Intermediate/WasamiArt/<名前>/<時刻>/`（git の外）の `out-NN.png`・`sheet.png`（上に参照、下に候補）・`prompt.txt`・`meta.json`。
3. **選ぶ**: `sheet.png` を Read で見て、本人に似ているか・型に合っているか・字が正しいか・余計な字や透かしが無いかを確かめる。合わなければ依頼書の `prompt` を直して作り直す（前の時刻の組は残る）。
4. **採用する**: `python Tools/wasami_art/wasami_art.py adopt <候補の png> [--dest SourceArt/...] [--size WxH]`。`SourceArt/` の下へ写し（Git LFS）、由来（依頼書・指示の全文・参照・頼んだ設定）を `SourceArt/Wasami/generated.json` に足す。同じ行き先は上書きし、由来も差し替える。
5. **取り込む**: ゲームで使うなら、ほかの `SourceArt/Wasami/UI` の画像と同じく取り込みのツール（`Content/Python/wasami_tools`）から `/Game/Wasami/...` へ入れ、使う側の実装記録に書く。

## 依頼書の項目

| 項目 | 要否 | 中身 |
|---|---|---|
| `name` | 省略可 | 出力のフォルダ名。省くとファイル名 |
| `style` | 省略可（`portrait`） | `styles.json` の型: `silhouette`（本作の白抜きの顔と同じ作り、透明）・`poster`（病院の壁の紙）・`portrait`（画面の絵）・`sprite`（UI の小物、透明）・`texture`（材質の模様） |
| `prompt` | 必須 | 何を描くか（英語）。作風と共通の決まりはツールが前に足すので、ここには中身だけを書く |
| `text` | 省略可 | 絵の中に出す字（そのまま出させ、ほかの字は出させない） |
| `refs` | 省略可（型の既定） | 参照の組の名前（`refs.json`）: `face`・`expression`・`pose`・`house_style` |
| `extra_refs` | 省略可 | 足す参照画像のパス（プロジェクトからの相対。PIE の撮影など） |
| `size`・`background`・`quality`・`model` | 省略可 | 型の既定を上書き。大きさは 16 の倍数・長辺 3840 以下・縦横比 3:1 以内 |
| `count` | 省略可（2） | 候補の枚数（1 枚ずつ頼む） |
| `dest`・`final_size` | 省略可 | 採用の行き先（`SourceArt/` の下）と縮める大きさ（`"512x512"`） |

## 参照画像（`Tools/wasami_art/refs.json`）

- ワサミの写真の原本は **ユーザーのスクリーンショットのフォルダ `C:\Users\User\Pictures\Screenshots`**（git の外。環境変数 `WASAMI_ART_SCREENSHOTS` で差し替え）。本家の素材は参照に使わない。
- **フォルダを丸ごと送らない。** 予約票・書類・表計算・コードの画面など無関係なもの、肌の出た写真が混ざっている。`refs.json` に載せた写真だけを送る。
- 写真を足すときは `python Tools/wasami_art/wasami_art.py catalog` で縮小一覧（`Intermediate/WasamiArt/catalog/`、番号 → ファイル名は `index.txt`）を作って見て選び、`refs.json` の組に足す。顔が小さい写真は `crop`（0〜1 の割合）で切る。1 回に送れるのは 16 枚まで。
- 写真の原本と `Intermediate/WasamiArt/` の写しは git に入れない（私的な写真のため）。git に入るのは採用した絵と `generated.json` だけ。

## 生成の窓口（`tmp/codex-gpt-image`）

- ユーザーが置いた codex-gpt-image の CLI（git の外）を呼ぶ。場所は環境変数 `WASAMI_GPT_IMAGE_CLI` で差し替え。認証は Codex の OAuth（`~/.codex/auth.json`）で、API キーは使わない。確かめ: `python tmp/codex-gpt-image/skills/codex-gpt-image/scripts/codex_gpt_image.py auth-status`。トークンの中身は表示しない。
- 401・403 はユーザーに `codex login` を頼む（無人モードでは飛ばして「要確認」に書く）。
- 2026-09-26 に確かめたこと: 参照があるときは編集の窓口（`/images/edits`）へ行く。`--count 2` でも 1 枚しか返らないので、ツールは 1 枚ずつ頼む。頼んだ大きさと実際が違う（1024² → 1254²）ので、決まった大きさが要る素材は `final_size` で縮める。どのモデルが使われたかは返事に無い。透明の背景は効いた（RGBA）。

## 無人運転で使うとき

- 作業一覧の項目が本作独自の 2D 素材を要するときは使ってよい。候補を Read で見て 1 枚を採用し、どれを選んだかと理由を進捗記録の「要確認（ユーザー）」に書く（見た目の仮の値の扱い）。
- 1 回の `gen` は枚数 × 約 1 分かかる。`claude -p` の 600 秒の打ち切りに掛からないよう、1 回の応答で頼むのは合わせて 6 枚までにする。
