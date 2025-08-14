#pragma once

#include <vpi/Status.h>
#include <stdexcept>

#define CHECK_VPI_STATUS(STMT) \
    do { \
        VPIStatus status = (STMT); \
        if (status != VPI_SUCCESS) { \
            throw std::runtime_error("VPI call failed: " #STMT); \
        } \
    } while (0);

#define VPI_SAFE_DESTROY(DESTROY_FUNC, VPI_OBJECT) \
    do { \
        if (VPI_OBJECT != nullptr) { \
            DESTROY_FUNC(VPI_OBJECT); \
            VPI_OBJECT = nullptr; \
        } \
    } while (0);
