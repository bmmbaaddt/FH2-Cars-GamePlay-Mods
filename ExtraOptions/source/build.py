"""Build the native mod with Zig 0.15.2, including cross-compilation on Linux.
Usage: python build.py /path/to/zig [--azimuth] [--test] [--output PATH]
"""

import argparse
from pathlib import Path
import subprocess
import tempfile

SOURCE = Path(__file__).resolve().parent
COMMON_SOURCES = [
    "dllmain.c",
    "runtime.c",
    "settings.c",
    "support.c",
    "logging.c",
    "patch_engine.c",
    "verification.c",
    "sha256.c",
    "patches_generated.c",
    "features/garage.c",
    "features/road_placement.c",
    "features/gamerpic.c",
    "features/treasure_map.c",
    "features/minimap.c",
    "features/rims.c",
    "features/stock_wheels.c",
    "features/drift_camera.c",
    "features/video_settings.c",
    "features/camera_fov.c",
    "features/video_options.c",
    "features/video_options_text.c",
    "features/video_options_xaml.c",
    "features/photo_sampling.c",
    "features/media_overrides.c",
    "features/vehicle_controls.c",
    "features/world_keys.c",
    "features/steering_wheel.c",
    "features/ghost_traffic.c",
    "features/sky_road.c",
    "features/roof_fade.c",
    "features/autodrive.c",
    "features/game_input.c",
    "features/autodrive_game.c",
    "features/autodrive_notice.c",
    "features/autodrive_ui.c",
    "features/png_validation.c",
    "features/png_decode.c",
]


def parse_arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    parser.add_argument("--test", action="store_true")
    parser.add_argument("--azimuth", action="store_true", help="Build the Azimuth TLS adapter")
    parser.add_argument(
        "--diagnostic", action="store_true", help="Exit Azimuth after startup checks"
    )
    parser.add_argument("--output", type=Path)
    parser.add_argument("--build-dir", type=Path)
    arguments = parser.parse_args()
    if arguments.diagnostic and (not arguments.azimuth or arguments.test):
        parser.error("--diagnostic requires --azimuth without --test")
    return arguments


def compiler_arguments(compiler, options):
    arguments = [
        str(compiler),
        "clang",
        "-target",
        "x86_64-w64-windows-gnu",
        "-c",
        "-ffreestanding",
        "-fno-builtin",
        "-fno-stack-protector",
        "-funwind-tables",
        "-O2",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(SOURCE),
        "-isystem",
        str(compiler.parent / "lib/include"),
        "-isystem",
        str(compiler.parent / "lib/libc/include/any-windows-any"),
    ]
    if options.test:
        arguments.append("-DFH2_TWEAKS_TEST=1")
    if options.azimuth:
        arguments.append("-DFH2_AZIMUTH=1")
    if options.diagnostic:
        arguments.append("-DFH2_AZIMUTH_DIAGNOSTIC=1")
    return arguments


def build(compiler, output, directory, options):
    directory.mkdir(parents=True, exist_ok=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    objects = []
    sources = [*COMMON_SOURCES]
    sources.extend(
        [
            "adapters/azimuth.c",
            "adapters/azimuth_host.c",
            "adapters/azimuth_livery.c",
            "adapters/azimuth_save.c",
            "adapters/azimuth_markers.c",
        ]
        if options.azimuth
        else ["adapters/mezcal.c"]
    )
    arguments = compiler_arguments(compiler, options)
    for filename in sources:
        object_file = directory / (Path(filename).stem + ".obj")
        subprocess.run([*arguments, str(SOURCE / filename), "-o", str(object_file)], check=True)
        objects.append(str(object_file))
    import_library = directory / "kernel32.lib"
    subprocess.run(
        [
            str(compiler),
            "dlltool",
            "-m",
            "i386:x86-64",
            "-d",
            str(SOURCE / "kernel32.def"),
            "-l",
            str(import_library),
        ],
        check=True,
    )
    link_arguments = [
        str(compiler),
        "lld-link",
        "/dll",
        "/entry:DllMain",
        "/machine:x64",
        "/nodefaultlib",
        "/dynamicbase",
        "/nxcompat",
        "/out:" + str(output),
        "/implib:" + str(directory / "extra_options.lib"),
        *objects,
        str(import_library),
    ]
    if options.azimuth:
        link_arguments.append("/include:_tls_used")
    subprocess.run(link_arguments, check=True)


def main():
    options = parse_arguments()
    compiler = options.zig.resolve()
    filename = (
        "fh2-extra-options-test.dll"
        if options.test
        else "azimuth-tls.dll" if options.azimuth else "xg.dll"
    )
    output = (options.output or SOURCE.parent / filename).resolve()
    if options.build_dir:
        build(compiler, output, options.build_dir.resolve(), options)
    else:
        with tempfile.TemporaryDirectory(prefix="fh2-extra-options-build-") as temporary:
            build(compiler, output, Path(temporary), options)
    print(output)


if __name__ == "__main__":
    main()
