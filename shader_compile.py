#!/usr/bin/env python3

import subprocess
import os
import os.path
import sys
import glob


def compile(path, output_path, glslc):
    subprocess.run([glslc, "-o", output_path, path], check=True)


if __name__ == "__main__":
    folder = sys.argv[1]
    output_folder = sys.argv[2]
    glslc = sys.argv[3]

    if not os.path.exists(output_folder):
        os.mkdir(output_folder)

    # extensions: .vert, .frag, .tesc, .tese, .geom, .comp

    def v(p):
        return os.path.splitext(p)[1] in (
            ".vert",
            ".frag",
            ".tesc",
            ".tese",
            ".geom",
            ".comp",
        )

    files = filter(v, glob.glob(os.path.join(folder, "**.*"), recursive=True))

    for file in files:
        compile(
            file,
            os.path.join(output_folder, os.path.relpath(file, folder) + ".spv"),
            glslc,
        )
        print(f"Compiled {file}")
