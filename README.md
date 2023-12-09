# lizardsNroaches

## Introduction

First-phase Systems Programming course project.

Simple game that uses ZMQ sockets to communicate between server and clients using REQ/REP and PUB/SUB patterns, and the ncurses library for the API.

## The Game

In the lizardsNroaches game, users control a lizard that moves in a field and eats
cockroaches. In those fields, cockroaches (represented by a digit) move randomly.
Each lizard score varies as follows:

- When a lizard rams another one on the head, the score of both lizards is equalized to the
average of their scores.

- If a lizard eats a cockroach, its score increases by the value of the ingested
cockroach

Although lizards have a long body, the previous interactions only affect the head of the
lizard

A lizard wins the game when it reachs 50 points, after that he gets to wear a beautiful * mantle for the rest of the game.

After being eaten a cockroach respawns after a 5 second interval.

## Extra-features

- Simple message authentication to avoid malicious or discard incorrect packages
    - ZMQ packages are not encrypted by itself so this game is susceptible to MIM attacks
        - A possible fix for the future is to use OpenSSL to encrypt the messages :^))

## Compile Instructions

To compile all game binaries simply run:

`make`

Before re-compiling the game binaries you must:

`make clean`

The game binaries can be found in the approprietly named `./bin` directory, after compiling, which will have the following tree:

`bin
├── clients
│   ├── cockroach
│   │   └── roaches-client
│   └── lizard
│       └── lizard-client
├── remote-display
│   └── display-app
└── server
    └── server`

## Run Instructions

### Server

`./server my_ip my_REQ/REP-port my_PUB/SUB-port`

### Lizard

`./lizard-client server_ip server_REQ/REP-port`

### Roach 

`./roaches-client server_ip server_REQ/REP-port`

### Remote Display app

`./display-app server_ip server_REQ/REP-port server_PUB/SUB-port`

## Authors

All code and documentations are authored by:

- Diogo Lee Leitão nº99917, MEEC-IST
- João Barreiros C. Rodrigues (Ex-Machina) nº99968, MEEC-IST

# Acknowledgments

The code is based on the sources made available by the Systems Programming Professors:

- João Nuno De Oliveira e Silva (jnos), INESC-ID
- Alexandra Sofia Martins de Carvalho, IT-Lx
