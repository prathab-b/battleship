# Battleship

## Description

A classic server-based, multiplayer battleship game implemented in C for Linux. The game runs as a TCP server and allows multiple players to connect and play against each other in real time using a text-based protocol. Each player registers with a name and ship placement, then guesses coordinates to hit opponent ships. All game events are broadcast to connected players.

## Tech Stack

**Language:** C <br>
**Platform:** Linux <br>
**Networking:** TCP Sockets

## Features
- Server-based multiplayer gameplay
- Multiple players can connect simultaneously
- Ship placement validation
- Hit / miss detection
- Automatic player elimination when a ship is destroyed
- Real-time broadcast of game events to all players

## How To Run

To create the battleship server, run the command:

`gcc battleship.c -o battleship`

Followed by:

`./battleship <port>` <br>

Example: `./battleship 8080` <br>
Note: If a port does not work, another port can be used.

## Connecting Players (Terminal-Based)

This project does not include a graphical client, so players can connect using terminal tools (e.g. netcat), as follows:

`nc localhost <port>` <br>

Example: `nc localhost 8080` <br>
Note: Each terminal represents one player.

## Client Protocol

All commands are terminated by a newline character (the enter key suffices).

**Register Players**

To register a player and their ship, run the command:

`REG <name> <x> <y> <orientation>`

Example: `REG Alice 5 5 -`

**Bombing Coordinates**

To bomb a coordinate, a player can run the command:

`BOMB <x> <y>`

Example: `BOMB 3 7`

**Server Responses**

The server will respond in response to the given command with one of the following:

- `WELCOME` - successful registration
- `JOIN <player>` - a new player has joined
- `HIT <attacker> <x> <y> <defender>` - a successful hit
- `MISS <attacker> <x> <y>` - a missed attack
- `GG <player>` - a player’s ship has been destroyed and they are eliminated
- `INVALID` - invalid command or parameters
- `TAKEN` - username already in use

## Future Improvements

Some potential future improvements for the game include:

- Implementing a dedicated client application
- Adding support for multiple ships per player
- Adding a turn-based enforcement system
- Support game lobbies or rounds

## License

This project is licensed under the MIT License.
