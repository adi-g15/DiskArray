#pragma once

namespace util {
    // https://stackoverflow.com/a/257382/12339402
    // SFINAE test to check if has a function or not
    template <typename T>
    class has_SerializeToOstream
    {
        typedef char one;
        struct two { char x[2]; };

        template <typename C> static one test( decltype(&C::SerializeToOstream) ) ;
        template <typename C> static two test(...);

    public:
        enum { value = sizeof(test<T>(0)) == sizeof(char) };
    };
}
