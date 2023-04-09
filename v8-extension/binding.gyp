{
    "targets": [
        {
            "target_name": "native",
            "cflags_cc": [
                "-std=c++20",
                "-fno-threadsafe-statics"
            ],
            "cflags_cc!": [
                "-fno-rtti",
                "-fno-exceptions",
                "-std=gnu++17"
            ],
            "sources": [
                "src/main.cxx",
                "src/function.cxx",
                "src/wrapper.cxx",
                "src/api/private.cxx",
                "src/api/context.cxx",
                "src/api/function-template.cxx",
            ]
        }
    ],
    "target_defaults": {
        "default_configuration": "Release",
        "configurations": {
            "Debug": {
                "defines": [ "DEBUG", "_DEBUG" ],
                "cflags_cc": [ "-g", "-O0", "--coverage" ],
                "ldflags": ["--coverage", "-lgcov"],
                "cflags_cc!": [ "-std=gnu++17" ]
            },
            "Release": {
                'defines': ['NDEBUG'],
                "cflags_cc": ["-O3"],
                "cflags_cc!": [ "-std=gnu++17" ]
            }
        }
    }
}