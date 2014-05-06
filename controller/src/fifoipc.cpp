/*
 *   Copyright (C) 2014 Pelagicore AB
 *   All rights reserved.
 */
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include "fifoipc.h"

static const int BUF_SIZE = 1024;

FifoIPC::FifoIPC(IPCMessage &message):
    m_message(message), m_fifoPath(""), m_fifoCreated(false)
{
}

FifoIPC::~FifoIPC()
{
    int ret = unlink(m_fifoPath.c_str());
    if (ret == -1) {
        perror("unlink: ");
    }
}

bool FifoIPC::initialize(const std::string &fifoPath)
{
    m_fifoPath = fifoPath;

    if (m_fifoCreated == false) {
        if (createFifo() == false) {
            std::cout << "Could not create FIFO!" << std::endl;
            return false;
        }
    }

    return loop();
}

bool FifoIPC::loop()
{
    int fd = open(m_fifoPath.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("FifoIPC open: ");
        return false;
    }

    struct pollfd pfd[2];
    pfd[0].fd = fd;
    pfd[0].events = POLLIN;

    int result = poll(pfd, 1, 100);
    if (result == -1) {
        perror("FifoIPC poll: ");
        return false;
    }

    if (!(pfd[0].revents & POLLIN)) {
        return true;
    }

    char buf[BUF_SIZE];
    char c;
    int i = 0;
    do {
        int status = read(fd, &c, 1);
        if (status > 0) {
            buf[i++] = c;
        } else if (status == 0) {
            // We've read 'end of file', just ignore it
            break;
        } else if (status == -1) {
            perror("FifoIPC read: ");
            return false;
        } else {
            std::cout << "Error: Unknown problem reading fifo" << std::endl;
            return false;
        }
    // Look for end of message or end of storage buffer
    } while ((c != '\0') && (i != sizeof(buf) - 1));

    buf[i] = '\0';
    std::string messageString(buf);
    bool messageProcessedOk = false;
    // If message is empty we don't need to handle it
    if (messageString.size() > 0) {
        messageProcessedOk = m_message.handleMessage(messageString);
    }
    if (!messageProcessedOk) {
        // The message was not understood by IPCMessage
        std::cout << "Warning: IPC message to Controller was not sent" << std::endl;
    }

    return true;
}

bool FifoIPC::createFifo()
{
    int ret = mkfifo(m_fifoPath.c_str(), 0666);
    // All errors except for when fifo already exist are bad
    if ((ret == -1) && (errno != EEXIST)) {
        perror("Error creating fifo: ");
        return false;
    }

    m_fifoCreated = true;

    return true;
}
