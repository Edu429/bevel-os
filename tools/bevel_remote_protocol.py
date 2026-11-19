import struct

WIDTH = 640
HEIGHT = 480

def encode_input(kind, payload=b""):
    data = bytes((kind,)) + payload
    if len(data) > 32:
        raise ValueError("input payload too large")
    return b"BV" + bytes((1, 2)) + struct.pack("<H", len(data)) + data


def decode_frame(header, payload):
    if len(header) != 12 or header[:4] != b"BV\x01\x01":
        raise ValueError("invalid frame header")
    width, height, size = struct.unpack_from("<HHI", header, 4)
    if size != len(payload):
        raise ValueError("truncated frame")
    out = bytearray(width * height)
    pos = 0
    for i in range(0, len(payload), 2):
        count, value = payload[i], payload[i + 1]
        if not count or pos + count > len(out):
            raise ValueError("invalid RLE")
        out[pos:pos + count] = bytes((value,)) * count
        pos += count
    if pos != len(out):
        raise ValueError("incomplete RLE")
    return width, height, out
