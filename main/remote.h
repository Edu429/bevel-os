#pragma once

// Starts the optional Wi-Fi remote desktop service on TCP port 2323.
void remote_start(void);

// Called by the GUI task after a compositor flush. The service sends a fresh keyframe to its
// client without blocking rendering.
void remote_frame_ready(void);
