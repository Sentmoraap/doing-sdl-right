#include <cstdint>
#include <array>
#include "Scenes/Scene.hpp"

class GhettoInputLag : public Scene
{
private:
    static constexpr uint8_t NB_BEATS = 20;
    static constexpr uint8_t SPREAD = 3;
    static constexpr uint64_t BEAT_TIME = 750000;
    static constexpr uint16_t BEAT_SPEED = 400;

    struct State
    {
        std::array<int64_t, NB_BEATS> diffs;
        uint64_t time = 0;
        int32_t lag = 0;
        uint8_t beat = 0;
        bool prevInput = false;
    };

    State cur, saved;
    
public:
    GhettoInputLag();
    void displayImGuiSettings() override;
    void update(uint64_t microseconds, Inputs::State inputs) override;
    void saveState() override;
    void loadState() override;
    void draw();
};