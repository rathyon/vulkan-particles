#ifndef UTILS_HPP
#define UTILS_HPP

namespace vkdemo {
    static void check(const bool result, const std::string_view message = "") {
        if (!result) {
            throw std::runtime_error(message.data());
        }
    }

    static void check(void* pointer, const std::string_view message = "") {
        if (pointer == nullptr) {
            throw std::runtime_error(message.data());
        }
    }
}

#endif // UTILS_HPP
