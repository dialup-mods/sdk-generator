#pragma once

static ptrdiff_t roundUp(const ptrdiff_t val, const ptrdiff_t align) {
    return (val + align - 1) & ~(align - 1);
}