

#ifndef INC_CSAFEARRAY_H
#define INC_CSAFEARRAY_H

// #include <stdio.h>
// #include <string.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "core/debug_impl.hpp"

namespace MetaEngine {

template <typename Type, typename SizeType = int>
class CSafeArray {
public:
    CSafeArray() : data(0), _size(SizeType()) {}
    CSafeArray(SizeType size) : data(new Type[size]), _size(size) {

        /*for( SizeType i = 0; i < Size(); ++i )
        data[ i ] = Type();*/

        memset(data, (int)Type(), Size() * sizeof(Type));
    }

    CSafeArray(const CSafeArray &other) : data(0), _size(SizeType()) { operator=(other); }

    ~CSafeArray() { Clear(); }

    const CSafeArray &operator=(const CSafeArray &other) {
        if (other._size != _size) {
            Clear();
            data = new Type[other._size];
            _size = other._size;
        }

        /*
    for( SizeType i = 0; i < Size(); ++i )
        data[ i ] = other.Rand( i );
    */
        // memcpy( data, other.data, Size() * sizeof( Type ) );
        fast_memcpy(data, other.data, Size() * sizeof(Type));
        // X_aligned_memcpy_sse2( data, other.data, Size() * sizeof( Type ) );

        return *this;
    }

    inline Type &operator[](SizeType i) {
        METADOT_ASSERT_E(!(i < 0 || i >= _size));
        return data[i];
    }

    inline const Type &operator[](SizeType i) const {
        METADOT_ASSERT_E(!(i < 0 || i >= _size));
        return data[i];
    }

    void Clear() {
        delete[] data;
        data = 0;
        _size = 0;
    }

    void clear() { Clear(); }

    SizeType Size() const { return _size; }
    SizeType size() const { return _size; }

    bool Empty() const { return _size == 0; }
    bool empty() const { return Empty(); }

    void Resize(SizeType s) {
        if (_size != s) {
            Clear();
            data = new Type[s];
            _size = s;

            /*for( SizeType i = 0; i < Size(); ++i )
            data[ i ] = Type();*/
            memset(data, (int)Type(), Size() * sizeof(Type));
        }
    }

    void resize(SizeType s) { Resize(s); }

    inline const Type &At(SizeType i) const {
        if (i < 0 || i >= _size) return Type();

        return data[i];
    }

    inline const Type &Rand(SizeType i) const {
        METADOT_ASSERT_E(!(i < 0 || i >= _size));

        return data[i];
    }

    inline Type &Rand(SizeType i) {
        METADOT_ASSERT_E(!(i < 0 || i >= _size));

        return data[i];
    }

    Type *data;

private:
    SizeType _size;
};

template <class T, int N>
class CStaticArray {
public:
    CStaticArray() : data(), length(N) {
        for (int i = 0; i < N; ++i) {
            data[i] = T();
        }
    }

    CStaticArray(const CStaticArray<T, N> &other) : data(), length(N) {
        for (int i = 0; i < N; ++i) {
            data[i] = other.data[i];
        }
    }

    ~CStaticArray() {
        // delete [] data;
    }

    CStaticArray &operator=(const CStaticArray<T, N> &other) {
        for (int i = 0; i < N; ++i) {
            data[i] = other.data[i];
        }

        return *this;
    }

    T &operator[](int i) {
        METADOT_ASSERT_E(i >= 0 && i < N);
        return data[i];
    }

    const T &operator[](int i) const {
        METADOT_ASSERT_E(i >= 0 && i < N);
        return data[i];
    }

    const int length;

private:
    T data[N];
};

template <class T, int N>
class StaticArray {
public:
    StaticArray() : length(N), data() {
        for (int i = 0; i < N; ++i) {
            data[i] = T();
        }
    }

    StaticArray(const StaticArray<T, N> &other) : length(N), data() {
        for (int i = 0; i < N; ++i) {
            data[i] = other.data[i];
        }
    }

    ~StaticArray() {
        // delete [] data;
    }

    StaticArray &operator=(const StaticArray<T, N> &other) {
        for (int i = 0; i < N; ++i) {
            data[i] = other.data[i];
        }

        return *this;
    }

    T &operator[](int i) {
        assert(i >= 0 && i < N);
        return data[i];
    }

    const T &operator[](int i) const {
        assert(i >= 0 && i < N);
        return data[i];
    }

    const int length;

private:
    T data[N];
};

// #include "Poro/utils/../../config/cengdef.h"
// __CENG_BEGIN

//! templated 2 dimensional array. Works way better than
//! std::vector< std::vector< T > >

// #define CENG_CARRAY2D_SAFE

template <class _Ty, class _A = std::allocator<_Ty>>
class CArray2D {
public:
    typedef typename _A::reference reference;
    typedef typename _A::const_reference const_reference;

    class CArray2DHelper {
    public:
        CArray2DHelper(CArray2D &array) : myArray(array) {}
        ~CArray2DHelper() {}

        reference operator[](int _y) { return myArray.Rand(myX, _y); }
        const_reference operator[](int _y) const { return myArray.Rand(myX, _y); }

        void SetX(int _x) const { myX = _x; }

    private:
        mutable int myX;
        CArray2D &myArray;
    };

    CArray2D() : myWidth(0), myHeight(0), mySize(0), myArraysLittleHelper(*this), myNullReference(_Ty()) {}

    CArray2D(int _width, int _height) : myWidth(_width), myHeight(_height), mySize(0), myArraysLittleHelper(*this), myNullReference(_Ty()) { Allocate(); }

    CArray2D(const CArray2D<_Ty, _A> &other)
        : myWidth(other.myWidth), myHeight(other.myHeight), mySize(other.mySize), myArraysLittleHelper(*this), myDataArray(other.myDataArray), myNullReference(_Ty()) {}

    CArray2DHelper &operator[](int _x) {
        myArraysLittleHelper.SetX(_x);
        return myArraysLittleHelper;
    }
    const CArray2DHelper &operator[](int _x) const {
        myArraysLittleHelper.SetX(_x);
        return myArraysLittleHelper;
    }

    const CArray2D<_Ty, _A> &operator=(const CArray2D<_Ty, _A> &other) {
        myWidth = other.myWidth;
        myHeight = other.myHeight;
        mySize = other.mySize;
        myDataArray = other.myDataArray;

        return *this;
    }

    inline int GetWidth() const { return myWidth; }

    inline int GetHeight() const { return myHeight; }

    inline bool IsValid(int x, int y) const {
        if (x < 0) return false;
        if (y < 0) return false;
        if (x >= myWidth) return false;
        if (y >= myHeight) return false;
        return true;
    }

    void Resize(int _width) { SetWidth(_width); }
    void Resize(int _width, int _height) { SetWidthAndHeight(_width, _height); }

    void SetWidth(int _width) {
        myWidth = _width;
        Allocate();
    }
    void SetHeight(int _height) {
        myHeight = _height;
        Allocate();
    }

    void SetWidthAndHeight(int _width, int _height) {
        myWidth = _width;
        myHeight = _height;
        Allocate();
    }

    void SetEverythingTo(const _Ty &_who) {
        int i;
        for (i = 0; i < mySize; i++) myDataArray[i] = _who;
    }

    inline reference At(int _x, int _y) {
#ifdef CENG_CARRAY2D_SAFE
        if (_x < 0 || _y < 0 || _x >= myWidth || _y >= myHeight) return myNullReference;
#else
        if (_x < 0) _x = 0;
        if (_y < 0) _y = 0;
        if (_x >= myWidth) _x = myWidth - 1;
        if (_y >= myHeight) _y = myHeight - 1;
#endif

        return myDataArray[(_y * myWidth) + _x];
    }

    inline const_reference At(int _x, int _y) const {
#ifdef CENG_CARRAY2D_SAFE
        if (_x < 0 || _y < 0 || _x >= myWidth || _y >= myHeight) return myNullReference;
#else

        if (_x < 0) _x = 0;
        if (_y < 0) _y = 0;
        if (_x >= myWidth) _x = myWidth - 1;
        if (_y >= myHeight) _y = myHeight - 1;
#endif

        return myDataArray[(_y * myWidth) + _x];
    }

    inline reference Rand(int _x, int _y) {
#ifdef CENG_CARRAY2D_SAFE
        if (_x < 0 || _y < 0 || _x >= myWidth || _y >= myHeight) return myNullReference;
#endif
        return myDataArray[(_y * myWidth) + _x];
    }

    inline const_reference Rand(int _x, int _y) const {
#ifdef CENG_CARRAY2D_SAFE
        if (_x < 0 || _y < 0 || _x >= myWidth || _y >= myHeight) return myNullReference;
#endif
        return myDataArray[(_y * myWidth) + _x];
    }

    void Rand(int _x, int _y, const _Ty &_who) { myDataArray[(_y * myWidth) + _x] = _who; }

    void Set(int _x, int _y, const _Ty &_who) {

        if (_x > myWidth) _x = myWidth;
        if (_y > myHeight) _y = myHeight;

        myDataArray[(_y * myWidth) + _x] = _who;
    }

    void Set(int _x, int _y, const CArray2D &_who) {
        int x, y;

        for (y = 0; y <= myHeight; y++) {
            for (x = 0; x <= myWidth; x++) {
                if (x >= _x && x <= _x + _who.GetWidth() && y >= _y && y <= _y + _who.GetHeight()) {
                    Set(x, y, _who.At(x - _x, y - _y));
                }
            }
        }
    }

    void Crop(const _Ty &_empty) {
        int left = myWidth;
        int right = 0;
        int top = myHeight;
        int bottom = 0;

        int x = 0;
        int y = 0;

        for (y = 0; y <= myHeight; y++) {
            for (x = 0; x <= myWidth; x++) {
                if (At(x, y) != _empty) {
                    if (x < left) left = x;
                    if (x > right) right = x;
                    if (y < top) top = y;
                    if (y > bottom) bottom = y;
                }
            }
        }

        Crop(left, top, right - left, bottom - top);
    }

    void Crop(int _x, int _y, int _w, int _h) {
        METADOT_ASSERT_E(false);

        /*
    std::vector< _Ty > tmpDataArray;

    tmpDataArray.resize( ( _w + 1 ) * ( _h + 1 ) );

    int x, y;
    for ( y = _y; y <= _y + _h; y++ )
    {
        for ( x = _x; x <= _x + _w; x++ )
        {
            tmpDataArray[ ( ( y - _y ) * _w ) + ( x - _x ) ] = At( x, y );
        }
    }

    myDataArray = tmpDataArray;
    myWidth = _w;
    myHeight = _h;*/
    }

    void Clear() {
        myWidth = 0;
        myHeight = 0;
        mySize = 0;
        myDataArray.clear();
    }

    bool Empty() const { return myDataArray.empty(); }

    CSafeArray<_Ty> &GetData() { return myDataArray; }
    const CSafeArray<_Ty> &GetData() const { return myDataArray; }

    CArray2D<_Ty, _A> *CopyCropped(int _x, int _y, int _w, int _h) {
        CArray2D<_Ty, _A> *result = new CArray2D<_Ty, _A>(_w, _h);

        int x, y;
        for (y = _y; y < _y + _h; y++) {
            for (x = _x; x < _x + _w; x++) {
                result->myDataArray[((y - _y) * _w) + (x - _x)] = At(x, y);
            }
        }

        return result;
    }

private:
    void Allocate() {
        /*
    int n_size = (myWidth + 1) * ( myHeight + 1 );
    if( n_size != mySize )
    {
        mySize = n_size;
        myDataArray.resize( mySize + 1 );
    }*/

        int n_size = (myWidth) * (myHeight);
        if (n_size != mySize) {
            mySize = n_size;
            myDataArray.resize(mySize + 1);
        }
    }

    int myWidth;
    int myHeight;

    int mySize;

    CArray2DHelper myArraysLittleHelper;

    _Ty myNullReference;

    CSafeArray<_Ty> myDataArray;
    // std::vector< _Ty > myDataArray;
};

template <class T>
class CAutoListInsertOperation {
public:
    CAutoListInsertOperation() {}

    bool operator()(T *insert_me, std::list<T *> &list) {
        list.push_back(insert_me);
        return false;
    }
};

//! Autolist pattern base class
/*!
The main idea is that the autolist pattern will keep a collection of
objects created from it.
*/
template <class T /*, class InsertOperation = CAutoListInsertOperation< T >*/>
class CAutoList {
public:
    virtual ~CAutoList() {
        GetList().erase(std::find(GetList().begin(), GetList().end(), reinterpret_cast<T *>(this)));

        if (GetList().empty()) {
            delete myAutoListPointerList;
            myAutoListPointerList = NULL;
        }
    }

    static bool Empty() { return myAutoListPointerList == NULL; }

    static std::list<T *> &GetList() {
        if (myAutoListPointerList == NULL) {
            myAutoListPointerList = new std::list<T *>;
        }

        return *myAutoListPointerList;
    }

protected:
    CAutoList() {
        // static InsertOperation insert_into_list;
        // insert_into_list( reinterpret_cast< T* >( this ), GetList() );
        GetList().push_back(reinterpret_cast<T *>(this));
    }

private:
    static std::list<T *> *myAutoListPointerList;
};

template <class T>
std::list<T *> *CAutoList<T>::myAutoListPointerList = NULL;

std::string base64_encode(std::string const &s);
std::string base64_decode(std::string const &s);

namespace bitmask {
namespace types {

typedef signed short int16;
typedef signed int int32;
typedef float float32;

}  // end of namespace types

struct PointMatrix;
struct PointXForm;

//-----------------------------------------------------------------------------

struct Point {

    Point() : x(0), y(0) {}
    Point(types::int16 x, types::int16 y) : x(x), y(y) {}
    Point(const Point &other) : x(other.x), y(other.y) {}

    // assign operator
    const Point &operator=(const Point &other) {
        Set(other.x, other.y);
        return *this;
    }

    // cmpr
    bool operator<(const Point &other) const { return GetInt32() < other.GetInt32(); }
    bool operator<=(const Point &other) const { return GetInt32() <= other.GetInt32(); }

    bool operator>(const Point &other) const { return GetInt32() > other.GetInt32(); }
    bool operator>=(const Point &other) const { return GetInt32() >= other.GetInt32(); }

    bool operator==(const Point &other) const { return other.Equals(*this); }
    bool operator!=(const Point &other) const { return !operator==(other); }

    // addition operator
    Point operator+(const Point &other) const {
        Point result(*this);
        result += other;
        return result;
    }
    const Point &operator+=(const Point &other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Point operator-(const Point &other) const {
        Point result(*this);
        result -= other;
        return result;
    }
    const Point &operator-=(const Point &other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    void Set(types::int16 ix, types::int16 iy) {
        x = ix;
        y = iy;
    }

    bool Equals(const Point &other) const { return (other.x == x && other.y == y); }

    types::int32 GetInt32() const {
        types::int32 lp1 = (types::int32)x << 16;
        types::int32 lp2 = (types::int32)y;

        types::int32 result = (lp1 + lp2);

        return result;
    }

    types::int16 GetX() const { return x; }
    types::int16 GetY() const { return y; }

    types::int16 x;
    types::int16 y;
};

//-----------------------------------------------------------------------------

types::int16 FloatToInt16(float x);

//-----------------------------------------------------------------------------
/*
 * Copyright (c) 2006-2007 Erin Catto http://www.gphysics.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

// Ripped from Erin Catto's Box2D b2Math.h

/// A 2-by-2 matrix. Stored in column-major order.
struct PointMatrix {
    /// The default constructor does nothing (for performance).
    PointMatrix() { SetIdentity(); }

    /// Construct this matrix using columns.
    PointMatrix(const Point &c1, const Point &c2) {
        col1 = c1;
        col2 = c2;
    }

    /// Construct this matrix using scalars.
    PointMatrix(types::int16 a11, types::int16 a12, types::int16 a21, types::int16 a22) {
        col1.x = a11;
        col1.y = a21;
        col2.x = a12;
        col2.y = a22;
    }

    /// Construct this matrix using an angle. This matrix becomes
    /// an orthonormal rotation matrix.
    explicit PointMatrix(float angle) {
        float c = cosf(angle), s = sinf(angle);
        col1.x = FloatToInt16(c);
        col2.x = FloatToInt16(-s);
        col1.y = FloatToInt16(s);
        col2.y = FloatToInt16(c);
    }

    bool operator==(const PointMatrix &other) const { return Compare(other); }
    bool operator!=(const PointMatrix &other) const { return !Compare(other); }

    bool Compare(const PointMatrix &other) const { return (col1 == other.col1 && col2 == other.col2); }

    /// Initialize this matrix using columns.
    void Set(const Point &c1, const Point &c2) {
        col1 = c1;
        col2 = c2;
    }

    /// Initialize this matrix using an angle. This matrix becomes
    /// an orthonormal rotation matrix.
    void Set(float angle) {
        float c = cosf(angle), s = sinf(angle);
        col1.x = FloatToInt16(c);
        col2.x = FloatToInt16(-s);
        col1.y = FloatToInt16(s);
        col2.y = FloatToInt16(c);
    }

    /// Set this to the identity matrix.
    void SetIdentity() {
        col1.x = 1;
        col2.x = 0;
        col1.y = 0;
        col2.y = 1;
    }

    /// Set this matrix to all zeros.
    void SetZero() {
        col1.x = 0;
        col2.x = 0;
        col1.y = 0;
        col2.y = 0;
    }

    /// Extract the angle from this matrix (assumed to be
    /// a rotation matrix).
    float GetAngle() const { return atan2((float)col1.y, (float)col1.x); }

    PointMatrix Invert() const {
        float a = col1.x, b = col2.x, c = col1.y, d = col2.y;
        PointMatrix B;
        float det = (float)(a * d - b * c);
        METADOT_ASSERT_E(det != 0);
        if (det == 0) return B;

        det = float(1.0f) / det;
        B.col1.x = FloatToInt16(det * d);
        B.col2.x = FloatToInt16(-det * b);
        B.col1.y = FloatToInt16(-det * c);
        B.col2.y = FloatToInt16(det * a);
        return B;
    }

    Point col1, col2;
};

//-----------------------------------------------------------------------------

types::int16 PointDot(const Point &a, const Point &b);

Point PointMul(const PointMatrix &A, const Point &v);
Point PointMul(const PointXForm &T, const Point &v);

PointMatrix PointMul(const PointMatrix &A, const PointMatrix &B);
PointXForm PointMul(const PointXForm &T, const PointXForm &v);

Point PointMulT(const PointMatrix &A, const Point &v);
Point PointMulT(const PointXForm &T, const Point &v);
PointMatrix PointMulT(const PointMatrix &A, const PointMatrix &B);
PointXForm PointMulT(const PointXForm &T, const PointXForm &v);

//-----------------------------------------------------------------------------

struct PointXForm {
    PointXForm() : position(), R() {}
    PointXForm(const Point &p, const PointMatrix &r) : position(p), R(r) {}
    PointXForm(const PointXForm &other) : position(other.position), R(other.R) {}

    Point position;
    PointMatrix R;
};

//-----------------------------------------------------------------------------

inline types::int16 PointDot(const Point &a, const Point &b) { return a.x * b.x + a.y * b.y; }

inline Point PointMul(const PointMatrix &A, const Point &v) {
    Point u;
    u.Set(A.col1.x * v.x + A.col2.x * v.y, A.col1.y * v.x + A.col2.y * v.y);
    return u;
}

inline Point PointMul(const PointXForm &T, const Point &v) { return T.position + PointMul(T.R, v); }

inline PointMatrix PointMul(const PointMatrix &A, const PointMatrix &B) {
    PointMatrix C;
    C.Set(PointMul(A, B.col1), PointMul(A, B.col2));
    return C;
}

inline PointXForm PointMul(const PointXForm &T, const PointXForm &v) {
    PointXForm result;

    result.position = PointMul(T.R, v.position);
    result.position += T.position;

    result.R = PointMul(T.R, v.R);

    return result;
}

inline Point PointMulT(const PointMatrix &A, const Point &v) {
    Point u;
    u.Set(PointDot(v, A.col1), PointDot(v, A.col2));
    return u;
}

inline Point PointMulT(const PointXForm &T, const Point &v) { return PointMulT(T.R, v - T.position); }

inline PointMatrix PointMulT(const PointMatrix &A, const PointMatrix &B) {
    PointMatrix C;
    Point c1;
    c1.Set(PointDot(A.col1, B.col1), PointDot(A.col2, B.col1));
    Point c2;
    c2.Set(PointDot(A.col1, B.col2), PointDot(A.col2, B.col2));
    C.Set(c1, c2);
    return C;
}

inline PointXForm PointMulT(const PointXForm &T, const PointXForm &v) {
    PointXForm result;

    result.position = PointMulT(T, v.position);
    result.R = PointMulT(T.R, v.R);

    return result;
}

inline types::int16 FloatToInt16(float x) {

    float add_this = (x >= 0) ? +0.5f : -0.5f;
    float result = x + add_this;
    return static_cast<types::int16>(result);
}

//-----------------------------------------------------------------------------

}  // end of namespace bitmask

template <typename T>
class CBitMask {
public:
    typedef bitmask::Point Pos;
    typedef typename std::map<Pos, T>::iterator Iterator;
    typedef typename std::map<Pos, T>::const_iterator ConstIterator;

    // constructors
    CBitMask() : mData() {}
    CBitMask(const CBitMask<T> &other) : mData(other.mData) {}
    ~CBitMask() {}

    // assign operators
    const CBitMask<T> &operator=(const CBitMask<T> &other) {
        Assign(other);
        return *this;
    }

    // [] operators
    T &operator[](const Pos &p) { return At(p); }
    T operator[](const Pos &p) const { return At(p); }

    bool Empty() const { return mData.empty(); }

    // iterators
    Iterator Begin() { return mData.begin(); }
    ConstIterator Begin() const { return mData.begin(); }

    Iterator End() { return mData.end(); }
    ConstIterator End() const { return mData.end(); }

    // assign stuff
    void Clear() { mData.clear(); }
    void Assign(const CBitMask<T> &other) { mData = other.mData; }

    // access stuff
    T &At(const Pos &p) { return mData[p]; }
    T At(const Pos &p) const {
        typename std::map<Pos, T>::const_iterator i = mData.find(p);
        if (i == mData.end())
            return T();
        else
            return i->second;
    }

    bool HasAnything(const Pos &p) const {
        typename std::map<Pos, T>::const_iterator i = mData.find(p);
        return (i != mData.end());
    }

    // multiply stuff
    const CBitMask<T> &Multiply(const bitmask::PointXForm &xform) { return MultiplyImpl(xform); }
    const CBitMask<T> &Multiply(const bitmask::PointMatrix &matrix) { return MultiplyImpl(matrix); }

private:
    template <typename MulType>
    const CBitMask<T> &MultiplyImpl(const MulType &multiply_with) {
        std::map<Pos, T> new_data;
        for (typename std::map<Pos, T>::iterator i = mData.begin(); i != mData.end(); ++i) {
            Pos p = i->first;
            Pos new_p = bitmask::PointMul(multiply_with, p);

            new_data.insert(std::pair<Pos, T>(new_p, i->second));
        }

        mData = new_data;

        return *this;
    }

    std::map<Pos, T> mData;
};

//! This is a map that can contain multiple elements under one key
template <class T1, class T2, class PR = std::less<T1>>
class CMapHelper {
public:
    typedef typename std::map<T1, std::list<T2>, PR>::iterator Iterator;
    typedef std::list<T2> List;
    typedef typename std::list<T2>::iterator ListIterator;

    List *HelperList;
    ListIterator i;

    Iterator Begin() { return myMap.begin(); }
    Iterator End() { return myMap.end(); }

    void Clear() { myMap.clear(); }

    void HelperGet(const T1 &me) { HelperList = &Get(me); }

    CMapHelper() {}
    ~CMapHelper() {}

    bool Find(const T1 &me) {
        // if ( myIterator != myMap.end() && myIterator->first == me ) return true;

        Iterator myIterator = myMap.find(me);

        return (myIterator != myMap.end());
    }

    std::list<T2> &Get(const T1 &me) {
        // if ( myIterator != myMap.end() && myIterator->first == me ) return myIterator->second;

        return myMap[me];
    }

    void Insert(const T1 &first, const T2 &second) {

        if (myMap.find(first) != myMap.end()) {
            myMap[first].push_back(second);
            return;
        }

        List tmp_list;
        tmp_list.push_back(second);

        myMap.insert(std::pair<T1, std::list<T2>>(first, tmp_list));
    }

    void RemoveFirst(const T1 &me) { myMap.erase(me); }

    void RemoveSecond(const T2 &me) {
        typename std::map<T1, std::list<T2>, PR>::iterator i = myMap.begin();

        while (i != myMap.end()) {
            ListIterator j = i->second.begin();
            while (j != i->second.end()) {
                if ((*j) == me) {
                    ListIterator remove = j;
                    ++j;
                    i->second.erase(remove);
                } else {
                    ++j;
                }
            }

            if (i->second.empty()) {
                typename std::map<T1, std::list<T2>, PR>::iterator remove = i;
                ++i;
                myMap.erase(remove);
            } else {
                ++i;
            }
        }
    }
    void Remove(const T1 &me) { RemoveFirst(me); }

    void Remove(const T1 &first, const T2 &second) {
        typename std::map<T1, std::list<T2>, PR>::iterator i;

        i = myMap.find(first);
        if (i == myMap.end()) return;

        ListIterator j = i->second.begin();
        int safty_count = 0;
        while (j != i->second.end() && safty_count < 10000) {
            if ((*j) == second) {
                ListIterator remove = j;
                ++j;
                i->second.erase(remove);
            } else {
                ++j;
            }

            safty_count++;
        }

        METADOT_ASSERT_E(safty_count < 10000);

        /*
    for ( j = i->second.begin(); j != i->second.end(); ++j )
    {
        if ( (*j) == second )
        {
            j = i->second.erase( j );
        }
    }
    */
        if (i->second.empty()) {
            myMap.erase(i);
        }
    }

    bool Empty() const { return myMap.empty(); }

private:
    std::map<T1, std::list<T2>, PR> myMap;
    // typename std::map< T1, std::list< T2 >, PR >::iterator	myIterator;
    typename std::list<T2>::iterator myListIterator;
};

#define CMapHelperForEach(map, varname, dacode)                                              \
    {                                                                                        \
        if (map.Find(varname)) {                                                             \
            map.HelperGet(varname);                                                          \
            for (map.i = map.HelperList->begin(); map.i != map.HelperList->end(); ++map.i) { \
                dacode;                                                                      \
            }                                                                                \
        }                                                                                    \
    }

template <class T>
struct CMapHelperSorter {
    bool operator()(const T &left, const T &right) const { return (left < right); }
};

template <class T1, class T2, class SORTER = CMapHelperSorter<T2>, class PR = std::less<T1>>
class CMapHelperSorted {
public:
    typedef typename std::map<T1, std::list<T2>, PR>::iterator Iterator;
    typedef std::list<T2> List;
    typedef typename std::list<T2>::iterator ListIterator;
    typedef SORTER ListSorter;

    List *HelperList;
    ListIterator i;

    Iterator Begin() { return myMap.begin(); }
    Iterator End() { return myMap.end(); }

    void HelperGet(const T1 &me) { HelperList = &Get(me); }

    CMapHelperSorted() {}
    ~CMapHelperSorted() {}

    bool Find(const T1 &me) {
        // if ( myIterator != myMap.end() && myIterator->first == me ) return true;

        Iterator myIterator = myMap.find(me);

        return (myIterator != myMap.end());
    }

    std::list<T2> &Get(const T1 &me) {
        // if ( myIterator != myMap.end() && myIterator->first == me ) return myIterator->second;

        return myMap[me];
    }

    void Insert(const T1 &first, const T2 &second) {

        if (myMap.find(first) != myMap.end()) {
            ListIterator i = myMap[first].end();
            --i;

            ListSorter sort;
            // bool pushed = false;
            for (; i != myMap[first].end(); --i) {
                if (i == myMap[first].begin()) {
                    if (sort(*i, second)) {
                        // for the fucking bug in insert
                        if (myMap[first].size() == 1) {
                            myMap[first].push_back(second);
                            return;
                        } else {
                            myMap[first].insert(i, second);
                            return;
                        }
                    } else {
                        myMap[first].push_front(second);
                        return;
                    }
                }

                if (sort(*i, second)) {
                    myMap[first].insert(i, second);
                    return;
                }
            }
            /*
        myMap[ first ].push_back( second );
        myMap[ first ].sort( ListSorter() );
        */
            METADOT_ASSERT_E(false && "this should happen");
            return;
        }

        List tmp_list;
        tmp_list.push_back(second);

        myMap.insert(std::pair<T1, std::list<T2>>(first, tmp_list));
    }

    void RemoveFirst(const T1 &me) { myMap.erase(me); }

    void RemoveSecond(const T2 &me) {
        typename std::map<T1, std::list<T2>, PR>::iterator i = myMap.begin();

        while (i != myMap.end()) {
            ListIterator j = i->second.begin();
            while (j != i->second.end()) {
                if ((*j) == me) {
                    ListIterator remove = j;
                    ++j;
                    i->second.erase(remove);
                } else {
                    ++j;
                }
            }

            if (i->second.empty()) {
                typename std::map<T1, std::list<T2>, PR>::iterator remove = i;
                ++i;
                myMap.erase(remove);
            } else {
                ++i;
            }
        }
    }
    void Remove(const T1 &me) { RemoveFirst(me); }

    void Remove(const T1 &first, const T2 &second) {
        typename std::map<T1, std::list<T2>, PR>::iterator i;

        i = myMap.find(first);
        if (i == myMap.end()) return;

        ListIterator j = i->second.begin();

        while (j != i->second.end()) {
            if ((*j) == second) {
                ListIterator remove = j;
                ++j;
                i->second.erase(remove);
            } else {
                ++j;
            }
        }

        if (i->second.empty()) {
            myMap.erase(i);
        }
    }

    bool Empty() const { return myMap.empty(); }

private:
    std::map<T1, std::list<T2>, PR> myMap;
    // typename std::map< T1, std::list< T2 >, PR >::iterator	myIterator;
    typename std::list<T2>::iterator myListIterator;
};

// Infrastructure.

#define ENUM_BODY(name, value) name = value,

#define AS_STRING_CASE(name, value) \
    case name:                      \
        return #name;

#define FROM_STRING_CASE(name, value) \
    if (str == #name) {               \
        return name;                  \
    }

#define DEFINE_ENUM(name, list)                                  \
    enum name { list(ENUM_BODY) };                               \
    static std::string asString(int n) {                         \
        switch (n) { list(AS_STRING_CASE) default : return ""; } \
    }                                                            \
    static name fromString(const std::string &str) { list(FROM_STRING_CASE) return name(); /* assert? throw? */ }

// #include "Poro/utils/../config/cengdef.h"
// __CENG_BEGIN	// NAMESPACE?

//! This thing works the same way as PHP explode it cuts up the _string with
//! _separator and pushes the peaces into a vector and returns that vector.
/*!
Description of explode from php.net

Returns an array of strings, each of which is a
substring of string formed by splitting it on
boundaries formed by the string separator.
*/
std::vector<std::string> Split(const std::string &_separator, std::string _string);

//! If limit is set, the returned array will contain a maximum of limit
//! elements with the last element containing the rest of string.
std::vector<std::string> Split(const std::string &_separator, std::string _string, int _limit);

//! This will perform so called string split, something I came up with. It
//! doesn't split the stuff that is inside the quetemarks ( " ). Why is this so
//! usefull well find out for our self... In console this is used in some handy
//! places
std::vector<std::string> StringSplit(const std::string &_separator, std::string _string);

//! Same thing as StringSplit( separator, string ) but with limit
std::vector<std::string> StringSplit(const std::string &_separator, std::string _string, int _limit);

//! Finds the first of _what in _line, that is not inside the quotemarks (").
//! Usefull in various places where string manipulation is used
size_t StringFind(const std::string &_what, const std::string &_line, size_t _begin = 0);

//! Finds the first character of what in line that is not inside quotemarks
size_t StringFindFirstOf(const std::string &what, const std::string &line, size_t begin = 0);

//! Replace all occurrences of the search string with the replacement string
/*!
Works the same way as php's str_replace() function
*/
std::string StringReplace(const std::string &what, const std::string &with, const std::string &in_here, int limit = -1);

//! for removing white space
std::string RemoveWhiteSpace(std::string line);

//! removes whitespace and line endings
std::string RemoveWhiteSpaceAndEndings(std::string line);

//! removes "" from the line
std::string RemoveQuotes(std::string line);

//! returns a string in uppercase
std::string Uppercase(const std::string &_string);
//! returns a string in lowercase
std::string Lowercase(const std::string &_string);

//! for some vector to string conversation
std::string ConvertNumbersToString(const std::vector<int> &array);

//! for some string to vector conversation
std::vector<int> ConvertStringToNumbers(const std::string &str);

//! Just a basic check if what is contained in_where.
bool ContainsString(const std::string &what, const std::string &in_where);

//! Just a basic check if what is contained in_where.
bool ContainsStringChar(const std::string &what, const char *in_where, int length);

//! Tells us if the string is just a number disguised as a string
bool IsNumericString(const std::string &what);

//! Tells us if the beginning of the line matches the string beginning
bool CheckIfTheBeginningIsSame(const std::string &beginning, const std::string &line);

//! Removes a given beginning from the line
std::string RemoveBeginning(const std::string &beginning, const std::string &line);

//! Removes non alphanumeric data from string
std::string MakeAlphaNumeric(const std::string &who);

//! converts all the character to alpha numeric charasters
std::string ConvertToAlphaNumeric(const std::string &who);

//-----------------------------------------------------------------------------

//! Just a helper utility to save some code lines
//! casts a variable to a string
template <class T>
std::string CastToString(const T &var) {
    std::stringstream ss;
    ss << var;
    return ss.str();
}

//! Just a helper utility to save some code lines,
//! casts a string to a variable
template <class T>
T CastFromString(const std::string &str) {
    T result;
    std::stringstream ss(str);
    ss.operator>>(result);
    return result;
}

//-----------------------------------------------------------------------------

//! Just a helper utility to save some code lines,
//! casts a string to a variable
template <class T>
T CastFromHexString(const std::string &str) {
    T result;
    std::stringstream ss(str);
    ss >> std::hex >> (result);
    return result;
}

template <class T>
std::string CastToHexString(const T &what) {
    std::stringstream ss;
    ss << std::hex << (what);
    return ss.str();
}

//-----------------------------------------------------------------------------

// adds an element to the vector if it doesn't exist in the container
// returns true if the element was added
// returns false if the element was in the container already
template <class T>
bool VectorAddUnique(std::vector<T> &container, const T &element) {
    for (std::size_t i = 0; i < container.size(); ++i) {
        if (container[i] == element) return false;
    }

    container.push_back(element);
    return true;
}

template <class T>
bool VectorContains(const std::vector<T> &container, const T &element) {
    // std::vector< T >::iterator i = std::find( container.begin(), container.end(), element );
    return (std::find(container.begin(), container.end(), element) != container.end());
}

// returns true if vector is sorted in ascending order
template <class T>
bool VectorIsSorted(const std::vector<T> &array) {
    int count = ((int)array.size()) - 1;
    for (int i = 0; i < count; ++i) {
        if (array[i] > array[i + 1]) return false;
    }
    return true;
}

// binary search in the sorted array... Maybe we should METADOT_ASSERT_E( that it's sorted... )
template <class T>
bool VectorContainsSorted(const std::vector<T> &array, const T &element) {
    if (array.empty()) return false;
    if (array.size() == 1) return array[0] == element;

    assert(VectorIsSorted(array));

    int first, last, middle;

    first = 0;
    last = (int)array.size() - 1;
    middle = (first + last) / 2;

    while (first <= last) {
        if (array[middle] < element)
            first = middle + 1;
        else if (array[middle] == element)
            return true;
        else
            last = middle - 1;

        middle = (first + last) / 2;
    }
    return false;
}

// the standard way of removing an element by swapping it with the last and popping the last
// stops at the first instance, use VectorRemoveAll to remove duplicates as well
// returns true if found an element, false if none was found
template <class T>
bool VectorRemove(std::vector<T> &container, const T &element) {
    for (std::size_t i = 0; i < container.size(); ++i) {
        if (container[i] == element) {
            container[i] = container[container.size() - 1];
            container.pop_back();
            return true;
        }
    }

    return false;
}

template <class T>
bool VectorRemoveAll(std::vector<T> &container, const T &element) {
    bool result = false;
    for (std::size_t i = 0; i < container.size();) {
        if (container[i] == element) {
            container[i] = container[container.size() - 1];
            container.pop_back();
            result = true;
        } else {
            ++i;
        }
    }

    return result;
}

// used to release a vector full of pointers
// deletes the pointers and clears the vector
template <class T>
void VectorClearPointers(std::vector<T *> &container) {
    for (std::size_t i = 0; i < container.size(); ++i) {
        delete container[i];
    }

    container.clear();
}

//-----------------------------------------------------------------------------
//
// ArgsToVector
// parses program arguments into a std::vector< std::string >, which is much
// nicer to use...
//
std::vector<std::string> ArgsToVector(int argc, char **args);

inline std::vector<std::string> ArgsToVector(int argc, char **args) {
    std::vector<std::string> return_value;

    int i;
    for (i = 1; i < argc; i++) return_value.push_back(args[i]);

    return return_value;
}

//-----------------------------------------------------------------------------

// load from a file, saves each element as a separate element
template <class T>
void VectorLoadFromTxtFile(std::vector<T> &array, const std::string &filename) {
    std::ifstream file(filename.c_str());
    std::string temp;
    while (std::getline(file, temp)) {
        T result = T();
        std::stringstream ss(temp);
        ss.operator>>(result);
        array.push_back(result);
    }

    file.close();
}

template <>
void VectorLoadFromTxtFile<std::string>(std::vector<std::string> &array, const std::string &filename);

inline void VectorLoadFromTxtFile(std::vector<std::string> &array, const std::string &filename) {
    std::ifstream file(filename.c_str());
    std::string temp;
    while (std::getline(file, temp)) {
        array.push_back(temp);
    }

    file.close();
}

// saves a vector to an file... each line is a separate element
template <class T>
void VectorSaveToTxtFile(std::vector<T> &array, const std::string &filename) {
    std::ofstream file(filename.c_str(), std::ios::out);
    for (std::size_t i = 0; i < array.size(); ++i) {
        file << array[i] << std::endl;
    }
    file.close();
}

//-----------------------------------------------------------------------------

}  // end of namespace MetaEngine

#endif
