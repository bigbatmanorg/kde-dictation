#!/usr/bin/env python3
"""Fast source-tree contract checks that do not require Qt/KF6."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
required = [
    "CMakeLists.txt",
    "config.example.yaml",
    "ARCHITECTURE.md",
    "src/main.cpp",
    "src/app/DictationController.cpp",
    "src/app/LiveTranscriptReconciler.cpp",
    "src/stt/BatchOpenAIProvider.cpp",
    "src/stt/RealtimeAsrProvider.cpp",
    "src/output/TextOutput.cpp",
    "src/shortcut/ShortcutController.cpp",
    "scripts/build.sh",
    "scripts/test.sh",
    "scripts/setup.sh",
    "scripts/install-deps.sh",
    "scripts/lib/distro.sh",
]
missing = [p for p in required if not (root / p).exists()]
assert not missing, f"missing required files: {missing}"

controller = (root / "src/app/DictationController.cpp").read_text()
assert "FallbackPolicy::onRealtimeFailure" in controller
assert "startBatchFallback" in controller
assert "WavEncoder::pcm16MonoToWav" in controller
assert "partialTranscript" in controller
assert "appendStream" in controller
assert "LiveTranscriptReconciler::reconcile" in controller

output = (root / "src/output/TextOutput.cpp").read_text()
assert "QGuiApplication::clipboard()->setText" in output
keys = (root / "src/output/PasteKeys.cpp").read_text()
assert 'QStringLiteral("29:1")' in keys and 'QStringLiteral("47:1")' in keys
assert "beginStream" in output and "appendStream" in output and "finishStream" in output

shortcut = (root / "src/shortcut/ShortcutController.cpp").read_text()
assert "globalShortcutActiveChanged" in shortcut

setup = (root / "scripts/setup.sh").read_text()
assert "install-deps.sh" in setup
assert "setup-uinput.sh" in setup
assert "install-user.sh" in setup

deps = (root / "scripts/install-deps.sh").read_text()
for manager in ("apt-get", "dnf", "pacman", "zypper"):
    assert manager in deps, f"install-deps.sh does not support {manager}"

print("repo contract: OK")
