/*
    Font Builder Tool
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/  

#include <jansson.h>
#include <stdio.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

template <typename T> const T & max( const T & a, const T & b )
{
    return ( a > b ) ? a : b;
} 

char charset[] = { "abcdefghijklmnopqrstuvwxyz"
                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                   "1234567890~!@#$%^&*()-=+;:"
                   "'\",./?[]|\\ <>`\xFF" };

struct GlyphEntry
{
    unsigned char ascii, width;
    unsigned short x, y;
};
 
template <class T> void WriteObject( FILE * file, const T & object )
{
    if ( fwrite( (const char*) &object, sizeof(object), 1, file ) != 1 )
    {
        printf( "error: failed to write data to file\n" );
        exit(1);
    }
}
 
void Create_Font( const char * fontfile, const char * outfile, size_t font_size )
{
    printf( "%s -> %s\n", fontfile, outfile );

    // Margins around characters to prevent them from 'bleeding' into
    // each other.
    const size_t margin = 3;
    size_t image_height = 0, image_width = 256;

      // This initializes FreeType (REALLY?! NO WAY!!!)
      FT_Library library;
      if ( FT_Init_FreeType(&library) != 0 )
      {
          printf( "error: failed to initialize freetype library\n" );
          exit( 1 );
      }

    // Load the font
    FT_Face face;
    if ( FT_New_Face(library, fontfile, 0, &face) != 0 )
    {
        printf( "error: could not load font file: %s\n", fontfile );
        exit( 1 );
    }

    // Abort if this is not a 'true type', scalable font.
    if (!(face->face_flags & FT_FACE_FLAG_SCALABLE) or
        !(face->face_flags & FT_FACE_FLAG_HORIZONTAL))
    {
        printf( "error: %s is not a truetype scalable font\n", fontfile );
        exit( 1 );
    }

    // Set the font size
    FT_Set_Pixel_Sizes( face, font_size, 0 );

    // First we go over all the characters to find the max descent
    // and ascent (space required above and below the base of a
    // line of text) and needed image size. There are simpler methods
    // to obtain these with FreeType but they are unreliable.
    int max_descent = 0, max_ascent = 0;
    size_t space_on_line = image_width - margin, lines = 1;

    for (size_t i = 0; i < sizeof( charset ); ++i)
    {
        // Look up the character in the font file.
        size_t char_index = FT_Get_Char_Index( face, static_cast<unsigned int>( charset[i] ) );
        if ( charset[i] == '\xFF' )
            char_index = 0;

        // Render the current glyph.
        FT_Load_Glyph( face, char_index, FT_LOAD_DEFAULT );
        FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );

        size_t advance = ( face->glyph->metrics.horiAdvance >> 6 ) + margin;

        // If the line is full go to the next line
        if ( advance > space_on_line )
        {
            space_on_line = image_width - margin;
            ++lines;
        }
        space_on_line -= advance;

        max_ascent = max( face->glyph->bitmap_top, max_ascent );
        max_descent = max( face->glyph->bitmap.rows - face->glyph->bitmap_top, max_descent );
    }

    // Compute how high the texture has to be.
    size_t needed_image_height = ( max_ascent + max_descent + margin ) * lines + margin;

    // Get the first power of two in which it fits.
    image_height = 16;
    while ( image_height < needed_image_height )
        image_height *= 2;

    // Allocate memory for the texture, and set it to 0
    unsigned char* image = new unsigned char[image_height * image_width];
    for ( size_t i = 0; i < image_height * image_width; ++i )
        image[i] = 0;

    // Allocate space for the GlyphEntries
    GlyphEntry entries[ sizeof( charset ) ];

    // These are the position at which to draw the next glyph
    size_t x = margin, y = margin + max_ascent;

    // Drawing loop
    for (size_t i = 0; i < sizeof( charset ); ++i)
    {
        size_t char_index = FT_Get_Char_Index( face, static_cast<unsigned int>( charset[i] ) );
        if ( charset[i] == '\xFF' )
            char_index = 0;

        // Render the glyph
        FT_Load_Glyph(face, char_index, FT_LOAD_DEFAULT);
        FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

        // See whether the character fits on the current line
        size_t advance = (face->glyph->metrics.horiAdvance >> 6) + margin;
        if (advance > image_width - x)
        {
            x = margin;
            y += (max_ascent + max_descent + margin);
        }

        // Fill in the GlyphEntry
        entries[i].ascii = charset[i];
        entries[i].width = advance - 3;
        entries[i].x = x;
        entries[i].y = y - max_ascent;

        // Copy the image gotten from FreeType onto the texture
        // at the correct position
        for (size_t row = 0; row < face->glyph->bitmap.rows; ++row)
        {
            for (size_t pixel = 0; pixel < face->glyph->bitmap.width; ++pixel)
            {
                image[(x + face->glyph->bitmap_left + pixel) +
                      (y - face->glyph->bitmap_top + row) * image_width] =
                face->glyph->bitmap.buffer[pixel + row * face->glyph->bitmap.pitch];
            }
        }

        x += advance;    
    }

    // Write everything to the output file (see top of this
    // file for the format specification)
    FILE * file = fopen( outfile, "wb" );
    if ( !file )
    {
        printf( "error: failed to open output file %s\n", outfile );
        exit(1);
    }
    fputc( 'F', file );
    fputc( '0', file );

    WriteObject( file, image_width );
    WriteObject( file, image_height );
    WriteObject( file, max_ascent + max_descent );
    WriteObject( file, sizeof( charset ) );

    // GlyphEntries
    for ( size_t i = 0; i < sizeof( charset ); ++i )
        WriteObject( file, entries[i] );

    // Texture data
    for ( size_t i = 0; i < image_width * image_height; ++i )
        WriteObject( file, image[i] );

    delete [] image;

    fclose( file );

    FT_Done_FreeType( library );
}
 
int main( int args, char * argv[] )
{
    // todo: parse JSON to determine set of fonts to process: input file, output file and size per-font

    // iterate across each output font and if it doesn't exist, or it is older that the Fonts.json
    // or the source ttf file (whichever is newer), mark it as dirty.

    // if there are no dirty font files, 
    //printf( "Fonts are up to date\n" );

    // todo: break down each output directory if it doesn't exist, create it, eg. bin/data, bin/data/fonts

    // todo: initialize FT library etc.

    // todo: iterate across all dirty fonts and build them

    Create_Font( "data/fonts/InputMonoNarrow/InputMonoNarrow-Black.ttf", "bin/data/fonts/Console1.font", 8 );
    Create_Font( "data/fonts/InputMonoNarrow/InputMonoNarrow-Black.ttf", "bin/data/fonts/Console2.font", 16 );
    Create_Font( "data/fonts/InputMonoNarrow/InputMonoNarrow-Black.ttf", "bin/data/fonts/Console3.font", 32 );
    Create_Font( "data/fonts/InputMonoNarrow/InputMonoNarrow-Black.ttf", "bin/data/fonts/Console4.font", 64 );

    // todo: shutdown FT library etc.

    return 0;
}


/*
// Default size
size_t size = 64;

if ( args < 3 )
{
    printf( "Need at least two arguments - font file and output file.\n" );
    return 1;
}

if ( args > 3 )
{
    size_t arg_size = std::atoi(argv[3]);
    if (arg_size != 0)
        size = arg_size;
}

Create_Font( argv[1], size, argv[2] );
*/
