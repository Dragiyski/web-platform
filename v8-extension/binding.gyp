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
                "src/js-string-table.cxx",
                "src/object.cxx",
                "src//api/frozen-map.cxx",
                "src/api/private.cxx",
                "src/api/template.cxx",
                "src/api/template/lazy-data-property.cxx",
                "src/api/template/native-data-property.cxx",
                "src/api/function-template.cxx",
                "src/api/object-template.cxx",
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