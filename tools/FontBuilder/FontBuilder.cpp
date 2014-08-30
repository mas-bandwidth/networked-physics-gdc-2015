/*
    Font Builder Tool
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/  

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <jansson.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

template <typename T> const T & max( const T & a, const T & b )
{
    return ( a > b ) ? a : b;
} 

bool file_exists( const char * filename )
{
    struct stat sb;
    return stat( filename, &sb ) == 0;
}

time_t file_time( const char * filename )
{
    struct stat sb;
    if ( stat( filename, &sb ) != 0 )
        return (time_t) 0;   
    else
        return sb.st_mtime;
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
 
FT_Library library;

template <class T> void WriteObject( FILE * file, const T & object )
{
    if ( fwrite( (const char*) &object, sizeof(object), 1, file ) != 1 )
    {
        printf( "error: failed to write data to file\n" );
        exit(1);
    }
}
 
void CreateFont( const char * fontfile, const char * outfile, int font_size )
{
    printf( "%s -> %s (%d)\n", fontfile, outfile, font_size );

    // Margins around characters to prevent them from 'bleeding' into each other.
    const size_t margin = 3;
    size_t image_height = 0, image_width = 256;

    // Load the font
    FT_Face face;
    if ( FT_New_Face(library, fontfile, 0, &face) != 0 )
    {
        printf( "error: could not load font file: \"%s\"\n", fontfile );
        exit( 1 );
    }

    // Abort if this is not a 'true type', scalable font.
    if (!(face->face_flags & FT_FACE_FLAG_SCALABLE) or
        !(face->face_flags & FT_FACE_FLAG_HORIZONTAL))
    {
        printf( "error: \"%s\" is not a truetype scalable font\n", fontfile );
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
        FT_Load_Glyph( face, char_index, FT_LOAD_DEFAULT );
        FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );

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
        printf( "error: failed to open output file \"%s\"\n", outfile );
        exit( 1 );
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
}
 
int main( int argc, char * argv[] )
{
    if ( argc != 2 )
    {
        printf( "error: you must supply the font JSON file, eg: FontBuilder data/fonts/Fonts.json\n" );
        return 1;
    }

    const char * json_filename = argv[1];

    if ( !file_exists( json_filename ) )
    {
        printf( "error: \"%s\" does not exist\n", json_filename );
        return 1;
    }

    json_t * root;
    json_error_t error;
    root = json_load_file( json_filename, 0, &error );
    if ( !root )
    {
        printf( "error: %s on line %d in \"%s\"\n", error.text, error.line, json_filename );
        return 1;
    }

    if ( !json_is_array( root ) )
    {
        printf( "error: JSON root is not an array\n");
        json_decref( root );
        return 1;
    }

    if ( FT_Init_FreeType( &library ) != 0 )
    {
        printf( "error: failed to initialize freetype library\n" );
        return 1;
    }

    time_t executable_time = file_time( argv[0] );
    time_t json_file_time = file_time( json_filename );

    int num_processed = 0;

    for ( int i = 0; i < json_array_size( root ); ++i )
    {
        json_t * font_data = json_array_get( root, i );

        if ( !json_is_array( font_data ) )
        {
            printf( "error: font data %d is not an array\n", i );
            json_decref( root );
            return 1;
        }

        if ( json_array_size( font_data ) != 3 )
        {
            printf( "error: font data %d has wrong number of array elements. expected 3, got %d\n", i, (int) json_array_size( font_data ) );
            json_decref( root );
            return 1;
        }

        json_t * json_output_filename = json_array_get( font_data, 0 );
        if ( !json_is_string( json_output_filename ) )
        {
            printf( "error: font data %d output filename is not a string\n", i );
            json_decref( root );
            return 1;
        }

        json_t * json_input_filename = json_array_get( font_data, 1 );
        if ( !json_is_string( json_input_filename ) )
        {
            printf( "error: font data %d input filename is not a string\n", i );
            json_decref( root );
            return 1;
        }

        json_t * json_font_size = json_array_get( font_data, 2 );
        if ( !json_is_integer( json_font_size ) )
        {
            printf( "error: font data %d input filename is not an integer\n", i );
            json_decref( root );
            return 1;
        }

        const char * input_filename = json_string_value( json_input_filename );
        const char * output_filename = json_string_value( json_output_filename );
        const int font_size = json_integer_value( json_font_size );

        time_t input_file_time = file_time( input_filename );
        time_t output_file_time = file_time( output_filename );

        if ( output_file_time < executable_time ||
             output_file_time < input_file_time || 
             output_file_time < json_file_time )
        {
            // todo: make_dir( ... )

            CreateFont( input_filename, output_filename, font_size );

            num_processed++;
        }
    }

    if ( num_processed == 0 )
    {
        printf( "All fonts up to date.\n" );
    }

    FT_Done_FreeType( library );

    json_decref( root );

    return 0;
}
