#include "state_saver.h"


namespace state_saver {

// методы класса StateSaver

    bool StateSaver::IsPathSet() const noexcept {
        return !path_to_state_file_.empty();
    }

    const std::string StateSaver::GetPath() const noexcept {
        return path_to_state_file_;
    }

    bool StateSaver::IsFileExists() const noexcept {
        return fs::exists(fs::path(path_to_state_file_));
    }

    bool StateSaver::HasSavePeriod() const noexcept {
        return save_state_period_ > 0;
    }

    int StateSaver::GetSavePeriod() const noexcept {
        return save_state_period_;
    }

    void StateSaver::SaveState() const {
        using OutputArchive = boost::archive::text_oarchive;
        namespace fs = std::filesystem;

        fs::path targetPath(path_to_state_file_);
        fs::path tempPath(path_to_temp_file_);

        if (!fs::exists(tempPath.parent_path())) {
            fs::create_directories(tempPath.parent_path());
        }

        std::ofstream ofs(tempPath, std::ios::out | std::ios::trunc);
        if (!ofs.is_open()) {
            throw std::runtime_error("Could not open file: "s + tempPath.string()); // логируем при вызове SaveState
        }

        OutputArchive output_archive{ ofs };

        try { // сериализуем игровое состояние - поля класса Application
            serialization::ApplicationRepr app_repr(app_);
            output_archive << app_repr;
        } catch (const std::exception& ex) {
            ofs.close();
            fs::remove(tempPath); // удаляем временный файл в случае ошибки
            throw std::runtime_error("Error by serialization: "s + ex.what()); // логируем при вызове SaveState
        }

        ofs.close();

        try { // переименовываем временный файл в целевой
            fs::rename(tempPath, targetPath);
        } catch (const std::exception& ex) {
            fs::remove(tempPath); // удаляем временный файл, если переименование не удалось
            throw std::runtime_error("Could not rename temp file: "s + ex.what()); // логируем при вызове SaveState
        }
    }

    void StateSaver::LoadState() const {
        using InputArchive = boost::archive::text_iarchive;

        fs::path p(path_to_state_file_);
        if (fs::exists(p) == false) {
            throw std::runtime_error("State file not exists: "s + p.string()); // логируем при вызове LoadState
        }
        std::ifstream ifs(p);
        InputArchive input_archive{ ifs };

        try { // выполняем десериализацию по этапам - поля класса Application
            serialization::ApplicationRepr app_repr;
            input_archive >> app_repr;
            app_repr.Restore(app_);
        } catch (const std::exception& ex) {
            ifs.close();
            throw std::runtime_error("Error by deserialization: "s + ex.what()); // логируем при вызове LoadState
        }

        ifs.close();
    }

} // namespace state_saver