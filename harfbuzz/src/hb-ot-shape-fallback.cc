/*
 * Copyright © 2011,2012  Google, Inc.
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
 */

#include "hb.hh"

#include "hb-ot-shape-fallback.hh"
#include "hb-kern.hh"

static unsigned int recategorize_combining_class(rb_codepoint_t u, unsigned int klass)
{
    if (klass >= 200)
        return klass;

    /* Thai / Lao need some per-character work. */
    if ((u & ~0xFF) == 0x0E00u) {
        if (unlikely(klass == 0)) {
            switch (u) {
            case 0x0E31u:
            case 0x0E34u:
            case 0x0E35u:
            case 0x0E36u:
            case 0x0E37u:
            case 0x0E47u:
            case 0x0E4Cu:
            case 0x0E4Du:
            case 0x0E4Eu:
                klass = RB_UNICODE_COMBINING_CLASS_ABOVE_RIGHT;
                break;

            case 0x0EB1u:
            case 0x0EB4u:
            case 0x0EB5u:
            case 0x0EB6u:
            case 0x0EB7u:
            case 0x0EBBu:
            case 0x0ECCu:
            case 0x0ECDu:
                klass = RB_UNICODE_COMBINING_CLASS_ABOVE;
                break;

            case 0x0EBCu:
                klass = RB_UNICODE_COMBINING_CLASS_BELOW;
                break;
            }
        } else {
            /* Thai virama is below-right */
            if (u == 0x0E3Au)
                klass = RB_UNICODE_COMBINING_CLASS_BELOW_RIGHT;
        }
    }

    switch (klass) {

        /* Hebrew */

    case RB_MODIFIED_COMBINING_CLASS_CCC10: /* sheva */
    case RB_MODIFIED_COMBINING_CLASS_CCC11: /* hataf segol */
    case RB_MODIFIED_COMBINING_CLASS_CCC12: /* hataf patah */
    case RB_MODIFIED_COMBINING_CLASS_CCC13: /* hataf qamats */
    case RB_MODIFIED_COMBINING_CLASS_CCC14: /* hiriq */
    case RB_MODIFIED_COMBINING_CLASS_CCC15: /* tsere */
    case RB_MODIFIED_COMBINING_CLASS_CCC16: /* segol */
    case RB_MODIFIED_COMBINING_CLASS_CCC17: /* patah */
    case RB_MODIFIED_COMBINING_CLASS_CCC18: /* qamats */
    case RB_MODIFIED_COMBINING_CLASS_CCC20: /* qubuts */
    case RB_MODIFIED_COMBINING_CLASS_CCC22: /* meteg */
        return RB_UNICODE_COMBINING_CLASS_BELOW;

    case RB_MODIFIED_COMBINING_CLASS_CCC23: /* rafe */
        return RB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE;

    case RB_MODIFIED_COMBINING_CLASS_CCC24: /* shin dot */
        return RB_UNICODE_COMBINING_CLASS_ABOVE_RIGHT;

    case RB_MODIFIED_COMBINING_CLASS_CCC25: /* sin dot */
    case RB_MODIFIED_COMBINING_CLASS_CCC19: /* holam */
        return RB_UNICODE_COMBINING_CLASS_ABOVE_LEFT;

    case RB_MODIFIED_COMBINING_CLASS_CCC26: /* point varika */
        return RB_UNICODE_COMBINING_CLASS_ABOVE;

    case RB_MODIFIED_COMBINING_CLASS_CCC21: /* dagesh */
        break;

        /* Arabic and Syriac */

    case RB_MODIFIED_COMBINING_CLASS_CCC27: /* fathatan */
    case RB_MODIFIED_COMBINING_CLASS_CCC28: /* dammatan */
    case RB_MODIFIED_COMBINING_CLASS_CCC30: /* fatha */
    case RB_MODIFIED_COMBINING_CLASS_CCC31: /* damma */
    case RB_MODIFIED_COMBINING_CLASS_CCC33: /* shadda */
    case RB_MODIFIED_COMBINING_CLASS_CCC34: /* sukun */
    case RB_MODIFIED_COMBINING_CLASS_CCC35: /* superscript alef */
    case RB_MODIFIED_COMBINING_CLASS_CCC36: /* superscript alaph */
        return RB_UNICODE_COMBINING_CLASS_ABOVE;

    case RB_MODIFIED_COMBINING_CLASS_CCC29: /* kasratan */
    case RB_MODIFIED_COMBINING_CLASS_CCC32: /* kasra */
        return RB_UNICODE_COMBINING_CLASS_BELOW;

        /* Thai */

    case RB_MODIFIED_COMBINING_CLASS_CCC103: /* sara u / sara uu */
        return RB_UNICODE_COMBINING_CLASS_BELOW_RIGHT;

    case RB_MODIFIED_COMBINING_CLASS_CCC107: /* mai */
        return RB_UNICODE_COMBINING_CLASS_ABOVE_RIGHT;

        /* Lao */

    case RB_MODIFIED_COMBINING_CLASS_CCC118: /* sign u / sign uu */
        return RB_UNICODE_COMBINING_CLASS_BELOW;

    case RB_MODIFIED_COMBINING_CLASS_CCC122: /* mai */
        return RB_UNICODE_COMBINING_CLASS_ABOVE;

        /* Tibetan */

    case RB_MODIFIED_COMBINING_CLASS_CCC129: /* sign aa */
        return RB_UNICODE_COMBINING_CLASS_BELOW;

    case RB_MODIFIED_COMBINING_CLASS_CCC130: /* sign i*/
        return RB_UNICODE_COMBINING_CLASS_ABOVE;

    case RB_MODIFIED_COMBINING_CLASS_CCC132: /* sign u */
        return RB_UNICODE_COMBINING_CLASS_BELOW;
    }

    return klass;
}

void _rb_ot_shape_fallback_mark_position_recategorize_marks(const rb_ot_shape_plan_t *plan RB_UNUSED,
                                                            rb_font_t *font RB_UNUSED,
                                                            rb_buffer_t *buffer)
{
    unsigned int count = rb_buffer_get_length(buffer);
    rb_glyph_info_t *info = rb_buffer_get_glyph_infos(buffer);
    for (unsigned int i = 0; i < count; i++)
        if (_rb_glyph_info_get_general_category(&info[i]) == RB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK) {
            unsigned int combining_class = _rb_glyph_info_get_modified_combining_class(&info[i]);
            combining_class = recategorize_combining_class(info[i].codepoint, combining_class);
            _rb_glyph_info_set_modified_combining_class(&info[i], combining_class);
        }
}

static void
zero_mark_advances(rb_buffer_t *buffer, unsigned int start, unsigned int end, bool adjust_offsets_when_zeroing)
{
    rb_glyph_info_t *info = rb_buffer_get_glyph_infos(buffer);
    for (unsigned int i = start; i < end; i++)
        if (_rb_glyph_info_get_general_category(&info[i]) == RB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK) {
            if (adjust_offsets_when_zeroing) {
                rb_buffer_get_glyph_positions(buffer)[i].x_offset -= rb_buffer_get_glyph_positions(buffer)[i].x_advance;
                rb_buffer_get_glyph_positions(buffer)[i].y_offset -= rb_buffer_get_glyph_positions(buffer)[i].y_advance;
            }
            rb_buffer_get_glyph_positions(buffer)[i].x_advance = 0;
            rb_buffer_get_glyph_positions(buffer)[i].y_advance = 0;
        }
}

static inline void position_mark(const rb_ot_shape_plan_t *plan RB_UNUSED,
                                 rb_font_t *font,
                                 rb_buffer_t *buffer,
                                 rb_glyph_extents_t &base_extents,
                                 unsigned int i,
                                 unsigned int combining_class)
{
    rb_glyph_extents_t mark_extents;
    if (!rb_font_get_glyph_extents(font, rb_buffer_get_glyph_infos(buffer)[i].codepoint, &mark_extents))
        return;

    rb_position_t y_gap = rb_font_get_upem(font) / 16;

    rb_glyph_position_t &pos = rb_buffer_get_glyph_positions(buffer)[i];
    pos.x_offset = pos.y_offset = 0;

    /* We don't position LEFT and RIGHT marks. */

    /* X positioning */
    switch (combining_class) {
    case RB_UNICODE_COMBINING_CLASS_DOUBLE_BELOW:
    case RB_UNICODE_COMBINING_CLASS_DOUBLE_ABOVE:
        if (rb_buffer_get_direction(buffer) == RB_DIRECTION_LTR) {
            pos.x_offset +=
                base_extents.x_bearing + base_extents.width - mark_extents.width / 2 - mark_extents.x_bearing;
            break;
        } else if (rb_buffer_get_direction(buffer) == RB_DIRECTION_RTL) {
            pos.x_offset += base_extents.x_bearing - mark_extents.width / 2 - mark_extents.x_bearing;
            break;
        }
        RB_FALLTHROUGH;

    default:
    case RB_UNICODE_COMBINING_CLASS_ATTACHED_BELOW:
    case RB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE:
    case RB_UNICODE_COMBINING_CLASS_BELOW:
    case RB_UNICODE_COMBINING_CLASS_ABOVE:
        /* Center align. */
        pos.x_offset += base_extents.x_bearing + (base_extents.width - mark_extents.width) / 2 - mark_extents.x_bearing;
        break;

    case RB_UNICODE_COMBINING_CLASS_ATTACHED_BELOW_LEFT:
    case RB_UNICODE_COMBINING_CLASS_BELOW_LEFT:
    case RB_UNICODE_COMBINING_CLASS_ABOVE_LEFT:
        /* Left align. */
        pos.x_offset += base_extents.x_bearing - mark_extents.x_bearing;
        break;

    case RB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE_RIGHT:
    case RB_UNICODE_COMBINING_CLASS_BELOW_RIGHT:
    case RB_UNICODE_COMBINING_CLASS_ABOVE_RIGHT:
        /* Right align. */
        pos.x_offset += base_extents.x_bearing + base_extents.width - mark_extents.width - mark_extents.x_bearing;
        break;
    }

    /* Y positioning */
    switch (combining_class) {
    case RB_UNICODE_COMBINING_CLASS_DOUBLE_BELOW:
    case RB_UNICODE_COMBINING_CLASS_BELOW_LEFT:
    case RB_UNICODE_COMBINING_CLASS_BELOW:
    case RB_UNICODE_COMBINING_CLASS_BELOW_RIGHT:
        /* Add gap, fall-through. */
        base_extents.height -= y_gap;
        RB_FALLTHROUGH;

    case RB_UNICODE_COMBINING_CLASS_ATTACHED_BELOW_LEFT:
    case RB_UNICODE_COMBINING_CLASS_ATTACHED_BELOW:
        pos.y_offset = base_extents.y_bearing + base_extents.height - mark_extents.y_bearing;
        /* Never shift up "below" marks. */
        if ((y_gap > 0) == (pos.y_offset > 0)) {
            base_extents.height -= pos.y_offset;
            pos.y_offset = 0;
        }
        base_extents.height += mark_extents.height;
        break;

    case RB_UNICODE_COMBINING_CLASS_DOUBLE_ABOVE:
    case RB_UNICODE_COMBINING_CLASS_ABOVE_LEFT:
    case RB_UNICODE_COMBINING_CLASS_ABOVE:
    case RB_UNICODE_COMBINING_CLASS_ABOVE_RIGHT:
        /* Add gap, fall-through. */
        base_extents.y_bearing += y_gap;
        base_extents.height -= y_gap;
        RB_FALLTHROUGH;

    case RB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE:
    case RB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE_RIGHT:
        pos.y_offset = base_extents.y_bearing - (mark_extents.y_bearing + mark_extents.height);
        /* Don't shift down "above" marks too much. */
        if ((y_gap > 0) != (pos.y_offset > 0)) {
            unsigned int correction = -pos.y_offset / 2;
            base_extents.y_bearing += correction;
            base_extents.height -= correction;
            pos.y_offset += correction;
        }
        base_extents.y_bearing -= mark_extents.height;
        base_extents.height += mark_extents.height;
        break;
    }
}

static inline void position_around_base(const rb_ot_shape_plan_t *plan,
                                        rb_font_t *font,
                                        rb_buffer_t *buffer,
                                        unsigned int base,
                                        unsigned int end,
                                        bool adjust_offsets_when_zeroing)
{
    rb_direction_t horiz_dir = RB_DIRECTION_INVALID;

    rb_buffer_unsafe_to_break(buffer, base, end);

    rb_glyph_extents_t base_extents;
    if (!rb_font_get_glyph_extents(font, rb_buffer_get_glyph_infos(buffer)[base].codepoint, &base_extents)) {
        /* If extents don't work, zero marks and go home. */
        zero_mark_advances(buffer, base + 1, end, adjust_offsets_when_zeroing);
        return;
    }
    base_extents.y_bearing += rb_buffer_get_glyph_positions(buffer)[base].y_offset;
    /* Use horizontal advance for horizontal positioning.
     * Generally a better idea.  Also works for zero-ink glyphs.  See:
     * https://github.com/harfbuzz/harfbuzz/issues/1532 */
    base_extents.x_bearing = 0;
    base_extents.width = rb_font_get_glyph_h_advance(font, rb_buffer_get_glyph_infos(buffer)[base].codepoint);

    unsigned int lig_id = _rb_glyph_info_get_lig_id(&rb_buffer_get_glyph_infos(buffer)[base]);
    /* Use integer for num_lig_components such that it doesn't convert to unsigned
     * when we divide or multiply by it. */
    int num_lig_components = _rb_glyph_info_get_lig_num_comps(&rb_buffer_get_glyph_infos(buffer)[base]);

    rb_position_t x_offset = 0, y_offset = 0;
    if (RB_DIRECTION_IS_FORWARD(rb_buffer_get_direction(buffer))) {
        x_offset -= rb_buffer_get_glyph_positions(buffer)[base].x_advance;
        y_offset -= rb_buffer_get_glyph_positions(buffer)[base].y_advance;
    }

    rb_glyph_extents_t component_extents = base_extents;
    int last_lig_component = -1;
    unsigned int last_combining_class = 255;
    rb_glyph_extents_t cluster_extents = base_extents; /* Initialization is just to shut gcc up. */
    rb_glyph_info_t *info = rb_buffer_get_glyph_infos(buffer);
    for (unsigned int i = base + 1; i < end; i++)
        if (_rb_glyph_info_get_modified_combining_class(&info[i])) {
            if (num_lig_components > 1) {
                unsigned int this_lig_id = _rb_glyph_info_get_lig_id(&info[i]);
                int this_lig_component = _rb_glyph_info_get_lig_comp(&info[i]) - 1;
                /* Conditions for attaching to the last component. */
                if (!lig_id || lig_id != this_lig_id || this_lig_component >= num_lig_components)
                    this_lig_component = num_lig_components - 1;
                if (last_lig_component != this_lig_component) {
                    last_lig_component = this_lig_component;
                    last_combining_class = 255;
                    component_extents = base_extents;
                    if (unlikely(horiz_dir == RB_DIRECTION_INVALID)) {
                        if (RB_DIRECTION_IS_HORIZONTAL(plan->props.direction))
                            horiz_dir = plan->props.direction;
                        else
                            horiz_dir = rb_script_get_horizontal_direction(plan->props.script);
                    }
                    if (horiz_dir == RB_DIRECTION_LTR)
                        component_extents.x_bearing +=
                            (this_lig_component * component_extents.width) / num_lig_components;
                    else
                        component_extents.x_bearing +=
                            ((num_lig_components - 1 - this_lig_component) * component_extents.width) /
                            num_lig_components;
                    component_extents.width /= num_lig_components;
                }
            }

            unsigned int this_combining_class = _rb_glyph_info_get_modified_combining_class(&info[i]);
            if (last_combining_class != this_combining_class) {
                last_combining_class = this_combining_class;
                cluster_extents = component_extents;
            }

            position_mark(plan, font, buffer, cluster_extents, i, this_combining_class);

            rb_buffer_get_glyph_positions(buffer)[i].x_advance = 0;
            rb_buffer_get_glyph_positions(buffer)[i].y_advance = 0;
            rb_buffer_get_glyph_positions(buffer)[i].x_offset += x_offset;
            rb_buffer_get_glyph_positions(buffer)[i].y_offset += y_offset;

        } else {
            if (RB_DIRECTION_IS_FORWARD(rb_buffer_get_direction(buffer))) {
                x_offset -= rb_buffer_get_glyph_positions(buffer)[i].x_advance;
                y_offset -= rb_buffer_get_glyph_positions(buffer)[i].y_advance;
            } else {
                x_offset += rb_buffer_get_glyph_positions(buffer)[i].x_advance;
                y_offset += rb_buffer_get_glyph_positions(buffer)[i].y_advance;
            }
        }
}

static inline void position_cluster(const rb_ot_shape_plan_t *plan,
                                    rb_font_t *font,
                                    rb_buffer_t *buffer,
                                    unsigned int start,
                                    unsigned int end,
                                    bool adjust_offsets_when_zeroing)
{
    if (end - start < 2)
        return;

    /* Find the base glyph */
    rb_glyph_info_t *info = rb_buffer_get_glyph_infos(buffer);
    for (unsigned int i = start; i < end; i++)
        if (!_rb_glyph_info_is_unicode_mark(&info[i])) {
            /* Find mark glyphs */
            unsigned int j;
            for (j = i + 1; j < end; j++)
                if (!_rb_glyph_info_is_unicode_mark(&info[j]))
                    break;

            position_around_base(plan, font, buffer, i, j, adjust_offsets_when_zeroing);

            i = j - 1;
        }
}

void _rb_ot_shape_fallback_mark_position(const rb_ot_shape_plan_t *plan,
                                         rb_font_t *font,
                                         rb_buffer_t *buffer,
                                         bool adjust_offsets_when_zeroing)
{
    unsigned int start = 0;
    unsigned int count = rb_buffer_get_length(buffer);
    rb_glyph_info_t *info = rb_buffer_get_glyph_infos(buffer);
    for (unsigned int i = 1; i < count; i++)
        if (likely(!_rb_glyph_info_is_unicode_mark(&info[i]))) {
            position_cluster(plan, font, buffer, start, i, adjust_offsets_when_zeroing);
            start = i;
        }
    position_cluster(plan, font, buffer, start, count, adjust_offsets_when_zeroing);
}

/* Performs font-assisted kerning. */
void _rb_ot_shape_fallback_kern(const rb_ot_shape_plan_t *plan, rb_font_t *font, rb_buffer_t *buffer) {}

/* Adjusts width of various spaces. */
void _rb_ot_shape_fallback_spaces(const rb_ot_shape_plan_t *plan RB_UNUSED, rb_font_t *font, rb_buffer_t *buffer)
{
    rb_glyph_info_t *info = rb_buffer_get_glyph_infos(buffer);
    rb_glyph_position_t *pos = rb_buffer_get_glyph_positions(buffer);
    bool horizontal = RB_DIRECTION_IS_HORIZONTAL(rb_buffer_get_direction(buffer));
    unsigned int count = rb_buffer_get_length(buffer);
    for (unsigned int i = 0; i < count; i++)
        if (_rb_glyph_info_is_unicode_space(&info[i]) && !_rb_glyph_info_ligated(&info[i])) {
            rb_space_t space_type = _rb_glyph_info_get_unicode_space_fallback_type(&info[i]);
            rb_codepoint_t glyph;
            switch (space_type) {
            case RB_SPACE_NOT_SPACE: /* Shouldn't happen. */
            case RB_SPACE:
                break;

            case RB_SPACE_EM:
            case RB_SPACE_EM_2:
            case RB_SPACE_EM_3:
            case RB_SPACE_EM_4:
            case RB_SPACE_EM_5:
            case RB_SPACE_EM_6:
            case RB_SPACE_EM_16:
                if (horizontal)
                    pos[i].x_advance = +(rb_font_get_upem(font) + ((int)space_type) / 2) / (int)space_type;
                else
                    pos[i].y_advance = -(rb_font_get_upem(font) + ((int)space_type) / 2) / (int)space_type;
                break;

            case RB_SPACE_4_EM_18:
                if (horizontal)
                    pos[i].x_advance = (int64_t) + rb_font_get_upem(font) * 4 / 18;
                else
                    pos[i].y_advance = (int64_t)-rb_font_get_upem(font) * 4 / 18;
                break;

            case RB_SPACE_FIGURE:
                for (char u = '0'; u <= '9'; u++)
                    if (rb_font_get_nominal_glyph(font, u, &glyph)) {
                        if (horizontal)
                            pos[i].x_advance = rb_font_get_glyph_h_advance(font, glyph);
                        else
                            pos[i].y_advance = rb_font_get_glyph_v_advance(font, glyph);
                        break;
                    }
                break;

            case RB_SPACE_PUNCTUATION:
                if (rb_font_get_nominal_glyph(font, '.', &glyph) || rb_font_get_nominal_glyph(font, ',', &glyph)) {
                    if (horizontal)
                        pos[i].x_advance = rb_font_get_glyph_h_advance(font, glyph);
                    else
                        pos[i].y_advance = rb_font_get_glyph_v_advance(font, glyph);
                }
                break;

            case RB_SPACE_NARROW:
                /* Half-space?
                 * Unicode doc https://unicode.org/charts/PDF/U2000.pdf says ~1/4 or 1/5 of EM.
                 * However, in my testing, many fonts have their regular space being about that
                 * size.  To me, a percentage of the space width makes more sense.  Half is as
                 * good as any. */
                if (horizontal)
                    pos[i].x_advance /= 2;
                else
                    pos[i].y_advance /= 2;
                break;
            }
        }
}
