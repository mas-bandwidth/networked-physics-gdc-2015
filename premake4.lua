solution "Protocol"
    language "C++"
    buildoptions "-std=c++11 -stdlib=libc++"
    pchheader "include/Common.h"
    pchsource "src/Common.cpp"
    includedirs { "include", "." }
    platforms { "x64", "x32" }
    configurations { "Debug", "Release" }
    configuration "Debug"
        flags { "Symbols" }
    configuration "Release"
        flags { "Optimize" }
        defines { "NDEBUG" }

project "UnitTest"
    kind "ConsoleApp"
    files { "tests/UnitTest.cpp", "test/Test*.cpp" }
    links { "Protocol" }
    location "build"
    targetdir "bin"

project "protocol"
    kind "StaticLib"
    files { "include/*.h", "src/*.cpp" }
    targetdir "lib"

project "SoakTest"
    kind "ConsoleApp"
    files { "test/SoakTest.cpp" }
    links { "Protocol" }
    targetdir "bin"
    location "build"

project "Profile"
    kind "ConsoleApp"
    files { "tests/Profile.cpp" }
    links { "Protocol" }
    targetdir "bin"
    location "build"

if _ACTION == "clean" then
    os.rmdir "bin"
    os.rmdir "lib"
    os.rmdir "obj"
    os.rmdir "build"
    if not os.is "windows" then
        os.execute "rm -f Protocol.zip"
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
            os.execute "wc -l src/*.cpp tests/*.cpp include/*.h"
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
        description = "Build and run all unit tests",
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
        trigger     = "soak",
        description = "Build and run soak test",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 SoakTest" == 0 then
                os.execute "cd bin; ./SoakTest"
            end
        end
    }

    newaction
    {
        trigger     = "profile",
        description = "Build and run profile",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 Profile" == 0 then
                os.execute "cd bin; ./Profile"
            end
        end
    }

end
