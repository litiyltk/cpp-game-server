#include "sdk.h"

#include "app.h"
#include "json_loader.h"
#include "logging_request_handler.h"
#include "magic_defs.h"
#include "postgres.h"
#include "request_handler.h"
#include "server_logger.h"
#include "state_saver.h"
#include "ticker.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/program_options.hpp>
#include <boost/signals2.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <memory>
#include <thread>


using namespace std::literals;
using milliseconds = std::chrono::milliseconds;

namespace sig = boost::signals2;
namespace net = boost::asio;
namespace sys = boost::system;
namespace fs = std::filesystem;

struct Args {
    int tick_period = 0; // по умолчанию не указан - 0
    fs::path config_file;
    fs::path www_root;
    bool randomize_spawn_points;
    std::string state_file = ""; // по умолчанию не задан
    int save_state_period = 0; // по умолчанию не указан - 0
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"Allowed options"s};

    Args args;
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.www_root)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions")
        ("state-file,s", po::value(&args.state_file), "set path to state file")
        ("save-state-period,p", po::value<int>(&args.save_state_period), "set period in ms for autosave");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }

    if (!vm.contains("config-file"s)) {
        throw std::runtime_error(std::string(ErrorMessage::UNKNOWN_SERVER_CONFIG));
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error(std::string(ErrorMessage::INVALID_STATIC_ROOT));
    }

    if (vm.contains("randomize-spawn-points"s)) {
        args.randomize_spawn_points = true; // указан ключ --randomize-spawn-points
    } else {
        args.randomize_spawn_points = false;
    }

    if (vm.contains("tick-period"s)) {
        args.tick_period = vm["tick-period"s].as<int>(); // указан ключ --tick-period и значение
    }

    return args;
}

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    // Инициализируем логирование
    server_logger::InitBoostLog();  

    try {
        // Парсим командную строку
        auto args = ParseCommandLine(argc, argv); 
        if (args == std::nullopt) { // ключ -h
            return EXIT_SUCCESS;
        }

        // Загружаем карту из файла и строим модель игры
        model::Game game = json_loader::LoadGame(args->config_file);
        
        // Получаем путь к каталогу статических файлов
        fs::path static_root = args->www_root;
        if (!fs::exists(static_root) || !fs::is_directory(static_root)) {
            server_logger::LogServerStop(EXIT_FAILURE, ErrorMessage::INVALID_STATIC_ROOT);
            return EXIT_FAILURE;
        }

        // Число потоков = число соединений в пуле ConnectionPool
        const unsigned num_threads = std::thread::hardware_concurrency(); 

        // Инициализируем io_context
        net::io_context ioc(num_threads);

        // Создаём объект Application, который содержит сценарии использования
        app::Application app{game, args->tick_period, args->randomize_spawn_points};

        // Создаем объект StateSaver для управления сохранением и загрузкой состояния игры
        state_saver::StateSaver state_saver(app, args->state_file, args->save_state_period);

        // Если в командной строке был указан путь, а файл был создан ранее, то загружаем состояние игры
        if (state_saver.IsPathSet() && state_saver.IsFileExists()) {
            try {
                state_saver.LoadState();
                server_logger::LogMessage(json::object{{"path", state_saver.GetPath()}}, "Game state loaded");
            } catch (const std::exception& ex) { // в случае ошибки при загрузке - логируем и завершаем
                server_logger::LogServerStop(EXIT_FAILURE, "Error loading state: "s + ex.what());
                return EXIT_FAILURE;
            }
        }

        // выполняем подключение к БД
        const char* db_url = std::getenv("GAME_DB_URL");
        if (!db_url) {
            server_logger::LogServerStop(EXIT_FAILURE, "GAME_DB_URL is not specified");
            return EXIT_FAILURE;
        }
        auto conn_pool = std::make_shared<postgres::ConnectionPool>(std::max(1u, num_threads), [db_url] {
            return std::make_shared<pqxx::connection>(db_url);
        });
        try {
            postgres::DataBase::Init(conn_pool);
            app.SetConnectionPool(conn_pool);
            server_logger::LogMessage(json::object{{"url", db_url}}, "DB initialized successfully");
        } catch (const std::exception& ex) {
            server_logger::LogServerStop(EXIT_FAILURE, "DB initialization failed: "s + ex.what());
            return EXIT_FAILURE;
        }

        // Сохраняем игровое состояния по таймеру, если был задан период, иначе в моменты шага Tick по времени
        sig::scoped_connection conn1 = app.DoOnTick(
            [total = 0ms, accumulated = 0ms, &state_saver](milliseconds delta) mutable {
                total += delta;
                accumulated += delta; // Увеличиваем накопленный интервал

                if (state_saver.IsPathSet()) { // Проверяем, указан ли путь
                    try {
                        if (state_saver.HasSavePeriod()) { // Если период сохранения задан
                            if (accumulated >= milliseconds(state_saver.GetSavePeriod())) {
                                state_saver.SaveState();
                                server_logger::LogMessage(json::object{{"path", state_saver.GetPath()}}, "Game state saved");
                                accumulated = 0ms; // Сбрасываем накопленный интервал после сохранения
                            }
                        } else { // Если период не указан, сохраняем каждый раз по сигналу
                            state_saver.SaveState();
                            server_logger::LogMessage(json::object{{"path", state_saver.GetPath()}}, "Game state saved");
                        }
                    } catch (const std::exception& ex) { // логируем ошибку и завершаем
                        server_logger::LogServerStop(EXIT_FAILURE, "Error saving state: "s + ex.what());
                        std::exit(EXIT_FAILURE);
                    }
                }
            });

        // Создаём strand для выполнения запросов к API
        auto api_strand = net::make_strand(ioc);

        // Настраиваем вызов метода Application::Tick каждые tick_period миллисекунд внутри strand
        if (app.HasTickPeriod()) {
            auto ticker = std::make_shared<ticker::Ticker>(api_strand, milliseconds(app.GetTickPeriod()),
                [&app](milliseconds delta) { app.Tick(delta); }
            );
            ticker->Start();
        }

        // Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM, при их получении завершаем работу сервера
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                server_logger::LogMessage(json::object{{"signal", signal_number}}, "Signal received"s);
                ioc.stop();
            }
        });

        // Создаём обработчик HTTP-запросов и связываем его с моделью игры и каталогом статических файлов
        auto handler = std::make_shared<http_handler::RequestHandler>(static_root, api_strand, app);

		const auto address = net::ip::make_address(ServerParam::ADDR);
		constexpr net::ip::port_type port = ServerParam::PORT;

        // Оборачиваем его в логирующий декоратор 
        server_logging::LoggingRequestHandler logging_handler{ // логирование
            [handler](auto&& req, auto&& send) {
                // Обрабатываем запрос
                (*handler)(std::forward<decltype(req)>(req),
                           std::forward<decltype(send)>(send));
            }
        };

        // Запускаем обработчик HTTP-запросов, делегируя их обработчику запросов
        http_server::ServeHttp(ioc, {address, port}, [&logging_handler](std::string_view client_ip, auto&& req, auto&& send) {
            logging_handler(client_ip, std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // Логирование запуска сервера
        server_logger::LogServerStart(address.to_string(), port);

        // Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        // Если указан путь, то сохраняем состояние игры
        if (state_saver.IsPathSet()) {
            try {
                state_saver.SaveState();
                server_logger::LogMessage(json::object{{"path", state_saver.GetPath()}}, "Game state saved");
            } catch (const std::exception& ex) { // в случае ошибки при сохранении - логируем и завершаем
                server_logger::LogServerStop(EXIT_FAILURE, "Error saving state: "s + ex.what());
                return EXIT_FAILURE;
            }
        }

    } catch (const std::exception& ex) {
        // Логирование остановки сервера с исключением
        server_logger::LogServerStop(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }

    // Логирование остановки сервера
    server_logger::LogServerStop(EXIT_SUCCESS);
}