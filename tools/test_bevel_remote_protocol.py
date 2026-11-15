import struct
import unittest

from bevel_remote_protocol import decode_frame, encode_input


class ProtocolTests(unittest.TestCase):
    def test_input_frame(self):
        self.assertEqual(encode_input(1, b"\x04\x01"), b"BV\x01\x02\x02\x00\x04\x01")

    def test_decode_rle(self):
        header = b"BV\x01\x01" + struct.pack("<HHI", 2, 2, 4)
        self.assertEqual(decode_frame(header, bytes((2, 7, 2, 9)))[2], bytes((7, 7, 9, 9)))

    def test_rejects_bad_rle(self):
        header = b"BV\x01\x01" + struct.pack("<HHI", 2, 2, 2)
        with self.assertRaises(ValueError):
            decode_frame(header, bytes((5, 7)))


if __name__ == "__main__":
    unittest.main()
