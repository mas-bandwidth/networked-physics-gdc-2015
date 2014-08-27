#include <string>
#include <fstream>
#include <iostream>
#include <vector>
 
// FreeType requires this stuff to include the correct headers
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
 
/**
 * A font file contains the following:
 * 2 chars: "F0" (Font, version 0)
 * size_t: Texture width
 * size_t: Texture height
 * size_t: Line height
 * size_t: Number of characters
 * number of characters * 6: One GlyphEntry struct per character
 * texture width * texture height: The texture data
 *
 * This tool completely ignores endian issues, it should work on a
 * Mac, but font files are not portable between Macs and Intels.
 */
 
// Every glyph/character has such a struct in the output file. This
// contains it's ascii code, the width of the character and the x 
// and y coordinates where this character can be found in the texture.
struct GlyphEntry
{
  unsigned char ascii, width;
  unsigned short x, y;
};
 
// Convenience function for writing simple objects to files.
template<class T, class S>
void Write_Object(const T& to_write, S& out)
{
  out.write(reinterpret_cast<const char*>(&to_write), sizeof(T));
}
 
// This function does all the work.
void Create_Font(const std::string& fontfile, size_t font_size,
                 const std::string& outfile)
{
  // These are the characters that get stored. The last one ('\xFF')
  // indicates the picture used to draw 'unknown' characters.
  const std::string chars("abcdefghijklmnopqrstuvwxyz"
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                          "1234567890~!@#$%^&*()-=+;:"
                          "'\",./?[]|\\ <>`\xFF");
  // Margins around characters to prevent them from 'bleeding' into
  // each other.
  const size_t margin = 3;
  size_t image_height = 0, image_width = 256;
 
  // This initializes FreeType
  FT_Library library;
  if (FT_Init_FreeType(&library) != 0)
    throw "Could not initialize FreeType2 library.";
 
  // Load the font
  FT_Face face;
  if (FT_New_Face(library, fontfile.c_str(), 0, &face) != 0)
    throw "Could not load font file.";
 
  // Abort if this is not a 'true type', scalable font.
  if (!(face->face_flags & FT_FACE_FLAG_SCALABLE) or
      !(face->face_flags & FT_FACE_FLAG_HORIZONTAL))
    throw "Error setting font size.";
 
  // Set the font size
  FT_Set_Pixel_Sizes(face, font_size, 0);
 
  // First we go over all the characters to find the max descent
  // and ascent (space required above and below the base of a
  // line of text) and needed image size. There are simpler methods
  // to obtain these with FreeType but they are unreliable.
  int max_descent = 0, max_ascent = 0;
  size_t space_on_line = image_width - margin, lines = 1;
 
  for (size_t i = 0; i < chars.size(); ++i){
    // Look up the character in the font file.
    size_t char_index = FT_Get_Char_Index(face, static_cast<unsigned int>(chars[i]));
    if (chars[i] == '\xFF')
      char_index = 0;
 
    // Render the current glyph.
    FT_Load_Glyph(face, char_index, FT_LOAD_DEFAULT);
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
 
    size_t advance = (face->glyph->metrics.horiAdvance >> 6) + margin;
    // If the line is full go to the next line
    if (advance > space_on_line){
      space_on_line = image_width - margin;
      ++lines;
    }
    space_on_line -= advance;
 
    max_ascent = std::max(face->glyph->bitmap_top, max_ascent);
    max_descent = std::max(face->glyph->bitmap.rows -
                           face->glyph->bitmap_top, max_descent);
  }
 
  // Compute how high the texture has to be.
  size_t needed_image_height = (max_ascent + max_descent + margin) * lines + margin;
  // Get the first power of two in which it fits.
  image_height = 16;
  while (image_height < needed_image_height)
    image_height *= 2;
 
  // Allocate memory for the texture, and set it to 0
  unsigned char* image = new unsigned char[image_height * image_width];
  for (size_t i = 0; i < image_height * image_width; ++i)
    image[i] = 0;
 
  // Allocate space for the GlyphEntries
  std::vector<GlyphEntry> entries(chars.size());
  // These are the position at which to draw the next glyph
  size_t x = margin, y = margin + max_ascent;
 
  // Drawing loop
  for (size_t i = 0; i < chars.size(); ++i){
    size_t char_index = FT_Get_Char_Index(face, static_cast<unsigned int>(chars[i]));
    if (chars[i] == '\xFF')
      char_index = 0;
 
    // Render the glyph
    FT_Load_Glyph(face, char_index, FT_LOAD_DEFAULT);
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
 
    // See whether the character fits on the current line
    size_t advance = (face->glyph->metrics.horiAdvance >> 6) + margin;
    if (advance > image_width - x){
      x = margin;
      y += (max_ascent + max_descent + margin);
    }
 
    // Fill in the GlyphEntry
    entries[i].ascii = chars[i];
    entries[i].width = advance - 3;
    entries[i].x = x;
    entries[i].y = y - max_ascent;
 
    // Copy the image gotten from FreeType onto the texture
    // at the correct position
    for (size_t row = 0; row < face->glyph->bitmap.rows; ++row){
      for (size_t pixel = 0; pixel < face->glyph->bitmap.width; ++pixel){
        image[(x + face->glyph->bitmap_left + pixel) +
              (y - face->glyph->bitmap_top + row) * image_width] =
          face->glyph->bitmap.buffer[pixel + row * face->glyph->bitmap.pitch];
      }
    }
 
    x += advance;    
  }
 
  // Write everything to the output file (see top of this
  // file for the format specification)
  std::ofstream out(outfile.c_str(), std::ios::binary);
  out.put('F'); out.put('0');
  Write_Object(image_width, out);
  Write_Object(image_height, out);
  Write_Object(max_ascent + max_descent, out);
  Write_Object(chars.size(), out);
 
  // GlyphEntries
  for (size_t i = 0; i < chars.size(); ++i)
    Write_Object(entries[i], out);
  // Texture data
  for (size_t i = 0; i < image_width * image_height; ++i)
    out.put(image[i]);
 
  delete[] image;
 
  FT_Done_FreeType(library);
  std::cout << "Wrote " << outfile << ", " << image_width << 
               " by " << image_height << " pixels.\n";
}
 
// Main interprets the arguments and handles errors.
int main(int args, char** argv)
{
  std::cout << argv[0] << ": ";
  try{
    // Default size
    size_t size = 11;
 
    if (args < 3){
      throw "Need at least two arguments - font file and output file.";
    }
    else{
      if (args > 3){
        size_t arg_size = std::atoi(argv[3]);
        if (arg_size != 0)
          size = arg_size;
      }
      Create_Font(argv[1], size, argv[2]);
    }
    return 0;
  }
  catch(const char*& error){
    std::cout << "Error - " << error << "\n";
    return 1;
  }
}