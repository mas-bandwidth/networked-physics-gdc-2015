solution "Protocol"
    includedirs { "src", "external", "tools", "." }
    platforms { "x64" }
    configurations { "Debug", "Release" }
    flags { "Symbols", "ExtraWarnings", "EnableSSE2", "FloatFast" , "NoRTTI", "NoExceptions" }
    configuration "Release"
        flags { "OptimizeSpeed" }
        defines { "NDEBUG" }

project "Core"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "StaticLib"
    files { "src/Core/*.h", "src/Core/*.cpp" }
    targetdir "lib"

project "Network"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "StaticLib"
    files { "src/network/*.h", "src/network/*.cpp" }
    links { "Core" }
    targetdir "lib"

project "Protocol"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "StaticLib"
    files { "src/protocol/*.h", "src/protocol/*.cpp" }
    links { "Core", "Network" }
    targetdir "lib"

project "ClientServer"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "StaticLib"
    files { "src/ClientServer/*.h", "src/ClientServer/*.cpp" }
    links { "Core", "Network", "Protocol" }
    targetdir "lib"

project "VirtualGo"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "StaticLib"
    files { "src/VirtualGo/*.h", "src/VirtualGo/*.cpp" }
    links { "Core" }
    targetdir "lib"

project "Cubes"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "StaticLib"
    files { "src/Cubes/*.h", "src/Cubes/*.cpp" }
    links { "Core" }
    targetdir "lib"

project "nvImage"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "StaticLib"
    files { "external/nvImage/*.h", "external/nvImage/*.cpp" }
    targetdir "lib"

project "tinycthread"
    language "C"
    kind "StaticLib"
    files { "external/tinycthread/*.h", "external/tinycthread/*.c" }
    targetdir "lib"

project "TestCore"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tests/Core/*.cpp" }
    links { "Core" }
    location "build"
    targetdir "bin"

project "TestNetwork"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tests/Network/Test*.cpp" }
    links { "Core", "Network", "Protocol", "ClientServer" }     -- todo: should not depend on protocol or client server
    location "build"
    targetdir "bin"

project "TestProtocol"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tests/Protocol/Test*.cpp" }
    links { "Core", "Network", "Protocol", "ClientServer" }     -- todo: should not depend on client server
    location "build"
    targetdir "bin"

project "TestClientServer"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tests/ClientServer/Test*.cpp" }
    links { "Core", "Network", "Protocol", "ClientServer" }
    location "build"
    targetdir "bin"

project "TestCubes"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tests/Cubes/*.cpp" }
    links { "Core", "Cubes", "ode" }
    location "build"
    targetdir "bin"

project "TestVirtualGo"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tests/VirtualGo/*.cpp" }
    links { "Core", "VirtualGo", "ode" }
    location "build"
    targetdir "bin"

project "SoakProtocol"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tests/Protocol/SoakProtocol.cpp" }
    links { "Core", "Network", "Protocol", "ClientServer" }     -- todo: should not depend on client/server
    targetdir "bin"
    location "build"

project "SoakClientServer"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tests/ClientServer/SoakClientServer.cpp" }
    links { "Core", "Network", "Protocol", "ClientServer" }
    targetdir "bin"
    location "build"

project "ProfileProtocol"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tests/Protocol/ProfileProtocol.cpp" }
    links { "Core", "Network", "Protocol", "ClientServer" }     -- todo: should not depend on client/server
    targetdir "bin"
    location "build"

project "ProfileClientServer"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tests/ClientServer/ProfileClientServer.cpp" }
    links { "Core", "Network", "Protocol", "ClientServer" }
    targetdir "bin"
    location "build"

project "FontTool"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tools/Font/*.cpp" }
    links { "Core", "Freetype", "Jansson" }
    location "build"
    targetdir "bin"

project "StoneTool"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "tools/Stone/*.cpp" }
    links { "Core", "VirtualGo", "Jansson" }
    location "build"
    targetdir "bin"

project "Client"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "src/game/*.cpp" }
    links { "Core", "Network", "Protocol", "ClientServer", "VirtualGo", "Cubes", "nvImage", "tinycthread", "ode", "glew", "glfw3", "GLUT.framework", "OpenGL.framework", "Cocoa.framework", "CoreVideo.framework", "IOKit.framework" }
    location "build"
    targetdir "bin"
    defines { "CLIENT" }

project "Server"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations"
    kind "ConsoleApp"
    files { "src/game/*.cpp" }
    links { "Core", "Network", "Protocol", "ClientServer" }
    location "build"
    targetdir "bin"

if _ACTION == "clean" then
    os.rmdir "bin"
    os.rmdir "lib"
    os.rmdir "obj"
    os.rmdir "build"
    if not os.is "windows" then
        os.execute "rm -f Protocol.zip"
        os.execute "rm -f *.txt"
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
            os.execute "make -j4 Core"
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
            os.execute "make -j4 Network"
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
            os.execute "make -j4 Protocol"
        end
    }

    newaction
    {
        trigger     = "client_server",
        description = "Build client/server library",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            os.execute "make -j4 ClientServer"
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
            os.execute "make -j4 VirtualGo"
        end
    }

    newaction
    {
        trigger     = "test",
        description = "Build and run all unit tests",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 TestCore; make -j4 TestNetwork; make -j4 TestProtocol; make -j4 TestCubes; make -j4 TestVirtualGo" == 0 then
                os.execute "cd bin; ./TestCore; ./TestNetwork; ./TestProtocol; ./TestCubes; ./TestVirtualGo"
            end
        end
    }

    newaction
    {
        trigger     = "test_core",
        description = "Build and run core unit tests",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 TestCore" == 0 then
                os.execute "cd bin; ./TestCore"
            end
        end
    }

    newaction
    {
        trigger     = "test_network",
        description = "Build and run network unit tests",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 TestNetwork" == 0 then
                os.execute "cd bin; ./TestNetwork"
            end
        end
    }

    newaction
    {
        trigger     = "test_protocol",
        description = "Build and run protocol unit tests",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 TestProtocol" == 0 then
                os.execute "cd bin; ./TestProtocol"
            end
        end
    }

    newaction
    {
        trigger     = "test_client_server",
        description = "Build and run client/server unit tests",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 TestClientServer" == 0 then
                os.execute "cd bin; ./TestClientServer"
            end
        end
    }

    newaction
    {
        trigger     = "test_cubes",
        description = "Build and run cubes unit tests",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 TestCubes" == 0 then
                os.execute "cd bin; ./TestCubes"
            end
        end
    }

    newaction
    {
        trigger     = "test_virtualgo",
        description = "Build and run virtualgo unit tests",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 TestVirtualGo" == 0 then
                os.execute "cd bin; ./TestVirtualGo"
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
            if os.execute "make -j4 FontTool" == 0 then
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
            if os.execute "make -j4 StoneTool" == 0 then
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
            if os.execute "make -j4 Client" ~= 0 then
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
            if os.execute "make -j4 Server" ~= 0 then
                os.exit(1)
            end
            os.execute "bin/Server"
        end
    }

    newaction
    {
        trigger     = "stone",
        description = "Build and run stone demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 Client" ~= 0 then
                os.exit(1)
            end
            os.execute "bin/Client +load stone"
        end
    }

    newaction
    {
        trigger     = "cubes",
        description = "Build and run cubes demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 Client" ~= 0 then
                os.exit(1)
            end
            os.execute "bin/Client +load cubes"
        end
    }

    newaction
    {
        trigger     = "interpolation",
        description = "Build and run interpolation demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 Client" ~= 0 then
                os.exit(1)
            end
            os.execute "bin/Client +load interpolation"
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
            if os.execute "make -j4 Server" == 0 then
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
            if os.execute "make -j4 SoakProtocol" == 0 then
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
            if os.execute "make -j4 SoakClientServer" == 0 then
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
            if os.execute "make -j4 ProfileProtocol" == 0 then
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
            if os.execute "make -j4 ProfileClientServer" == 0 then
                os.execute "bin/ProfileClientServer"
            end
        end
    }

end
