#!/usr/bin/env bash
# Deprecated name kept for compatibility: the scripts are distro-aware now.
exec "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/setup.sh" "$@"
