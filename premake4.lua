solution "tinyscheme"
   configurations { "Debug", "Release" }
 
   -- A project defines one build target
   project "tinyscheme"
      kind "ConsoleApp"
      language "C"
      includedirs { "./include" }
      links { "m", "dl" }
      files { "include/**.h", "src/**.h", "src/**.c" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }    
