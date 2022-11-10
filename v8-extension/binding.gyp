{
    "targets": [
        {
            "target_name": "native",
            "cflags_cc": [
                "-std=c++20"
            ],
            "sources": [
                "src/main.cc",
                "src/compile-function.cc",
                "src/context.cc",
                "src/function.cc",
                "src/native-function.cc",
                "src/security-token.cc",
                "src/script.cc",
            ]
        }
    ]
}