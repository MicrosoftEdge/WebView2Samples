#pragma once

#include "Class.g.h"

namespace winrt::WinRTAdapter::implementation
{
    struct Class : ClassT<Class>
    {
        Class() = default;

        int32_t MyProperty();
        void MyProperty(int32_t value);
    };
}

namespace winrt::WinRTAdapter::factory_implementation
{
    struct Class : ClassT<Class, implementation::Class>
    {
    };
}
