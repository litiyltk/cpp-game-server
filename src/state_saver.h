#pragma once

#include "app.h"
#include "app_serialization.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <filesystem>
#include <fstream>


namespace state_saver {

using namespace std::literals;
namespace fs = std::filesystem;

// для сохранения и загрузки состояния игры
class StateSaver {

public:
    explicit StateSaver(app::Application& app, const std::string& path, int period)
        : app_(app)
        , path_to_state_file_(path)
        , save_state_period_(period) {
            path_to_temp_file_ = path_to_state_file_ + ".tmp"; // временный файл
    }

    bool IsPathSet() const noexcept;
    const std::string GetPath() const noexcept;
    bool IsFileExists() const noexcept;
    bool HasSavePeriod() const noexcept;
    int GetSavePeriod() const noexcept;
    void SaveState() const;
    void LoadState() const;

private:
    app::Application& app_;

    std::string path_to_state_file_ = "";
    std::string path_to_temp_file_;
    int save_state_period_ = 0;
};

} // namespace state_saver