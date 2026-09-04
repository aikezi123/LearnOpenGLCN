#pragma once
#include <array>
#include <cstddef>
#include <stdexcept>

namespace learnopengl::infrastructure {

template<typename T, std::size_t Capacity>
class ObjectPool {
public:
    static_assert(Capacity > 0, "ObjectPool capacity must be greater than 0");

    ObjectPool() = default;
    ~ObjectPool() = default;

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    // 申请一个空闲槽位，返回槽位下标
    std::size_t acquireSlot() {
        if (full()) {
            throw std::runtime_error("ObjectPool is full");
        }

        for (std::size_t index = 0; index < Capacity; ++index) {
            if (!m_used[index]) {
                m_used[index] = true;
                ++m_size;
                return index;
            }
        }

        throw std::runtime_error("ObjectPool internal state error");
    }

    void releaseSlot(std::size_t index) {
        if (index >= Capacity) {
            throw std::out_of_range("ObjectPool slot index out of range");
        }
        if (!m_used[index]) {
            throw std::logic_error("ObjectPool slot is already free");
        }

        m_used[index] = false;
        --m_size;
    }

    // 判断指定槽位是否正在使用
    [[nodiscard]] bool isUsed(std::size_t index) const {
        if (index >= Capacity) {
            throw std::out_of_range("ObjectPool slot index out of range");
        }

        return m_used[index];
    }

    // 总槽位数量
    [[nodiscard]] constexpr std::size_t capacity() const noexcept {
        return Capacity;
    }

    // 当前正在使用槽位数量
    [[nodiscard]] constexpr std::size_t  size() const noexcept {
        return m_size;
    }


    [[nodiscard]] bool empty() const noexcept {
        return m_size == 0;
    }

    [[nodiscard]] bool full() const noexcept {
        return m_size == Capacity;
    }
private:
    std::array<bool, Capacity> m_used{};
    std::size_t m_size{0};
};




}