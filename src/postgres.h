#pragma once

#include <pqxx/pqxx> // Для работы с библиотекой PostgreSQL
#include <memory> // Для std::shared_ptr
#include <vector> // Для std::vector
#include <mutex> // Для std::mutex
#include <condition_variable> // Для std::condition_variable


namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

struct PlayerRecord {
    std::string uuid;
    std::string name;
    int score;
    int play_time_ms;
};

class ConnectionPool {
    using PoolType = ConnectionPool;
    using ConnectionPtr = std::shared_ptr<pqxx::connection>;

public:
    class ConnectionWrapper { // класс для управления соединениями
    public:
        ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept
            : conn_{std::move(conn)}
            , pool_{&pool} {
        }

        ConnectionWrapper(const ConnectionWrapper&) = delete;
        ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

        ConnectionWrapper(ConnectionWrapper&&) = default;
        ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

        pqxx::connection& operator*() const& noexcept { // для доступа к соединению через обёртку
            return *conn_;
        }

        pqxx::connection& operator*() const&& = delete;

        pqxx::connection* operator->() const& noexcept {
            return conn_.get();
        }

        ~ConnectionWrapper() {
            if (conn_) {
                pool_->ReturnConnection(std::move(conn_));
            }
        }

    private:
        std::shared_ptr<pqxx::connection> conn_;
        PoolType* pool_;
    };

    // ConnectionFactory is a functional object returning std::shared_ptr<pqxx::connection>
    template <typename ConnectionFactory>
    ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
        pool_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            pool_.emplace_back(connection_factory());
        }
    }

    // используем таймаут для возвращения соединения в пул
    ConnectionWrapper GetConnection(std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {
        std::unique_lock lock{mutex_};
        // Блокируем текущий поток и ждём, пока cond_var_ не получит уведомление и не освободится
        // хотя бы одно соединение
        if (!cond_var_.wait_for(lock, timeout, [this] {
            return used_connections_ < pool_.size();
        })) {
            throw std::runtime_error("Timeout while waiting for a database connection");
        }
        // После выхода из цикла ожидания мьютекс остаётся захваченным

        return {std::move(pool_[used_connections_++]), *this};
    }

private:
    void ReturnConnection(ConnectionPtr&& conn) {
        {
        // Возвращаем соединение обратно в пул
            std::lock_guard lock{mutex_};
            pool_[--used_connections_] = std::move(conn);
        }
        // Уведомляем один из ожидающих потоков об изменении состояния пула
        cond_var_.notify_one();
    }

    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::vector<ConnectionPtr> pool_;
    size_t used_connections_ = 0;
};

using ConnectionPoolPtr = std::shared_ptr<ConnectionPool>;
using PlayersRecords = std::vector<PlayerRecord>;

constexpr int DELAY_TIME_MS = 500;

class DataBase {
public:
    static void Init(ConnectionPoolPtr pool);
    static PlayersRecords GetRecords(ConnectionPoolPtr pool, int start, int maxItems);
    static void AddRecord(ConnectionPoolPtr pool, PlayerRecord record);
};


} // namespace postgres
