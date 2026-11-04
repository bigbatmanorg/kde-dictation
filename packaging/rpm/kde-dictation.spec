Name:           kde-dictation
Version:        0.3.1
Release:        1%{?dist}
Summary:        Global shortcut dictation client for KDE Plasma
License:        MIT
URL:            https://github.com/bigbatmanorg/kde-dictation
Source0:        %{url}/archive/refs/tags/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.24
BuildRequires:  gcc-c++
BuildRequires:  ninja-build
BuildRequires:  cmake(Qt6Core)
BuildRequires:  cmake(Qt6Gui)
BuildRequires:  cmake(Qt6Widgets)
BuildRequires:  cmake(Qt6Network)
BuildRequires:  cmake(Qt6Multimedia)
BuildRequires:  cmake(Qt6WebSockets)
BuildRequires:  cmake(Qt6DBus)
BuildRequires:  cmake(Qt6Test)
BuildRequires:  cmake(KF6GlobalAccel)
BuildRequires:  cmake(KF6StatusNotifierItem)
BuildRequires:  cmake(yaml-cpp)
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib
BuildRequires:  python3

Recommends:     ydotool
Recommends:     xdotool
Recommends:     qt6-qtmultimedia

%description
KDE Dictation records the microphone on a global KDE shortcut, sends the audio
to an OpenAI-compatible speech-to-text service (batch, or realtime through
vLLM/LiteLLM) and pastes the transcript into the focused application. It works
on Plasma Wayland (through ydotool) and X11 (through xdotool or ydotool).

Create ~/.config/kde-dictation/config.yaml from
%{_datadir}/%{name}/config.example.yaml and set your service URL and API key.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -G Ninja -DBUILD_TESTING=ON -DKDICT_INSTALL_AUTOSTART=OFF
%cmake_build

%install
%cmake_install

%check
export QT_QPA_PLATFORM=offscreen
%ctest
desktop-file-validate %{buildroot}%{_datadir}/applications/org.amvc.kdedictation.desktop
appstream-util validate-relax --nonet %{buildroot}%{_datadir}/metainfo/org.amvc.kdedictation.metainfo.xml || :

%files
%license LICENSE
%doc README.md CHANGELOG.md ARCHITECTURE.md
%{_bindir}/kde-dictation
%{_datadir}/applications/org.amvc.kdedictation.desktop
%{_datadir}/metainfo/org.amvc.kdedictation.metainfo.xml
%{_datadir}/%{name}/

%changelog
* Wed Nov 04 2026 Adam Moravcik <moravcik.adam@gmail.com> - 0.3.1-1
- Multi-distribution support: pluggable paste backend, ydotool socket discovery
