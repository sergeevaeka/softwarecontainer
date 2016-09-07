
/*
 * Copyright (C) 2016 Pelagicore AB
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * For further information see LICENSE
 */

#include "generators.h"
#include "devicenodegateway.h"
#include <sys/stat.h>
#include <sys/types.h>


DeviceNodeGateway::DeviceNodeGateway() :
    Gateway(ID)
{
}

ReturnCode DeviceNodeGateway::readConfigElement(const json_t *element)
{
    DeviceNodeGateway::Device dev;

    if (!read(element, "name", dev.name)) {
        log_error() << "Key \"name\" missing or not a string in json configuration";
        return ReturnCode::FAILURE;
    }

    if (!read(element, "mode", dev.mode)) {
        log_error() << "Key \"mode\" missing or not a string in json configuration";
        return ReturnCode::FAILURE;
    }
    
    read(element, "major", dev.major);
    read(element, "minor", dev.minor);

    if (dev.minor.length() == 0 ^ dev.minor.length() == 0) {
        log_error() << "Either only minor or only major version specified."
                       " This is not allowed, specify both major and minor version or none of them";
        return ReturnCode::FAILURE;
    }


    m_devList.push_back(dev);
    return ReturnCode::SUCCESS;
}


bool DeviceNodeGateway::activateGateway()
{
    for (auto &dev : m_devList) {
        log_info() << "Mapping device " << dev.name;
        const int mode = std::atoi(dev.mode.c_str());

        if (dev.major.length() != 0) {
            const int majorVersion = std::atoi(dev.major.c_str());
            const int minorVersion = std::atoi(dev.minor.c_str());

            // mknod dev.name c dev.major dev.minor
            if (mknod(dev.name.c_str(), S_IFCHR | mode, makedev(majorVersion, minorVersion)) != 0) {
                log_error() << "Failed to create device " << dev.name;
                return false;
            }
        } else {
            // No major & minor numbers specified => simply map the device from the host into the container
            getContainer()->mountDevice(dev.name);

            if (chmod(dev.name.c_str(), mode) != 0) {
                log_error() << "Could not 'chmod " << dev.mode << "' the mounted device " << dev.name;
                return false;
            }
        }
    }

    m_state = GatewayState::ACTIVATED;
    return true;
}

bool DeviceNodeGateway::teardownGateway()
{
    return true;
}
