#!/usr/bin/env bash
#
# Builds, packages and plays this project on a Mac. The Windows PC keeps making the game; the Mac only receives it,
# cooks it for Metal and runs it. Nothing here needs the editor's UI, the MCP server or the reference data
# (pak_reference*): the assets arrive already built. See .claude/guides/distribution.md「Mac 版のパッケージ」.
#
#     bash Tools/mac_build.sh --sync /Volumes/wasami_deception   # 取り込む → ビルド → パッケージ → 検査
#     bash Tools/mac_build.sh --sync user@windows:/Users/User/Desktop/wasami_deception
#     bash Tools/mac_build.sh --check                            # 前提だけ確かめて終わる
#     bash Tools/mac_build.sh --run                              # ビルドして、出来た .app を起動する
#     bash Tools/mac_build.sh --config Shipping
#
# Steps: 同期（rsync）→ 前提チェック → エディタのビルド → BuildCookRun → 中身の検査 → 起動。
# Env: UE_ENGINE_DIR  エンジンの Engine フォルダ（既定 "/Users/Shared/Epic Games/UE_5.8/Engine"）
#
# Run it with `bash Tools/mac_build.sh`: the file arrives over rsync from Windows, where it has no execute bit.
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

# Editor-only plugins that may be missing from the Mac engine (all three are NoRedist). The .uproject allow-lists
# them to the editor, so leaving them out changes nothing in the package — only the tools Windows uses.
OPTIONAL_PLUGINS="ModelContextProtocol AllToolsets LiveCodingToolset"

# rsync: what never travels. Intermediate/（9.3 GB）と Binaries/ は Mac で作り直す物、Saved/ は Mac 側の成果物、
# 参照データ（29 GB）はパッケージに要らない。
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

CONFIG=Development
SYNC_SRC=""
SYNC_ONLY=0
CHECK_ONLY=0
DO_PACKAGE=1
DO_RUN=0
FORCE=0
REST=()

usage() { sed -n '2,15p' "${BASH_SOURCE[0]}" | sed 's/^#[ ]\{0,1\}//'; }
say() { printf '\n== %s\n' "$*"; }
note() { printf '   %s\n' "$*"; }
die() { printf '\n!! %s\n' "$*" >&2; exit 1; }
secs_since() { echo $(($(date +%s) - $1)); }

while [ $# -gt 0 ]; do
	case "$1" in
		--sync) SYNC_SRC="$2"; [ -n "$SYNC_SRC" ] || die "--sync には取り込み元のパスが要る"; shift 2 ;;
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
# rsync は既定で一時ファイルに書いてから rename するので、走っている最中の自分自身を入れ替えても走り続ける
# （--inplace を足すとそれが崩れる）。それでも取り込んだ後の手順は新しい方でやりたいので、同期の後に自分を
# 起動し直す。-rlt にしているのは、SMB で共有したフォルダから取るときに所有者とパーミッションで転けないため。
if [ -n "$SYNC_SRC" ]; then
	START=$(date +%s)
	say "同期: $SYNC_SRC → $ROOT"
	command -v rsync >/dev/null || die "rsync が無い"
	rsync -rlt --delete --human-readable "${SYNC_EXCLUDES[@]}" "${SYNC_SRC%/}/" "$ROOT/"
	note "$(secs_since "$START") 秒"
	if [ $SYNC_ONLY -eq 1 ]; then
		exit 0
	fi
	exec bash "$ROOT/Tools/mac_build.sh" "${REST[@]}"
fi

# ---- 前提チェック ------------------------------------------------------------------------------------------------
say "前提チェック"
[ "$(uname -s)" = "Darwin" ] || die "macOS で動かす道具（Windows 側は Tools/editor_cycle.py と .claude/guides/distribution.md）"
[ -f "$UPROJECT" ] || die "$UPROJECT が無い（--sync で取り込む）"
[ -x "$ENGINE/Build/BatchFiles/Mac/Build.sh" ] || die "エンジンが $ENGINE に無い（UE_ENGINE_DIR で指定できる）"
[ -x "$ENGINE/Build/BatchFiles/RunUAT.sh" ] || die "RunUAT.sh が $ENGINE/Build/BatchFiles に無い"

ENGINE_VERSION="$(python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); print("%d.%d.%d" % (d["MajorVersion"], d["MinorVersion"], d["PatchVersion"]))' "$ENGINE/Build/Build.version")"
note "エンジン: $ENGINE_VERSION（$ENGINE）"
case "$ENGINE_VERSION" in
	"$ENGINE_WANTED".*) ;;
	*)
		[ $FORCE -eq 1 ] || die "エンジンが $ENGINE_WANTED 系でない（Windows 側は $ENGINE_WANTED 系。アセットは開けない）。承知のうえなら --force"
		note "警告: 版が違うまま続ける（--force）"
		;;
esac

command -v xcodebuild >/dev/null || die "Xcode が無い（入れてから 'sudo xcode-select -s /Applications/Xcode.app' と 'sudo xcodebuild -license accept'）"
note "Xcode: $(xcodebuild -version 2>/dev/null | head -1)"
xcrun -sdk macosx metal --version >/dev/null 2>&1 || die "Metal のコンパイラが呼べない（シェーダーをクックできない）。'xcodebuild -downloadComponent MetalToolchain' で入れる"

[ -f "$ROOT/Content/Stage/Maps/L_Title.umap" ] || die "Content/ が入っていない（L_Title.umap が無い）。--sync で取り込む"
PACKAGES=$(find "$ROOT/Content" \( -name '*.uasset' -o -name '*.umap' \) | wc -l | tr -d ' ')
note "Content: $PACKAGES パッケージ"

# 足りないプラグインは .uproject から外してビルドし、終わったら必ず戻す。
MISSING=""
for name in $OPTIONAL_PLUGINS; do
	grep -q "\"$name\"" "$UPROJECT" || continue
	found=$(find "$ENGINE/Plugins" -maxdepth 4 -type d -name "$name" 2>/dev/null | head -1)
	[ -n "$found" ] || MISSING="$MISSING $name"
done

restore_uproject() {
	if [ -f "$UPROJECT.macbuild.bak" ]; then
		mv -f "$UPROJECT.macbuild.bak" "$UPROJECT"
		note ".uproject を元に戻した"
	fi
}

if [ -n "$MISSING" ]; then
	note "このエンジンに無いプラグイン:$MISSING → ビルドの間だけ .uproject から外す（パッケージの中身は変わらない）"
	cp "$UPROJECT" "$UPROJECT.macbuild.bak"
	trap restore_uproject EXIT INT TERM
	python3 - "$UPROJECT" $MISSING <<'PY'
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

if [ $CHECK_ONLY -eq 1 ]; then
	say "前提は揃っている"
	exit 0
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
"$ENGINE/Build/BatchFiles/RunUAT.sh" BuildCookRun \
	-project="$UPROJECT" \
	-noP4 -platform=Mac -clientconfig="$CONFIG" \
	-cook -build -stage -pak -archive \
	-archivedirectory="$ARCHIVE" 2>&1 | tee "$LOG_DIR/uat_package.log"
note "$(secs_since "$START") 秒（ログ: $LOG_DIR/uat_package.log）"

# ---- 中身の検査 --------------------------------------------------------------------------------------------------
# BUILD SUCCESSFUL は中身を保証しない（2026-09-21 に Windows で、/Game が L_Title 1 つしか入らないパッケージが
# 黙って出来た。.claude/guides/distribution.md）。コンテナに入ったパッケージの数を Content と突き合わせる。
say "中身の検査"
[ -f "$COOKED_META" ] || die "$COOKED_META が無い（クックが走っていない）"
COOKED=$(grep -c '^/game/' "$COOKED_META" || true)
note "/Game のパッケージ: クック $COOKED / Content $PACKAGES"
[ "$COOKED" = "$PACKAGES" ] || die "数が合わない。本編が入っていないか、消したはずのアセットが Mac に残っている（同期は --delete 付きで取り込む）"

APP=$(find "$ARCHIVE/Mac" -maxdepth 1 -name '*.app' 2>/dev/null | head -1)
[ -n "$APP" ] || die "$ARCHIVE/Mac に .app が無い"
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
