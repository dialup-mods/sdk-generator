#pragma once
template<typename T> class TArray;

template <typename T>
auto isProbablyValidTArray(const TArray<T*>& arr) -> bool {
    if (!arr.data()) { return false; }
    if (arr.size() < 0 || arr.capacity() < arr.size()) { return false; }

    // fixme, use optional constructor arg for min / max number of objects to be considered valid
    if (arr.size() < 100000) { return false; }
    if (arr.size() > 500000) { return false; }

    return true;
}
