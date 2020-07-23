/*!
`rustybuzz` is an attempt to incrementally port [harfbuzz](https://github.com/harfbuzz/harfbuzz) to Rust.
*/

#![doc(html_root_url = "https://docs.rs/rustybuzz/0.1.1")]
#![warn(missing_docs)]

mod buffer;
mod common;
mod ffi;
mod font;
mod tag;
mod tag_table;
mod text_parser;
mod unicode;
mod complex;
mod ot;

pub use ttf_parser::Tag;

pub use crate::buffer::{
    GlyphPosition, GlyphInfo, BufferClusterLevel,
    SerializeFlags, UnicodeBuffer, GlyphBuffer
};
pub use crate::common::{Direction, Script, Language, Feature, Variation, script};
pub use crate::font::Font;

type Mask = u32;


/// Shapes the buffer content using provided font and features.
///
/// Consumes the buffer. You can then run `GlyphBuffer::clear` to get the `UnicodeBuffer` back
/// without allocating a new one.
pub fn shape(font: &Font<'_>, features: &[Feature], mut buffer: UnicodeBuffer) -> GlyphBuffer {
    buffer.guess_segment_properties();
    unsafe {
        ffi::rb_shape(
            font.as_ptr(),
            buffer.0.as_ptr(),
            features.as_ptr() as *mut _,
            features.len() as u32,
        )
    };

    GlyphBuffer(buffer.0)
}
