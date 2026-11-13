#pragma once
#include <shared/spacemouse.h>
#include <string>

namespace all {
struct ModelNavParameters;
class StereoCamera;

// The 3D mouse through libspnav and the spacenavd service. The camera is written only from processEvents(), which
// the owner calls on the camera's thread when fd() is readable: the camera's properties are not thread-safe.
class SpacemouseSpnav : public Spacemouse
{
public:
    enum class State {
        NoService, // nothing answers on the service's socket
        NoDevice, // the service runs and knows no device
        Connected
    };

    SpacemouseSpnav(all::StereoCamera* camera, std::shared_ptr<all::ModelNavParameters>);
    ~SpacemouseSpnav();

public:
    void setUseUserPivot(bool) override
    {
    }

    // Tried at construction; the owner calls it again while state() is NoService, so a service that starts later is found
    bool connect();
    void disconnect();
    // The descriptor to watch for reading, -1 without a service
    int fd() const;
    // Reads every pending event and applies it to the camera. False when the service has closed the connection.
    bool processEvents();

    State state() const
    {
        return m_state;
    }
    const std::string& deviceName() const
    {
        return m_deviceName;
    }

    float m_rotFactor = 0.0001;
    float m_translFactor = 0.001;

protected:
    void refreshDevice();

    State m_state{ State::NoService };
    std::string m_deviceName;
};
} // namespace all
