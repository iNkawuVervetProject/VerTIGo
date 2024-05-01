string(TIMESTAMP TIME "%s")
file(WRITE build_timestamp.h "#pragma once\n")
file(APPEND build_timestamp.h "#define BUILD_EPOCH ${TIME}\n")
