# WASAMI DECEPTION（UE5 版）— Claude Code 索引

Dark Deception のワサミ版ファンゲームの UE 5.8 版。WebGL 版（Babylon.js）の後継で、エディタを MCP 経由で操作して作る。目標は最初から最後まで遊べるゲーム。
このファイルは `.claude/` 配下の索引だけを持つ。本文はここに書かず、各ファイルを参照する。

## 必読（作業前）
- 対話と報告の言語（**セッション内の対話・応答・実装報告はすべて日本語**）: `.claude/guides/communication.md`
- 実装進捗記録（**ファイルやアセットの変更を伴う作業は `.claude/progress/` に進捗を先に書いてから進める。セッション開始・/clear・圧縮の後は、まず未完了の記録と main 以外のローカルブランチを確認して再開する（`/continue`）。実装が 1 つ終わったら `.claude/` に記録を残し、ユーザーに `/clear` をお願いする。大規模な実装はステップに分け、ステップごとに同じことをする。記録は続きに要ることだけにし、ステップを閉じるときにその分を畳む〈目安 20 KB・上限 30 KB。今も効く決定は実装記録へ移してから消す〉**）: `.claude/guides/progress-tracking.md`
- 作業一覧（**最終目標〈2026-09-17〉を 21 項目に分解した順序つきの一覧。1 項目 = 進捗記録 1 件。未完了の記録が無いときはここの「未着手」で依存が満たされた最初の項目から始める**）: `.claude/roadmap.md`
- 中断した実装の再開（**未完了の記録と git・エディタの状態を照合してから、次のステップを 1 つ実行する**）: `/continue`（`.claude/skills/continue/SKILL.md`）
- 実装記録（**ソースを変更したら対応する記録を直し、`python .claude/scripts/check_records.py --update` を通す**）: `.claude/guides/implementation-records.md`、索引は `.claude/implementation-records/_index.md`
- 無人運転（**夜間は駆動役 `Tools/overnight.py` が `/continue` を繰り返す。無人モード〈`WASAMI_UNATTENDED=1`〉ではユーザーに質問せず、本家のコード → 実機 → WebGL 版 → 仮の値の順に決めて「要確認（ユーザー）」にまとめ、ステップを終えたら `/clear` を頼まずに状態ファイルを書いて応答を終える。ゲームの規則の値が本家のコードに無いときと、変更を捨てる操作・配布・本家のセーブの編集は行わずに飛ばす**）: `.claude/guides/autonomy.md`
- 症状索引（**エラーやおかしな挙動に会ったら先に grep する。解決にエディタの開き直し 1 回以上か 30 分以上かかったら、次に進む前にその場で書く**）: `.claude/references/troubleshooting.md`
- エディタの操作（**MCP のツールは 1 つずつ順に呼び、結果を必ず確かめる。一括の変更の前後で保存する。取り込みとステージの組み立ては `Content/Python/wasami_tools` のツールセットを MCP から呼ぶ。C++ のビルドの手順**）: `.claude/guides/unreal-workflow.md`
- 検証（**作業の許可は求めずに進める（エディタの開き直し〈C++ のビルドのための `Tools/editor_cycle.py` も、C++ を書き終えたら尋ねずに走らせる〉・PIE・エディタへの入力・本家の起動。明示的な禁止があれば従う）。ただし変更を捨てる操作・配布・本家のセーブの編集は先に確認する。エディタはユーザーのアプリでもあるので、閉じる前に保存し、閉じたら開き直す。PIE は必ず止める。本家はエディタと同時に動かさず、終わったら閉じる。OS 全体の入力は操作しない。**画面の操作は `Tools/desktop.py` で Claude が行う**（入力は許可した窓だけ）。MOD は観察の足場までで、World Editor は使わない**）: `.claude/guides/verification.md`
- 観察の手順（**本家と PIE を同じ場所・同じ速さ・同じ撮り方・同じ測り方で比べる台本。撮るものの一覧を先に作り、本家は 1 回・30 分を目安に撮り終えて閉じる。MOD のメニューは上端に戻してから表の座標で押す（W-Editor は押さない）。無敵は当てにせず敵は消す。PIE は `Tools/pie.py`、測るのは `Tools/video_probe.py`**）: `.claude/guides/observation.md`
- 原作への忠実さ（**本家 Dark Deception の原作データ `pak_reference/` に忠実に倣う。ステージも本家の病院「Torment Therapy」の Zone 1・Zone 2（`pak_reference_2/` の `06_Hospital_Zone_01`・`06_Hospital_Zone_02`。入口 `06_Hospital` は作らず、Zone 1 のエレベーターの到着から始める〈2026-09-18〉。ボス戦は作らず、Zone 2 のガレージの祭壇とポータルで脱出）。本家に無いギミックは複雑にせず、壊せる物は本家のホテルの板張りのバリケードのように 1 クリックで崩れて消える。原作のロゴとキャラクターのモデルは使わない。UE に同じ仕組みがあれば値を写すだけにし、作り直さない。視覚的な比較が要るときは手元で遊べる本家（旧版は `pak_reference`、最新版は `pak_reference_2` と同一のビルド。病院は最新版だけ）で観察してよいが、根拠は原則コード**）: `.claude/guides/original-fidelity.md`
- コミットとブランチ（**実装ごとにコミット、大規模改修は作業ブランチ→main へマージ→ローカルブランチ削除、最終コミットから 10 分経過・未 push 2 件以上・大規模改修のマージ後のいずれかで main を push。参照データから作り直せる素材は git の外、手作りのアセットは Git LFS**）: `.claude/guides/git-workflow.md`
- 性能とメモリ（**この PC は GTX 1660 SUPER の VRAM 6 GB・RAM 32 GB。開発中は VRAM と RAM の逼迫を避ける設定にし、そのための設定はエディタにだけ効く場所に置く。パッケージした本編の品質は落とさない**）: `.claude/guides/performance.md`
- note の進捗記事（**実装が 1 つ終わるたびに、原稿 `docs/note/progress.md` を「いま何が出来るか」に合わせて簡潔に直し、`tmp/note-cli` で同じ記事〈id はガイドに〉を書き換える。新しい記事は作らず、削除はしない。セッションの値はファイルに書かず、会話で渡されていなければ原稿だけ直して「note へは未反映」と報告する**）: `.claude/guides/note-progress.md`
- 配布とパッケージ（**パッケージには原作の素材が入る。配布の話が出たら必ずユーザーに確認する**）: `.claude/guides/distribution.md`

## 参考資料
- WebGL 版からの引き継ぎ調査（何を持ち越し、何を UE に置き換えるか、マイルストーン、ユーザーの決定、現状と次の一歩）: `.claude/references/handover.md`
- WebGL 版の仕様と実装記録（ゲームの流れ・値・ユーザーの決定の経緯。**ゲームの規則の正本**）: `.claude/references/webgl/README.md`、`.claude/references/webgl/implementation-records/`（00 が全体像）
- 本家の原作データ（UE 4.21。**実装値の根拠として最優先**）: `pak_reference/README.md`
- 手元で遊べる本家 2 つ（旧版 = `pak_reference` と同一、最新版 = `pak_reference_2` と同一。コードで確定できない見え方の観察に使う。起動の作法は verification.md、どちらで何を観察するかとステージの出し方は original-fidelity.md）: `C:\Users\User\AppData\Local\DDeception\Launch-Classic-Ch3.cmd`・`C:\Users\User\AppData\Local\DDeception\Launch-Latest.cmd`
- 敵ワサミ・ボスワサミ・ワサミ餅のモデルとモーション（**`tmp/enemy_wasami_v3`・`boss_wasami`・`wasami_mochi_v3` の中身と、役 → アニメの対応・追跡中のランダムの動き・場面の代用。コードは役の名前でアニメを選ぶ**）: `.claude/references/enemy-wasami-motions.md`
- タブレットのパワー 6 種の原作調査（枠・入力・クールダウン・強化段階、各パワーの処理と演出。テレポーテーションは旧版、ほかは最新版）: `.claude/references/powers/README.md`
- 本家の新しい版（UE 4.24。pak_reference に無いものの根拠。両方にあって違うものはユーザーに確認）: `pak_reference_2/README.md`
- 旧方針のステージ（ファンゲーム CC2）の調査（2026-09-16 に不採用。経緯として残す）: `.claude/references/chaotic-customer-2/README.md`（書き出しの本体は `cc2_reference/`）
- 本家の用語・敵・仕組み・演出と本作との対応（Web 調査。[A]/[B] だけを値の根拠にする）: `.claude/references/dark-deception/README.md`

## 構成
- `Source/wasami_deception/` … ゲームの C++ モジュール
- `Content/Python/` … エディタの Python（`init_unreal.py` がプロジェクトのツールセットを登録する）
- `Tools/` … エディタの外で動くスクリプト（ステージの前処理 `Tools/dd/`、リモート実行 `Tools/ue_remote.py`、開き直し `Tools/editor_cycle.py`）
- `.claude/scripts/` … 運用の仕組み（実装記録の同期チェックと hooks）
- `.claude/skills/` … スラッシュコマンド（`/continue` … 中断した実装の再開）
- `Intermediate/Pipeline/` … 前処理の出力（git の対象外、作り直せる）
- `pak_reference/`・`pak_reference_2/`・`cc2_reference/` … 原作データ（git の対象外、読み取り専用）

## よく使うコマンド
- ステージの前処理: `python Tools/dd/prepare_stage.py`
- エディタで Python を実行: `python Tools/ue_remote.py <file.py>`
- エディタを閉じて C++ をビルドし開き直す: `python Tools/editor_cycle.py`
- 対話デスクトップ（コンソールのセッション）でプログラムを起動する: `python Tools/console_session.py <exe> [--wait <画像名>]`
- PIE を始める・プレイヤーを置く・コンソールコマンド・止める: `python Tools/pie.py start` → `place X Y --yaw N` / `cmd "slomo 0.25"` / `state` → `stop`
- 収録を測る（本家と PIE で同じ測り方）: `python Tools/video_probe.py frames` / `sheet` / `series` / `period`
- 画面を撮る・入力を送る（対話デスクトップ）: `python Tools/desktop.py start` → `shot` / `click` / `key` / `hold` / `look` → `stop`
- 実装記録の同期チェック / ハッシュ更新: `python .claude/scripts/check_records.py [--update]`
- 夜間の無人運転（Claude Code の外の端末から）: `python Tools/overnight.py --until 07:00 --usage-cmd "<使用量を JSON で出すコマンド>"`（`--no-usage-check` / `--dry-run` / `--max-iterations N` / `--no-discord`。応答と本作の画面は送り主「Claude」で Discord の webhook へも送る〈`Tools/discord_notify.py`。URL は git の外の `Tools/overnight.local.json`〉。決まりは `.claude/guides/autonomy.md`）
