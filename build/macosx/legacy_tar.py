"""USTAR headers compatible with the original Mac OS X tar reader."""
import tarfile


class LegacyTarInfo(tarfile.TarInfo):
    def create_ustar_header(self, info, encoding, errors):
        header = bytearray(super().create_ustar_header(info, encoding, errors))
        # Cheetah's reader treats a full 100-byte name as a C string and
        # appends the following mode field. Use the USTAR prefix to leave
        # space for a terminator, including after Python's own path split.
        if b"\0" not in header[:100]:
            raw = bytes(header[:100])
            directory = raw.endswith(b"/")
            prefix, separator, name = raw.rstrip(b"/").rpartition(b"/")
            if not separator:
                raise ValueError("Original tar cannot read this filename: " + info["name"])
            if directory:
                name += b"/"
            previous = bytes(header[345:500]).rstrip(b"\0")
            if previous:
                prefix = previous + b"/" + prefix
            if len(prefix) >= 155 or len(name) >= 100:
                raise ValueError("Original tar cannot read this pathname: " + info["name"])
            header[:100] = name.ljust(100, b"\0")
            header[345:500] = prefix.ljust(155, b"\0")
            header[148:156] = b" " * 8
            header[148:156] = ("%06o\0 " % sum(header)).encode("ascii")
        if b"\0" not in header[157:257] or b"\0" not in header[345:500]:
            raise ValueError("Unterminated original-tar link or prefix: " + info["name"])
        return bytes(header)
