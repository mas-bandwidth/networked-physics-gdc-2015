#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#ifdef CLIENT

#include "core/Types.h"

class Font;

class FontManager
{
public:

    FontManager( core::Allocator & allocator );
    
    ~FontManager();

    void Reload();

    Font * GetFont( const char * name );

private:

    void Load();
    void Unload();

    core::Hash<Font*> m_fonts;
};

#endif

#endif
