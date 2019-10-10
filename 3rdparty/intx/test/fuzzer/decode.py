#!/usr/bin/env python3

import os
import sys

ops_filter = ()

ops = ('/', '*', '<<', '>>', '+', '-', 's/')


def err(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def decode_file(file):
    with open(file, 'rb') as f:
        print("Decoding {}".format(file))
        decode_data(f.read())

def decode_data(data):
    arg_size = (len(data) - 1) // 2
    if arg_size not in (16, 32, 64):
        err("Incorrect argument size: {}".format(arg_size))
        return

    op_index = int(data[0])
    if op_index >= len(ops):
        return
    op = ops[op_index]

    if ops_filter and op not in ops_filter:
        return

    x = int.from_bytes(data[1:1 + arg_size], byteorder='big')
    y = int.from_bytes(data[1 + arg_size:], byteorder='big')

    print("argument size: {}".format(arg_size))
    print(x, op, y)
    print(hex(x), op, hex(y))

    if op in ('/', 's/'):
        print("Test:")
        print("{")
        print("    {}_u512,".format(hex(x)))
        print("    {}_u512,".format(hex(y)))
        print("    {}_u512,".format(hex(x // y)))
        print("    {}_u512,".format(hex(x % y)))
        print("},")

    if op == 's/':
        ax = (-x) % 2**512
        ay = (-y) % 2**512
        print("Test:")
        print("{")
        print("    {}_u512,".format(hex(ax)))
        print("    {}_u512,".format(hex(ay)))
        print("    {}_u512,".format(hex(ax // ay)))
        print("    {}_u512,".format(hex(ax % ay)))
        print("},")


assert len(sys.argv) > 1

path = sys.argv[1]

if (os.path.isfile(path)):
    decode_file(path)
else:
    for root, _, files in os.walk(path):
        for file in files:
            decode_file(os.path.join(root, file))
