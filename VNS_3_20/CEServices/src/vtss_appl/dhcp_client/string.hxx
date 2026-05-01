/*

 Vitesse API software.

 Copyright (c) 2002-2012 Vitesse Semiconductor Corporation "Vitesse". All
 Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted. Permission to
 integrate into other products, disclose, transmit and distribute the software
 in an absolute machine readable format (e.g. HEX file) is also granted.  The
 source code of the software may not be disclosed, transmitted or distributed
 without the written permission of Vitesse. The software and its source code
 may only be used in products utilizing the Vitesse switch products.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software. Vitesse retains all ownership,
 copyright, trade secret and proprietary rights in the software.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS," WITHOUT EXPRESS OR IMPLIED WARRANTY
 INCLUDING, WITHOUT LIMITATION, IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR USE AND NON-INFRINGEMENT.

*/

#ifndef _STRING_H_
#define _STRING_H_

extern "C" {
#include "vtss_types.h"
}


#if defined(VTSS_OPSYS_ECOS)
#include <cyg/infra/cyg_type.h>
extern "C" {
#include <cli_api.h>
}
#else
#include <cstring>
#endif

#include "tuple.hxx"
#include "algorithm.hxx"

namespace VTSS {
struct str
{
    str() : begin_(0), end_(0) { }
    str(const Pair<const char*, const char *>& pair) :
        begin_(pair.first),
        end_(pair.second) { }
    str(const char * b) : begin_(b) { end_ = find_end(b); }
    str(const char * b, size_t l) : begin_(b), end_(b+l) { }
    str(const char * b, const char * e) : begin_(b), end_(e) { }
    str(const str& rhs) : begin_(rhs.begin_), end_(rhs.end_) { }

    size_t size() const { return end_ - begin_; }
    const char * begin() const { return begin_; }
    const char * end() const { return end_; }

private:
    const char * begin_;
    const char * end_;
};

inline bool operator==(const str& lhs, const str& rhs)
{
    return equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

inline bool operator!=(const str& lhs, const str& rhs)
{
    return equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<unsigned SIZE>
struct StaticBuffer
{
    StaticBuffer() { }

    const char * const begin() const { return buf; }
    const char * const end() const { return buf+SIZE; }
    char * const begin() { return buf; }
    char * const end() { return buf+SIZE; }
    unsigned size() const { return SIZE; }

private:
    char buf[SIZE];
};
typedef StaticBuffer<4>    SBuf4;
typedef StaticBuffer<8>    SBuf8;
typedef StaticBuffer<16>   SBuf16;
typedef StaticBuffer<32>   SBuf32;
typedef StaticBuffer<64>   SBuf64;
typedef StaticBuffer<128>  SBuf128;

template<unsigned SIZE>
struct FixedBuffer
{
    FixedBuffer() {
        buf = new char[SIZE];
    }

    virtual ~FixedBuffer() {
        delete buf;
    }

    const char * const begin() const { return buf; }
    const char * const end() const {
        if( buf ) return buf+SIZE;
        else return buf;
    }

    char * const begin() { return buf; }

    char * const end() {
        if( buf ) return buf+SIZE;
        else return buf;
    }

    unsigned size() const {
        if( buf ) return SIZE;
        else return 0;
    }

private:
    char * buf;
};
typedef FixedBuffer<256>    FBuf256;
typedef FixedBuffer<512>    FBuf512;
typedef FixedBuffer<1024>   FBuf1024;
typedef FixedBuffer<2048>   FBuf2048;
typedef FixedBuffer<4096>   FBuf4096;

struct ostream
{
    virtual bool ok() const = 0;
    virtual bool push(char) = 0;
    virtual void fill(unsigned, char) = 0;
};

template <typename T0, typename S = ostream>
class obuffer_iterator
{
public:
    typedef T0 value_type;
    typedef value_type* pointer;
    typedef value_type& reference;

    explicit  obuffer_iterator(S& os) : os_(os) {}
    obuffer_iterator(const obuffer_iterator& iter) : os_(iter.os_) {}
    obuffer_iterator& operator= (const T0& v) { os_ << v; return (*this); }
    obuffer_iterator& operator* (void) { return (*this); }
    obuffer_iterator& operator++ (void) { return (*this); }
    obuffer_iterator  operator++ (int) { return (*this); }
    obuffer_iterator& operator-- (void) { return (*this); }
    obuffer_iterator  operator-- (int) { return (*this); }

    bool operator== (const obuffer_iterator& i) const {
        return (os_.pos() == i.os_.pos());
    }

    bool operator< (const obuffer_iterator& i) const {
        return (os_.pos() < i.os_.pos());
    }

private:
    S& os_;
};

template<typename Buf>
struct BufStream :
    public ostream,
    public Buf
{
    BufStream () : ostream(), Buf(), ok_(true), pos_(Buf::begin()) { }
    ~BufStream () { }

    bool ok() const { return ok_; }

    bool push(char c) {
        if( pos_ != Buf::end() ) {
            *pos_++ = c;
            return true;
        } else{
            ok_ = false;
            return false;
        }
    }

    void fill(unsigned i, char c = ' ') {
        while( i-- ) push(c);
    }

    char * pos() { return pos_; }
    const char * pos() const { return pos_; }

    void clear() {
        pos_ = Buf::begin();
        ok_ = true;
    }

protected:
    bool ok_;
    char * pos_;
};

#if defined(VTSS_OPSYS_ECOS)
struct CliOstream : public ostream
{
    CliOstream() {
        pIO = (cli_iolayer_t *) cli_get_io_handle();
    }

    CliOstream(cli_iolayer_t * io) : pIO(io) { }

    bool ok() const {
        return pIO != 0;
    }

    bool push(char c ) {
        if (!pIO) {
            return false;
        }

        pIO->cli_putchar(pIO, c);
        return true;
    }

    void fill(unsigned i, char c = ' ') {
        while( i-- ) push(c);
    }

private:
    cli_iolayer_t *pIO;
};
#endif

template<unsigned S>
struct AlignStream :
    public StaticBuffer<S>,
    public ostream
{
    AlignStream ( ostream& o ) :
        o_(o), ok_(true),
        pos_(StaticBuffer<S>::begin()),
        flush_(StaticBuffer<S>::begin()) { }

    bool ok() const { return o_.ok(); }
    virtual void flush() = 0;

    bool push(char c) {
        if( pos_ != StaticBuffer<S>::end() ) {
            *pos_++ = c;
            return true;
        } else {
            flush();
            return o_.push(c);
        }
    }

    void fill(unsigned i, char c) { while( i-- ) push(c); }
    char * pos() { return pos_; }
    const char * pos() const { return pos_; }

protected:
    ostream& o_;
    bool ok_;
    char * pos_;
    char * flush_;
};

template<unsigned S>
struct RightAlignStream : public AlignStream<S>
{
    RightAlignStream( ostream& o, char f ) :
        AlignStream<S>( o ), fill_char(f) { }
    ~RightAlignStream() { flush(); }
    void flush() {
        typedef AlignStream<S> B;
        B::o_.fill(B::end() - B::pos_, fill_char);
        copy(B::flush_, B::pos_, obuffer_iterator<char>(B::o_));
        B::flush_ = B::pos_;
    }

    const char fill_char;
};

template<unsigned S>
struct LeftAlignStream : public AlignStream<S>
{
    LeftAlignStream( ostream& o, char f ) :
        AlignStream<S>( o ), fill_char(f) { }
    ~LeftAlignStream() { flush(); }
    void flush() {
        typedef AlignStream<S> B;
        copy(B::flush_, B::pos_, obuffer_iterator<char>(B::o_));
        B::o_.fill(B::end() - B::pos_, fill_char);
        B::flush_ = B::pos_;
    }

    const char fill_char;
};

template<typename T0>
struct FormatHex {
    FormatHex(const T0& _t, char _base, bool _prefix) :
        t0(_t), base(_base), prefix(_prefix) { }
    const T0& t0;
    char base;
    bool prefix;
};

template<typename T0>
FormatHex<T0> hex(const T0& t0)
{
    return FormatHex<T0>(t0, 'a', false);
}

template<typename T0>
FormatHex<T0> HEX(const T0& t0)
{
    return FormatHex<T0>(t0, 'A', false);
}

template<typename T0>
FormatHex<T0> hex_(const T0& t0)
{
    return FormatHex<T0>(t0, 'a', true);
}

template<typename T0>
FormatHex<T0> HEX_(const T0& t0)
{
    return FormatHex<T0>(t0, 'A', true);
}

template<unsigned S, typename T0>
struct FormatLeft
{
    FormatLeft (const T0& _t, char f) : t0(_t), fill(f) { }
    const T0& t0;
    const char fill;
};

template<unsigned S, typename T0>
ostream& operator<< (ostream& o, const FormatLeft<S, T0>& l)
{
    LeftAlignStream<S> los(o, l.fill);
    los << l.t0;
    return o;
}

template<unsigned S, typename T0>
FormatLeft<S, T0> left(const T0& t0) { return FormatLeft<S, T0>(t0, ' '); }

template<unsigned S, typename T0>
struct FormatRight
{
    FormatRight (const T0& _t, char f) : t0(_t), fill(f) { }
    const T0& t0;
    const char fill;
};

template<unsigned S, typename T0>
ostream& operator<< (ostream& o, const FormatRight<S, T0>& r)
{
    RightAlignStream<S> ros(o, r.fill);
    ros << r.t0;
    return o;
}

template<class I, class S, class O>
O& join(O& o, const S& s, I begin, I end)
{
    if( begin == end )
        return o;

    o << *begin;
    ++begin;

    for( ; begin != end; ++begin )
        o << s << *begin;

    return o;
}

template<class I, class S, class O, class FMT>
O& join(O& o, const S& s, I begin, I end, FMT fmt)
{
    if( begin == end )
        return o;

    o << fmt(*begin);
    ++begin;

    for( ; begin != end; ++begin )
        o << s << fmt(*begin);

    return o;
}

template<unsigned S, typename T0>
FormatRight<S, T0> right(const T0& t0)
{
    return FormatRight<S, T0>(t0, ' ');
}

template<unsigned S, typename T0>
FormatRight<S, T0> fill(const T0& t0, char f = '0')
{
    return FormatRight<S, T0>(t0, f);
}

inline bool isgraph(const char c) {
        return (c >= 0x21 && c <= 0x7E);
}

inline str trim(const str& s)
{
    const char * begin = find_first_if(s.begin(), s.end(), &isgraph);
    const char * end = find_last_if(begin, s.end(), &isgraph);
    return str(begin, end);
}

} /* VTSS */

#endif /* _STRING_H_ */
