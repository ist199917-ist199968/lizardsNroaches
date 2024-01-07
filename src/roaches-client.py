import signal
import zmq
import random
from message_pb2 import proto_direction , proto_char_message
import sys
import time
import socket
import struct

requester = None
m = proto_char_message()

def sigint_handler(signum, frame):
    m.msg_type = 8
    packed_size = m.ByteSize()
    packed_buffer = m.SerializeToString()
    requester.send(packed_buffer)
    recvm = proto_char_message()
    recvm.ParseFromString(requester.recv())
    exit(signum)

def suppress_sigint():
    signal.signal(signal.SIGINT, signal.SIG_IGN)

def allow_sigint():
    signal.signal(signal.SIGINT, signal.SIG_DFL)

def is_valid_ipv4(candidate):
    try:
        socket.inet_pton(socket.AF_INET, candidate)
        return True
    except socket.error:
        return False

def is_valid_port(candidate):
    try:
        port = int(candidate)
        return 1023 < port <= 65535
    except ValueError:
        return False

def main():
    global requester, m

    signal.signal(signal.SIGINT, sigint_handler)

    if len(sys.argv) != 3:
        print("Wrong number of arguments")
        return 1

    ip, port1 = sys.argv[1], sys.argv[2]

    if not is_valid_ipv4(ip) or not is_valid_port(port1):
        print("ERROR[000]: Incorrect Host-Server data. Check host IPv4 and TCP ports.")
        exit(1)

    candidate1 = f"tcp://{ip}:{port1}"

    context = zmq.Context()
    requester = context.socket(zmq.REQ)
    requester.connect(candidate1)

    ncock = 0
    while True:
        try:
            ncock = int(input("How many cockroaches (1-10)?: "))
            if 1 <= ncock <= 10:
                break
            else:
                print("Invalid input. Please enter a number between 1 and 10.")
        except ValueError:
            print("Invalid input. Please enter a number.")

    password = ''.join(chr(random.randint(32, 125)) for _ in range(49))
    m.msg_type = 3
    m.ncock = ncock
    m.password = password

    suppress_sigint()
    packed_buffer = m.SerializeToString()
    requester.send(packed_buffer)
    recvm = proto_char_message()
    recvm.ParseFromString(requester.recv())

    if recvm.ncock == 0:
        print("Tamos cheios :/")
        return 0

    m.ch = recvm.ch
    allow_sigint()
    m.msg_type = 4

    n = 0
    while True:
        n += 1
        sleep_delay = random.randint(0, 699999)
        time.sleep(sleep_delay / 1000000)

        for i in range(ncock):
            move = random.randint(0, 1)
            if move == 1:
                direction = random.randint(0, proto_direction.RIGHT)
                moves = {
                    proto_direction.LEFT: "Left",
                    proto_direction.RIGHT: "Right",
                    proto_direction.DOWN: "Down",
                    proto_direction.UP: "Up",
                }
                m.cockdir[i] = direction
                print(f"{n} Going {moves[direction]}")
            else:
                m.cockdir[i] = 5
                print("A mimir")

        # send the movement message
        suppress_sigint()
        packed_buffer = m.SerializeToString()
        requester.send(packed_buffer)
        recvm.ParseFromString(requester.recv())
        allow_sigint()

if __name__ == "__main__":
    main()

# Eu odeio python imaginem ter uma liguagem lê whitespaces
