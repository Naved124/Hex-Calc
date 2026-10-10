#!/usr/bin/env bash
#
# Hex Calc installer.
#
#   ./install.sh               build from source and install to ~/.local
#   ./install.sh --binary      install the prebuilt release instead of building
#   ./install.sh --uninstall   remove Hex Calc
#
# Also works straight from the web:
#   curl -fsSL https://raw.githubusercontent.com/naved124/hex-calc/main/install.sh | bash
#
set -euo pipefail

REPO="naved124/hex-calc"
BRANCH="${HEXCALC_BRANCH:-main}"
APP_ID="io.github.naved124.HexCalc"
RELEASE_ASSET="hex-calc-linux-x86_64.tar.gz"

PREFIX="${PREFIX:-$HOME/.local}"
MODE="source"
ASSUME_YES=0
WITH_DEPS=1
PURGE=0
WORKDIR=""

usage() {
    cat <<EOF
Hex Calc installer

Usage: install.sh [options]

  --binary         Install the prebuilt release (x86_64) instead of building
  --prefix DIR     Install under DIR (default: ~/.local)
  --uninstall      Remove Hex Calc from the prefix
  --purge          With --uninstall, also delete settings and history
  --no-deps        Never install packages, just report what is missing
  -y, --yes        Don't ask before installing packages
  -h, --help       Show this help
EOF
}

info() { printf '\033[1;35m==>\033[0m \033[1m%s\033[0m\n' "$*"; }
warn() { printf '\033[1;33mwarning:\033[0m %s\n' "$*" >&2; }
die() { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; exit 1; }

cleanup() {
    if [ -n "$WORKDIR" ] && [ -d "$WORKDIR" ]; then
        rm -rf "$WORKDIR"
    fi
}
trap cleanup EXIT

while [ $# -gt 0 ]; do
    case "$1" in
        --binary) MODE="binary" ;;
        --uninstall) MODE="uninstall" ;;
        --purge) PURGE=1 ;;
        --prefix)
            [ $# -ge 2 ] || die "--prefix needs a directory"
            PREFIX="$2"
            shift
            ;;
        --prefix=*) PREFIX="${1#--prefix=}" ;;
        --no-deps) WITH_DEPS=0 ;;
        -y|--yes) ASSUME_YES=1 ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; die "unknown option: $1" ;;
    esac
    shift
done

PREFIX="${PREFIX%/}"
[ -n "$PREFIX" ] || die "the prefix must not be empty"
BINDIR="$PREFIX/bin"
SHAREDIR="$PREFIX/share"

# Use sudo only when the prefix isn't writable (e.g. --prefix /usr/local).
SUDO=""
prefix_needs_root() {
    local dir="$PREFIX"
    while [ ! -d "$dir" ]; do dir="$(dirname "$dir")"; done
    [ ! -w "$dir" ]
}
if [ "$(id -u)" -ne 0 ] && prefix_needs_root; then
    command -v sudo >/dev/null 2>&1 || die "$PREFIX is not writable and sudo is not available"
    SUDO="sudo"
fi

ask() {
    local answer
    [ "$ASSUME_YES" -eq 1 ] && return 0
    # stdin may be the script itself (curl | bash), so read from the terminal.
    if [ -r /dev/tty ] && [ -w /dev/tty ]; then
        printf '%s [Y/n] ' "$1" > /dev/tty
        read -r answer < /dev/tty || answer="n"
        case "$answer" in [nN]*) return 1 ;; *) return 0 ;; esac
    fi
    return 1
}

as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo "$@"
    else
        die "need root to run: $*"
    fi
}

download() { # url dest
    if command -v curl >/dev/null 2>&1; then
        curl -fL --retry 3 --progress-bar -o "$2" "$1"
    elif command -v wget >/dev/null 2>&1; then
        wget -q --show-progress -O "$2" "$1"
    else
        die "curl or wget is required to download Hex Calc"
    fi
}

# --------------------------------------------------------------------------
# Packages

PKG_MANAGER=""
detect_pkg_manager() {
    local pm
    for pm in apt-get dnf yum pacman zypper apk xbps-install; do
        if command -v "$pm" >/dev/null 2>&1; then
            PKG_MANAGER="$pm"
            return
        fi
    done
}

build_packages() {
    case "$PKG_MANAGER" in
        apt-get)
            if apt-cache show qt6-base-dev >/dev/null 2>&1; then
                echo "build-essential cmake pkg-config qt6-base-dev libgl1-mesa-dev"
            else
                echo "build-essential cmake pkg-config qtbase5-dev"
            fi ;;
        dnf|yum) echo "gcc gcc-c++ make cmake qt6-qtbase-devel" ;;
        pacman) echo "base-devel cmake qt6-base" ;;
        zypper) echo "gcc gcc-c++ make cmake qt6-base-devel" ;;
        apk) echo "build-base cmake qt6-qtbase-dev" ;;
        xbps-install) echo "gcc make cmake qt6-base-devel" ;;
    esac
}

runtime_packages() {
    case "$PKG_MANAGER" in
        apt-get) echo "libqt6widgets6 qt6-qpa-plugins" ;;
        dnf|yum) echo "qt6-qtbase-gui" ;;
        pacman) echo "qt6-base" ;;
        zypper) echo "libQt6Widgets6" ;;
        apk) echo "qt6-qtbase" ;;
        xbps-install) echo "qt6-base" ;;
    esac
}

install_packages() { # packages...
    case "$PKG_MANAGER" in
        apt-get) as_root apt-get update && as_root apt-get install -y "$@" ;;
        dnf|yum) as_root "$PKG_MANAGER" install -y "$@" ;;
        pacman) as_root pacman -S --needed --noconfirm "$@" ;;
        zypper) as_root zypper --non-interactive install "$@" ;;
        apk) as_root apk add "$@" ;;
        xbps-install) as_root xbps-install -Sy "$@" ;;
    esac
}

offer_packages() { # what packages...
    local what="$1"
    shift
    if [ -z "$PKG_MANAGER" ] || [ $# -eq 0 ]; then
        die "$what is missing. Install it with your package manager and run this script again."
    fi
    echo "Hex Calc needs $what. Packages: $*"
    if [ "$WITH_DEPS" -eq 1 ] && ask "Install them now with $PKG_MANAGER?"; then
        install_packages "$@"
    else
        die "install the packages above and run this script again"
    fi
}

have_build_tools() {
    command -v cmake >/dev/null 2>&1 || return 1
    command -v c++ >/dev/null 2>&1 || command -v g++ >/dev/null 2>&1 || command -v clang++ >/dev/null 2>&1 || return 1
    command -v make >/dev/null 2>&1 || command -v ninja >/dev/null 2>&1 || return 1
    if command -v pkg-config >/dev/null 2>&1; then
        pkg-config --exists Qt6Widgets || pkg-config --exists Qt5Widgets || return 1
    else
        command -v qmake6 >/dev/null 2>&1 || command -v qmake >/dev/null 2>&1 || return 1
    fi
}

# --------------------------------------------------------------------------
# Install / uninstall

APP_FILES=(
    "bin/hex-calc"
    "share/hex-calc"
    "share/applications/$APP_ID.desktop"
    "share/icons/hicolor/scalable/apps/hex-calc.svg"
    "share/icons/hicolor/256x256/apps/hex-calc.png"
)

refresh_desktop() {
    if command -v update-desktop-database >/dev/null 2>&1; then
        $SUDO update-desktop-database "$SHAREDIR/applications" >/dev/null 2>&1 || true
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1 && [ -f "$SHAREDIR/icons/hicolor/index.theme" ]; then
        $SUDO gtk-update-icon-cache -q -t "$SHAREDIR/icons/hicolor" >/dev/null 2>&1 || true
    fi
}

uninstall() {
    local f removed=0
    info "Removing Hex Calc from $PREFIX"
    for f in "${APP_FILES[@]}"; do
        if [ -e "$PREFIX/$f" ]; then
            $SUDO rm -rf "${PREFIX:?}/$f"
            echo "  removed $PREFIX/$f"
            removed=1
        fi
    done
    [ "$removed" -eq 1 ] || echo "  nothing to remove"
    if [ "$PURGE" -eq 1 ]; then
        rm -rf "${XDG_CONFIG_HOME:-$HOME/.config}/hex-calc" "${XDG_CACHE_HOME:-$HOME/.cache}/hex-calc"
        echo "  removed settings and history"
    fi
    refresh_desktop
    info "Done"
}

# Copies an unpacked release (bin/ + share/) into the prefix.
install_tree() { # dir
    local src="$1"
    [ -x "$src/bin/hex-calc" ] || die "$src does not look like a Hex Calc release"

    $SUDO mkdir -p "$BINDIR" "$SHAREDIR/applications" \
        "$SHAREDIR/icons/hicolor/scalable/apps" "$SHAREDIR/icons/hicolor/256x256/apps"
    $SUDO install -m755 "$src/bin/hex-calc" "$BINDIR/hex-calc"
    $SUDO rm -rf "$SHAREDIR/hex-calc"
    $SUDO cp -R "$src/share/hex-calc" "$SHAREDIR/hex-calc"
    $SUDO install -m644 "$src/share/icons/hicolor/scalable/apps/hex-calc.svg" \
        "$SHAREDIR/icons/hicolor/scalable/apps/hex-calc.svg"
    $SUDO install -m644 "$src/share/icons/hicolor/256x256/apps/hex-calc.png" \
        "$SHAREDIR/icons/hicolor/256x256/apps/hex-calc.png"
    sed "s|^Exec=.*|Exec=$BINDIR/hex-calc|" "$src/share/applications/$APP_ID.desktop" \
        | $SUDO tee "$SHAREDIR/applications/$APP_ID.desktop" >/dev/null
}

check_runtime() { # binary
    local missing
    missing="$(ldd "$1" 2>/dev/null | awk '/not found/ {print $1}' | tr '\n' ' ')"
    if [ -n "$missing" ]; then
        detect_pkg_manager
        # shellcheck disable=SC2046
        offer_packages "the Qt 6 runtime (missing: $missing)" $(runtime_packages)
    fi
}

install_binary() {
    local src="$1"
    [ "$(uname -m)" = "x86_64" ] || die "prebuilt releases are x86_64 only; run without --binary to build from source"
    if [ -z "$src" ]; then
        WORKDIR="$(mktemp -d)"
        info "Downloading the latest release"
        download "https://github.com/$REPO/releases/latest/download/$RELEASE_ASSET" "$WORKDIR/$RELEASE_ASSET"
        tar -xzf "$WORKDIR/$RELEASE_ASSET" -C "$WORKDIR"
        src="$WORKDIR/${RELEASE_ASSET%.tar.gz}"
    fi
    check_runtime "$src/bin/hex-calc"
    info "Installing to $PREFIX"
    install_tree "$src"
}

install_source() {
    local src="$1" build jobs
    if [ -z "$src" ]; then
        WORKDIR="$(mktemp -d)"
        info "Downloading Hex Calc ($BRANCH)"
        download "https://github.com/$REPO/archive/refs/heads/$BRANCH.tar.gz" "$WORKDIR/src.tar.gz"
        mkdir "$WORKDIR/src"
        tar -xzf "$WORKDIR/src.tar.gz" -C "$WORKDIR/src" --strip-components=1
        src="$WORKDIR/src"
    fi

    if ! have_build_tools; then
        detect_pkg_manager
        # shellcheck disable=SC2046
        offer_packages "a C/C++ compiler, CMake and the Qt development files" $(build_packages)
        have_build_tools || die "build tools still not found after installing packages"
    fi

    build="${WORKDIR:-$(mktemp -d)}/build"
    [ -n "$WORKDIR" ] || WORKDIR="$(dirname "$build")"
    jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"

    info "Configuring"
    cmake -S "$src" -B "$build" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" -DHEXCALC_BUILD_TESTS=OFF >/dev/null
    info "Building"
    cmake --build "$build" --parallel "$jobs"
    info "Installing to $PREFIX"
    $SUDO cmake --install "$build" >/dev/null
}

# --------------------------------------------------------------------------

SCRIPT_DIR=""
if [ -n "${BASH_SOURCE[0]:-}" ] && [ -f "${BASH_SOURCE[0]}" ]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
fi

case "$MODE" in
    uninstall)
        uninstall
        exit 0
        ;;
    binary)
        if [ -n "$SCRIPT_DIR" ] && [ -x "$SCRIPT_DIR/bin/hex-calc" ]; then
            install_binary "$SCRIPT_DIR"
        else
            install_binary ""
        fi
        ;;
    source)
        if [ -n "$SCRIPT_DIR" ] && [ -x "$SCRIPT_DIR/bin/hex-calc" ] && [ ! -f "$SCRIPT_DIR/CMakeLists.txt" ]; then
            # Running from an unpacked release: nothing to build.
            install_binary "$SCRIPT_DIR"
        elif [ -n "$SCRIPT_DIR" ] && [ -f "$SCRIPT_DIR/CMakeLists.txt" ]; then
            install_source "$SCRIPT_DIR"
        else
            install_source ""
        fi
        ;;
esac

refresh_desktop

info "Hex Calc is installed"
echo "  Launch it from your applications menu, or run: hex-calc"
case ":$PATH:" in
    *":$BINDIR:"*) ;;
    *) warn "$BINDIR is not in your PATH. Add this to your shell profile:
    export PATH=\"$BINDIR:\$PATH\"" ;;
esac
