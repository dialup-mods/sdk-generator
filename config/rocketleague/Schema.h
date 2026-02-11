//
// Schema.h - Unreal Engine type definitions
//
// See docs/SCHEMA.md for design philosophy and decorator usage.
//
#pragma once
#if defined(SDK_BUILD)
#define SDK_API __declspec(dllexport)
#else
#define SDK_API __declspec(dllimport)
#endif

#include <Windows.h> // for wide char conversion
#include "StringUtil.h"
#include <optional>
#include <string>

struct FFrame;
class FNameEntry;
class UClass;
class UField;
class UFunction;
class UObject;
class UStruct;
struct FStateFrame;
template<typename E, typename T> class TArray;
// for TArray
#include <cassert>
inline void check(bool expr) {
    assert(expr);
}
constexpr int INDEX_NONE = -1;
enum ENoInit { E_NoInit };
// end

#include "Flags.h"

using NativeFunc = void (UObject::*)(FFrame&, void*);
using tProcessEvent = void(__fastcall*)(UObject* self, UFunction* fn, void* params, void* result);

#pragma pack(push, 0x4)

/// @final
template<typename T>
class SDK_API TIterator {
    T* IteratorData;

public:
    explicit TIterator(T* inElementPointer)
        : IteratorData(inElementPointer) {}

    TIterator& operator++() {
        ++IteratorData;
        return *this;
    }

    TIterator operator++(int) {
        TIterator copy = *this;
        ++IteratorData;
        return copy;
    }

    auto operator->() -> T* { return IteratorData; }
    auto operator*() -> T& { return *IteratorData; }

    auto operator==(const TIterator& other) const -> bool { return (*this == other); }
    auto operator!=(const TIterator& other) const -> bool { return !(*this == other); }
};

// alias for convenience
// usage:
//   TSimpleIterator<int> it;
/// @final
//template<typename T>
//using TSimpleIterator = TIterator<TArray<T>>;

// Safe wrapper that prevents accidental copies
/// @final
template<typename T>
class TArrayView {
    T* Data;
    int32_t Count;

public:
    TArrayView(T* data, int32_t count) : Data(data), Count(count) {}

    // Delete copy/move - force const&
    TArrayView(const TArrayView&) = delete;
    TArrayView& operator=(const TArrayView&) = delete;

    // Iteration
    T* begin() { return Data; }
    T* end() { return Data + Count; }
    const T* begin() const { return Data; }
    const T* end() const { return Data + Count; }

    // Access
    T& operator[](int32_t i) { return Data[i]; }
    const T& operator[](int32_t i) const { return Data[i]; }

    int32_t size() const { return Count; }
    bool empty() const { return Count == 0; }
};

//template<typename T>
//class TArray {
//public:
//    T* ArrayData{};
//    int32_t ArrayCount{};
//    int32_t ArrayMax{};
//
//    TArray() = default;
//    ~TArray() = default;  // does NOTHING
//
//    // Must allow copy for generated classes
//    TArray(const TArray&) = default;
//    TArray& operator=(const TArray&) = default;
//
//    // Safe iteration via view
//    TArrayView<T> view() { return TArrayView<T>(ArrayData, ArrayCount); }
//    TArrayView<const T> view() const { return TArrayView<const T>(ArrayData, ArrayCount); }
//
//    auto operator[](int32_t i) -> T& { return ArrayData[i]; }
//    auto operator[](int32_t i) const -> const T& { return ArrayData[i]; }
//
//    auto data() -> T* { return ArrayData; }
//    auto data() const -> T* { return ArrayData; }
//    auto size() const -> int32_t { return ArrayCount; }
//    auto capacity() const -> int32_t { return ArrayMax; }
//    auto empty() const -> bool { return ArrayCount == 0; }
//
//    auto at(int32_t i) -> T& { return (*this)[i]; }
//    auto at(int32_t i) const -> const T& { return (*this)[i]; }
//    bool isValidIndex(const int32_t index) const {
//        return index >= 0 && index < size();
//    }
//
//    T* begin() { return data(); }
//    T* end() { return data() + size(); }
//    const T* begin() const { return data(); }
//    const T* end() const { return data() + size(); }
//};

/**
 * Generic non-const iterator which can operate on types that expose the following:
 * - A type called ElementType representing the contained type.
 * - A method IndexType Num() const that returns the number of items in the container.
 * - A method bool IsValidIndex(IndexType index) which returns whether a given index is valid in the container.
 * - A method T& operator(IndexType index) which returns a reference to a contained object by index.
 */
/// @final
template<typename ContainerType, typename IndexType = int >
class TIndexedContainerIterator
{
public:
    typedef ContainerType::ElementType ElementType;

    TIndexedContainerIterator(ContainerType& InContainer)
        :    Container( InContainer )
        ,    Index(0)
    {}

    TIndexedContainerIterator(const TIndexedContainerIterator& Other)
        :    Container( Other.Container )
        ,    Index( Other.Index )
    {}

    /** Advances iterator to the next element in the container. */
    TIndexedContainerIterator& operator++() {
        ++Index;
        return *this;
    }
    TIndexedContainerIterator operator++(int) {
        TIndexedContainerIterator Tmp(*this);
        ++Index;
        return Tmp;
    }

    /** Moves iterator to the previous element in the container. */
    TIndexedContainerIterator& operator--() {
        --Index;
        return *this;
    }
    TIndexedContainerIterator operator--(int) {
        TIndexedContainerIterator Tmp(*this);
        --Index;
        return Tmp;
    }

    /** pointer arithmetic support */
    TIndexedContainerIterator& operator+=(int Offset) {
        Index += Offset;
        return *this;
    }

    TIndexedContainerIterator operator+(int Offset) const {
        TIndexedContainerIterator Tmp(*this);
        return Tmp += Offset;
    }

    TIndexedContainerIterator& operator-=(int Offset) {
        return *this += -Offset;
    }

    TIndexedContainerIterator operator-(int Offset) const {
        TIndexedContainerIterator Tmp(*this);
        return Tmp -= Offset;
    }

    TIndexedContainerIterator operator()(int Offset) const {
        return *this + Offset;
    }

    /** @name Element access */
    ElementType& operator* () const {
        return Container( Index );
    }

    ElementType* operator-> () const {
        return &Container( Index );
    }

    /** conversion to "bool" returning true if the iterator has not reached the last element. */
    typedef bool PrivateBooleanType;
    operator PrivateBooleanType() const {
        return Container.IsValidIndex(Index) ? &TIndexedContainerIterator::Index : nullptr;
    }

    /** inverse of the "bool" operator */
    bool operator !() const {
        return !operator PrivateBooleanType();
    }

    /** Returns an index to the current element. */
    IndexType GetIndex() const {
        return Index;
    }

    /** Resets the iterator to the first element. */
    void Reset() {
        Index = 0;
    }
private:
    ContainerType&    Container;
    IndexType    Index;
};

/** operator + */
/// @final
template< typename ContainerType, typename IndexType >
TIndexedContainerIterator<ContainerType, IndexType> operator+(
	int Offset,
	TIndexedContainerIterator<ContainerType, IndexType> RHS) {
	return RHS + Offset;
}

/**
* Generic const iterator which can operate on types that expose the following:
* - A type called ElementType representing the contained type.
* - A method IndexType Num() const that returns the number of items in the container.
* - A method bool IsValidIndex(IndexType index) which returns whether a given index is valid in the container.
* - A method T& operator(IndexType index) const which returns a reference to a contained object by index.
*/
/// @final
template< typename ContainerType, typename IndexType = int >
class SDK_API TIndexedContainerConstIterator {
public:
    typedef typename ContainerType::ElementType ElementType;

    TIndexedContainerConstIterator(const ContainerType& InContainer)
        :    Container( InContainer )
        ,    Index(0)
    {}

    TIndexedContainerConstIterator(const TIndexedContainerConstIterator& Other)
        :    Container( Other.Container )
        ,    Index( Other.Index )
    {}

    /** Advances iterator to the next element in the container. */
    TIndexedContainerConstIterator& operator++() {
        ++Index;
        return *this;
    }
    TIndexedContainerConstIterator operator++(int) {
        TIndexedContainerConstIterator Tmp(*this);
        ++Index;
        return Tmp;
    }

    /** Moves iterator to the previous element in the container. */
    TIndexedContainerConstIterator& operator--() {
        --Index;
        return *this;
    }
    TIndexedContainerConstIterator operator--(int) {
        TIndexedContainerConstIterator Tmp(*this);
        --Index;
        return Tmp;
    }

    /** iterator arithmetic support */
    //TIndexedContainerConstIterator& operator+=(int Offset) {
    //    Index += Offset;
    //    return *this;
    //}

    TIndexedContainerConstIterator operator+(int Offset) const {
        TIndexedContainerConstIterator Tmp(*this);
        return Tmp += Offset;
    }

    TIndexedContainerConstIterator& operator-=(int Offset) {
        return *this += -Offset;
    }

    TIndexedContainerConstIterator operator-(int Offset) const {
        TIndexedContainerConstIterator Tmp(*this);
        return Tmp -= Offset;
    }

    TIndexedContainerConstIterator operator()(int Offset) const {
        return *this + Offset;
    }

    /** @name Element access */
    const ElementType& operator* () const {
        return Container( Index );
    }

    const ElementType* operator-> () const {
        return &Container( Index );
    }

    /** conversion to "bool" returning true if the iterator has not reached the last element. */
    typedef bool PrivateBooleanType;
    operator PrivateBooleanType() const {
        return Container.IsValidIndex(Index) ? &TIndexedContainerConstIterator::Index : nullptr;
    }

    bool operator !() const {
        return !operator PrivateBooleanType();
    }

    IndexType GetIndex() const {
        return Index;
    }

    void Reset() {
        Index = 0;
    }
private:
    const ContainerType&    Container;
    IndexType                Index;
};

/// @final
inline void DestructItems(int, int) {
    // intentionally empty
}

/// @final
struct SDK_API FArrayAllocator {
    void* Data = nullptr;
    static constexpr bool NeedsElementType = false;
    using ForAnyElementType = FArrayAllocator;

    void* GetAllocation() const { return Data; }

    void ResizeAllocation(int OldNum, int NewNum, size_t ElementSize) {
        Data = std::realloc(Data, NewNum * ElementSize);
    }

    size_t GetAllocatedSize(int Num, size_t ElementSize) const {
        return Num * ElementSize;
    }

    int CalculateSlack(int Num, int Max, size_t /*ElementSize*/) const {
        return Num; // no growth strategy for now
    }
};

/**
 * Templated dynamic array
 *
 * A dynamically sized array of typed elements.  Makes the assumption that your elements are relocate-able;
 * i.e. that they can be transparently moved to new memory without a copy constructor.  The main implication
 * is that pointers to elements in the TArray may be invalidated by adding or removing other elements to the array.
 * Removal of elements is O(N) and invalidates the indices of subsequent elements.
 *
 * Caution: as noted below some methods are not safe for element types that require constructors.
 *
 **/
/// @final

template<typename InElementType, typename Allocator = FArrayAllocator>
class TArray {
public:
    typedef InElementType ElementType;

    TArray()
    :   ArrayNum( 0 )
    ,    ArrayMax( 0 )
    {}
    TArray( ENoInit )
    :    AllocatorInstance(E_NoInit)
    {}

    /** Caution, this will create elements without calling the constructor and this is not appropriate for element types that require a constructor to function properly. */
    explicit TArray( int InNum )
    :   ArrayNum( InNum )
    ,    ArrayMax( InNum ) {
        AllocatorInstance.ResizeAllocation(0,ArrayMax,sizeof(ElementType));
    }

    auto empty() const -> bool { return ArrayNum == 0; }
    // Bounds-checked (UE-style: debug check, no throw)
    ElementType& at(int32_t i) {
        check(i >= 0 && i < ArrayNum);
        return (*this)(i);
    }

    ElementType& operator[](int i) {
        check(i >= 0 && i < ArrayNum);
        return (*this)(i);
    }

    const ElementType& operator[](int i) const {
        check(i >= 0 && i < ArrayNum);
        return (*this)(i);
    }

    const ElementType& at(int32_t i) const {
        check(i >= 0 && i < ArrayNum);
        return (*this)(i);
    }

    // STL-style iteration
    ElementType* begin() {
        return GetTypedData();
    }

    ElementType* end() {
        return GetTypedData() + ArrayNum;
    }

    const ElementType* begin() const {
        return GetTypedData();
    }

    const ElementType* end() const {
        return GetTypedData() + ArrayNum;
    }

    auto capacity() const -> int32_t { return ArrayMax; }
    ElementType* data() {
        return (ElementType*)AllocatorInstance.GetAllocation();
    }


    /**
     * Copy constructor. Use the common routine to perform the copy
     *
     * @param Other the source array to copy
     */
    //template<typename OtherAllocator>
    //TArray(const TArray<ElementType,OtherAllocator>& Other)
    //:   ArrayNum( 0 )
    //,    ArrayMax( 0 ) {
    //    Copy(Other);
    //}

    //TArray(const TArray<ElementType,Allocator>& Other)
    //:   ArrayNum( 0 )
    //,    ArrayMax( 0 ) {
    //    Copy(Other);
    //}

    //~TArray() {
    //    DestructItems(0,ArrayNum);
    //    ArrayNum = ArrayMax = 0;
    //}

    /**
     * Helper function for returning a typed pointer to the first array entry.
     *
     * @return pointer to first array entry or nullptr if ArrayMax==0
     */
    ElementType* GetTypedData() {
        return (ElementType*)AllocatorInstance.GetAllocation();
    }
    ElementType* GetData() {
        return (ElementType*)AllocatorInstance.GetAllocation();
    }
    /**
     * Helper function for returning a typed pointer to the first array entry.
     *
     * @return pointer to first array entry or nullptr if ArrayMax==0
     */
    const ElementType* GetTypedData() const {
        return (const ElementType*)AllocatorInstance.GetAllocation();
    }
    const ElementType* GetData() const {
        return (const ElementType*)AllocatorInstance.GetAllocation();
    }
    /**
     * Helper function returning the size of the inner type
     *
     * @return size in bytes of array type
     */
    uint32_t GetTypeSize() const {
        return sizeof(ElementType);
    }

    /**
     * Helper function to return the amount of memory allocated by this container
     *
     * @return number of bytes allocated by this container
     */
    uint32_t GetAllocatedSize( void ) const {
        return AllocatorInstance.GetAllocatedSize(ArrayMax, sizeof(ElementType));
    }

    /**
     * Returns the amount of slack in this array in elements.
     */
    int GetSlack() const {
        return ArrayMax - ArrayNum;
    }

    bool IsValidIndex( int i ) const {
        return i>=0 && i<ArrayNum;
    }
    int Num() const {
        return ArrayNum;
    }
    int size() const {
        return ArrayNum;
    }

    ElementType& operator()( int i ) {
        return GetTypedData()[i];
    }
    const ElementType& operator()( int i ) const {
        return GetTypedData()[i];
    }
    ElementType Pop() {
        check(ArrayNum>0);
        ElementType Result = GetTypedData()[ArrayNum-1];
        Remove( ArrayNum-1 );
        return Result;
    }
    void Push( const ElementType& Item ) {
        AddItem(Item);
    }
    ElementType& Top() {
        return Last();
    }
    const ElementType& Top() const {
        return Last();
    }
    ElementType& Last( int c=0 ) {
        check(AllocatorInstance.GetAllocation());
        check(c<ArrayNum);
        return GetTypedData()[ArrayNum-c-1];
    }
    const ElementType& Last( int c=0 ) const {
        check(GetTypedData());
        return GetTypedData()[ArrayNum-c-1];
    }
    void Shrink() {
        if( ArrayMax != ArrayNum )
        {
            ArrayMax = ArrayNum;
            AllocatorInstance.ResizeAllocation(ArrayNum,ArrayMax,sizeof(ElementType));
        }
    }
    bool FindItem( const ElementType& Item, int& Index ) const {
        const ElementType* const DataEnd = GetTypedData() + ArrayNum;
        for(const ElementType* Data = GetTypedData();
            Data < DataEnd;
            Data++
            )
        {
            if( *Data==Item )
            {
                Index = (int)(Data - GetTypedData());
                return true;
            }
        }
        return false;
    }
    int FindItemIndex( const ElementType& Item ) const {
        const ElementType* const DataEnd = GetTypedData() + ArrayNum;
        for(const ElementType* Data = GetTypedData();
            Data < DataEnd;
            Data++
            )
        {
            if( *Data==Item ) {
                return (int)(Data - GetTypedData());
            }
        }
        return INDEX_NONE;
    }
    /**
     * Finds an item by key (assuming the ElementType overloads operator== for the comparison).
     * @param Key    The key to search by
     * @return        Index to the first matching element, or INDEX_NONE if none is found
     */
    template <typename KeyType>
    int FindItemIndexByKey( const KeyType& Key ) const {
        const ElementType* const DataEnd = GetTypedData() + ArrayNum;
        for(const ElementType* Data = GetTypedData();
            Data < DataEnd;
            Data++
            )
        {
            if( *Data==Key ) {
                return (int)(Data - GetTypedData());
            }
        }
        return INDEX_NONE;
    }
    /**
     * Finds an item by key (assuming the ElementType overloads operator== for the comparison).
     * @param Key    The key to search by
     * @return        Pointer to the first matching element, or nullptr if none is found
     */
    template <typename KeyType>
    const ElementType* FindItemByKey( const KeyType& Key ) const {
        const ElementType* const DataEnd = GetTypedData() + ArrayNum;
        for(const ElementType* Data = GetTypedData();
            Data < DataEnd;
            Data++
            )
        {
            if( *Data==Key ) { return Data; }
        }
        return nullptr;
    }
    /**
     * Finds an item by key (assuming the ElementType overloads operator== for the comparison).
     * @param Key    The key to search by
     * @return        Pointer to the first matching element, or nullptr if none is found
     */
    template <typename KeyType>
    ElementType* FindItemByKey( const KeyType& Key ) {
        ElementType* const DataEnd = GetTypedData() + ArrayNum;
        for(ElementType* Data = GetTypedData();
            Data < DataEnd;
            Data++
            )
        {
            if( *Data==Key ) { return Data; }
        }
        return nullptr;
    }
    bool ContainsItem( const ElementType& Item ) const {
        return ( FindItemIndex(Item) != INDEX_NONE );
    }
    bool operator==(const TArray& OtherArray) const {
        if(Num() != OtherArray.Num()) { return false; }
        for(int Index = 0;Index < Num();Index++) {
            if(!((*this)(Index) == OtherArray(Index))) {
                return false;
            }
        }
        return true;
    }
    bool operator!=(const TArray& OtherArray) const {
        if(Num() != OtherArray.Num())
            return true;
        for(int Index = 0;Index < Num();Index++) {
            if(!((*this)(Index) == OtherArray(Index))) {
                return true;
            }
        }
        return false;
    }

    // Add, Insert, Remove, Empty interface.
    /** Caution, Add() will create elements without calling the constructor and this is not appropriate for element types that require a constructor to function properly. */
    int Add( int Count=1 ) {
        check(Count>=0);

        const int OldNum = ArrayNum;
        if( (ArrayNum+=Count)>ArrayMax ) {
            ArrayMax = AllocatorInstance.CalculateSlack( ArrayNum, ArrayMax, sizeof(ElementType) );
            AllocatorInstance.ResizeAllocation(OldNum,ArrayMax, sizeof(ElementType));
        }

        return OldNum;
    }
    /** Caution, Insert() will create elements without calling the constructor and this is not appropriate for element types that require a constructor to function properly. */
    void Insert( int Index, int Count=1 ) {
        check(Count>=0);
        check(ArrayNum>=0);
        check(ArrayMax>=ArrayNum);
        check(Index>=0);
        check(Index<=ArrayNum);

        const int OldNum = ArrayNum;
        if( (ArrayNum+=Count)>ArrayMax ) {
            ArrayMax = AllocatorInstance.CalculateSlack( ArrayNum, ArrayMax, sizeof(ElementType) );
            AllocatorInstance.ResizeAllocation(OldNum,ArrayMax,sizeof(ElementType));
        }
        std::memmove
        (
            (uint8_t*)AllocatorInstance.GetAllocation() + (Index+Count )*sizeof(ElementType),
            (uint8_t*)AllocatorInstance.GetAllocation() + (Index       )*sizeof(ElementType),
                                                           (OldNum-Index)*sizeof(ElementType)
        );
    }
    /** Caution, InsertZeroed() will create elements without calling the constructor and this is not appropriate for element types that require a constructor to function properly. */
    void InsertZeroed( int Index, int Count=1 ) {
        Insert( Index, Count );
        std::memset( (uint8_t*)AllocatorInstance.GetAllocation()+Index*sizeof(ElementType), Count*sizeof(ElementType) );
    }
    int InsertItem( const ElementType& Item, int Index ) {
        // construct a copy in place at Index (this new operator will insert at
        // Index, then construct that memory with Item)
        Insert(Index,1);
        new(GetTypedData() + Index) ElementType(Item);
        return Index;
    }
    void Remove( int Index, int Count=1 ) {
        check(Index>=0);
        check(Index<=ArrayNum);
        check(Index+Count<=ArrayNum);

        DestructItems(Index,Count);

        // Skip memmove in the common case that there is nothing to move.
        int NumToMove = ArrayNum - Index - Count;
        if( NumToMove ) {
            std::memmove
            (
                (uint8_t*)AllocatorInstance.GetAllocation() + (Index      ) * sizeof(ElementType),
                (uint8_t*)AllocatorInstance.GetAllocation() + (Index+Count) * sizeof(ElementType),
                NumToMove * sizeof(ElementType)
            );
        }
        ArrayNum -= Count;

        const int NewArrayMax = AllocatorInstance.CalculateSlack(ArrayNum,ArrayMax,sizeof(ElementType));
        if(NewArrayMax != ArrayMax) {
            ArrayMax = NewArrayMax;
            AllocatorInstance.ResizeAllocation(ArrayNum,ArrayMax,sizeof(ElementType));
        }
    }
    // RemoveSwap, this version is much more efficient O(Count) instead of O(ArrayNum), but does not preserve the order
    void RemoveSwap( int Index, int Count=1 ) {
        check(Index>=0);
        check(Index<=ArrayNum);
        check(Index+Count<=ArrayNum);

        DestructItems(Index,Count);


        // Replace the elements in the hole created by the removal with elements from the end of the array, so the range of indices used by the array is contiguous.
        const int NumElementsInHole = Count;
        const int NumElementsAfterHole = ArrayNum - (Index + Count);
        const int NumElementsToMoveIntoHole = std::min(NumElementsInHole,NumElementsAfterHole);
        if(NumElementsToMoveIntoHole) {
            std::memcpy(
                (uint8_t*)AllocatorInstance.GetAllocation() + (Index                             ) * sizeof(ElementType),
                (uint8_t*)AllocatorInstance.GetAllocation() + (ArrayNum-NumElementsToMoveIntoHole) * sizeof(ElementType),
                NumElementsToMoveIntoHole * sizeof(ElementType)
                );
        }
        ArrayNum -= Count;

        const int NewArrayMax = AllocatorInstance.CalculateSlack(ArrayNum,ArrayMax,sizeof(ElementType));
        if(NewArrayMax != ArrayMax) {
            ArrayMax = NewArrayMax;
            AllocatorInstance.ResizeAllocation(ArrayNum,ArrayMax,sizeof(ElementType));
        }
    }
    void Empty( int Slack=0 ) {
        DestructItems(0,ArrayNum);

        ArrayNum = 0;
        // only reallocate if we need to, I don't trust realloc to the same size to work
        if (ArrayMax != Slack) {
            ArrayMax = Slack;
            AllocatorInstance.ResizeAllocation(0,ArrayMax,sizeof(ElementType));
        }
    }
    void SetNum(int NewNum) {
        if (NewNum > Num()) {
            Add(NewNum-Num());
        }
        else if (NewNum < Num()) {
            Remove(NewNum, Num() - NewNum);
        }
    }

    /**
     * Appends the specified array to this array.
     * Cannot append to self.
     */
    // need TContainerTraits
    //void Append(const TArray& Source) {
    //    // Do nothing if the source and target match, or the source is empty.
    //    if ( this != &Source && Source.Num() > 0 ) {
    //        // Allocate memory for the new elements.
    //        Reserve( ArrayNum + Source.ArrayNum );

    //        if ( TContainerTraits<ElementType>::NeedsConstructor ) {
    //            // Construct each element.
    //            for ( int Index = 0 ; Index < Source.ArrayNum ; ++Index ) {
    //                new(GetTypedData() + ArrayNum + Index) ElementType(Source(Index));
    //            }
    //        }
    //        else {
    //            // Do a bulk copy.
    //            appMemcpy( (uint8_t*)AllocatorInstance.GetAllocation() + ArrayNum * sizeof(ElementType), Source.AllocatorInstance.GetAllocation(), sizeof(ElementType) * Source.ArrayNum );
    //        }
    //        ArrayNum += Source.ArrayNum;
    //    }
    //}

    //TArray& operator+=( const TArray& Other ) {
    //    Append( Other );
    //    return *this;
    //}

    //template<typename OtherAllocator>
    //TArray& operator=( const TArray<ElementType,OtherAllocator>& Other ) {
    //    Copy(Other);
    //    return *this;
    //}

    //TArray& operator=( const TArray<ElementType,Allocator>& Other ) {
    //    Copy(Other);
    //    return *this;
    //}

    /**
     * Adds a new item to the end of the array, possibly reallocating the whole array to fit.
     *
     * @param Item    The item to add
     * @return        Index to the new item
     */
    int AddItem( const ElementType& Item ) {
        const int Index = Add(1);
        new(GetTypedData() + Index) ElementType(Item);
        return Index;
    }
    /** Caution, AddZeroed() will create elements without calling the constructor and this is not appropriate for element types that require a constructor to function properly. */
    int AddZeroed( int Count=1 ) {
        const int Index = Add( Count );
        std::memset( (uint8_t*)AllocatorInstance.GetAllocation()+Index*sizeof(ElementType), Count*sizeof(ElementType) );
        return Index;
    }
    int AddUniqueItem( const ElementType& Item ) {
        for( int Index=0; Index<ArrayNum; Index++ )
            if( (*this)(Index)==Item )
                return Index;
        return AddItem( Item );
    }

    /**
     * Reserves memory such that the array can contain at least Number elements.
     */
    void Reserve(int Number) {
        if (Number > ArrayMax)
        {
            ArrayMax = Number;
            AllocatorInstance.ResizeAllocation(ArrayNum,ArrayMax,sizeof(ElementType));
        }
    }

    /** Sets the size of the array. */
    void Init(int Number) {
        Empty(Number);
        Add(Number);
    }

    /** Sets the size of the array, filling it with the given element. */
    void Init(const ElementType& Element,int Number) {
        for (int Index = 0; Index < Number; ++Index) {
            new(GetTypedData() + Index) ElementType(Element);
        }
        ArrayNum = Number;
    }

    /**
     * Removes the first occurrence of the specified item in the array, maintaining order but not indices.
     *
     * @param    Item    The item to remove
     *
     * @return    The number of items removed.  For RemoveSingleItem, this is always either 0 or 1.
     */
    int RemoveSingleItem( const ElementType& Item ) {
        // It isn't valid to specify an Item that is in the array, since removing that item will change Item's value.
        check( ((&Item) < GetTypedData()) || ((&Item) >= GetTypedData()+ArrayMax) );

        for( int Index=0; Index<ArrayNum; Index++ ) {
            if( GetTypedData()[Index] == Item ) {
                // Destruct items that match the specified Item.
                DestructItems(Index,1);
                const int NextIndex = Index + 1;
                if( NextIndex < ArrayNum ) {
                    const int NumElementsToMove = ArrayNum - NextIndex;
                    appMemmove(&GetTypedData()[Index],&GetTypedData()[NextIndex],sizeof(ElementType) * NumElementsToMove);
                }

                // Update the array count
                --ArrayNum;

                // Removed one item
                return 1;
            }
        }

        // Specified item was not found.  Removed zero items.
        return 0;
    }

    /** Removes as many instances of Item as there are in the array, maintaining order but not indices. */
    //int RemoveItem( const ElementType& Item ) {
    //    // It isn't valid to specify an Item that is in the array, since removing that item will change Item's value.
    //    check( ((&Item) < GetTypedData()) || ((&Item) >= GetTypedData()+ArrayMax) );

    //    const int OriginalNum = ArrayNum;
    //    if (!OriginalNum) {
    //        return 0; // nothing to do, loop assumes one item so need to deal with this edge case here
    //    }

    //    int WriteIndex = 0;
    //    int ReadIndex = 0;
    //    bool NotMatch = !(GetTypedData()[ReadIndex] == Item); // use a ! to guarantee it can't be anything other than zero or one
    //    do {
    //        int RunStartIndex = ReadIndex++;
    //        while (ReadIndex < OriginalNum && NotMatch == !(GetTypedData()[ReadIndex] == Item)) {
    //            ReadIndex++;
    //        }
    //        int RunLength = ReadIndex - RunStartIndex;
    //        if (NotMatch) {
    //            // this was a non-matching run, we need to move it
    //            if (WriteIndex != RunStartIndex)
    //            {
    //                appMemmove( &GetTypedData()[ WriteIndex ], &GetTypedData()[ RunStartIndex ], sizeof(ElementType) * RunLength );
    //            }
    //            WriteIndex += RunLength;
    //        } else {
    //            // this was a matching run, delete it
    //            DestructItems( RunStartIndex, RunLength );
    //        }
    //        NotMatch = !NotMatch;
    //    } while (ReadIndex < OriginalNum);

    //    ArrayNum = WriteIndex;
    //    return OriginalNum - ArrayNum;
    //}


    /**
     * Removes the first occurrence of the specified item in the array.  This version is much more efficient
     * O(Count) instead of O(ArrayNum), but does not preserve the order
     *
     * @param    Item    The item to remove
     *
     * @return    The number of items removed.  For RemoveSingleItem, this is always either 0 or 1.
     */
    int RemoveSingleItemSwap( const ElementType& Item ) {
        check( ((&Item) < (ElementType*)AllocatorInstance.GetAllocation()) || ((&Item) >= (ElementType*)AllocatorInstance.GetAllocation()+ArrayMax) );
        const int OriginalNum=ArrayNum;
        for( int Index=0; Index<ArrayNum; Index++ ) {
            if( (*this)(Index)==Item ) {
                RemoveSwap( Index-- );
                return 1; // Removed one item
            }
        }
        return 0;
    }

    /** RemoveItemSwap, this version is much more efficient O(Count) instead of O(ArrayNum), but does not preserve the order */
    int RemoveItemSwap( const ElementType& Item ) {
        check( ((&Item) < (ElementType*)AllocatorInstance.GetAllocation()) || ((&Item) >= (ElementType*)AllocatorInstance.GetAllocation()+ArrayMax) );
        const int OriginalNum=ArrayNum;
        for( int Index=0; Index<ArrayNum; Index++ ) {
            if( (*this)(Index)==Item ) {
                RemoveSwap( Index-- );
            }
        }
        return OriginalNum - ArrayNum;
    }

    void Swap(int A, int B) {
        uint8_t Temp[sizeof(ElementType)];
        std::memcpy(Temp,
            (uint8_t*)AllocatorInstance.GetAllocation() + sizeof(ElementType)*A,
            sizeof(ElementType));

        std::memcpy(
            (uint8_t*)AllocatorInstance.GetAllocation() + sizeof(ElementType)*A,
            (uint8_t*)AllocatorInstance.GetAllocation() + sizeof(ElementType)*B,
            sizeof(ElementType));

        std::memcpy(
            (uint8_t*)AllocatorInstance.GetAllocation() + sizeof(ElementType)*B,
            Temp,
            sizeof(ElementType));
    }

    void SwapItems(int A, int B) {
        check((A >= 0) && (B >= 0));
        check((ArrayNum > A) && (ArrayNum > B));
        if (A != B)
        {
            Swap(A,B);
        }
    }

    /**
     * Same as empty, but doesn't change memory allocations, unless the new size is larger than
     * the current array. It calls the destructors on held items if needed and then zeros the ArrayNum.
     *
     * @param NewSize the expected usage size
     */
    void Reset(int NewSize = 0) {
        // If we have space to hold the excepted size, then don't reallocate
        if (NewSize <= ArrayMax) {
            DestructItems(0,ArrayNum);
            ArrayNum = 0;
        }
        else {
            Empty(NewSize);
        }
    }

    /**
     * Searches for the first entry of the specified type, will only work
     * with TArray<UObject*>.  Optionally return the item's index, and can
     * specify the start index.
     */
    template<typename SearchType> bool FindItemByClass(SearchType **Item = nullptr, int *ItemIndex = nullptr, int StartIndex = 0) {
        UClass* SearchClass = SearchType::StaticClass();
        for (int Idx = StartIndex; Idx < ArrayNum; Idx++) {
            if ((*this)(Idx) != nullptr && (*this)(Idx)->IsA(SearchClass)) {
                if (Item != nullptr) {
                    *Item = (SearchType*)((*this)(Idx));
                }
                if (ItemIndex != nullptr) {
                    *ItemIndex = Idx;
                }
                return true;
            }
        }
        return false;
    }

    // Iterators
    typedef TIndexedContainerIterator< TArray<ElementType,Allocator> >  TIterator;
    typedef TIndexedContainerConstIterator< TArray<ElementType,Allocator> >  TConstIterator;

#if defined(_MSC_VER)
private:
    /**
    * Helper function that can be used inside the debuggers watch window to debug TArrays. E.g. "*Class->Defaults.DebugGet(5)".
    *
    * @param    i    Index
    * @return        pointer to type T at Index i
    */
    const ElementType& DebugGet( int i ) const {
        return GetTypedData()[i];
    }
#endif

protected:

    /**
     * Copies data from one array into this array. Uses the fast path if the
     * data in question does not need a constructor.
     *
     * @param Source the source array to copy
     */
    //template<typename OtherAllocator>
    //void Copy(const TArray<ElementType,OtherAllocator>& Source) {
    //    if ((void*)this != (void*)&Source) {
    //        // Just empty our array if there is nothing to copy
    //        if (Source.Num() > 0) {
    //            // Presize the array so there are no extra allocs/memcpys
    //            Empty(Source.Num());
    //            // Determine whether we need per element construction or bulk
    //            // copy is fine
    //            if (TContainerTraits<ElementType>::NeedsConstructor) {
    //                // Use the inplace new to copy the element to an array element
    //                for (int Index = 0; Index < Source.Num(); Index++) {
    //                    new(GetTypedData() + Index) ElementType(Source(Index));
    //                }
    //            }
    //            else {
    //                // Use the much faster path for types that allow it
    //                appMemcpy(AllocatorInstance.GetAllocation(),&Source(0),sizeof(ElementType) * Source.Num());
    //            }
    //            ArrayNum = Source.Num();
    //        }
    //        else {
    //            Empty(0);
    //        }
    //    }
    //}

    /** Destructs a range of items in the array. */
    //void DestructItems(int Index,int Count) {
    //    if( TContainerTraits<ElementType>::NeedsDestructor ) {
    //        for( int i=Index; i<Index+Count; i++ ) {
    //            (&(*this)(i))->~ElementType();
    //        }
    //    }
    //}

    using ElementAllocatorType = Allocator;
    ElementAllocatorType AllocatorInstance;
    int      ArrayNum;
    int      ArrayMax;
};

// view only
/// @final
template<typename TKey, typename TValue>
class SDK_API TMap {
public:
    struct TPair {
        TKey Key;
        TValue Value;
        //int32_t* HashNext;
    };

    using Iterator = TIterator<TPair>;

    // Public so you can read game's TMap layout
    TArray<TPair> Elements;
    uintptr_t IndirectData;
    int32_t InlineData[4];
    int32_t NumBits;
    int32_t MaxBits;
    int32_t FirstFreeIndex;
    int32_t NumFreeIndices;
    int64_t InlineHash;
    int32_t* Hash;
    int32_t HashCount;

    TMap() = default;

    // Read-only operations
    const TValue* find(const TKey& key) const {
        for (const TPair& pair : Elements) {
            if (pair.Key == key) {
                return &pair.Value;
            }
        }
        return nullptr;
    }

    const TPair& at_index(int32_t index) const {
        return Elements[index];
    }

    int32_t size() const { return Elements.size(); }
    bool empty() const { return Elements.empty(); }

    Iterator begin() { return Elements.begin(); }
    Iterator end() { return Elements.end(); }

    // No operator[] - use find() instead
    // No modifications - read-only for game maps
};

// consider these entries in a "string pool" or the base string
/// @final
class SDK_API FNameEntry {
  public:
    uint64_t Flags{0};
    int32_t Index{-1};
    uint32_t Pad0C{}; // for alignment on 64-bit
    FNameEntry* HashNext{};
    wchar_t Name[0x400]{ {} }; // NOLuint32_t(*-avoid-magic-numbers)

    FNameEntry() = default;
    ~FNameEntry() = default;

    auto GetFlags() const -> uint64_t { return Flags; }
    auto GetIndex() const -> int32_t { return Index; }
    auto GetWideName() const -> const wchar_t* { return &Name[0]; }
    [[nodiscard]] auto ToWideString() const -> std::wstring {
        return GetWideName();
    }

    [[nodiscard]] auto ToString() const -> std::string {
        const std::wstring wstr = ToWideString();
        if (wstr.empty())
            return {};

        const int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), sizeNeeded, nullptr, nullptr);
        return str;
    }
};

/// @final
class SDK_API FName {
  public:
    int32_t FNameEntryId{-1};
    int32_t InstanceNumber{0};

    FName() = default;
    FName(const FName&) = default;
    auto operator=(const FName&) -> FName& = default;

    //FName& operator=(const FName& other) {
    //    // Validate the entry exists before copying
    //    if (Runtime::fname::game_pool::isValid(other.FNameEntryId)) {
    //        FNameEntryId = other.FNameEntryId;
    //        InstanceNumber = other.InstanceNumber;
    //    } else {
    //        // Fallback to None
    //        FNameEntryId = 0;
    //        InstanceNumber = 0;
    //    }
    //    return *this;
    //}
    //
    //FName(int32_t id, int32_t instance) {
    //    if (Runtime::fname::::isValid(id)) {
    //        FNameEntryId = id;
    //        InstanceNumber = instance;
    //    } else {
    //        FNameEntryId = 0;
    //        InstanceNumber = 0;
    //    }
    //}

    auto operator==(const FName& other) const -> bool {
        return ((FNameEntryId == other.FNameEntryId) && (InstanceNumber == other.InstanceNumber));
    }
    auto operator!=(const FName& other) const -> bool { return !(*this == other); }

    // FNames are baked into the game
    // They are not modifiable
    // The engine can't/won't use plugin-defined FNames
    // DO NOT construct copies of game-owned objects with FName

    // Only allow construction from existing IDs
    explicit FName(const int32_t id, const int32_t instance = 0)
        : FNameEntryId(id), InstanceNumber(instance) {}

    auto ToString() const -> std::string;
    auto IsValid() const -> bool;
};

/// @final
inline uint32_t appStrlen(const wchar_t* s) {
    return s ? static_cast<uint32_t>(std::wcslen(s)) : 0;
}

/// @final
class SDK_API FString {
public:
    TArray<wchar_t> Data;

    FString() = default;

    explicit FString(const wchar_t* In) {
        if (In && *In) {
            const int32_t len = static_cast<int32_t>(std::wcslen(In));
            Data.Add(len + 1); // AddUninitialized
            std::memcpy(
                Data.GetData(),
                In,
                (len + 1) * sizeof(wchar_t)
            );
        }
    }
    explicit FString(const std::wstring& In) : FString(In.c_str()) {}
    explicit FString(std::wstring_view In) : FString(In.data()) {}


    // For in-place mutation, you need direct buffer access:
    wchar_t* GetMutableBuffer() {
        if (Data.Num() == 0) {
            Data.Add(256);  // Allocate space
            Data[0] = L'\0';
        }
        return Data.GetData();
    }

    // Safe version that ensures capacity
    void SetText(const std::wstring& text) {
        Data.Empty();
        Data.Reserve(text.length() + 1);
        for (wchar_t c : text) {
            Data.AddItem(c);
        }
        Data.AddItem(L'\0');
    }

    static const wchar_t* EmptyString() {
        static const wchar_t empty[1] = { L'\0' };
        return empty;
    }

    wchar_t& operator[](int i) { return Data(i); }
    const wchar_t& operator[](int i) const { return Data(i); }

    auto c_str() const -> const wchar_t* {
        return Data.Num() ? Data.GetData() : EmptyString();
    }

    auto ToWideString() const -> std::wstring {
        if (Data.Num() > 1) {
            return std::wstring(Data.GetData(), Data.Num() - 1);
        }
        return {};
    }

    auto ToString() const -> std::string {
        if (Data.Num() <= 1) return {};

        const wchar_t* w = Data.GetData();
        const int wlen = static_cast<int>(Data.Num() - 1);

        int size_needed = WideCharToMultiByte(
            CP_UTF8, 0,
            w, wlen,
            nullptr, 0,
            nullptr, nullptr
        );

        std::string str(size_needed, '\0');

        WideCharToMultiByte(
            CP_UTF8, 0,
            w, wlen,
            str.data(), size_needed,
            nullptr, nullptr
        );

        return str;
    }

    const wchar_t* operator*() const {
        return Data.Num() ? Data.GetData() : L"";
    }

    int32_t Len() const {
        return Data.Num() ? Data.Num() - 1 : 0;
    }

    bool empty() const {
        return Data.Num() == 0 || Data.GetData() == nullptr;
    }

    // Allows slack to be specified
    //FString( const FString& Other , uint32_t ExtraSlack) {
    //    TArray<wchar_t>::Empty(Other.ArrayNum + ExtraSlack);
    //    if( Other.ArrayNum ) {
    //        Add(Other.ArrayNum);
    //        std::memcpy( GetData(), Other.GetData(), ArrayNum*sizeof(wchar_t) );
    //    }
    //}
    //FString( const wchar_t* In )
    //: TArray<wchar_t>( In && *In ? (appStrlen(In)+1) : 0 ) {
    //    if( ArrayNum ) {
    //        std::memcpy( GetData(), In, ArrayNum*sizeof(wchar_t) );
    //    }
    //}

    //explicit FString( uint32_t InCount, const wchar_t* InSrc )
    //: TArray<wchar_t>( InCount ? InCount+1 : 0 ) {
    //    if( ArrayNum ) {
    //        std::memcpy( &(*this)(0), InSrc, InCount+1 );
    //    }
    //}

    //FString( ENoInit )
    //: TArray<wchar_t>( E_NoInit ) {}
    //FString& operator=( const wchar_t* Other ) {
    //    if( GetTypedData() != Other ) {
    //        ArrayNum = ArrayMax = *Other ? appStrlen(Other)+1 : 0;
    //        AllocatorInstance.ResizeAllocation(0,ArrayMax,sizeof(wchar_t));
    //        if( ArrayNum ) {
    //            std::memcpy( GetData(), Other, ArrayNum*sizeof(wchar_t) );
    //        }
    //    }
    //    return *this;
    //}
    //FString& operator=( const FString& Other )
    //{
    //    if( this != &Other ) {
    //        ArrayNum = ArrayMax = Other.Num();
    //        AllocatorInstance.ResizeAllocation(0,ArrayMax,sizeof(wchar_t));
    //        if( ArrayNum ) {
    //            std::memcpy( GetData(), *Other, ArrayNum*sizeof(wchar_t) );
    //        }
    //    }
    //    return *this;
    //}
    //wchar_t& operator[]( uint32_t i ) {
    //    check(i>=0);
    //    check(i<ArrayNum);
    //    check(ArrayMax>=ArrayNum);
    //    return GetTypedData()[i];
    //}
    //const wchar_t& operator[]( uint32_t i ) const {
    //    check(i>=0);
    //    check(i<ArrayNum);
    //    check(ArrayMax>=ArrayNum);
    //    return GetTypedData()[i];
    //}

    //void CheckInvariants() const {
    //    check(ArrayNum>=0);
    //    check(!ArrayNum || !(GetTypedData()[ArrayNum-1]));
    //    check(ArrayMax>=ArrayNum);
    //}
    //void Empty( uint32_t Slack=0 ) {
    //    TArray<wchar_t>::Empty(Slack > 0 ? Slack + 1 : Slack);
    //}
    ///** better than using Num() or Len() as string implementation might change and it can be more efficient */
    //bool IsEmpty() const {
    //    // can be optimized
    //    return Len() == 0;
    //}

    //void Shrink() {
    //    TArray<wchar_t>::Shrink();
    //}
    ///** Adjusts ArrayNum and ArrayMax to match the null terminated string stored in
    //    the data. Anything after the first null character is discarded. */
    //void TrimToNullTerminator() {
    //    if( GetTypedData() )
    //    {
    //        uint32_t DataLen = appStrlen(GetTypedData());
    //        check(DataLen==0||DataLen<ArrayMax);
    //        ArrayNum = ArrayMax = DataLen>0 ? DataLen+1 : 0;
    //        AllocatorInstance.ResizeAllocation(0,ArrayMax,sizeof(wchar_t));
    //    }
    //}
    ////const wchar_t* operator*() const {
    ////    return Num() ? &(*this)(0) : TEXT("");
    ////}
    //TArray<wchar_t>& GetCharArray() {
    //    //warning: Operations on the TArray<CHAR> can be unsafe, such as adding
    //    // non-terminating 0's or removing the terminating zero.
    //    return (TArray<wchar_t>&)*this;
    //}
    //const TArray<wchar_t>& GetCharArray() const {
    //    return (TArray<wchar_t>&)*this;
    //}
    //FString& operator+=( const wchar_t* Str ) {
    //    check(Str);
    //    CheckInvariants();

    //    if( *Str ) {
    //        uint32_t Index = ArrayNum;
    //        uint32_t NumAdd = appStrlen(Str)+1;
    //        uint32_t NumCopy = NumAdd;
    //        if (ArrayNum)
    //        {
    //            NumAdd--;
    //            Index--;
    //        }
    //        Add( NumAdd );
    //        appMemcpy( &(*this)(Index), Str, NumCopy * sizeof(wchar_t) );
    //    }
    //    return *this;
    //}

    //FString& operator +=( const ANSICHAR* Str ) {
    //    return *this += ANSI_TO_wchar_t(Str);
    //}

    //FString& operator+=(const wchar_t inChar) {
    //    check(ArrayMax>=ArrayNum);
    //    check(ArrayNum>=0);

    //    if ( inChar != '\0' ) {
    //        // position to insert the character.
    //        // At the end of the string if we have existing characters, otherwise at the 0 position
    //        uint32_t InsertIndex = (ArrayNum > 0) ? ArrayNum-1 : 0;

    //        // number of characters to add.  If we don't have any existing characters,
    //        // we'll need to append the terminating zero as well.
    //        uint32_t InsertCount = (ArrayNum > 0) ? 1 : 2;

    //        Add(InsertCount);
    //        (*this)(InsertIndex) = inChar;
    //        (*this)(InsertIndex+1) = '\0';
    //    }
    //    return *this;
    //}
    //FString& AppendChar(const wchar_t inChar) {
    //    *this += inChar;
    //    return *this;
    //}
    //FString& operator+=( const FString& Str ) {
    //    CheckInvariants();
    //    Str.CheckInvariants();
    //    if( Str.ArrayNum )
    //    {
    //        uint32_t Index = ArrayNum;
    //        uint32_t NumAdd = Str.ArrayNum;
    //        if (ArrayNum) {
    //            NumAdd--;
    //            Index--;
    //        }
    //        Add( NumAdd );
    //        std::memcpy( &(*this)(Index), Str.GetData(), Str.ArrayNum * sizeof(wchar_t) );
    //    }
    //    return *this;
    //}

    //FString operator+( const wchar_t inChar ) const {
    //    CheckInvariants();
    //    return FString(*this, 2) += inChar; // may have an extra character of "slack"
    //}

    //FString operator+( const wchar_t* Str ) const {
    //    check(Str);
    //    CheckInvariants();

    //    if( *Str )
    //    {
    //        uint32_t Index = ArrayNum;
    //        uint32_t NumAdd = appStrlen(Str)+1;
    //        uint32_t NumCopy = NumAdd;
    //        if (ArrayNum)
    //        {
    //            NumAdd--;
    //            Index--;
    //        }
    //        FString Ret( *this, NumAdd ) ;
    //        Ret.Add( NumAdd );
    //        appMemcpy( &(Ret)(Index), Str, NumCopy * sizeof(wchar_t) );
    //        return Ret;
    //    }
    //    return *this;
    //}

    //FString operator+( const FString& Str ) const {
    //    CheckInvariants();
    //    Str.CheckInvariants();
    //    if( Str.ArrayNum )
    //    {
    //        uint32_t Index = ArrayNum;
    //        uint32_t NumAdd = Str.ArrayNum;
    //        if (ArrayNum)
    //        {
    //            NumAdd--;
    //            Index--;
    //        }
    //        FString ret( *this, NumAdd ) ;
    //        ret.Add( NumAdd );
    //        std::memcpy( &(ret)(Index), Str.GetData(), Str.ArrayNum * sizeof(wchar_t) );
    //        return ret;
    //    }
    //    return *this;
    //}
    //FString& operator*=( const wchar_t* Str ) {
    //    if( ArrayNum>1 && (*this)(ArrayNum-2)!=PATH_SEPARATOR[0] )
    //        *this += PATH_SEPARATOR;
    //    return *this += Str;
    //}
    //FString& operator*=( const FString& Str ) {
    //    return operator*=( *Str );
    //}
    //FString operator*( const wchar_t* Str ) const {
    //    return FString( *this ) *= Str;
    //}
    //FString operator*( const FString& Str ) const {
    //    return operator*( *Str );
    //}
    //bool operator<=( const wchar_t* Other ) const {
    //    return !(appStricmp( **this, Other ) > 0);
    //}
    //bool operator<( const wchar_t* Other ) const {
    //    return appStricmp( **this, Other ) < 0;
    //}
    //bool operator<( const FString& Other ) const {
    //    return appStricmp( **this, *Other ) < 0;
    //}
    //bool operator>=( const wchar_t* Other ) const {
    //    return !(appStricmp( **this, Other ) < 0);
    //}
    //bool operator>( const wchar_t* Other ) const {
    //    return appStricmp( **this, Other ) > 0;
    //}
    //bool operator>( const FString& Other ) const {
    //    return appStricmp( **this, *Other ) > 0;
    //}
    //bool operator==( const wchar_t* Other ) const {
    //    return appStricmp( **this, Other )==0;
    //}
    //bool operator==( const FString& Other ) const {
    //    return appStricmp( **this, *Other )==0;
    //}
    //bool operator!=( const wchar_t* Other ) const {
    //    return appStricmp( **this, Other )!=0;
    //}
    //bool operator!=( const FString& Other ) const {
    //    return appStricmp( **this, *Other )!=0;
    //}
    //bool operator<=(const FString& Str) const {
    //    return !(appStricmp(**this, *Str) > 0);
    //}
    //bool operator>=(const FString& Str) const {
    //    return !(appStricmp(**this, *Str) < 0);
    //}
    //FString Left( uint32_t Count ) const {
    //    return FString( Clamp(Count,0,Len()), **this );
    //}
    //FString LeftChop( uint32_t Count ) const {
    //    return FString( Clamp(Len()-Count,0,Len()), **this );
    //}
    //FString Right( uint32_t Count ) const {
    //    return FString( **this + Len()-Clamp(Count,0,Len()) );
    //}
    //FString RightChop( uint32_t Count ) const {
    //    return FString( **this + Len()-Clamp(Len()-Count,0,Len()) );
    //}
    //FString Mid( uint32_t Start, uint32_t Count=MAXuint32_t ) const {
    //    DWORD End = Start+Count;
    //    Start    = Clamp( (DWORD)Start, (DWORD)0,     (DWORD)Len() );
    //    End      = Clamp( (DWORD)End,   (DWORD)Start, (DWORD)Len() );
    //    return FString( End-Start, **this + Start );
    //}

    //@{
    /**
     * Searches the string for a substring, and returns index into this string
     * of the first found instance. Can search from beginning or end, and ignore case or not.
     *
     * @param SubStr The string to search for
     * @param bSearchFromEnd If TRUE, the search will start at the end of the string and go backwards
     * @param bIgnoreCase If TRUE, the search will be case insensitive
     */
    //uint32_t InStr( const wchar_t* SubStr, bool bSearchFromEnd=FALSE, bool bIgnoreCase=FALSE, uint32_t StartPosition=INDEX_NONE ) const {
    //    if ( SubStr == NULL ) {
    //        return INDEX_NONE;
    //    }
    //    if( !bSearchFromEnd ) {
    //        const wchar_t* Start = **this;
    //        if ( StartPosition != INDEX_NONE ) {
    //            Start += Clamp(StartPosition, 0, Len() - 1);
    //        }
    //        const wchar_t* Tmp = bIgnoreCase
    //            ? appStristr(Start, SubStr)
    //            : appStrstr(Start, SubStr);

    //        return Tmp ? (Tmp-**this) : -1;
    //    } else {
    //        // if ignoring, do a onetime ToUpper on both strings, to avoid ToUppering multiple
    //        // times in the loop below
    //        if (bIgnoreCase) {
    //            return ToUpper().InStr(FString(SubStr).ToUpper(), TRUE, FALSE, StartPosition);
    //        } else {
    //            const uint32_t SearchStringLength=Max(1, appStrlen(SubStr));
    //            if ( StartPosition == INDEX_NONE ) {
    //                StartPosition = Len();
    //            }
    //            for( uint32_t i = StartPosition - SearchStringLength; i >= 0; i-- ) {
    //                uint32_t j;
    //                for( j=0; SubStr[j]; j++ ) {
    //                    if( (*this)(i+j)!=SubStr[j] ) {
    //                        break;
    //                    }
    //                }
    //                if( !SubStr[j] ) {
    //                    return i;
    //                }
    //            }
    //            return -1;
    //        }
    //    }
    //}
    //uint32_t InStr( const FString& SubStr, bool bSearchFromEnd=FALSE, bool bIgnoreCase=FALSE, uint32_t StartPosition=INDEX_NONE ) const {
    //    return InStr( *SubStr, bSearchFromEnd, bIgnoreCase, StartPosition );
    //}

    //bool Split( const FString& InS, FString* LeftS, FString* RightS, bool InRight=0 ) const {
    //    uint32_t InPos = InStr(InS,InRight);
    //    if( InPos<0 )
    //        return 0;
    //    if( LeftS )
    //        *LeftS = Left(InPos);
    //    if( RightS )
    //        *RightS = Mid(InPos+InS.Len());
    //    return 1;
    //}
    //FString ToUpper() const {
    //    FString New( **this );
    //    for( uint32_t i=0; i< New.ArrayNum; i++ )
    //        New(i) = appToUpper(New(i));
    //    return New;
    //}
    //FString ToLower() const {
    //    FString New( **this );
    //    for( uint32_t i=0; i<New.ArrayNum; i++ )
    //        New(i) = appToLower(New(i));
    //    return New;
    //}
    //bool Tobool() const {
    //    bool bIsTrue
    //        =    !appStricmp(**this, TEXT("On"))
    //        ||    !appStricmp(**this, TEXT("True"))
    //        ||    !appStricmp(**this, GTrue)
    //        ||    !appStricmp(**this, TEXT("1"));
    //    return bIsTrue;
    //}
    //FString LeftPad( uint32_t ChCount ) const;
    //FString RightPad( uint32_t ChCount ) const;

    //bool IsNumeric() const;

    //VARARG_DECL( static FString, static FString, return, Printf, VARARG_NONE, const wchar_t*, VARARG_NONE, VARARG_NONE );

    //static FString Chr( wchar_t Ch );
    //friend FArchive& operator<<( FArchive& Ar, FString& S );
    //friend struct FStringNoInit;

    //bool StartsWith(const FString& InPrefix ) const {
    //    return InPrefix.Len() > 0 && !appStrnicmp(**this, *InPrefix, InPrefix.Len());
    //}

    //bool EndsWith(const FString& InSuffix ) const {
    //    return InSuffix.Len() > 0 &&
    //           Len() >= InSuffix.Len() &&
    //           !appStricmp( &(*this)( Len() - InSuffix.Len() ), *InSuffix );
    //}

    //FString Trim() {
    //    uint32_t Pos = 0;
    //    while(Pos < Len()) {
    //        if( appIsWhitespace( (*this)[Pos] ) ) {
    //            Pos++;
    //        } else {
    //            break;
    //        }
    //    }
    //    *this = Right( Len()-Pos );
    //    return *this;
    //}

    //FString TrimTrailing() {
    //    uint32_t Pos = Len() - 1;
    //    while( Pos >= 0 ) {
    //        if( !appIsWhitespace( ( *this )[Pos] ) ) {
    //            break;
    //        }
    //        Pos--;
    //    }
    //    *this = Left( Pos + 1 );
    //    return( *this );
    //}

    /**
     * Returns a copy of this string with wrapping quotation marks removed.
     */
    //FString TrimQuotes( bool* bQuotesRemoved=NULL ) const {
    //    bool bQuotesWereRemoved=FALSE;
    //    uint32_t Start = 0, Count = Len();
    //    if ( Count > 0 ) {
    //        if ( (*this)[0] == wchar_t('"') ) {
    //            Start++;
    //            Count--;
    //            bQuotesWereRemoved=TRUE;
    //        }
    //        if ( Len() > 1 && (*this)[Len() - 1] == wchar_t('"') ) {
    //            Count--;
    //            bQuotesWereRemoved=TRUE;
    //        }
    //    }

    //    if ( bQuotesRemoved != NULL ) {
    //        *bQuotesRemoved = bQuotesWereRemoved;
    //    }
    //    return Mid(Start, Count);
    //}

    /**

    /**
     * Takes an array of strings and removes any zero length entries.
     * @param    InArray    The array to cull
     * @return    The number of elements left in InArray
     */
    //static uint32_t CullArray( TArray<FString>* InArray ) {
    //    check(InArray);
    //    FString Empty;
    //    InArray->RemoveItem(Empty);
    //    return InArray->Num();
    //}

    // copy
    //FString Reverse() const {
    //    FString New(*this);
    //    New.ReverseString();
    //    return New;
    //}

    //// in place
    //void ReverseString() {
    //    if ( Len() > 0 ) {
    //        wchar_t* StartChar = &(*this)(0);
    //        wchar_t* EndChar = &(*this)(Len()-1);
    //        wchar_t TempChar;
    //        do
    //        {
    //            TempChar = *StartChar;    // store the current value of StartChar
    //            *StartChar = *EndChar;    // change the value of StartChar to the value of EndChar
    //            *EndChar = TempChar;    // change the value of EndChar to the character that was previously at StartChar

    //            StartChar++;
    //            EndChar--;

    //        } while( StartChar < EndChar );    // repeat until we've reached the midpoint of the string
    //    }
    //}

    //// Replace all occurrences of a substring
    //FString Replace(const wchar_t* From, const wchar_t* To, bool bIgnoreCase=FALSE) const;
    //uint32_t ReplaceInline( const wchar_t* SearchText, const wchar_t* ReplacementText );

    //// fixme RESEARCH THESE
    //FString ReplaceQuotesWithEscapedQuotes() const;
    //FString ReplaceCharWithEscapedChar( const TArray<wchar_t>* Chars=NULL ) const;
    //FString ReplaceEscapedCharWithChar( const TArray<wchar_t>* Chars=NULL ) const;
    //FString ConvertTabsToSpaces (const uint32_t InSpacesPerTab);

    //static FString FormatAsNumber( uint32_t InNumber ) {
    //    FString Number = appItoa( InNumber ), Result;

    //    int dec = 0;
    //    for( int x = Number.Len()-1 ; x > -1 ; --x ) {
    //        Result += Number.Mid(x,1);

    //        dec++;
    //        if( dec == 3 && x > 0 ) {
    //            Result += TEXT(",");
    //            dec = 0;
    //        }
    //    }

    //    return Result.Reverse();
    //}

    //// To allow more efficient memory handling, automatically adds one for the string termination.
    //void Reserve(const Uuint32_t CharacterCount) {
    //    // plus one the for terminator
    //    TArray<wchar_t>::Reserve(CharacterCount + 1);
    //}
};

//struct FStringNoInit : public FString {
//    FStringNoInit() : FString( E_NoInit ) {}
//    explicit FStringNoInit( EForceInit ) {};
//
//    FStringNoInit& operator=( const wchar_t* Other ) {
//        if( GetTypedData() != Other ) {
//            ArrayNum = ArrayMax = *Other ? appStrlen(Other)+1 : 0;
//            AllocatorInstance.ResizeAllocation(0,ArrayMax,sizeof(wchar_t));
//            if( ArrayNum ) {
//                appMemcpy( GetData(), Other, ArrayNum*sizeof(wchar_t) );
//            }
//        }
//        return *this;
//    }
//    FStringNoInit& operator=( const FString& Other ) {
//        if( this != &Other ) {
//            ArrayNum = ArrayMax = Other.Num();
//            AllocatorInstance.ResizeAllocation(0,ArrayMax,sizeof(wchar_t));
//            if( ArrayNum ) {
//                appMemcpy( GetData(), *Other, ArrayNum*sizeof(wchar_t) );
//            }
//        }
//        return *this;
//    }
//};

/// @final
//class SDK_API FString {
//  public:
//    wchar_t* ArrayData{};
//    int32_t ArrayCount{};
//    int32_t ArrayMax{};
//
//    FString() = default;
//    ~FString() = default;
//
//    FString(const std::wstring_view sv) {
//        if (sv.empty()) {
//            ArrayData = nullptr;
//            ArrayCount = ArrayMax = 0;
//            return;
//        }
//
//        ArrayCount = static_cast<int32_t>(sv.size() + 1);
//        ArrayMax   = ArrayCount;
//        ArrayData  = new wchar_t[ArrayCount];
//
//        wmemcpy(ArrayData, sv.data(), sv.size());
//        ArrayData[sv.size()] = L'\0';
//    }
//
//    [[nodiscard]] auto ToWideString() const -> std::wstring {
//        if (!empty()) {
//            return c_str();
//        }
//        return L"";
//    }
//
//    [[nodiscard]] auto ToString() const -> std::string {
//        std::wstring wstr = ToWideString();
//        if (wstr.empty()) return {};
//
//        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
//        std::string str(size_needed, 0);
//        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), size_needed, nullptr, nullptr);
//        return str;
//    }
//
//    auto c_str() const -> const wchar_t* { return ArrayData; }
//
//    [[nodiscard]] auto empty() const -> bool {
//        if (ArrayData) {
//            return (ArrayCount == 0);
//        }
//        return true;
//    }
//
//    auto length() const -> int32_t { return ArrayCount; }
//    auto size() const -> int32_t { return ArrayMax; }
//
//    //auto operator=(const wchar_t* other) -> FString& { assign(other); return *this; }
//    //auto operator=(const FString& other) -> FString& { assign(other.c_str()); return *this; }
//
//    auto operator==(const FString& other) const -> bool {
//        if (ArrayData == other.ArrayData) { return true; }
//        if (!ArrayData || !other.ArrayData) { return false; }
//        return (wcscmp(ArrayData, other.ArrayData) == 0);
//    }
//
//    auto operator!=(const FString& other) const -> bool {
//        if (ArrayData == other.ArrayData) { return false; }
//        if (!ArrayData || !other.ArrayData) { return true; }
//        return (wcscmp(ArrayData, other.ArrayData) != 0);
//    }
//};

//struct FStringNoInit : public FString {
//    FStringNoInit() : FString( E_NoInit ) {}
//    explicit FStringNoInit( EForceInit ) {};
//
//    FStringNoInit& operator=( const wchar_t* Other ) {
//        if( GetTypedData() != Other ) {
//            ArrayNum = ArrayMax = *Other ? appStrlen(Other)+1 : 0;
//            AllocatorInstance.ResizeAllocation(0,ArrayMax,sizeof(wchar_t));
//            if( ArrayNum ) {
//                appMemcpy( GetData(), Other, ArrayNum*sizeof(wchar_t) );
//            }
//        }
//        return *this;
//    }
//    FStringNoInit& operator=( const FString& Other ) {
//        if( this != &Other ) {
//            ArrayNum = ArrayMax = Other.Num();
//            AllocatorInstance.ResizeAllocation(0,ArrayMax,sizeof(wchar_t));
//            if( ArrayNum ) {
//                appMemcpy( GetData(), *Other, ArrayNum*sizeof(wchar_t) );
//            }
//        }
//        return *this;
//    }
//};

// THIS STRUCT CAN BE GAME SPECIFIC
/// @final
struct SDK_API FScriptDelegate {
    UObject* Object;
    uint8_t UnknownData[0x10]; // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)
    FName FunctionName;
};

/// @final
struct SDK_API FScriptInterface {
    UObject* Object{nullptr};
    void* Interface{nullptr};

    FScriptInterface() = default;
};

/// @final
struct SDK_API FPointer {
    // not a real UE class
    void* Ptr;
    explicit operator bool() const {
        return Ptr != nullptr;
    }
};

/// @final
struct SDK_API FQWord {
    int32_t A;
    int32_t B;
};

class SDK_API UObject {
  public:
    FPointer VfTableObject{};
    FPointer HashNext{};
    uint64_t ObjectFlags{};
    FPointer HashOuterNext{};
    FPointer StateFrame{};
    UObject* Linker{};
    FPointer LinkerIndex{};
    int32_t ObjectInternalInteger{};
    int32_t NetIndex{};
    UObject* Outer{};
    FName Name{};
    class UClass* Class{};
    UObject* ObjectArchetype{};

    static constexpr std::string_view className = "Class Core.Object";

    /// @inject-methods
    auto HasAnyFlags(EObjectFlags Flags) const -> bool;
    auto HasAllFlags(EObjectFlags Flags) const -> bool;

    auto GetName() const -> std::string;
    auto GetNameCPP() -> std::string;
    auto GetFullName() const -> std::string;
    auto GetPackageObj() const -> UObject*;
    /// @end-inject
    auto IsA(const UClass* uClass) const -> bool;
    ///
};

/// @replace
class SDK_API UField : public UObject {
  public:
    UField* Next{nullptr};
    //UField* SuperField{nullptr};
    uint8_t UnknownData00[0x8]{}; // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)

    static constexpr std::string_view className = "Class Core.Field";
};

/// @replace
class SDK_API UEnum : public UField {
  public:
    TArray<FName> Names{};

    static constexpr std::string_view className = "Class Core.Enum";
};

/// @replace
class SDK_API UConst : public UField {
  public:
    FString Value{};

    static constexpr std::string_view className = "Class Core.Const";
};

/// @replace
class SDK_API UProperty : public UField {
  public:
    int32_t ArrayDim{};
    int32_t ElementSize{};
    uint64_t PropertyFlags{};
    uint8_t UnknownData00[0x10]{}; // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)
    uint32_t PropertySize{};
    uint8_t UnknownData01[0x4]{}; // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)
    int32_t Offset{};
    uint8_t UnknownData02[0x2C]{}; // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)

    static constexpr std::string_view className = "Class Core.Property";
};

/// @replace
class SDK_API UStruct : public UField {
  public:
    uint8_t UnknownData00[0x10]{}; // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)
    UField* SuperField{nullptr};
    UField* Children{nullptr};
    int32_t PropertySize{};
    int32_t MinAlignment{};
    uint8_t UnknownData01[0x98]{}; // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)

    static constexpr std::string_view className = "Class Core.Struct";
};

class UStructProperty;

/// @replace
class SDK_API UFunction : public UStruct {
  public:
    uint64_t FunctionFlags{};
    uint16_t iNative{};
    uint16_t RepOffset{};
    FName FriendlyName{};
    uint8_t OperatorPrecedence{};
    uint8_t NumParms{};
    uint16_t ParmsSize{};
    unsigned long ReturnValueOffset{};
    //uint8_t UnknownData00[0xC]{}; // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)
    UStructProperty* FirstStructWithDefaults; // 4 bytes (32-bit)
    uint8_t UnknownData00[0x8];
    FPointer Func{};

    static constexpr std::string_view className = "Class Core.Function";
};

/// @replace
class SDK_API UScriptStruct : public UStruct {
  public:
    uint8_t UnknownData00[0x28]{}; // [USE THIS CLASSES PROPERTYSIZE IN RECLASS TO DETERMINE THE SIZE OF THE UNKNOWNDATA] // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)

    static constexpr std::string_view className = "Class Core.ScriptStruct";
};

/// @replace
class SDK_API UState : public UStruct {
  public:
    uint8_t UnknownData00[0x60]{}; // [USE THIS CLASSES PROPERTYSIZE IN RECLASS TO DETERMINE THE SIZE OF THE UNKNOWNDATA] // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)

    static constexpr std::string_view className = "Class Core.State";
};

/// @replace
class SDK_API UClass : public UState {
  public:
    uint8_t UnknownData00[0x228]{};

    static constexpr std::string_view className = "Class Core.Class";
};

/// @replace
class SDK_API UStructProperty : public UProperty {
  public:
    UStruct* Struct{};

    static constexpr std::string_view className = "Class Core.StructProperty";
};

/// @replace
class SDK_API UStrProperty : public UProperty {
  public:
    static constexpr std::string_view className = "Class Core.StrProperty";
};

/// @replace
class SDK_API UQWordProperty : public UProperty {
  public:
    static constexpr std::string_view className = "Class Core.UQWordProperty";
};

/// @replace
class SDK_API UObjectProperty : public UProperty {
  public:
    UClass* PropertyClass{nullptr};
    uint8_t UnknownData00[0x8]{}; // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)

    static constexpr std::string_view className = "Class Core.ObjectProperty";
};

/// @replace
class SDK_API UClassProperty : public UObjectProperty {
  public:
    UClass* MetaClass{nullptr};

    static constexpr std::string_view className = "Class Core.ClassProperty";
};

/// @replace
class SDK_API UComponentProperty : public UObjectProperty {
  public:
    static constexpr std::string_view className = "Class Core.ComponentProperty";
};

/// @replace
class SDK_API UNameProperty : public UProperty {
  public:
    static constexpr std::string_view className = "Class Core.NameProperty";
};

/// @replace
class SDK_API UMapProperty : public UProperty {
  public:
    UProperty* Key{nullptr};
    UProperty* Value{nullptr};

    static constexpr std::string_view className = "Class Core.MapProperty";
};

/// @replace
class SDK_API UIntProperty : public UProperty {
  public:
    static constexpr std::string_view className = "Class Core.IntProperty";
};

/// @replace
class SDK_API UInterfaceProperty : public UProperty {
  public:
    UClass* InterfaceClass{nullptr};
    uint8_t UnknownData00[0x8]{}; // NOLuint32_t(*-avoid-c-arrays, *-avoid-magic-numbers)

    static constexpr std::string_view className = "Class Core.InterfaceProperty";
};

/// @replace
class SDK_API UFloatProperty : public UProperty {
  public:
    static constexpr std::string_view className = "Class Core.FloatProperty";
};

/// @replace
class SDK_API UDelegateProperty : public UProperty {
  public:
    UFunction* DelegateFunction{};
    UFunction* SourceDelegate{};

    static constexpr std::string_view className = "Class Core.DelegateProperty";
};

/// @replace
class SDK_API UByteProperty : public UProperty {
  public:
    UEnum* Enum{nullptr};

    static constexpr std::string_view className = "Class Core.ByteProperty";
};

/// @replace
class SDK_API UBoolProperty : public UProperty {
  public:
    uint32_t BitMask {};

    static constexpr std::string_view className = "Class Core.BoolProperty";
};

/// @replace
class SDK_API UArrayProperty : public UProperty {
  public:
    UProperty* Inner{};

    static constexpr std::string_view className = "Class Core.ArrayProperty";
};

/// @final
struct FFrame {
    FFrame* PreviousFrame; // 0x00
    void* OutParms;        // 0x08 ← pointer
    void* ReturnValue;     // 0x10 ← pointer
    uint32_t ProbeMask;    // 0x18 ← 4-byte flags
    uint32_t CallDepth;    // 0x1C ← 4-byte depth value
    UStruct* Node;         // 0x28 ← points to UFunction/UStruct
    UObject* Object;       // 0x30
    uint8_t* Code;         // 0x38 ← bytecode pointer (nullptr in native)
    uint8_t* Locals;       // 0x40 ← stack locals for function
    int LineNumber;        // 0x48 ← optional, not always meaningful
};

#pragma pack(pop)