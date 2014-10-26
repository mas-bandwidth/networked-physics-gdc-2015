solution "Protocol"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    includedirs { "src", "." }
    platforms { "x64", "x32" }
    configurations { "Debug", "Release" }
    flags { "Symbols", "ExtraWarnings", "EnableSSE2", "FloatFast" , "NoRTTI", "NoExceptions" }
    configuration "Release"
        flags { "OptimizeSpeed" }
        defines { "NDEBUG" }

project "core"
    kind "StaticLib"
    files { "src/core/*.h", "src/core/*.cpp" }
    targetdir "lib"

project "network"
    kind "StaticLib"
    files { "src/network/*.h", "src/network/*.cpp" }
    links { "core" }
    targetdir "lib"

project "protocol"
    kind "StaticLib"
    files { "src/protocol/*.h", "src/protocol/*.cpp" }
    links { "core", "network" }
    targetdir "lib"

project "virtualgo"
    kind "StaticLib"
    files { "src/virtualgo/*.h", "src/virtualgo/*.cpp" }
    links { "core" }
    targetdir "lib"

project "UnitTest"
    kind "ConsoleApp"
    files { "tests/UnitTest.cpp", "tests/Test*.cpp" }
    links { "core", "network", "protocol" }
    location "build"
    targetdir "bin"

project "SoakProtocol"
    kind "ConsoleApp"
    files { "tests/SoakProtocol.cpp" }
    links { "core", "network", "protocol" }
    targetdir "bin"
    location "build"

project "SoakClientServer"
    kind "ConsoleApp"
    files { "tests/SoakClientServer.cpp" }
    links { "core", "network", "protocol" }
    targetdir "bin"
    location "build"

project "ProfileProtocol"
    kind "ConsoleApp"
    files { "tests/ProfileProtocol.cpp" }
    links { "core", "network", "protocol" }
    targetdir "bin"
    location "build"

project "ProfileClientServer"
    kind "ConsoleApp"
    files { "tests/ProfileClientServer.cpp" }
    links { "core", "network", "protocol" }
    targetdir "bin"
    location "build"

project "FontTool"
    kind "ConsoleApp"
    files { "tools/Font/*.cpp" }
    links { "core", "freetype", "jansson" }
    location "build"
    targetdir "bin"

project "StoneTool"
    kind "ConsoleApp"
    files { "tools/Stone/*.cpp" }
    links { "core", "virtualgo", "jansson" }
    location "build"
    targetdir "bin"

project "Client"
    kind "ConsoleApp"
    files { "src/game/*.cpp" }
    links { "core", "network", "protocol", "virtualgo", "glew", "glfw3", "GLUT.framework", "OpenGL.framework", "Cocoa.framework" }
    location "build"
    targetdir "bin"
    defines { "CLIENT" }

project "Server"
    kind "ConsoleApp"
    files { "src/game/*.cpp" }
    links { "core", "network", "protocol", "virtualgo" }
    location "build"
    targetdir "bin"

if _ACTION == "clean" then
    os.execute "rm -rf bin"     -- IMPORTANT: LUA os.rmdif follows symlinks and we don't want that!!!
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
            os.execute "find . -name *.h -o -name *.cpp | xargs wc -l"
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
        trigger     = "core",
        description = "Build core library",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            os.execute "make -j32 core"
        end
    }

    newaction
    {
        trigger     = "network",
        description = "Build network library",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            os.execute "make -j32 network"
        end
    }

    newaction
    {
        trigger     = "protocol",
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
        trigger     = "virtualgo",
        description = "Build virtualgo library",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            os.execute "make -j32 virtualgo"
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
            if os.execute "make -j32 FontTool" == 0 then
                if os.execute "bin/FontTool assets/fonts/Fonts.json" ~= 0 then
                    os.exit(1)
                end
            end
        end
    }

    newaction
    {
        trigger     = "stones",
        description = "Build stones",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 StoneTool" == 0 then
                if os.execute "rm -rf data/stones; mkdir -p data/stones; bin/StoneTool" ~= 0 then
                    os.exit(1)
                end
            end
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
            os.execute "bin/Client"
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
                os.execute "bin/Server"
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
                os.execute "bin/SoakProtocol"
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
                os.execute "bin/SoakClientServer"
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
                os.execute "bin/ProfileProtocol"
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
                os.execute "bin/ProfileClientServer"
            end
        end
    }

end
