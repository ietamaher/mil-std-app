#pragma once
#include "interfaces/IDevice.h"
#include "qreadwritelock.h"
#include <memory>

template<typename TData>
class TemplatedDevice : public IDevice {
public:
    using DataPtr = std::shared_ptr<const TData>;

    explicit TemplatedDevice(QObject* parent = nullptr) : IDevice(parent) {
        m_data = std::make_shared<const TData>();
    }

    // This class is still abstract because it doesn't know *which*
    // transport to close. It cannot implement shutdown() fully.
    // That's why RadarDevice must do it.

    DataPtr data() const {
        QReadLocker locker(&m_dataLock);
        return m_data;
    }

protected:
    void updateData(DataPtr newData) {
        {
            QWriteLocker locker(&m_dataLock);
            m_data = newData;
        }
    }

private:
    mutable QReadWriteLock m_dataLock;
    DataPtr m_data;
};

