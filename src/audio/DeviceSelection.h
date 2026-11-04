#pragma once

#include <QList>
#include <QString>

namespace kdict {

struct AudioDeviceInfo {
    QString id;
    QString description;
    bool isDefault = false;
};

// Resolves the `audio.device` setting against the available input devices.
// "default" or empty selects the system default. Otherwise the value is matched
// against the backend id (exact), then the description (exact, then
// case-insensitive substring). Returns -1 when nothing matches.
int selectAudioDevice(const QList<AudioDeviceInfo> &devices, const QString &wanted);

} // namespace kdict
