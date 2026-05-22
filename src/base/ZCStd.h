// ZCStd.h - STL switcher header.
//
// Provides the `mstd` namespace alias that resolves either to the system C++
// standard library (`std`) or to the freestanding `zstl` implementation
// (`sys`). The switch is controlled by ZOCOS_USE_SYS_STL, defined by the
// build system when targeting an environment without a hosted libstdc++.
//
// This header is used as the engine PCH, so any translation unit gets these
// includes for free. Source files are still encouraged to `#include
// "base/ZCStd.h"` explicitly for IDE / clangd accuracy.
#pragma once

#if defined(ZOCOS_USE_SYS_STL)

#include <sys/algorithm.hpp>
#include <sys/array.hpp>
#include <sys/cstddef.hpp>
#include <sys/cstdint.hpp>
#include <sys/functional.hpp>
#include <sys/iterator.hpp>
#include <sys/limits.hpp>
#include <sys/memory.hpp>
#include <sys/new.hpp>
#include <sys/set.hpp>
#include <sys/string.hpp>
#include <sys/type_traits.hpp>
#include <sys/unordered_map.hpp>
#include <sys/utility.hpp>
#include <sys/vector.hpp>

namespace mstd = sys;

#else // hosted: route through libstdc++ / MSVC STL.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mstd = std;

#endif
