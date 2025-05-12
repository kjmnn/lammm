#ifndef UTIL_H_
#define UTIL_H_

#include <memory>
#include <type_traits>
#include <string>

namespace lammm {

/// @brief Base class for exceptions
class Exception
{
  public:
    /// @brief Get exception type name
    /// @return Exception type name
    virtual std::string name() const = 0;
    /// @brief Get exception message
    /// @return Message explaining the exception
    virtual std::string message() const = 0;
};

/// @brief Overloaded visitor for @p std::variant copied directly from cppreference
/// @tparam ...Ts Overload types
template <typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

/// @brief Like @p std::unique_ptr but copyable and propagates const.
///        Inspired by https://www.foonathan.net/2022/05/recursive-variant-box/.
///        Pointer operations are simply delegated to the underlying unique pointer.
/// @tparam T Type of boxed object
template <typename T>
class Box
{
  public:
    using pointer = T*;
    using const_pointer = const T*;
    using element_type = T;

    /// @brief Unique pointer move constructor
    /// @param ptr Pointer to convert to Box
    Box(std::unique_ptr<T>&& ptr)
        : ptr{std::move(ptr)}
    {
    }

    /// @brief Move constructor
    /// @param other Box to move from
    Box(Box&& other)
        : ptr{std::move(other.ptr)}
    {
    }

    /// @brief Copy constructor
    /// @param other Box to copy from
    Box(const Box& other)
        : ptr{other.ptr ? new T(*other) : nullptr}
    {
    }

    /// @brief Copy assignment
    /// @param other Box to copy from
    /// @return Reference to this Box
    Box& operator=(const Box& other)
    {
        if (*this != other) {
            ptr = std::make_unique<T>(*other);
        }
        return *this;
    }

    /// @brief Make a new Box with a new object of type T
    /// @tparam ...Args Types of constructor arguments
    /// @param ...args Args to forward
    /// @return A new Box with a new object of type T
    template <typename... Args>
    static Box make(Args&&... args)
    {
        return Box{std::make_unique<T>(std::forward<Args>(args)...)};
    }

    pointer get()
    {
        return ptr.get();
    }

    const_pointer get() const
    {
        return ptr.get();
    }

    element_type& operator*()
    {
        return *ptr;
    }

    const element_type& operator*() const
    {
        return *ptr;
    }

    pointer operator->()
    {
        return ptr.get();
    }

    const_pointer operator->() const
    {
        return ptr.get();
    }

    operator bool() const
    {
        return ptr;
    }

    friend bool operator==(const Box a, const Box b)
    {
        return a.ptr == b.ptr;
    }

    friend bool operator!=(const Box a, const Box b)
    {
        return a.ptr != b.ptr;
    }

  private:
    /// @brief This friend declaration enables the derived move constructor
    /// @tparam Other Any other type
    template <typename Other>
    friend class Box;

    /// @brief The underlying unique pointer
    std::unique_ptr<T> ptr;
};

/// @brief A utility function to create a Box from a temporary object
/// @tparam T The type of the object
/// @param t The object
/// @return @p t in a Box
template <typename T>
Box<T> box(T&& t)
{
    return Box<T>::make(std::move(t));
}

} // namespace lammm

#endif