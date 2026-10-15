#pragma once
#include <string>
#include <vector>

enum class JLinkConnectionState { Disconnected, Connecting, Connected, Reconnecting, Failed, Stopping };
struct JLinkConnectionStatus {
    JLinkConnectionState state = JLinkConnectionState::Disconnected;
    unsigned attempt = 0;
    unsigned retryInMs = 0;
    bool writeMayHaveCompleted = false; // sticky until the next manual Connect
    std::string lastError;
};

// Copies a consistent snapshot; never calls the SDK or waits for USB I/O.
JLinkConnectionStatus GetJLinkConnectionStatus();
bool IsCommunicationRequested();
// Nonblocking with respect to concurrent SDK I/O; retry on a later UI frame.
bool TryGetJLinkDeviceNames(std::vector<std::string>& names);
