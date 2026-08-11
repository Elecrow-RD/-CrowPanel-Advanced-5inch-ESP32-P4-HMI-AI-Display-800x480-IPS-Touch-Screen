/*---------------------------------------------------------------
 * Teaching module overview: Audio processing
 * This file groups the audio_debugger responsibilities so learners can
 * follow the subsystem boundary before reading individual routines.
 *--------------------------------------------------------------*/

#ifndef AUDIO_DEBUGGER_H
#define AUDIO_DEBUGGER_H

#include <vector>
#include <cstdint>

#include <sys/socket.h>
#include <netinet/in.h>

class AudioDebugger {
public:
    AudioDebugger();
    ~AudioDebugger();

    void Feed(const std::vector<int16_t>& data);

private:
    int udp_sockfd_ = -1;
    struct sockaddr_in udp_server_addr_;
};

#endif 
