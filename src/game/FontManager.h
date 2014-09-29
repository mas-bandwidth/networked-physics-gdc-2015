#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#ifdef CLIENT

#include "Types.h"

class Font;

class FontManager
{
public:

    FontManager( protocol::Allocator & allocator );
    
    ~FontManager();

    void Reload();

    Font * GetFont( const char * name );

private:

    void Load();
    void Unload();

    protocol::Hash<Font*> m_fonts;
};

#endif

#endif
