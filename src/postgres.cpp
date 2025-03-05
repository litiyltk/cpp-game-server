#include "postgres.h"


namespace postgres {

// методы класс DataBase
    void DataBase::Init(ConnectionPoolPtr pool) {
        try {
            auto connection_ = pool->GetConnection(std::chrono::milliseconds(DELAY_TIME_MS));
            pqxx::work work{ *connection_ };

            // Создание таблицы
            work.exec(R"(
                CREATE TABLE IF NOT EXISTS retired_players (
                    id UUID PRIMARY KEY,
                    name VARCHAR(100),
                    score INT,
                    play_time_ms INT
                );
            )"_zv);

            // Создание мультииндекса
            work.exec(R"(
                CREATE INDEX IF NOT EXISTS idx_score_playtime_name
                ON retired_players (score DESC, play_time_ms, name);
            )"_zv);

            work.commit();
        } catch (const std::exception& ex) {
            throw std::runtime_error("Error by init DB: "s + ex.what());
        }
    }

    PlayersRecords DataBase::GetRecords(ConnectionPoolPtr pool, int start, int maxItems) {
        PlayersRecords result;
        try {
            auto connection_ = pool->GetConnection(std::chrono::milliseconds(DELAY_TIME_MS));
            pqxx::work work{ *connection_ };

            pqxx::result res = work.exec_params(
                R"(
                    SELECT id, name, score, play_time_ms 
                    FROM retired_players 
                    ORDER BY score DESC, play_time_ms, name 
                    OFFSET $1 LIMIT $2;
                )"_zv, start, maxItems);

            for (const auto& row : res) {
                result.push_back({
                    row[0].as<std::string>(), // uuid
                    row[1].as<std::string>(), // name
                    row[2].as<int>(), // score
                    row[3].as<int>() // play_time_ms
                });
            }
        } catch (const std::exception& ex) {
            throw std::runtime_error("Error fetching records: "s + ex.what());
        }

        return result;
    }

    void DataBase::AddRecord(ConnectionPoolPtr pool, PlayerRecord record) {
        try {
            auto connection_ = pool->GetConnection(std::chrono::milliseconds(DELAY_TIME_MS));
            pqxx::work work{ *connection_ };

            work.exec_params(R"(
                INSERT INTO retired_players (id, name, score, play_time_ms)
                VALUES($1, $2, $3, $4) 
                ON CONFLICT (id) 
                DO UPDATE SET name = excluded.name, score = excluded.score, play_time_ms = excluded.play_time_ms;
            )"_zv, record.uuid, record.name, record.score, record.play_time_ms);

            work.commit();
        } catch (const std::exception& ex) {
            throw std::runtime_error("Error saving record: "s + ex.what());
        }
    }

} // namespace postgres