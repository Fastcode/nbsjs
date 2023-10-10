{
    "targets": [
        {
            "target_name": "nbs_decoder",
            "sources": [
                "src/binding.cpp",
                "src/Decoder.cpp",
                "src/Encoder.cpp",
                "src/Hash.cpp",
                "src/Packet.cpp",
                "src/Timestamp.cpp",
                "src/TypeSubtype.cpp",
                "src/xxhash/xxhash.c",
            ],
            "cflags": [],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
            ],
            "defines": [
                # Restrict NAPI to v6 (to support Node v10)
                # Changing this should have a corresponding change to "engines" in package.json
                # See https://nodejs.org/api/n-api.html#node-api-version-matrix for which
                # versions of NAPI support which versions of Node
                "NAPI_VERSION=6"
            ],
            "msvs_settings": {"VCCLCompilerTool": {"ExceptionHandling": 1}},
            "conditions": [
                [
                    'OS=="linux"',
                    {
                        "ccflags": [
                            "-std=c++14",
                            "-fPIC",
                            "-fext-numeric-literals",
                            "-fexceptions",
                        ],
                        "ccflags!": ["-fno-exceptions"],
                        "cflags_cc": ["-std=c++14", "-fext-numeric-literals"],
                        "cflags_cc!": ["-fno-exceptions", "-fno-rtti"],
                    },
                ],
                [
                    'OS=="mac"',
                    {
                        "ccflags": [
                            "-std=c++14",
                            "-fPIC",
                            "-fext-numeric-literals",
                            "-fexceptions",
                        ],
                        "ccflags!": ["-fno-exceptions"],
                        "cflags_cc": ["-std=c++14", "-fext-numeric-literals"],
                        "cflags_cc!": ["-fno-exceptions", "-fno-rtti"],
                        "xcode_settings": {
                            "MACOSX_DEPLOYMENT_TARGET": "10.9",
                            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
                            "GCC_ENABLE_CPP_RTTI": "YES",
                            "OTHER_CPLUSPLUSFLAGS": ["-std=c++14", "-stdlib=libc++"],
                            "OTHER_LDFLAGS": ["-stdlib=libc++"],
                        },
                    },
                ],
                [
                    'OS=="win"',
                    {
                        "defines": ["_HAS_EXCEPTIONS=1", "NOMINMAX=1"],
                        "msvs_settings": {
                            "VCCLCompilerTool": {
                                "AdditionalOptions": ["-std:c++14"],
                            },
                        },
                    },
                ],
            ],
        }
    ]
}
