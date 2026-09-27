"""Exercise Wine prefix extraction with real, long-path and broken tarballs."""

import gzip
import io
import subprocess
import tarfile
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
LONG_NAME = (
    "prefix/drive_c/Program Files (x86)/Steam/controller_base/localization/"
    "steam_controller_portuguese.txt"
)


def make_tar(path: Path, name: str = LONG_NAME, truncate: bool = False) -> None:
    out = io.BytesIO()
    with tarfile.open(fileobj=out, mode="w", format=tarfile.USTAR_FORMAT) as tar:
        for filename, data in [
            ("prefix/.update-timestamp", b"complete\n"),
            (name, b"Steam path preserved\n"),
        ]:
            entry = tarfile.TarInfo(filename)
            entry.size = len(data)
            tar.addfile(entry, io.BytesIO(data))
    raw = out.getvalue().rstrip(b"\0") if truncate else out.getvalue()
    path.write_bytes(gzip.compress(raw))


def main() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        tmpdir = Path(tmp)
        binary = tmpdir / "extract-test"
        subprocess.run(
            [
                "clang", "-Wall", "-Wextra", "-I", str(ROOT / "app/Madeira"),
                str(ROOT / "tools/test_prefix_extractor_main.c"),
                str(ROOT / "app/Madeira/PrefixExtractor.c"), "-lz", "-o", str(binary),
            ],
            check=True,
        )

        base = tmpdir / "base"
        subprocess.run(
            [str(binary), str(ROOT / "app/Madeira/prefix-template.tar.gz"), str(base)],
            check=True,
        )
        assert (base / "drive_c").is_dir()
        assert (base / ".update-timestamp").is_file()

        long_archive = tmpdir / "long.tar.gz"
        make_tar(long_archive)
        long_dest = tmpdir / "long"
        subprocess.run([str(binary), str(long_archive), str(long_dest)], check=True)
        assert (long_dest / LONG_NAME.removeprefix("prefix/")).read_bytes() == b"Steam path preserved\n"
        assert (long_dest / ".update-timestamp").is_file()

        incomplete = tmpdir / "incomplete.tar.gz"
        make_tar(incomplete, truncate=True)
        incomplete_dest = tmpdir / "incomplete"
        result = subprocess.run([str(binary), str(incomplete), str(incomplete_dest)])
        assert result.returncode != 0
        assert not (incomplete_dest / ".update-timestamp").exists()

        unsafe = tmpdir / "unsafe.tar.gz"
        make_tar(unsafe, name="prefix/../escape")
        unsafe_dest = tmpdir / "unsafe"
        result = subprocess.run([str(binary), str(unsafe), str(unsafe_dest)])
        assert result.returncode != 0
        assert not (tmpdir / "escape").exists()

    print("Prefix extractor: base, long path, interrupted and unsafe archives passed")


if __name__ == "__main__":
    main()
