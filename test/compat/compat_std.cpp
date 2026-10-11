#include "compat_types.hpp"

namespace compat
{
    bool std_reflection_enabled() { return FIELD_RFL_STD_REFLECTION; }

    result_map describe_std() { return describe_all(); }
}  // namespace compat
