#pragma once

#include "Memory.h"
#include "Debug/DebugLog.h"

namespace std
{
    template<class F, class...Args>
    auto bind_front(F&& f, Args&&...args) {
        return[f = std::forward<F>(f), tup = std::make_tuple(std::forward<Args>(args)...)](auto&&... more_args)
            ->decltype(auto)
        {
            return std::apply([&](auto&&...args)->decltype(auto) {
                return std::invoke(f, decltype(args)(args)..., decltype(more_args)(more_args)...);
                }, tup);
        };
    }
}
