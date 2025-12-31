#!/bin/sh
# NERD Compiler Installer
# Usage: curl -fsSL https://raw.githubusercontent.com/Nerd-Lang/nerd-lang-core/main/install.sh | sh

set -e

REPO="Nerd-Lang/nerd-lang-core"
INSTALL_DIR="${NERD_INSTALL:-$HOME/.nerd}"
BIN_DIR="$INSTALL_DIR/bin"

# Colors (if terminal supports them)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

info() {
    printf "${GREEN}info${NC}: %s\n" "$1"
}

warn() {
    printf "${YELLOW}warn${NC}: %s\n" "$1"
}

error() {
    printf "${RED}error${NC}: %s\n" "$1"
    exit 1
}

# Detect OS and architecture
detect_platform() {
    OS="$(uname -s)"
    ARCH="$(uname -m)"

    case "$OS" in
        Linux)
            PLATFORM="linux"
            ;;
        Darwin)
            PLATFORM="darwin"
            ;;
        MINGW*|MSYS*|CYGWIN*)
            PLATFORM="windows"
            ;;
        *)
            error "Unsupported operating system: $OS"
            ;;
    esac

    case "$ARCH" in
        x86_64|amd64)
            ARCH="x86_64"
            ;;
        arm64|aarch64)
            ARCH="arm64"
            ;;
        *)
            error "Unsupported architecture: $ARCH"
            ;;
    esac

    TARGET="${PLATFORM}-${ARCH}"
    info "Detected platform: $TARGET"
}

# Check prerequisites
check_prerequisites() {
    info "Checking prerequisites..."

    # Check for clang (required for compiling LLVM IR to native)
    if ! command -v clang >/dev/null 2>&1; then
        warn "clang not found - required for compiling NERD programs to native binaries"
        echo ""
        echo "Install LLVM/clang:"
        case "$PLATFORM" in
            darwin)
                echo "  brew install llvm"
                echo "  # or install Xcode Command Line Tools:"
                echo "  xcode-select --install"
                ;;
            linux)
                echo "  # Ubuntu/Debian:"
                echo "  sudo apt install clang"
                echo ""
                echo "  # Fedora:"
                echo "  sudo dnf install clang"
                echo ""
                echo "  # Arch:"
                echo "  sudo pacman -S clang"
                ;;
            windows)
                echo "  # Install LLVM from: https://releases.llvm.org/"
                echo "  # Or use: winget install LLVM.LLVM"
                ;;
        esac
        echo ""
    else
        CLANG_VERSION=$(clang --version | head -n 1)
        info "Found: $CLANG_VERSION"
    fi
}

# Download and install
install_nerd() {
    VERSION="${NERD_VERSION:-latest}"

    if [ "$VERSION" = "latest" ]; then
        info "Fetching latest release..."
        VERSION=$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest" | grep '"tag_name"' | sed -E 's/.*"([^"]+)".*/\1/')
        if [ -z "$VERSION" ]; then
            error "Could not determine latest version. Building from source instead."
        fi
    fi

    BINARY_NAME="nerd-${TARGET}"
    if [ "$PLATFORM" = "windows" ]; then
        BINARY_NAME="${BINARY_NAME}.exe"
    fi

    DOWNLOAD_URL="https://github.com/$REPO/releases/download/$VERSION/$BINARY_NAME"

    info "Downloading NERD $VERSION for $TARGET..."

    # Create install directory
    mkdir -p "$BIN_DIR"

    # Download binary
    if curl -fsSL "$DOWNLOAD_URL" -o "$BIN_DIR/nerd" 2>/dev/null; then
        chmod +x "$BIN_DIR/nerd"
        info "Installed to $BIN_DIR/nerd"
    else
        warn "Pre-built binary not available for $TARGET"
        echo ""
        echo "Building from source instead..."
        install_from_source
        return
    fi
}

# Build from source as fallback
install_from_source() {
    info "Building from source..."

    # Check for C compiler
    if ! command -v cc >/dev/null 2>&1 && ! command -v gcc >/dev/null 2>&1 && ! command -v clang >/dev/null 2>&1; then
        error "No C compiler found. Please install gcc or clang."
    fi

    # Create temp directory
    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"

    info "Cloning repository..."
    git clone --depth 1 "https://github.com/$REPO.git" nerd-lang

    cd nerd-lang/bootstrap

    info "Compiling..."
    make

    # Install
    mkdir -p "$BIN_DIR"
    cp nerd "$BIN_DIR/nerd"
    chmod +x "$BIN_DIR/nerd"

    # Cleanup
    cd /
    rm -rf "$TEMP_DIR"

    info "Built and installed to $BIN_DIR/nerd"
}

# Update shell profile
update_path() {
    PROFILE=""

    if [ -n "$BASH_VERSION" ]; then
        if [ -f "$HOME/.bashrc" ]; then
            PROFILE="$HOME/.bashrc"
        elif [ -f "$HOME/.bash_profile" ]; then
            PROFILE="$HOME/.bash_profile"
        fi
    elif [ -n "$ZSH_VERSION" ]; then
        PROFILE="$HOME/.zshrc"
    fi

    # Check if already in PATH
    case ":$PATH:" in
        *":$BIN_DIR:"*)
            info "Already in PATH"
            return
            ;;
    esac

    EXPORT_LINE="export PATH=\"\$PATH:$BIN_DIR\""

    if [ -n "$PROFILE" ]; then
        if ! grep -q "$BIN_DIR" "$PROFILE" 2>/dev/null; then
            echo "" >> "$PROFILE"
            echo "# NERD compiler" >> "$PROFILE"
            echo "$EXPORT_LINE" >> "$PROFILE"
            info "Added $BIN_DIR to PATH in $PROFILE"
        fi
    fi

    echo ""
    echo "To use nerd in this shell, run:"
    echo "  $EXPORT_LINE"
    echo ""
    echo "Or restart your terminal."
}

# Verify installation
verify_install() {
    if [ -x "$BIN_DIR/nerd" ]; then
        echo ""
        info "Installation complete!"
        echo ""
        echo "Usage:"
        echo "  nerd compile <file.nerd> -o output.ll   # Compile to LLVM IR"
        echo "  clang output.ll -o program              # Build native binary"
        echo ""
        echo "Example:"
        echo "  echo 'fn add a b"
        echo "  ret a plus b' > test.nerd"
        echo "  nerd compile test.nerd -o test.ll"
        echo ""
    else
        error "Installation failed"
    fi
}

# Main
main() {
    echo ""
    echo "  _   _ _____ ____  ____  "
    echo " | \ | | ____|  _ \|  _ \ "
    echo " |  \| |  _| | |_) | | | |"
    echo " | |\  | |___|  _ <| |_| |"
    echo " |_| \_|_____|_| \_\____/ "
    echo ""
    echo " No Effort Required, Done"
    echo ""

    detect_platform
    check_prerequisites
    install_nerd
    update_path
    verify_install
}

main "$@"
