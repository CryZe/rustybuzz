/*
 * Copyright © 2017  Google, Inc.
 * Copyright © 2019  Facebook, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod
 * Facebook Author(s): Behdad Esfahbod
 */

#ifndef RB_ALGS_HH
#define RB_ALGS_HH

#include "hb.hh"
#include "hb-meta.hh"
#include "hb-null.hh"

/* Encodes three unsigned integers in one 64-bit number.  If the inputs have more than 21 bits,
 * values will be truncated / overlap, and might not decode exactly. */
#define RB_CODEPOINT_ENCODE3(x, y, z) (((uint64_t)(x) << 42) | ((uint64_t)(y) << 21) | (uint64_t)(z))
#define RB_CODEPOINT_DECODE3_1(v) ((rb_codepoint_t)((v) >> 42))
#define RB_CODEPOINT_DECODE3_2(v) ((rb_codepoint_t)((v) >> 21) & 0x1FFFFFu)
#define RB_CODEPOINT_DECODE3_3(v) ((rb_codepoint_t)(v)&0x1FFFFFu)

/* Custom encoding used by hb-ucd. */
#define RB_CODEPOINT_ENCODE3_11_7_14(x, y, z)                                                                          \
    (((uint32_t)((x)&0x07FFu) << 21) | (((uint32_t)(y)&0x007Fu) << 14) | (uint32_t)((z)&0x3FFFu))
#define RB_CODEPOINT_DECODE3_11_7_14_1(v) ((rb_codepoint_t)((v) >> 21))
#define RB_CODEPOINT_DECODE3_11_7_14_2(v) ((rb_codepoint_t)(((v) >> 14) & 0x007Fu) | 0x0300)
#define RB_CODEPOINT_DECODE3_11_7_14_3(v) ((rb_codepoint_t)(v)&0x3FFFu)

struct
{
    /* Note.  This is dangerous in that if it's passed an rvalue, it returns rvalue-reference. */
    template <typename T> constexpr auto operator()(T &&v) const RB_AUTO_RETURN(rb_forward<T>(v))
} RB_FUNCOBJ(rb_identity);
struct
{
    /* Like identity(), but only retains lvalue-references.  Rvalues are returned as rvalues. */
    template <typename T> constexpr T &operator()(T &v) const
    {
        return v;
    }

    template <typename T> constexpr rb_remove_reference<T> operator()(T &&v) const
    {
        return v;
    }
} RB_FUNCOBJ(rb_lidentity);
struct
{
    /* Like identity(), but always returns rvalue. */
    template <typename T> constexpr rb_remove_reference<T> operator()(T &&v) const
    {
        return v;
    }
} RB_FUNCOBJ(rb_ridentity);

struct
{
    template <typename T> constexpr bool operator()(T &&v) const
    {
        return bool(rb_forward<T>(v));
    }
} RB_FUNCOBJ(rb_bool);

struct
{
private:
    template <typename T>
    constexpr auto impl(const T &v, rb_priority<1>) const RB_RETURN(uint32_t, rb_deref(v).hash())

        template <typename T, rb_enable_if(rb_is_integral(T))>
        constexpr auto impl(const T &v, rb_priority<0>) const RB_AUTO_RETURN(
            /* Knuth's multiplicative method: */
            (uint32_t)v * 2654435761u)

            public :

        template <typename T>
        constexpr auto
        operator()(const T &v) const RB_RETURN(uint32_t, impl(v, rb_prioritize))
} RB_FUNCOBJ(rb_hash);

struct
{
private:
    /* Pointer-to-member-function. */
    template <typename Appl, typename T, typename... Ts>
    auto impl(Appl &&a, rb_priority<2>, T &&v, Ts &&... ds) const
        RB_AUTO_RETURN((rb_deref(rb_forward<T>(v)).*rb_forward<Appl>(a))(rb_forward<Ts>(ds)...))

        /* Pointer-to-member. */
        template <typename Appl, typename T>
        auto impl(Appl &&a, rb_priority<1>, T &&v) const
        RB_AUTO_RETURN((rb_deref(rb_forward<T>(v))).*rb_forward<Appl>(a))

        /* Operator(). */
        template <typename Appl, typename... Ts>
        auto impl(Appl &&a, rb_priority<0>, Ts &&... ds) const
        RB_AUTO_RETURN(rb_deref(rb_forward<Appl>(a))(rb_forward<Ts>(ds)...))

            public :

        template <typename Appl, typename... Ts>
        auto
        operator()(Appl &&a, Ts &&... ds) const
        RB_AUTO_RETURN(impl(rb_forward<Appl>(a), rb_prioritize, rb_forward<Ts>(ds)...))
} RB_FUNCOBJ(rb_invoke);

template <unsigned Pos, typename Appl, typename V> struct rb_partial_t
{
    rb_partial_t(Appl a, V v)
        : a(a)
        , v(v)
    {
    }

    static_assert(Pos > 0, "");

    template <typename... Ts, unsigned P = Pos, rb_enable_if(P == 1)>
    auto operator()(Ts &&... ds) -> decltype(rb_invoke(rb_declval(Appl), rb_declval(V), rb_declval(Ts)...))
    {
        return rb_invoke(rb_forward<Appl>(a), rb_forward<V>(v), rb_forward<Ts>(ds)...);
    }
    template <typename T0, typename... Ts, unsigned P = Pos, rb_enable_if(P == 2)>
    auto operator()(T0 &&d0, Ts &&... ds)
        -> decltype(rb_invoke(rb_declval(Appl), rb_declval(T0), rb_declval(V), rb_declval(Ts)...))
    {
        return rb_invoke(rb_forward<Appl>(a), rb_forward<T0>(d0), rb_forward<V>(v), rb_forward<Ts>(ds)...);
    }

private:
    rb_reference_wrapper<Appl> a;
    V v;
};
template <unsigned Pos = 1, typename Appl, typename V>
auto rb_partial(Appl &&a, V &&v) RB_AUTO_RETURN((rb_partial_t<Pos, Appl, V>(a, v)))

/* The following, RB_PARTIALIZE, macro uses a particular corner-case
 * of C++11 that is not particularly well-supported by all compilers.
 * What's happening is that it's using "this" in a trailing return-type
 * via decltype().  Broken compilers deduce the type of "this" pointer
 * in that context differently from what it resolves to in the body
 * of the function.
 *
 * One probable cause of this is that at the time of trailing return
 * type declaration, "this" points to an incomplete type, whereas in
 * the function body the type is complete.  That doesn't justify the
 * error in any way, but is probably what's happening.
 *
 * In the case of MSVC, we get around this by using C++14 "decltype(auto)"
 * which deduces the type from the actual return statement.  For gcc 4.8
 * we use "+this" instead of "this" which produces an rvalue that seems
 * to be deduced as the same type with this particular compiler, and seem
 * to be fine as default code path as well.
 */
// clang-format off
#ifdef _MSC_VER
/* https://github.com/harfbuzz/harfbuzz/issues/1730 */ \
#define RB_PARTIALIZE(Pos) \
  template <typename _T> \
  decltype(auto) operator () (_T&& _v) const \
  { return rb_partial<Pos> (this, rb_forward<_T> (_v)); } \
  static_assert (true, "")
#else
/* https://github.com/harfbuzz/harfbuzz/issues/1724 */
#define RB_PARTIALIZE(Pos) \
  template <typename _T> \
  auto operator () (_T&& _v) const RB_AUTO_RETURN \
  (rb_partial<Pos> (+this, rb_forward<_T> (_v))) \
  static_assert (true, "")
#endif
    // clang-format on

    struct
{
private:
    template <typename Pred, typename Val>
    auto impl(Pred &&p, Val &&v, rb_priority<1>) const
        RB_AUTO_RETURN(rb_deref(rb_forward<Pred>(p)).has(rb_forward<Val>(v)))

            template <typename Pred, typename Val>
            auto impl(Pred &&p, Val &&v, rb_priority<0>) const
        RB_AUTO_RETURN(rb_invoke(rb_forward<Pred>(p), rb_forward<Val>(v)))

            public :

        template <typename Pred, typename Val>
        auto
        operator()(Pred &&p, Val &&v) const
        RB_RETURN(bool, impl(rb_forward<Pred>(p), rb_forward<Val>(v), rb_prioritize))
} RB_FUNCOBJ(rb_has);

struct
{
private:
    template <typename Pred, typename Val>
    auto impl(Pred &&p, Val &&v, rb_priority<1>) const RB_AUTO_RETURN(rb_has(rb_forward<Pred>(p), rb_forward<Val>(v)))

        template <typename Pred, typename Val>
        auto impl(Pred &&p, Val &&v, rb_priority<0>) const RB_AUTO_RETURN(rb_forward<Pred>(p) == rb_forward<Val>(v))

            public :

        template <typename Pred, typename Val>
        auto
        operator()(Pred &&p, Val &&v) const
        RB_RETURN(bool, impl(rb_forward<Pred>(p), rb_forward<Val>(v), rb_prioritize))
} RB_FUNCOBJ(rb_match);

struct
{
private:
    template <typename Proj, typename Val>
    auto impl(Proj &&f, Val &&v, rb_priority<2>) const
        RB_AUTO_RETURN(rb_deref(rb_forward<Proj>(f)).get(rb_forward<Val>(v)))

            template <typename Proj, typename Val>
            auto impl(Proj &&f, Val &&v, rb_priority<1>) const
        RB_AUTO_RETURN(rb_invoke(rb_forward<Proj>(f), rb_forward<Val>(v)))

            template <typename Proj, typename Val>
            auto impl(Proj &&f, Val &&v, rb_priority<0>) const RB_AUTO_RETURN(rb_forward<Proj>(f)[rb_forward<Val>(v)])

                public :

        template <typename Proj, typename Val>
        auto
        operator()(Proj &&f, Val &&v) const RB_AUTO_RETURN(impl(rb_forward<Proj>(f), rb_forward<Val>(v), rb_prioritize))
} RB_FUNCOBJ(rb_get);

template <typename T1, typename T2> struct rb_pair_t
{
    typedef T1 first_t;
    typedef T2 second_t;
    typedef rb_pair_t<T1, T2> pair_t;

    rb_pair_t(T1 a, T2 b)
        : first(a)
        , second(b)
    {
    }

    template <typename Q1, typename Q2, rb_enable_if(rb_is_convertible(T1, Q1) && rb_is_convertible(T2, T2))>
    operator rb_pair_t<Q1, Q2>()
    {
        return rb_pair_t<Q1, Q2>(first, second);
    }

    rb_pair_t<T1, T2> reverse() const
    {
        return rb_pair_t<T1, T2>(second, first);
    }

    bool operator==(const pair_t &o) const
    {
        return first == o.first && second == o.second;
    }
    bool operator!=(const pair_t &o) const
    {
        return !(*this == o);
    }
    bool operator<(const pair_t &o) const
    {
        return first < o.first || (first == o.first && second < o.second);
    }
    bool operator>=(const pair_t &o) const
    {
        return !(*this < o);
    }
    bool operator>(const pair_t &o) const
    {
        return first > o.first || (first == o.first && second > o.second);
    }
    bool operator<=(const pair_t &o) const
    {
        return !(*this > o);
    }

    T1 first;
    T2 second;
};
#define rb_pair_t(T1, T2) rb_pair_t<T1, T2>
template <typename T1, typename T2> static inline rb_pair_t<T1, T2> rb_pair(T1 &&a, T2 &&b)
{
    return rb_pair_t<T1, T2>(a, b);
}

struct
{
    template <typename Pair> constexpr typename Pair::first_t operator()(const Pair &pair) const
    {
        return pair.first;
    }
} RB_FUNCOBJ(rb_first);

struct
{
    template <typename Pair> constexpr typename Pair::second_t operator()(const Pair &pair) const
    {
        return pair.second;
    }
} RB_FUNCOBJ(rb_second);

/* Note.  In min/max impl, we can use rb_type_identity<T> for second argument.
 * However, that would silently convert between different-signedness integers.
 * Instead we accept two different types, such that compiler can err if
 * comparing integers of different signedness. */
struct
{
    template <typename T, typename T2>
    constexpr auto operator()(T &&a, T2 &&b) const
        RB_AUTO_RETURN(rb_forward<T>(a) <= rb_forward<T2>(b) ? rb_forward<T>(a) : rb_forward<T2>(b))
} RB_FUNCOBJ(rb_min);
struct
{
    template <typename T, typename T2>
    constexpr auto operator()(T &&a, T2 &&b) const
        RB_AUTO_RETURN(rb_forward<T>(a) >= rb_forward<T2>(b) ? rb_forward<T>(a) : rb_forward<T2>(b))
} RB_FUNCOBJ(rb_max);
struct
{
    template <typename T, typename T2, typename T3>
    constexpr auto operator()(T &&x, T2 &&min, T3 &&max) const
        RB_AUTO_RETURN(rb_min(rb_max(rb_forward<T>(x), rb_forward<T2>(min)), rb_forward<T3>(max)))
} RB_FUNCOBJ(rb_clamp);

/*
 * Bithacks.
 */

/* Return the number of 1 bits in v. */
template <typename T> static inline RB_CONST_FUNC unsigned int rb_popcount(T v)
{
#if (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__)
    if (sizeof(T) <= sizeof(unsigned int))
        return __builtin_popcount(v);

    if (sizeof(T) <= sizeof(unsigned long))
        return __builtin_popcountl(v);

    if (sizeof(T) <= sizeof(unsigned long long))
        return __builtin_popcountll(v);
#endif

    if (sizeof(T) <= 4) {
        /* "HACKMEM 169" */
        uint32_t y;
        y = (v >> 1) & 033333333333;
        y = v - y - ((y >> 1) & 033333333333);
        return (((y + (y >> 3)) & 030707070707) % 077);
    }

    if (sizeof(T) == 8) {
        unsigned int shift = 32;
        return rb_popcount<uint32_t>((uint32_t)v) + rb_popcount((uint32_t)(v >> shift));
    }

    if (sizeof(T) == 16) {
        unsigned int shift = 64;
        return rb_popcount<uint64_t>((uint64_t)v) + rb_popcount((uint64_t)(v >> shift));
    }

    assert(0);
    return 0; /* Shut up stupid compiler. */
}

/* Returns the number of bits needed to store number */
template <typename T> static inline RB_CONST_FUNC unsigned int rb_bit_storage(T v)
{
    if (unlikely(!v))
        return 0;

#if (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__)
    if (sizeof(T) <= sizeof(unsigned int))
        return sizeof(unsigned int) * 8 - __builtin_clz(v);

    if (sizeof(T) <= sizeof(unsigned long))
        return sizeof(unsigned long) * 8 - __builtin_clzl(v);

    if (sizeof(T) <= sizeof(unsigned long long))
        return sizeof(unsigned long long) * 8 - __builtin_clzll(v);
#endif

#if (defined(_MSC_VER) && _MSC_VER >= 1500) || (defined(__MINGW32__) && (__GNUC__ < 4))
    if (sizeof(T) <= sizeof(unsigned int)) {
        unsigned long where;
        _BitScanReverse(&where, v);
        return 1 + where;
    }
#if defined(_WIN64)
    if (sizeof(T) <= 8) {
        unsigned long where;
        _BitScanReverse64(&where, v);
        return 1 + where;
    }
#endif
#endif

    if (sizeof(T) <= 4) {
        /* "bithacks" */
        const unsigned int b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
        const unsigned int S[] = {1, 2, 4, 8, 16};
        unsigned int r = 0;
        for (int i = 4; i >= 0; i--)
            if (v & b[i]) {
                v >>= S[i];
                r |= S[i];
            }
        return r + 1;
    }
    if (sizeof(T) <= 8) {
        /* "bithacks" */
        const uint64_t b[] = {0x2ULL, 0xCULL, 0xF0ULL, 0xFF00ULL, 0xFFFF0000ULL, 0xFFFFFFFF00000000ULL};
        const unsigned int S[] = {1, 2, 4, 8, 16, 32};
        unsigned int r = 0;
        for (int i = 5; i >= 0; i--)
            if (v & b[i]) {
                v >>= S[i];
                r |= S[i];
            }
        return r + 1;
    }
    if (sizeof(T) == 16) {
        unsigned int shift = 64;
        return (v >> shift) ? rb_bit_storage<uint64_t>((uint64_t)(v >> shift)) + shift
                            : rb_bit_storage<uint64_t>((uint64_t)v);
    }

    assert(0);
    return 0; /* Shut up stupid compiler. */
}

/* Returns the number of zero bits in the least significant side of v */
template <typename T> static inline RB_CONST_FUNC unsigned int rb_ctz(T v)
{
    if (unlikely(!v))
        return 8 * sizeof(T);

#if (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__)
    if (sizeof(T) <= sizeof(unsigned int))
        return __builtin_ctz(v);

    if (sizeof(T) <= sizeof(unsigned long))
        return __builtin_ctzl(v);

    if (sizeof(T) <= sizeof(unsigned long long))
        return __builtin_ctzll(v);
#endif

#if (defined(_MSC_VER) && _MSC_VER >= 1500) || (defined(__MINGW32__) && (__GNUC__ < 4))
    if (sizeof(T) <= sizeof(unsigned int)) {
        unsigned long where;
        _BitScanForward(&where, v);
        return where;
    }
#if defined(_WIN64)
    if (sizeof(T) <= 8) {
        unsigned long where;
        _BitScanForward64(&where, v);
        return where;
    }
#endif
#endif

    if (sizeof(T) <= 4) {
        /* "bithacks" */
        unsigned int c = 32;
        v &= -(int32_t)v;
        if (v)
            c--;
        if (v & 0x0000FFFF)
            c -= 16;
        if (v & 0x00FF00FF)
            c -= 8;
        if (v & 0x0F0F0F0F)
            c -= 4;
        if (v & 0x33333333)
            c -= 2;
        if (v & 0x55555555)
            c -= 1;
        return c;
    }
    if (sizeof(T) <= 8) {
        /* "bithacks" */
        unsigned int c = 64;
        v &= -(int64_t)(v);
        if (v)
            c--;
        if (v & 0x00000000FFFFFFFFULL)
            c -= 32;
        if (v & 0x0000FFFF0000FFFFULL)
            c -= 16;
        if (v & 0x00FF00FF00FF00FFULL)
            c -= 8;
        if (v & 0x0F0F0F0F0F0F0F0FULL)
            c -= 4;
        if (v & 0x3333333333333333ULL)
            c -= 2;
        if (v & 0x5555555555555555ULL)
            c -= 1;
        return c;
    }
    if (sizeof(T) == 16) {
        unsigned int shift = 64;
        return (uint64_t)v ? rb_bit_storage<uint64_t>((uint64_t)v)
                           : rb_bit_storage<uint64_t>((uint64_t)(v >> shift)) + shift;
    }

    assert(0);
    return 0; /* Shut up stupid compiler. */
}

/*
 * Tiny stuff.
 */

/* ASCII tag/character handling */
static inline bool ISALPHA(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static inline bool ISALNUM(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}
static inline bool ISSPACE(unsigned char c)
{
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}
static inline unsigned char TOUPPER(unsigned char c)
{
    return (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c;
}
static inline unsigned char TOLOWER(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
}
static inline bool ISHEX(unsigned char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static inline unsigned char TOHEX(uint8_t c)
{
    return (c & 0xF) <= 9 ? (c & 0xF) + '0' : (c & 0xF) + 'a' - 10;
}
static inline uint8_t FROMHEX(unsigned char c)
{
    return (c >= '0' && c <= '9') ? c - '0' : TOLOWER(c) - 'a' + 10;
}

static inline unsigned int DIV_CEIL(const unsigned int a, unsigned int b)
{
    return (a + (b - 1)) / b;
}

#undef ARRAY_LENGTH
template <typename Type, unsigned int n> static inline unsigned int ARRAY_LENGTH(const Type (&)[n])
{
    return n;
}
/* A const version, but does not detect erratically being called on pointers. */
#define ARRAY_LENGTH_CONST(__array) ((signed int)(sizeof(__array) / sizeof(__array[0])))

static inline int rb_memcmp(const void *a, const void *b, unsigned int len)
{
    /* It's illegal to pass NULL to memcmp(), even if len is zero.
     * So, wrap it.
     * https://sourceware.org/bugzilla/show_bug.cgi?id=23878 */
    if (unlikely(!len))
        return 0;
    return memcmp(a, b, len);
}

static inline void *rb_memset(void *s, int c, unsigned int n)
{
    /* It's illegal to pass NULL to memset(), even if n is zero. */
    if (unlikely(!n))
        return 0;
    return memset(s, c, n);
}

static inline unsigned int rb_ceil_to_4(unsigned int v)
{
    return ((v - 1) | 3) + 1;
}

template <typename T> static inline bool rb_in_range(T u, T lo, T hi)
{
    static_assert(!rb_is_signed<T>::value, "");

    /* The casts below are important as if T is smaller than int,
     * the subtract results will become a signed int! */
    return (T)(u - lo) <= (T)(hi - lo);
}
template <typename T> static inline bool rb_in_ranges(T u, T lo1, T hi1, T lo2, T hi2)
{
    return rb_in_range(u, lo1, hi1) || rb_in_range(u, lo2, hi2);
}
template <typename T> static inline bool rb_in_ranges(T u, T lo1, T hi1, T lo2, T hi2, T lo3, T hi3)
{
    return rb_in_range(u, lo1, hi1) || rb_in_range(u, lo2, hi2) || rb_in_range(u, lo3, hi3);
}

/*
 * Overflow checking.
 */

/* Consider __builtin_mul_overflow use here also */
static inline bool rb_unsigned_mul_overflows(unsigned int count, unsigned int size)
{
    return (size > 0) && (count >= ((unsigned int)-1) / size);
}

/*
 * Sort and search.
 */

template <typename K, typename V, typename... Ts>
static int _rb_cmp_method(const void *pkey, const void *pval, Ts... ds)
{
    const K &key = *(const K *)pkey;
    const V &val = *(const V *)pval;

    return val.cmp(key, ds...);
}

template <typename V, typename K, typename... Ts>
static inline bool rb_bsearch_impl(unsigned *pos, /* Out */
                                   const K &key,
                                   V *base,
                                   size_t nmemb,
                                   size_t stride,
                                   int (*compar)(const void *_key, const void *_item, Ts... _ds),
                                   Ts... ds)
{
    /* This is our *only* bsearch implementation. */

    int min = 0, max = (int)nmemb - 1;
    while (min <= max) {
        int mid = ((unsigned int)min + (unsigned int)max) / 2;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        V *p = (V *)(((const char *)base) + (mid * stride));
#pragma GCC diagnostic pop
        int c = compar((const void *)rb_addressof(key), (const void *)p, ds...);
        if (c < 0)
            max = mid - 1;
        else if (c > 0)
            min = mid + 1;
        else {
            *pos = mid;
            return true;
        }
    }
    *pos = min;
    return false;
}

template <typename V, typename K>
static inline V *rb_bsearch(const K &key,
                            V *base,
                            size_t nmemb,
                            size_t stride = sizeof(V),
                            int (*compar)(const void *_key, const void *_item) = _rb_cmp_method<K, V>)
{
    unsigned pos;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
    return rb_bsearch_impl(&pos, key, base, nmemb, stride, compar) ? (V *)(((const char *)base) + (pos * stride))
                                                                   : nullptr;
#pragma GCC diagnostic pop
}
template <typename V, typename K, typename... Ts>
static inline V *rb_bsearch(const K &key,
                            V *base,
                            size_t nmemb,
                            size_t stride,
                            int (*compar)(const void *_key, const void *_item, Ts... _ds),
                            Ts... ds)
{
    unsigned pos;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
    return rb_bsearch_impl(&pos, key, base, nmemb, stride, compar, ds...) ? (V *)(((const char *)base) + (pos * stride))
                                                                          : nullptr;
#pragma GCC diagnostic pop
}

/* From https://github.com/noporpoise/sort_r
   Feb 5, 2019 (c8c65c1e)
   Modified to support optional argument using templates */

/* Isaac Turner 29 April 2014 Public Domain */

/*
rb_qsort function to be exported.
Parameters:
  base is the array to be sorted
  nel is the number of elements in the array
  width is the size in bytes of each element of the array
  compar is the comparison function
  arg (optional) is a pointer to be passed to the comparison function

void rb_qsort(void *base, size_t nel, size_t width,
              int (*compar)(const void *_a, const void *_b, [void *_arg]),
              [void *arg]);
*/

#define SORT_R_SWAP(a, b, tmp) ((tmp) = (a), (a) = (b), (b) = (tmp))

/* swap a and b */
/* a and b must not be equal! */
static inline void sort_r_swap(char *__restrict a, char *__restrict b, size_t w)
{
    char tmp, *end = a + w;
    for (; a < end; a++, b++) {
        SORT_R_SWAP(*a, *b, tmp);
    }
}

/* swap a, b iff a>b */
/* a and b must not be equal! */
/* __restrict is same as restrict but better support on old machines */
template <typename... Ts>
static inline int sort_r_cmpswap(char *__restrict a,
                                 char *__restrict b,
                                 size_t w,
                                 int (*compar)(const void *_a, const void *_b, Ts... _ds),
                                 Ts... ds)
{
    if (compar(a, b, ds...) > 0) {
        sort_r_swap(a, b, w);
        return 1;
    }
    return 0;
}

/*
Swap consecutive blocks of bytes of size na and nb starting at memory addr ptr,
with the smallest swap so that the blocks are in the opposite order. Blocks may
be internally re-ordered e.g.
  12345ab  ->   ab34512
  123abc   ->   abc123
  12abcde  ->   deabc12
*/
static inline void sort_r_swap_blocks(char *ptr, size_t na, size_t nb)
{
    if (na > 0 && nb > 0) {
        if (na > nb) {
            sort_r_swap(ptr, ptr + na, nb);
        } else {
            sort_r_swap(ptr, ptr + nb, na);
        }
    }
}

/* Implement recursive quicksort ourselves */
/* Note: quicksort is not stable, equivalent values may be swapped */
template <typename... Ts>
static inline void
sort_r_simple(void *base, size_t nel, size_t w, int (*compar)(const void *_a, const void *_b, Ts... _ds), Ts... ds)
{
    char *b = (char *)base, *end = b + nel * w;

    /* for(size_t i=0; i<nel; i++) {printf("%4i", *(int*)(b + i*sizeof(int)));}
    printf("\n"); */

    if (nel < 10) {
        /* Insertion sort for arbitrarily small inputs */
        char *pi, *pj;
        for (pi = b + w; pi < end; pi += w) {
            for (pj = pi; pj > b && sort_r_cmpswap(pj - w, pj, w, compar, ds...); pj -= w) {
            }
        }
    } else {
        /* nel > 9; Quicksort */

        int cmp;
        char *pl, *ple, *pr, *pre, *pivot;
        char *last = b + w * (nel - 1), *tmp;

        /*
        Use median of second, middle and second-last items as pivot.
        First and last may have been swapped with pivot and therefore be extreme
        */
        char *l[3];
        l[0] = b + w;
        l[1] = b + w * (nel / 2);
        l[2] = last - w;

        /* printf("pivots: %i, %i, %i\n", *(int*)l[0], *(int*)l[1], *(int*)l[2]); */

        if (compar(l[0], l[1], ds...) > 0) {
            SORT_R_SWAP(l[0], l[1], tmp);
        }
        if (compar(l[1], l[2], ds...) > 0) {
            SORT_R_SWAP(l[1], l[2], tmp);
            if (compar(l[0], l[1], ds...) > 0) {
                SORT_R_SWAP(l[0], l[1], tmp);
            }
        }

        /* swap mid value (l[1]), and last element to put pivot as last element */
        if (l[1] != last) {
            sort_r_swap(l[1], last, w);
        }

        /*
        pl is the next item on the left to be compared to the pivot
        pr is the last item on the right that was compared to the pivot
        ple is the left position to put the next item that equals the pivot
        ple is the last right position where we put an item that equals the pivot
                                               v- end (beyond the array)
          EEEEEELLLLLLLLuuuuuuuuGGGGGGGEEEEEEEE.
          ^- b  ^- ple  ^- pl   ^- pr  ^- pre ^- last (where the pivot is)
        Pivot comparison key:
          E = equal, L = less than, u = unknown, G = greater than, E = equal
        */
        pivot = last;
        ple = pl = b;
        pre = pr = last;

        /*
        Strategy:
        Loop into the list from the left and right at the same time to find:
        - an item on the left that is greater than the pivot
        - an item on the right that is less than the pivot
        Once found, they are swapped and the loop continues.
        Meanwhile items that are equal to the pivot are moved to the edges of the
        array.
        */
        while (pl < pr) {
            /* Move left hand items which are equal to the pivot to the far left.
               break when we find an item that is greater than the pivot */
            for (; pl < pr; pl += w) {
                cmp = compar(pl, pivot, ds...);
                if (cmp > 0) {
                    break;
                } else if (cmp == 0) {
                    if (ple < pl) {
                        sort_r_swap(ple, pl, w);
                    }
                    ple += w;
                }
            }
            /* break if last batch of left hand items were equal to pivot */
            if (pl >= pr) {
                break;
            }
            /* Move right hand items which are equal to the pivot to the far right.
               break when we find an item that is less than the pivot */
            for (; pl < pr;) {
                pr -= w; /* Move right pointer onto an unprocessed item */
                cmp = compar(pr, pivot, ds...);
                if (cmp == 0) {
                    pre -= w;
                    if (pr < pre) {
                        sort_r_swap(pr, pre, w);
                    }
                } else if (cmp < 0) {
                    if (pl < pr) {
                        sort_r_swap(pl, pr, w);
                    }
                    pl += w;
                    break;
                }
            }
        }

        pl = pr; /* pr may have gone below pl */

        /*
        Now we need to go from: EEELLLGGGGEEEE
                            to: LLLEEEEEEEGGGG
        Pivot comparison key:
          E = equal, L = less than, u = unknown, G = greater than, E = equal
        */
        sort_r_swap_blocks(b, ple - b, pl - ple);
        sort_r_swap_blocks(pr, pre - pr, end - pre);

        /*for(size_t i=0; i<nel; i++) {printf("%4i", *(int*)(b + i*sizeof(int)));}
        printf("\n");*/

        sort_r_simple(b, (pl - ple) / w, w, compar, ds...);
        sort_r_simple(end - (pre - pr), (pre - pr) / w, w, compar, ds...);
    }
}

static inline void rb_qsort(void *base, size_t nel, size_t width, int (*compar)(const void *_a, const void *_b))
{
#if defined(__OPTIMIZE_SIZE__) && !defined(RB_USE_INTERNAL_QSORT)
    qsort(base, nel, width, compar);
#else
    sort_r_simple(base, nel, width, compar);
#endif
}

static inline void
rb_qsort(void *base, size_t nel, size_t width, int (*compar)(const void *_a, const void *_b, void *_arg), void *arg)
{
#ifdef HAVE_GNU_QSORT_R
    qsort_r(base, nel, width, compar, arg);
#else
    sort_r_simple(base, nel, width, compar, arg);
#endif
}

template <typename T, typename T2, typename T3>
static inline void rb_stable_sort(T *array, unsigned int len, int (*compar)(const T2 *, const T2 *), T3 *array2)
{
    for (unsigned int i = 1; i < len; i++) {
        unsigned int j = i;
        while (j && compar(&array[j - 1], &array[i]) > 0)
            j--;
        if (i == j)
            continue;
        /* Move item i to occupy place for item j, shift what's in between. */
        {
            T t = array[i];
            memmove(&array[j + 1], &array[j], (i - j) * sizeof(T));
            array[j] = t;
        }
        if (array2) {
            T3 t = array2[i];
            memmove(&array2[j + 1], &array2[j], (i - j) * sizeof(T3));
            array2[j] = t;
        }
    }
}

template <typename T> static inline void rb_stable_sort(T *array, unsigned int len, int (*compar)(const T *, const T *))
{
    rb_stable_sort(array, len, compar, (int *)nullptr);
}

/* Operators. */

struct rb_bitwise_and
{
    RB_PARTIALIZE(2);
    static constexpr bool passthru_left = false;
    static constexpr bool passthru_right = false;
    template <typename T> constexpr auto operator()(const T &a, const T &b) const RB_AUTO_RETURN(a &b)
} RB_FUNCOBJ(rb_bitwise_and);
struct rb_bitwise_or
{
    RB_PARTIALIZE(2);
    static constexpr bool passthru_left = true;
    static constexpr bool passthru_right = true;
    template <typename T> constexpr auto operator()(const T &a, const T &b) const RB_AUTO_RETURN(a | b)
} RB_FUNCOBJ(rb_bitwise_or);
struct rb_bitwise_xor
{
    RB_PARTIALIZE(2);
    static constexpr bool passthru_left = true;
    static constexpr bool passthru_right = true;
    template <typename T> constexpr auto operator()(const T &a, const T &b) const RB_AUTO_RETURN(a ^ b)
} RB_FUNCOBJ(rb_bitwise_xor);
struct rb_bitwise_sub
{
    RB_PARTIALIZE(2);
    static constexpr bool passthru_left = true;
    static constexpr bool passthru_right = false;
    template <typename T> constexpr auto operator()(const T &a, const T &b) const RB_AUTO_RETURN(a & ~b)
} RB_FUNCOBJ(rb_bitwise_sub);
struct
{
    template <typename T> constexpr auto operator()(const T &a) const RB_AUTO_RETURN(~a)
} RB_FUNCOBJ(rb_bitwise_neg);

struct
{
    RB_PARTIALIZE(2);
    template <typename T, typename T2> constexpr auto operator()(const T &a, const T2 &b) const RB_AUTO_RETURN(a + b)
} RB_FUNCOBJ(rb_add);
struct
{
    RB_PARTIALIZE(2);
    template <typename T, typename T2> constexpr auto operator()(const T &a, const T2 &b) const RB_AUTO_RETURN(a - b)
} RB_FUNCOBJ(rb_sub);
struct
{
    RB_PARTIALIZE(2);
    template <typename T, typename T2> constexpr auto operator()(const T &a, const T2 &b) const RB_AUTO_RETURN(a *b)
} RB_FUNCOBJ(rb_mul);
struct
{
    RB_PARTIALIZE(2);
    template <typename T, typename T2> constexpr auto operator()(const T &a, const T2 &b) const RB_AUTO_RETURN(a / b)
} RB_FUNCOBJ(rb_div);
struct
{
    RB_PARTIALIZE(2);
    template <typename T, typename T2> constexpr auto operator()(const T &a, const T2 &b) const RB_AUTO_RETURN(a % b)
} RB_FUNCOBJ(rb_mod);
struct
{
    template <typename T> constexpr auto operator()(const T &a) const RB_AUTO_RETURN(+a)
} RB_FUNCOBJ(rb_pos);
struct
{
    template <typename T> constexpr auto operator()(const T &a) const RB_AUTO_RETURN(-a)
} RB_FUNCOBJ(rb_neg);
struct
{
    template <typename T> constexpr auto operator()(T &a) const RB_AUTO_RETURN(++a)
} RB_FUNCOBJ(rb_inc);
struct
{
    template <typename T> constexpr auto operator()(T &a) const RB_AUTO_RETURN(--a)
} RB_FUNCOBJ(rb_dec);

/* Compiler-assisted vectorization. */

/* Type behaving similar to vectorized vars defined using __attribute__((vector_size(...))),
 * basically a fixed-size bitset. */
template <typename elt_t, unsigned int byte_size> struct rb_vector_size_t
{
    elt_t &operator[](unsigned int i)
    {
        return v[i];
    }
    const elt_t &operator[](unsigned int i) const
    {
        return v[i];
    }

    void clear(unsigned char v = 0)
    {
        memset(this, v, sizeof(*this));
    }

    template <typename Op> rb_vector_size_t process(const Op &op) const
    {
        rb_vector_size_t r;
        for (unsigned int i = 0; i < ARRAY_LENGTH(v); i++)
            r.v[i] = op(v[i]);
        return r;
    }
    template <typename Op> rb_vector_size_t process(const Op &op, const rb_vector_size_t &o) const
    {
        rb_vector_size_t r;
        for (unsigned int i = 0; i < ARRAY_LENGTH(v); i++)
            r.v[i] = op(v[i], o.v[i]);
        return r;
    }
    rb_vector_size_t operator|(const rb_vector_size_t &o) const
    {
        return process(rb_bitwise_or, o);
    }
    rb_vector_size_t operator&(const rb_vector_size_t &o) const
    {
        return process(rb_bitwise_and, o);
    }
    rb_vector_size_t operator^(const rb_vector_size_t &o) const
    {
        return process(rb_bitwise_xor, o);
    }
    rb_vector_size_t operator~() const
    {
        return process(rb_bitwise_neg);
    }

private:
    static_assert(0 == byte_size % sizeof(elt_t), "");
    elt_t v[byte_size / sizeof(elt_t)];
};

#endif /* RB_ALGS_HH */
