#!/usr/bin/env bash
#
# Builds, packages and plays this project on a Mac. The Windows PC keeps making the game; the Mac only receives it,
# cooks it for Metal and runs it. Nothing here needs the editor's UI, the MCP server or the reference data
# (pak_reference*): the assets arrive already built. See .claude/guides/distribution.md「Mac 版のパッケージ」.
#
#     bash Tools/mac_build.sh --sync --run     # git pull → Content を scp → ビルド → パッケージ → 検査 → 起動
#     bash Tools/mac_build.sh --check          # 前提だけ確かめて終わる
#     bash Tools/mac_build.sh --config Shipping
#     bash Tools/mac_build.sh --sync desktop:Desktop/wasami_deception   # 取り込み元を明示する
#
# Steps: 同期（git pull + scp）→ 前提チェック → エディタのビルド → BuildCookRun → 中身の検査 → 起動。
# 同期は Windows の SSH（~/.ssh/config の desktop、Tailscale 越し）を使う。**Windows には何も入れない**:
# 追跡ファイルは GitHub から git pull、git に入らない Content（1.2 GB）は scp で取り直す。scp は sftp
# サブシステムを通るので、Windows の既定シェル（PowerShell）に左右されない。
# Env: UE_ENGINE_DIR  エンジンの Engine フォルダ（既定 "/Users/Shared/Epic Games/UE_5.8/Engine"）
#
# Run it with `bash Tools/mac_build.sh`（git から来たファイルに実行ビットが無くても動く）。
# Written for the bash 3.2 that macOS ships (no `set -u`, no associative arrays).
set -eo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_NAME="wasami_deception"
UPROJECT="$ROOT/$PROJECT_NAME.uproject"
ENGINE="${UE_ENGINE_DIR:-/Users/Shared/Epic Games/UE_5.8/Engine}"
ENGINE_WANTED="5.8"                       # Windows 側は 5.8.2（.claude/implementation-records/00-overview.md）
ARCHIVE="$ROOT/Saved/Archive"
COOKED_META="$ROOT/Saved/Cooked/Mac/$PROJECT_NAME/Metadata/ReferencedSet.txt"
LOG_DIR="$ROOT/Intermediate/MacBuild"

# 取り込み元の既定。WIN_HOST は ~/.ssh/config の別名、WIN_REPO は Windows のホーム（C:\Users\User）からの相対パス。
WIN_HOST="desktop"
WIN_REPO="Desktop/wasami_deception"

# Mac のビルドの間だけ .uproject から外すプラグイン（2026-09-22）。
# - `ModelContextProtocol` は起動のたびに 127.0.0.1:8000 へ HTTP サーバーを立てようとし、塞がっていると
#   `LogHttpListener: Error: HttpListener unable to bind to 127.0.0.1:8000` を 1 件出す。クックのコマンドレットは
#   **エラーが 1 件でもログに出ると失敗を返す**ので、クックの中身が正しくても UAT が ExitCode=25 で落ちる
#   （2026-09-22 に Mac で実際に起きた。`Failure - 1 error(s)` でクックは 3m 42s で完走していた）。Mac では
#   MCP を使わないので外して困らない。`TargetAllowList` が Editor なのでパッケージの中身も変わらない。
# - **`AllToolsets` と `LiveCodingToolset` は外さない**。`AllToolsets` は `GameFeatures` を連れてきており、
#   `Config/DefaultGame.ini` の `GameFeatureData` の規則はそれが読み込まれている前提で書いてある（外すと今度は
#   クラスを解決できずにエラーが出る）。Windows と同じ顔ぶれのままクックするのがいちばん安全。
# - エンジンにそのプラグインが無いとき（`NoRedist` なので配り方によっては入っていない）の対処も、これで兼ねる。
EDITOR_ONLY_PLUGINS="ModelContextProtocol"

# --sync-rsync を使うとき（Windows に rsync を入れた場合）の除外。Intermediate/（9.3 GB）と Binaries/ は Mac で
# 作り直す物、Saved/ は Mac 側の成果物、参照データ（29 GB）はパッケージに要らない。
SYNC_EXCLUDES=(
	--exclude 'Intermediate/'
	--exclude 'Binaries/'
	--exclude 'Saved/'
	--exclude 'DerivedDataCache/'
	--exclude 'pak_reference/'
	--exclude 'pak_reference_2/'
	--exclude 'cc2_reference/'
	--exclude 'tmp/'
	--exclude 'observations/'
	--exclude '__pycache__/'
	--exclude '.DS_Store'
)

ORIG_ARGS=("$@")                          # git pull の後に自分を起動し直すときに渡す
PULLED="${WASAMI_MAC_BUILD_PULLED:-}"     # その起動し直しの目印（pull を繰り返さない）

CONFIG=Development
DO_SYNC=0
DO_GIT=1
RSYNC_SRC=""
SYNC_ONLY=0
CHECK_ONLY=0
DO_PACKAGE=1
DO_RUN=0
FORCE=0
REST=()

usage() { sed -n '2,18p' "${BASH_SOURCE[0]}" | sed 's/^#[ ]\{0,1\}//'; }
say() { printf '\n== %s\n' "$*"; }
note() { printf '   %s\n' "$*"; }
warn() { printf '   ** %s\n' "$*"; }
die() { printf '\n!! %s\n' "$*" >&2; exit 1; }
secs_since() { echo $(($(date +%s) - $1)); }

while [ $# -gt 0 ]; do
	case "$1" in
		--sync)
			DO_SYNC=1
			# 引数は省略できる（省略すると desktop:Desktop/wasami_deception）
			case "$2" in
				""|-*) shift ;;
				*) WIN_HOST="${2%%:*}"; case "$2" in *:*) WIN_REPO="${2#*:}" ;; esac; shift 2 ;;
			esac
			;;
		--sync-rsync) RSYNC_SRC="$2"; [ -n "$RSYNC_SRC" ] || die "--sync-rsync には取り込み元のパスが要る"; shift 2 ;;
		--no-git) DO_GIT=0; REST+=("$1"); shift ;;
		--sync-only) SYNC_ONLY=1; shift ;;
		--check) CHECK_ONLY=1; REST+=("$1"); shift ;;
		--no-package) DO_PACKAGE=0; REST+=("$1"); shift ;;
		--run) DO_RUN=1; REST+=("$1"); shift ;;
		--config) CONFIG="$2"; [ -n "$CONFIG" ] || die "--config には Development か Shipping が要る"; REST+=("$1" "$2"); shift 2 ;;
		--force) FORCE=1; REST+=("$1"); shift ;;
		-h|--help) usage; exit 0 ;;
		*) echo "知らない引数: $1" >&2; usage >&2; exit 2 ;;
	esac
done

# ---- 同期 --------------------------------------------------------------------------------------------------------
# 追跡ファイルは GitHub から、Content は Windows から直に。Content は git の外（1204 ファイル・1.2 GB）で、
# 消えたアセットが残るとクック（bCookAll=True）が必ずそれも焼いて落ちるので、**毎回まるごと取り直す**。
sync_from_windows() {
	local start head_win head_mac dirty tmp
	if [ $DO_GIT -eq 1 ] && [ "$PULLED" != "1" ]; then
		say "同期 1/2: git pull（追跡ファイル）"
		[ -d "$ROOT/.git" ] || die "$ROOT は git のクローンではない（GitHub から clone してから使う。--no-git で飛ばせる）"
		git -C "$ROOT" pull --ff-only
		# この道具自身が新しくなっているかもしれないので、**ここから先は新しい方で走らせる**
		# （Content の取り込みも、その後のビルドも。目印の環境変数で pull は繰り返さない）。
		export WASAMI_MAC_BUILD_PULLED=1
		exec bash "$ROOT/Tools/mac_build.sh" "${ORIG_ARGS[@]}"
	fi
	[ "$PULLED" = "1" ] && note "git pull 済み（取り込んだ版で続けている）"

	say "同期 2/2: Content を $WIN_HOST から取り直す"
	command -v ssh >/dev/null || die "ssh が無い"

	# Windows 側の HEAD と突き合わせる（PowerShell 越しでも壊れない形の引数だけを送る）。出力は CRLF なので \r を落とす。
	local out
	out=$(ssh "$WIN_HOST" "git -C $WIN_REPO rev-parse HEAD; echo ---; git -C $WIN_REPO status --porcelain" | tr -d '\r') \
		|| die "$WIN_HOST に ssh できない（~/.ssh/config の別名と Tailscale を確かめる）"
	head_win=$(echo "$out" | sed -n '1p')
	dirty=$(echo "$out" | sed -n '/^---$/,$p' | sed '1d' | grep -c . || true)
	head_mac=$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo "(git 無し)")
	note "Windows の HEAD: ${head_win:0:7} / この Mac: ${head_mac:0:7}"
	if [ "$head_win" != "$head_mac" ]; then
		[ $FORCE -eq 1 ] || die "Windows と Mac で HEAD が違う（Windows に未 push のコミットがある見込み）。Windows で git push してから、もう一度。承知のうえなら --force"
		warn "HEAD が違うまま続ける（--force）。コードとアセットが食い違うかもしれない"
	fi
	[ "$dirty" = "0" ] || warn "Windows 側に未コミットの変更が $dirty 件ある（作業中かもしれない。同期は Windows が止まっているときに取る）"

	# scp は sftp サブシステムを通す（OpenSSH 9 以降は既定。8 以前は -s を付けないとログインシェルを通ってしまう）。
	local proto="" major
	major=$(ssh -V 2>&1 | sed -n 's/^OpenSSH_\([0-9]*\).*/\1/p')
	if [ -n "$major" ] && [ "$major" -lt 9 ] 2>/dev/null; then
		proto="-s"
	fi

	start=$(date +%s)
	tmp="$ROOT/Content.new"
	rm -rf "$tmp"
	# 1204 ファイル・1.2 GB を黙って運ぶと生きているのか分からないので、5 秒ごとに受け取った量を出す。
	# 総量の見込みには前回の Content の大きさを使う（初回は出さない）。ServerAlive で、切れた接続は 1 分ほどで諦める。
	local total_kb=0
	[ -d "$ROOT/Content" ] && total_kb=$(du -sk "$ROOT/Content" | cut -f1)
	scp -rpq $proto -o ServerAliveInterval=15 -o ServerAliveCountMax=4 "$WIN_HOST:$WIN_REPO/Content" "$tmp" &
	local scp_pid=$!
	trap 'kill '"$scp_pid"' 2>/dev/null; exit 130' INT TERM
	while kill -0 "$scp_pid" 2>/dev/null; do
		sleep 5
		local now_kb
		now_kb=$(du -sk "$tmp" 2>/dev/null | cut -f1)
		[ -n "$now_kb" ] || now_kb=0
		if [ "$total_kb" -gt 0 ]; then
			local pct=$((now_kb*100/total_kb))
			[ "$pct" -gt 99 ] && pct=99      # 見込みは前回の大きさなので、超えたら 99 で止める
			printf '\r   取り込み中: %d / %d MB (%d%%)   ' $((now_kb/1024)) $((total_kb/1024)) "$pct"
		else
			printf '\r   取り込み中: %d MB   ' $((now_kb/1024))
		fi
	done
	printf '\r%*s\r' 60 ""
	trap - INT TERM
	wait "$scp_pid" || { rm -rf "$tmp"; die "scp に失敗した（$WIN_HOST:$WIN_REPO/Content）"; }
	[ -f "$tmp/Stage/Maps/L_Title.umap" ] || { rm -rf "$tmp"; die "取ってきた Content に L_Title.umap が無い（取り込み元のパスを確かめる）"; }
	rm -rf "$ROOT/Content"
	mv "$tmp" "$ROOT/Content"
	note "$(du -sh "$ROOT/Content" | cut -f1) を $(secs_since "$start") 秒で取り直した"
}

if [ -n "$RSYNC_SRC" ]; then
	# Windows に rsync を入れたときの差分同期（既定は使わない。distribution.md「別の運び方」）。
	# rsync は既定で一時ファイルに書いてから rename するので、走っている最中の自分自身を入れ替えても走り続ける。
	say "同期: $RSYNC_SRC → $ROOT（rsync）"
	command -v rsync >/dev/null || die "rsync が無い"
	START=$(date +%s)
	rsync -rlt --delete --human-readable "${SYNC_EXCLUDES[@]}" "${RSYNC_SRC%/}/" "$ROOT/"
	note "$(secs_since "$START") 秒"
	if [ $SYNC_ONLY -eq 1 ]; then
		exit 0
	fi
	# 取り込みで自分自身が新しくなっているかもしれないので、続きは新しい方で走らせる。
	exec bash "$ROOT/Tools/mac_build.sh" "${REST[@]}"
elif [ $DO_SYNC -eq 1 ]; then
	sync_from_windows          # git pull の直後に、この道具の新しい版で起動し直す
	if [ $SYNC_ONLY -eq 1 ]; then
		exit 0
	fi
fi

# ---- 前提チェック ------------------------------------------------------------------------------------------------
say "前提チェック"
[ "$(uname -s)" = "Darwin" ] || die "macOS で動かす道具（Windows 側は Tools/editor_cycle.py と .claude/guides/distribution.md）"
[ -f "$UPROJECT" ] || die "$UPROJECT が無い"
[ -x "$ENGINE/Build/BatchFiles/Mac/Build.sh" ] || die "エンジンが $ENGINE に無い（UE_ENGINE_DIR で指定できる）"
[ -x "$ENGINE/Build/BatchFiles/RunUAT.sh" ] || die "RunUAT.sh が $ENGINE/Build/BatchFiles に無い"

# Build.version（JSON、1 行 1 キー）から版を読む。python3 には頼らない（macOS では Xcode の入り具合で
# 呼べないことがあり、2026-09-22 に版の表示が化けた）。
version_field() { sed -n "s/.*\"$1\"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p" "$ENGINE/Build/Build.version" | tr -d '\r' | head -1; }
ENGINE_VERSION="$(version_field MajorVersion).$(version_field MinorVersion).$(version_field PatchVersion)"
case "$ENGINE_VERSION" in
	[0-9]*.[0-9]*.[0-9]*) ;;
	*) die "$ENGINE/Build/Build.version から版が読めない（読めたのは '$ENGINE_VERSION'）" ;;
esac
note "エンジン: $ENGINE_VERSION（$ENGINE）"
case "$ENGINE_VERSION" in
	"$ENGINE_WANTED".*) ;;
	*)
		[ $FORCE -eq 1 ] || die "エンジンが $ENGINE_WANTED 系でない（Windows 側は $ENGINE_WANTED 系。アセットは開けない）。承知のうえなら --force"
		warn "版が違うまま続ける（--force）"
		;;
esac

command -v xcodebuild >/dev/null || die "Xcode が無い（入れてから 'sudo xcode-select -s /Applications/Xcode.app' と 'sudo xcodebuild -license accept'）"
note "Xcode: $(xcodebuild -version 2>/dev/null | head -1)"
xcrun -sdk macosx metal --version >/dev/null 2>&1 || die "Metal のコンパイラが呼べない（シェーダーをクックできない）。'xcodebuild -downloadComponent MetalToolchain' で入れる"

[ -f "$ROOT/Content/Stage/Maps/L_Title.umap" ] || die "Content/ が入っていない（L_Title.umap が無い）。--sync で取り込む"
PACKAGES=$(find "$ROOT/Content" \( -name '*.uasset' -o -name '*.umap' \) | wc -l | tr -d ' ')
note "Content: $PACKAGES パッケージ"

# エディタ専用のプラグインは .uproject から外してビルドし、終わったら必ず戻す。
STRIP=""
for name in $EDITOR_ONLY_PLUGINS; do
	if grep -q "\"$name\"" "$UPROJECT"; then
		STRIP="$STRIP $name"
	fi
done

restore_uproject() {
	if [ -f "$UPROJECT.macbuild.bak" ]; then
		mv -f "$UPROJECT.macbuild.bak" "$UPROJECT"
		note ".uproject を元に戻した"
	fi
}

if [ -n "$STRIP" ]; then
	note "ビルドの間だけ .uproject から外すプラグイン:$STRIP（Mac では使わない。パッケージの中身は変わらない）"
	command -v python3 >/dev/null || die "python3 が無いので .uproject を書き換えられない。手で$STRIP の項目を .uproject から外してから走らせる（終わったら戻す）"
fi

if [ $CHECK_ONLY -eq 1 ]; then
	say "前提は揃っている"
	exit 0
fi

if [ -n "$STRIP" ]; then
	cp "$UPROJECT" "$UPROJECT.macbuild.bak"
	trap restore_uproject EXIT INT TERM
	python3 - "$UPROJECT" $STRIP <<'PY'
import json, sys
path, names = sys.argv[1], set(sys.argv[2:])
with open(path, encoding="utf-8") as f:
	data = json.load(f)
data["Plugins"] = [p for p in data.get("Plugins", []) if p.get("Name") not in names]
with open(path, "w", encoding="utf-8") as f:
	json.dump(data, f, indent="\t", ensure_ascii=False)
	f.write("\n")
PY
fi

mkdir -p "$LOG_DIR"

# ---- エディタのビルド --------------------------------------------------------------------------------------------
# クックはエディタのコマンドレットが走るので、遊ぶだけでもエディタのビルドは要る。
say "エディタのビルド（${PROJECT_NAME}Editor Mac Development）"
START=$(date +%s)
"$ENGINE/Build/BatchFiles/Mac/Build.sh" "${PROJECT_NAME}Editor" Mac Development -Project="$UPROJECT" 2>&1 | tee "$LOG_DIR/build_editor.log"
note "$(secs_since "$START") 秒（ログ: $LOG_DIR/build_editor.log）"

if [ $DO_PACKAGE -eq 0 ]; then
	say "パッケージは作らない（--no-package）"
	exit 0
fi

# ---- パッケージ --------------------------------------------------------------------------------------------------
# 初回は Metal のシェーダーを全部コンパイルするので数時間かかることがある。2 回目からは変わった分だけ。
say "パッケージ（BuildCookRun -platform=Mac -clientconfig=$CONFIG）"
START=$(date +%s)
# 落ちたときは、ログのどこを見ればよいかをその場で出す。クックのコマンドレットは**エラーが 1 件でもログに
# 出ると失敗を返す**ので、クックの中身が正しくても UAT は ExitCode=25 で落ちる（症状索引）。
if ! "$ENGINE/Build/BatchFiles/RunUAT.sh" BuildCookRun \
	-project="$UPROJECT" \
	-noP4 -platform=Mac -clientconfig="$CONFIG" \
	-cook -build -stage -pak -archive \
	-archivedirectory="$ARCHIVE" 2>&1 | tee "$LOG_DIR/uat_package.log"
then
	say "パッケージに失敗した（$(secs_since "$START") 秒）。エラーの要約:"
	sed -n '/Warning\/Error Summary/,/Failure - /p' "$LOG_DIR/uat_package.log" | head -30
	echo
	note "Error の行:"
	grep -nE ": (Error|Fatal)" "$LOG_DIR/uat_package.log" | grep -v "0 error" | head -20 || true
	die "ログ全体: $LOG_DIR/uat_package.log"
fi
note "$(secs_since "$START") 秒（ログ: $LOG_DIR/uat_package.log）"

# ---- 中身の検査 --------------------------------------------------------------------------------------------------
# BUILD SUCCESSFUL は中身を保証しない（2026-09-21 に Windows で、/Game が L_Title 1 つしか入らないパッケージが
# 黙って出来た。.claude/guides/distribution.md）。コンテナに入ったパッケージの数を Content と突き合わせる。
say "中身の検査"
[ -f "$COOKED_META" ] || die "$COOKED_META が無い（クックが走っていない）"
COOKED=$(grep -c '^/game/' "$COOKED_META" || true)
note "/Game のパッケージ: クック $COOKED / Content $PACKAGES"
[ "$COOKED" = "$PACKAGES" ] || die "数が合わない。本編が入っていないか、消したはずのアセットが Mac に残っている（--sync は Content をまるごと取り直す）"

APP=$(find "$ARCHIVE/Mac" -maxdepth 1 -name '*.app' 2>/dev/null | head -1)
[ -n "$APP" ] || die "$ARCHIVE/Mac に .app が無い"

# ---- 足りない dylib を同梱する ------------------------------------------------------------------------------------
# UE 5.8 の Mac のステージは、本体が @rpath で読む ThirdParty の dylib を .app に入れてくれない（2026-09-22。
# Windows は同じものを Binaries/Win64/tbb12.dll として入れている）。エンジンへ戻る rpath
# （@loader_path/../…×8/Shared/Epic Games/…）は `<Project>/Binaries/Mac/` に置かれた .app の深さ向けなので、
# `Saved/Archive/Mac/` の成果物からは 1 階層ずれて届かない。そこで、実行ファイルの隣（rpath の @loader_path/）へ
# エンジンから拾って置く。置いた後は ad-hoc で署名し直す（Apple Silicon は署名が壊れた bundle を起動しない）。
copy_missing_dylibs() {
	local app="$1" pass=0 copied=1 target lib found
	while [ $copied -eq 1 ] && [ $pass -lt 4 ]; do
		copied=0
		pass=$((pass + 1))
		for target in "$app/Contents/MacOS/$PROJECT_NAME" "$app/Contents/MacOS/"*.dylib; do
			[ -f "$target" ] || continue
			for lib in $(otool -L "$target" 2>/dev/null | sed -n 's/^[[:space:]]*@rpath\/\([^ ]*\.dylib\).*/\1/p'); do
				if [ -f "$app/Contents/MacOS/$lib" ]; then
					continue
				fi
				found=$(find "$ENGINE/Binaries/ThirdParty" "$ENGINE/Source/ThirdParty" "$ENGINE/Plugins" \
					-name "$lib" -type f 2>/dev/null | head -1)
				if [ -n "$found" ]; then
					cp "$found" "$app/Contents/MacOS/"
					note "同梱した: $lib"
					copied=1
				else
					warn "エンジンに見つからない: $lib（起動時に落ちる）"
				fi
			done
		done
	done
}

BEFORE=$(find "$APP/Contents/MacOS" -maxdepth 1 -name '*.dylib' | wc -l | tr -d ' ')
copy_missing_dylibs "$APP"
AFTER=$(find "$APP/Contents/MacOS" -maxdepth 1 -name '*.dylib' | wc -l | tr -d ' ')
if [ "$AFTER" != "$BEFORE" ]; then
	codesign --force --sign - "$APP" >/dev/null 2>&1 || warn "ad-hoc の署名し直しに失敗した（起動できないときは codesign --force --sign - \"$APP\"）"
	note "dylib を $((AFTER - BEFORE)) 個入れて署名し直した"
fi

note "出来上がり: $APP（$(du -sh "$APP" | cut -f1)）"

# ---- 起動 --------------------------------------------------------------------------------------------------------
if [ $DO_RUN -eq 1 ]; then
	say "起動: $APP"
	note "ログは ~/Library/Logs/$PROJECT_NAME/$PROJECT_NAME.log"
	open "$APP"
else
	say "出来た。遊ぶには:"
	note "open \"$APP\""
	note "ログを見ながらなら: \"$APP/Contents/MacOS/$PROJECT_NAME\""
fi
