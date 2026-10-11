#ifndef FIELD_RFL_DISABLE_STD_REFLECTION
#define FIELD_RFL_DISABLE_STD_REFLECTION
#endif
#include "compat_types.hpp"

namespace compat
{
    result_map describe_legacy()
    {
        static_assert(!FIELD_RFL_STD_REFLECTION);
        return describe_all();
    }
}  // namespace compat
