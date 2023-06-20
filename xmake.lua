add_rules("mode.release")

set_languages("c++17")

add_requires("ffmpeg")
add_requires("libsdl")
add_requires("boost")
add_includedirs("D:/msys64/home/ffmpeg/include")
add_linkdirs("D:/msys64/home/ffmpeg/lib")
add_includedirs("D:/Qt/Qt5.14.2/5.14.2/mingw73_64/include")
add_linkdirs("D:/Qt/Qt5.14.2/5.14.2/mingw73_64/lib")

target("media")
    set_kind("shared")

    if is_mode("mix") then
        add_defines("DEBUG")
        set_symbols("debug")
        set_optimize("none")
    end 
    add_files("mediaTool/*.cpp")
    add_includedirs("mediaTool")
    add_packages("ffmpeg")
    add_packages("boost")

target("player")
    set_kind("shared")
    add_deps("media")

    if is_mode("mix") then
        add_defines("DEBUG")
        set_symbols("debug")
        set_optimize("none")
    end 

    add_headerfiles("mediaTool/*.h")
    add_includedirs("mediaTool")
    add_files("gui/*.cpp")
    add_includedirs("gui")
    add_packages("libsdl")

target("LiveStreamCapture")
    set_kind("binary")
    add_rules("qt.widgetapp")
    add_deps("player")
    add_includedirs("gui")
    add_includedirs("src")
    add_files("gui/*.h")
    add_files("src/*.cpp")
    
    -- add_files("gui/*.cpp")
    -- add_files("mediaTool/*.cpp")
    -- add_packages("ffmpeg")
    -- add_packages("libsdl")
    -- add_includedirs("mediaTool")
    -- add_files("src/*.h")
    -- add_files("mediaTool/*.h")

    if is_mode("mix") then
        add_defines("DEBUG")
        set_symbols("debug")
        set_optimize("none")
    end 

    add_files("src/mainwindow.ui")
    -- add files with Q_OBJECT meta (only for qt.moc)
    
    add_files("src/mainwindow.h")

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro defination
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

