#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#ifdef CLIENT

class ShaderManager
{
public:

    ShaderManager();
    
    ~ShaderManager();

    void Reload();

    unsigned int GetShader( const char * name );

private:

    // ...
};

#endif

#endif
