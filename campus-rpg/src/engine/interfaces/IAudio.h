#pragma once

#include <string>

namespace engine
{

// Abstract audio interface. Sound playback requests go through this interface
// so that game logic (e.g. SocialLinkManager's rank-up callback) does not
// depend on SFML directly.
//
// Sound assets are referenced by string id (e.g. "sfx_rankup_nailong").
// The concrete implementation (SfmlAudio) maps ids to loaded buffers/files.
//
// Mock implementations can record play calls for unit testing.
class IAudio
{
public:
    virtual ~IAudio() = default;

    // Play a one-shot sound effect by id. Volume in [0,100].
    // If the id is unknown / not loaded the call is a no-op.
    virtual void playSound(const std::string &soundId, int volume = 80) = 0;

    // Play / stop looping background music. Empty id stops the current track.
    virtual void playMusic(const std::string &musicId, int volume = 60) = 0;
    virtual void stopMusic() = 0;
};

} // namespace engine
