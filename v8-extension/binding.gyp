{
    "targets": [
        {
            "target_name": "native",
            "cflags_cc": [
                "-std=c++20"
            ],
            "sources": [
                "src/main.cc",
                "src/string-table.cc",
                "src/object.cc",
                "src/function.cc",
                "src/security-token.cc",
                "src/api/context.cc",
                "src/api/user-context.cc",
                "src/api/user-context/time-schedule.cc",
                "src/api/user-context/secure-user-invoke.cc",
                "src/api/user-context/secure-user-apply.cc",
                "src/api/user-context/secure-user-construct.cc",
                "src/api/template.cc",
                "src/api/function-template.cc",
                "src/api/object-template.cc",
                "src/api/private.cc"
            ]
        }
    ]
}