#include "audio/DeviceSelection.h"

namespace kdict {

int selectAudioDevice(const QList<AudioDeviceInfo> &devices, const QString &wanted)
{
    const QString name = wanted.trimmed();
    if (name.isEmpty() || name.compare(QStringLiteral("default"), Qt::CaseInsensitive) == 0) {
        for (int i = 0; i < devices.size(); ++i) {
            if (devices[i].isDefault) return i;
        }
        return devices.isEmpty() ? -1 : 0;
    }
    for (int i = 0; i < devices.size(); ++i) {
        if (devices[i].id == name) return i;
    }
    for (int i = 0; i < devices.size(); ++i) {
        if (devices[i].description.compare(name, Qt::CaseInsensitive) == 0) return i;
    }
    for (int i = 0; i < devices.size(); ++i) {
        if (devices[i].description.contains(name, Qt::CaseInsensitive)) return i;
    }
    return -1;
}

} // namespace kdict
