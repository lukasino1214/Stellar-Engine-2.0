#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <fmt/format.h>

namespace Stellar {
	class Logger {
	public:
		static void init();

		static auto get_core_logger() -> std::shared_ptr<spdlog::logger>& { return core_logger; }
		static auto get_client_logger() -> std::shared_ptr<spdlog::logger>& { return client_logger; }
        static auto get_logs() -> std::shared_ptr<std::vector<std::string>>& { return logs; }

		static std::shared_ptr<spdlog::logger> core_logger;
		static std::shared_ptr<spdlog::logger> client_logger;
        static std::shared_ptr<std::vector<std::string>> logs;
	};

}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}

#define CORE_TRACE(...)                                             \
{                                                                   \
    std::string log = fmt::format(__VA_ARGS__);                     \
    ::Stellar::Logger::get_logs()->push_back(log);                  \
    ::Stellar::Logger::get_core_logger()->trace(__VA_ARGS__;)       \
}                                                                   \

#define CORE_INFO(...)                                              \
{                                                                   \
    std::string log = fmt::format(__VA_ARGS__);                     \
    ::Stellar::Logger::get_logs()->push_back(log);                  \
    ::Stellar::Logger::get_core_logger()->info(__VA_ARGS__);        \
}                                                                   \

#define CORE_WARN(...)                                              \
{                                                                   \
    std::string log = fmt::format(__VA_ARGS__);                     \
    ::Stellar::Logger::get_logs()->push_back(log);                  \
    ::Stellar::Logger::get_core_logger()->warn(__VA_ARGS__);        \
}                                                                   \

#define CORE_ERROR(...)                                             \
{                                                                   \
    std::string log = fmt::format(__VA_ARGS__);                     \
    ::Stellar::Logger::get_logs()->push_back(log);                  \
    ::Stellar::Logger::get_core_logger()->error(__VA_ARGS__);       \
}                                                                   \

#define CORE_CRITICAL(...)                                          \
{                                                                   \
    std::string log = fmt::format(__VA_ARGS__);                     \
    ::Stellar::Logger::get_logs()->push_back(log);                  \
    ::Stellar::Logger::get_core_logger()->critical(__VA_ARGS__);    \
}   





#define TRACE(...)                                                  \
{                                                                   \
    std::string log = fmt::format(__VA_ARGS__);                     \
    ::Stellar::Logger::get_logs()->push_back(log);                  \
    ::Stellar::Logger::get_client_logger()->trace(__VA_ARGS__;)     \
}                                                                   \

#define INFO(...)                                                   \
{                                                                   \
    std::string log = fmt::format(__VA_ARGS__);                     \
    ::Stellar::Logger::get_logs()->push_back(log);                  \
    ::Stellar::Logger::get_client_logger()->info(__VA_ARGS__);      \
}                                                                   \

#define WARN(...)                                                   \
{                                                                   \
    std::string log = fmt::format(__VA_ARGS__);                     \
    ::Stellar::Logger::get_logs()->push_back(log);                  \
    ::Stellar::Logger::get_client_logger()->warn(__VA_ARGS__);      \
}                                                                   \

#define ERROR(...)                                                  \
{                                                                   \
    std::string log = fmt::format(__VA_ARGS__);                     \
    ::Stellar::Logger::get_logs()->push_back(log);                  \
    ::Stellar::Logger::get_client_logger()->error(__VA_ARGS__);     \
}                                                                   \

#define CRITICAL(...)                                               \
{                                                                   \
    std::string log = fmt::format(__VA_ARGS__);                     \
    ::Stellar::Logger::get_logs()->push_back(log);                  \
    ::Stellar::Logger::get_client_logger()->critical(__VA_ARGS__);  \
}   

