// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/edger8r/enclave.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/safecrt.h>
#include <openenclave/internal/safemath.h>
#include <openenclave/internal/stack_alloc.h>

#include "core_t.h"

/**
 * Declare the prototypes of the following functions to avoid the
 * missing-prototypes warning.
 */
oe_result_t _oe_log_is_supported_ocall();
oe_result_t _oe_log_ocall(uint32_t log_level, const char* message);
oe_result_t _oe_write_ocall(int device, const char* str, size_t maxlen);

/**
 * Make the following OCALLs weak to support the system EDL opt-in.
 * When the user does not opt into (import) the EDL, the linker will pick
 * the following default implementations. If the user opts into the EDL,
 * the implementions (which are strong) in the oeedger8r-generated code will be
 * used.
 */
oe_result_t _oe_log_is_supported_ocall()
{
    return OE_UNSUPPORTED;
}
OE_WEAK_ALIAS(_oe_log_is_supported_ocall, oe_log_is_supported_ocall);

oe_result_t _oe_log_ocall(uint32_t log_level, const char* message)
{
    OE_UNUSED(log_level);
    OE_UNUSED(message);
    return OE_UNSUPPORTED;
}
OE_WEAK_ALIAS(_oe_log_ocall, oe_log_ocall);

oe_result_t _oe_write_ocall(int device, const char* str, size_t maxlen)
{
    OE_UNUSED(device);
    OE_UNUSED(str);
    OE_UNUSED(maxlen);
    return OE_UNSUPPORTED;
}
OE_WEAK_ALIAS(_oe_write_ocall, oe_write_ocall);

void* oe_host_malloc(size_t size)
{
    uint64_t arg_in = size;
    uint64_t arg_out = 0;

    if (oe_ocall(OE_OCALL_MALLOC, arg_in, &arg_out) != OE_OK)
    {
        return NULL;
    }

    if (arg_out && !oe_is_outside_enclave((void*)arg_out, size))
        oe_abort();

    return (void*)arg_out;
}

oe_result_t _oe_realloc_ocall(void** retval, void* ptr, size_t size)
{
    OE_UNUSED(retval);
    OE_UNUSED(ptr);
    OE_UNUSED(size);
    return OE_UNEXPECTED;
}

OE_WEAK_ALIAS(_oe_realloc_ocall, oe_realloc_ocall);

void* oe_host_realloc(void* ptr, size_t size)
{
    void* retval = NULL;

    if (!ptr)
        return oe_host_malloc(size);

    if (oe_realloc_ocall(&retval, ptr, size) != OE_OK)
        return NULL;

    if (retval && !oe_is_outside_enclave(retval, size))
    {
        oe_assert("oe_host_realloc_ocall() returned non-host memory" == NULL);
        oe_abort();
    }

    return retval;
}

void oe_host_free(void* ptr)
{
    oe_ocall(OE_OCALL_FREE, (uint64_t)ptr, NULL);
}

int oe_host_vfprintf(int device, const char* fmt, oe_va_list ap_)
{
    char buf[256];
    char* p = buf;
    int n;

    /* Try first with a fixed-length scratch buffer */
    {
        oe_va_list ap;
        oe_va_copy(ap, ap_);
        n = oe_vsnprintf(buf, sizeof(buf), fmt, ap);
        oe_va_end(ap);
    }

    /* If string was truncated, retry with correctly sized buffer */
    if (n >= (int)sizeof(buf))
    {
        if (!(p = oe_stack_alloc((uint32_t)n + 1)))
            return -1;

        oe_va_list ap;
        oe_va_copy(ap, ap_);
        n = oe_vsnprintf(p, (size_t)n + 1, fmt, ap);
        oe_va_end(ap);
    }

    oe_host_write(device, p, (size_t)-1);

    return n;
}

int oe_host_printf(const char* fmt, ...)
{
    int n;

    oe_va_list ap;
    oe_va_start(ap, fmt);
    n = oe_host_vfprintf(0, fmt, ap);
    oe_va_end(ap);

    return n;
}

int oe_host_fprintf(int device, const char* fmt, ...)
{
    int n;

    oe_va_list ap;
    oe_va_start(ap, fmt);
    n = oe_host_vfprintf(device, fmt, ap);
    oe_va_end(ap);

    return n;
}

int oe_host_write(int device, const char* str, size_t len)
{
    if (oe_write_ocall(device, str, len) != OE_OK)
        return -1;

    return 0;
}
