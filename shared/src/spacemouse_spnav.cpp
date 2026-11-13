#include <shared/spacemouse_spnav.h>
#include <shared/stereo_camera.h>
#include <spnav.h>

#include <glm/ext/matrix_transform.hpp>

#include <sys/socket.h>

using namespace all;

SpacemouseSpnav::SpacemouseSpnav(all::StereoCamera* camera, std::shared_ptr<all::ModelNavParameters> p)
    : Spacemouse(camera, p)
{
    connect();
}

SpacemouseSpnav::~SpacemouseSpnav()
{
    disconnect();
}

bool SpacemouseSpnav::connect()
{
    if (m_state != State::NoService)
        return true;
    if (spnav_open() == -1)
        return false;
    // Device changes too, so a 3D mouse plugged in later is named
    spnav_evmask(SPNAV_EVMASK_INPUT | SPNAV_EVMASK_DEV);
    refreshDevice();
    return true;
}

void SpacemouseSpnav::disconnect()
{
    if (m_state == State::NoService)
        return;
    spnav_close();
    m_state = State::NoService;
    m_deviceName.clear();
}

int SpacemouseSpnav::fd() const
{
    return m_state == State::NoService ? -1 : spnav_fd();
}

void SpacemouseSpnav::refreshDevice()
{
    m_deviceName.clear();
    // A service older than protocol 1 cannot say which device it has
    if (spnav_protocol() < 1) {
        m_state = State::Connected;
        return;
    }
    char name[256];
    if (spnav_dev_name(name, sizeof(name)) >= 0 && name[0] != '\0') {
        m_deviceName = name;
        m_state = State::Connected;
    } else {
        m_state = State::NoDevice;
    }
}

bool SpacemouseSpnav::processEvents()
{
    if (m_state == State::NoService)
        return false;

    // Readable with nothing to read: the service closed its end
    char probe;
    if (recv(spnav_fd(), &probe, 1, MSG_PEEK | MSG_DONTWAIT) == 0) {
        disconnect();
        return false;
    }

    spnav_event sev;
    while (spnav_poll_event(&sev)) {
        switch (sev.type) {
        case SPNAV_EVENT_MOTION: {
            const glm::mat4 rotateX = glm::rotate(glm::mat4(1.0f), sev.motion.rx * m_rotFactor, glm::vec3(1.0f, 0.0f, 0.0f));
            const glm::mat4 rotateY = glm::rotate(glm::mat4(1.0f), sev.motion.ry * m_rotFactor, glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 rotateZ = glm::rotate(glm::mat4(1.0f), sev.motion.rz * m_rotFactor, glm::vec3(0.0f, 0.0f, 1.0f));

            // Combine the rotation matrices
            const glm::mat4 combinedRotation = rotateX * rotateY * rotateZ;

            m_camera->position = m_camera->position() + glm::vec3{ sev.motion.x, sev.motion.y, sev.motion.z } * m_translFactor;
            m_camera->setForwardVector(glm::vec3(combinedRotation * glm::vec4(m_camera->forwardVector(), 0.0f)));
            break;
        }
        case SPNAV_EVENT_BUTTON:
            /* 0-based button number in sev.button.bnum.
             * button state in sev.button.press (non-zero means pressed).
             */
            break;
        case SPNAV_EVENT_DEV:
            refreshDevice();
            break;
        default:;
        }
    }
    return true;
}
