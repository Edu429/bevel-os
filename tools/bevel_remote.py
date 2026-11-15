#!/usr/bin/env python3
"""View and control Bevel OS over its lightweight Wi-Fi TCP protocol."""
import argparse
import socket
import struct
import tkinter as tk

from bevel_remote_protocol import decode_frame, encode_input

KEY_USAGE = {
    "Return": 0x28, "Escape": 0x29, "BackSpace": 0x2A, "Tab": 0x2B, "space": 0x2C,
    "Right": 0x4F, "Left": 0x50, "Down": 0x51, "Up": 0x52, "Delete": 0x4C,
    "Home": 0x4A, "End": 0x4D, "Prior": 0x4B, "Next": 0x4E,
    "Shift_L": 0xE1, "Shift_R": 0xE5, "Control_L": 0xE0, "Control_R": 0xE4,
    "Alt_L": 0xE2, "Alt_R": 0xE6, "Super_L": 0xE3, "Caps_Lock": 0x39,
}
for _i, _name in enumerate("abcdefghijklmnopqrstuvwxyz"):
    KEY_USAGE[_name] = 0x04 + _i
for _i, _name in enumerate("1234567890"):
    KEY_USAGE[_name] = 0x1E + _i
for _i in range(12):
    KEY_USAGE["F%d" % (_i + 1)] = 0x3A + _i


def rgb222(p):
    r = (p & 3) * 85
    g = ((p >> 2) & 3) * 85
    b = ((p >> 4) & 3) * 85
    return r, g, b


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("host")
    parser.add_argument("--port", type=int, default=2323)
    args = parser.parse_args()
    sock = socket.create_connection((args.host, args.port))
    root = tk.Tk()
    root.title("Bevel OS")
    photo = tk.PhotoImage(width=640, height=480)
    tk.Label(root, image=photo).pack()

    def send(kind, payload):
        sock.sendall(encode_input(kind, payload))

    def key(event, down):
        usage = KEY_USAGE.get(event.keysym, 0)
        if not usage:
            return
        send(1, bytes((usage, down)))

    root.bind("<KeyPress>", lambda e: key(e, True))
    root.bind("<KeyRelease>", lambda e: key(e, False))
    root.focus_force()

    def mouse(event):
        send(2, struct.pack("<HH", max(0, min(639, event.x)), max(0, min(479, event.y))))

    root.bind("<Motion>", mouse)
    root.bind("<ButtonPress-1>", lambda e: send(4, b"\x01"))
    root.bind("<ButtonRelease-1>", lambda e: send(4, b"\x00"))

    def poll():
        def recv_exact(size):
            data = b""
            while len(data) < size:
                chunk = sock.recv(size - len(data))
                if not chunk:
                    return None
                data += chunk
            return data

        header = recv_exact(12)
        if header is None:
            root.destroy()
            return
        size = struct.unpack_from("<I", header, 8)[0]
        payload = recv_exact(size)
        if payload is None:
            root.destroy()
            return
        width, height, pixels = decode_frame(header, payload)
        rows = []
        for y in range(height):
            row = "{" + " ".join("#%02x%02x%02x" % rgb222(pixels[y * width + x]) for x in range(width)) + "}"
            rows.append(row)
        photo.put(" ".join(rows))
        root.after(10, poll)

    root.after(0, poll)
    root.mainloop()


if __name__ == "__main__":
    main()
