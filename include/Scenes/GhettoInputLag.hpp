#include <cstdint>
#include <array>
#include "Scenes/Scene.hpp"

class GhettoInputLag : public Scene
{
private:
    static constexpr uint8_t NB_BEATS = 20;
    static constexpr uint8_t SPREAD = 3;
    static constexpr uint16_t BEAT_SPEED = 400;
    uint16_t time = 0;
    int32_t lag = 0;
    uint8_t beat = 0;
    uint16_t beatTime;
    std::array<int16_t, NB_BEATS> diffs;
    bool prevInput = false;

public:
    GhettoInputLag();
    void displayImGuiSettings() override;
    void update(uint16_t frameRate) override;
    void draw();
};