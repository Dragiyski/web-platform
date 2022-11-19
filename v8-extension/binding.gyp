{
    "targets": [
        {
            "target_name": "native",
            "cflags_cc": [
                "-std=c++20",
                "-g"
            ],
            "cflags_cc!": [
                "-fno-rtti",
                "-fno-exceptions",
                "-std=gnu++17"
            ],
            "sources": [
                "src/main.cxx",
                "src/js-string-table.cxx",
                "src/wrapper.cxx",
            ]
        }
    ],
    "target_defaults": {
        "default_configuration": "Release",
        "configurations": {
            "Debug": {
                "defines": [ "DEBUG", "_DEBUG" ],
                "cflags_cc": [ "-g", "-O0" ],
            },
            "Release": {
                'defines': ['NDEBUG'],
                "cflags_cc": ["-O3"]
            }
        }
    }
}