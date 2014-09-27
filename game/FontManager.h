#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#ifdef CLIENT

class Font;

class FontManager
{
public:

    FontManager();
    
    ~FontManager();

    void Reload();

    Font * GetFont( const char * name );

private:

    // ...
};

#endif

#endif
