// A value of 0 does not disable the C++26 backend.
#ifndef FIELD_RFL_DISABLE_STD_REFLECTION
#define FIELD_RFL_DISABLE_STD_REFLECTION 0
#define COMPAT_DISABLE_MACRO_IS_ZERO
#endif
#include "compat_types.hpp"

namespace compat
{
    bool std_reflection_enabled() { return FIELD_RFL_STD_REFLECTION; }

    result_map describe_std() { return describe_all(); }
}  // namespace compat

#if defined(COMPAT_DISABLE_MACRO_IS_ZERO) && defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
static_assert(FIELD_RFL_STD_REFLECTION);
#endif
