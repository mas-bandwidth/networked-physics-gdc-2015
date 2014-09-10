solution "Protocol"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    includedirs { "include", "." }
    platforms { "x64", "x32" }
    configurations { "Debug", "Release" }
    flags { "Symbols", "ExtraWarnings", "EnableSSE2", "FloatFast" , "NoRTTI", "NoExceptions" }
    configuration "Release"
        flags { "OptimizeSpeed" }
        defines { "NDEBUG" }

project "protocol"
    kind "StaticLib"
    files { "include/*.h", "src/*.cpp" }
    targetdir "lib"

project "UnitTest"
    kind "ConsoleApp"
    files { "tests/UnitTest.cpp", "tests/Test*.cpp" }
    links { "protocol" }
    location "build"
    targetdir "bin"

project "SoakProtocol"
    kind "ConsoleApp"
    files { "tests/SoakProtocol.cpp" }
    links { "protocol" }
    targetdir "bin"
    location "build"

project "SoakClientServer"
    kind "ConsoleApp"
    files { "tests/SoakClientServer.cpp" }
    links { "protocol" }
    targetdir "bin"
    location "build"

project "ProfileProtocol"
    kind "ConsoleApp"
    files { "tests/ProfileProtocol.cpp" }
    links { "protocol" }
    targetdir "bin"
    location "build"

project "ProfileClientServer"
    kind "ConsoleApp"
    files { "tests/ProfileClientServer.cpp" }
    links { "protocol" }
    targetdir "bin"
    location "build"

project "FontBuilder"
    kind "ConsoleApp"
    files { "tools/FontBuilder/*.cpp" }
    links { "freetype", "jansson" }
    location "build"
    targetdir "bin"

project "Client"
    kind "ConsoleApp"
    files { "game/*.cpp" }
    links { "protocol", "glew", "glfw3", "GLUT.framework", "OpenGL.framework", "Cocoa.framework" }
    location "build"
    targetdir "bin"
    defines { "CLIENT" }

project "Server"
    kind "ConsoleApp"
    files { "game/*.cpp" }
    links { "protocol" }
    location "build"
    targetdir "bin"

if _ACTION == "clean" then
    os.execute "rm -rf bin"     -- IMPORTANT: os.rmdif follows symlinks and we don't want that
    os.rmdir "lib"
    os.rmdir "obj"
    os.rmdir "build"
    if not os.is "windows" then
        os.execute "rm -f Protocol.zip"
        os.execute "rm -f p*.txt"
        os.execute "find . -name *.DS_Store -type f -exec rm {} \\;"
    else
        os.rmdir "ipch"
        os.execute "del /F /Q Protocol.zip"
    end
end

if not os.is "windows" then

    newaction 
    {
        trigger     = "loc",
        description = "Count lines of code",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,

        execute = function ()
            os.execute "wc -l src/*.cpp tests/*.h tests/*.cpp include/*.h"
        end
    }

    newaction
    {
        trigger     = "zip",
        description = "Zip up archive of this project",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            _ACTION = "clean"
            premake.action.call( "clean" )
            os.execute "zip -9r Protocol.zip *"
        end
    }

    newaction
    {
        trigger     = "lib",
        description = "Build protocol library",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            os.execute "make -j32 protocol"
        end
    }

    newaction
    {
        trigger     = "test",
        description = "Build and run unit tests",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 UnitTest" == 0 then
                os.execute "cd bin; ./UnitTest"
            end
        end
    }

    newaction
    {
        trigger     = "fonts",
        description = "Build fonts",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 FontBuilder" == 0 then
                if os.execute "bin/FontBuilder data/fonts/Fonts.json" ~= 0 then
                    os.exit(1)
                end
            end
        end
    }

    newaction
    {
        trigger     = "shaders",
        description = "Build shaders",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "mkdir -p bin/data; rm -f bin/data/shaders; ln -s ~/git/protocol/data/shaders bin/data/shaders" ~= 0 then
                os.exit(1)
            end
        end
    }

    newaction
    {
        trigger     = "data",
        description = "Build game data",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            premake.action.call( "fonts" )
            premake.action.call( "shaders" )
        end
    }

    newaction
    {
        trigger     = "client",
        description = "Build and run game client",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 Client" ~= 0 then
                os.exit(1)
            end
            premake.action.call( "data" )
            os.execute "cd bin; ./Client"
        end
    }

    newaction
    {
        trigger     = "server",
        description = "Build and run game server",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 Server" == 0 then
                os.execute "cd bin; ./Server"
            end
        end
    }

    newaction
    {
        trigger     = "soak_protocol",
        description = "Build and run protocol soak test",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 SoakProtocol" == 0 then
                os.execute "cd bin; ./SoakProtocol"
            end
        end
    }

    newaction
    {
        trigger     = "soak_client_server",
        description = "Build and run client/server soak test",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 SoakClientServer" == 0 then
                os.execute "cd bin; ./SoakClientServer"
            end
        end
    }

    newaction
    {
        trigger     = "profile_protocol",
        description = "Build and run protocol profile",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 ProfileProtocol" == 0 then
                os.execute "cd bin; ./ProfileProtocol"
            end
        end
    }

    newaction
    {
        trigger     = "profile_client_server",
        description = "Build and run client server profile",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 ProfileClientServer" == 0 then
                os.execute "cd bin; ./ProfileClientServer"
            end
        end
    }

end
