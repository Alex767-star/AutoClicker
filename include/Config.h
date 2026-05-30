#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <cstdint>

struct ClickProfile {
    std::string name;
    uint32_t delay_ms;
    uint8_t button;
    uint8_t click_mode;
    uint8_t target_type;
    int32_t saved_x;
    int32_t saved_y;
    uint16_t move_radius;
    uint16_t burst_count;
    uint8_t grid_rows;
    uint8_t grid_cols;
    std::vector<uint32_t> sequence_delays;
    std::vector<uint8_t> sequence_buttons;
    bool sequence_loop;
    std::vector<std::pair<int32_t, int32_t>> pattern_points;
};

class ConfigManager {
public:
    static ConfigManager& getInstance();
    bool loadProfile(const std::string& name);
    bool saveProfile(const std::string& name);
    bool deleteProfile(const std::string& name);
    std::vector<std::string> listProfiles();
    ClickProfile getCurrentProfile() const;
    void setCurrentProfile(const ClickProfile& profile);
    
private:
    ConfigManager() = default;
    std::string getConfigPath() const;
    ClickProfile current_profile;
};

#endif
