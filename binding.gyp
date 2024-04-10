{
    "targets": [
        {
            "target_name": "zlib",
            "type": "static_library",
            "cflags": ["-Wno-error"],
            "sources": [
                "src/third-party/zlib/adler32.c",
                "src/third-party/zlib/compress.c",
                "src/third-party/zlib/crc32.c",
                "src/third-party/zlib/deflate.c",
                "src/third-party/zlib/gzclose.c",
                "src/third-party/zlib/gzlib.c",
                "src/third-party/zlib/gzread.c",
                "src/third-party/zlib/gzwrite.c",
                "src/third-party/zlib/infback.c",
                "src/third-party/zlib/inffast.c",
                "src/third-party/zlib/inflate.c",
                "src/third-party/zlib/inftrees.c",
                "src/third-party/zlib/trees.c",
                "src/third-party/zlib/uncompr.c",
                "src/third-party/zlib/zutil.c",
            ],
        },
        {
            "dependencies": [
                "zlib",
            ],
            "target_name": "nbsdecoder",
            "sources": [
                "src/binding.cpp",
                "src/Decoder.cpp",
                "src/Encoder.cpp",
                "src/Hash.cpp",
                "src/Packet.cpp",
                "src/Timestamp.cpp",
                "src/third-party/xxhash/xxhash.c",
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                "src/third-party/zlib",
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
        },
    ],
}
