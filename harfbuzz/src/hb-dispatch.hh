/*
 * Copyright © 2007,2008,2009,2010  Red Hat, Inc.
 * Copyright © 2012,2018  Google, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef RB_DISPATCH_HH
#define RB_DISPATCH_HH

#include "hb.hh"

/*
 * Dispatch
 */

template <typename Context, typename Return> struct rb_dispatch_context_t
{
    rb_dispatch_context_t()
    {}

private:
    /* https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern */
    const Context *thiz() const
    {
        return static_cast<const Context *>(this);
    }
    Context *thiz()
    {
        return static_cast<Context *>(this);
    }

public:
    typedef Return return_t;
    template <typename T, typename... Ts> return_t dispatch(const T &obj, Ts &&... ds)
    {
        return obj.dispatch(thiz(), rb_forward<Ts>(ds)...);
    }
};

#endif /* RB_DISPATCH_HH */
