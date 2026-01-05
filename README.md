# Battleship

## Description

A classic server-based, multiplayer battleship game implemented in C for Linux.
The game runs as a TCP server and allows multiple players to connect and play against each other in real time using a text-based protocol.
Each player registers with a name and ship placement, then takes turns bombing coordinates to hit opponent ships. All game events are broadcast to connected players.

## Tech Stack

**Language:** C
**Platform:** Linux
**Networking:** TCP Sockets

## Features
- Server-based multiplayer gameplay
- Multiple players can connect simultaneously
- Ship placement validation
- Hit / miss detection
- Automatic player elimination when a ship is destroyed
- Real-time broadcast of game events to all players

## How To Run

gcc battleship.c -o battleship

./server <port>
Example: ./server 8080
Note: If a port does not work, another port can be used.

## Connecting A Player (Terminal-Based)

This project does not include a graphical client, so players can connect using terminal tools (e.g. netcat), as follows:

nc localhost <port>
Example: nc localhost 8080
