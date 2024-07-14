# ENet Server/Client Game Example

Example of a simple client/server "game" using SFML for graphics and ENnet for networking.

Resources used to create this:

- [Fast-Paced Multiplayer (Gabriel Gambetta)](https://www.gabrielgambetta.com/client-server-game-architecture.html)

- [Gaffer on Games (Glenn Fiedler)](https://gafferongames.com/post/snapshot_interpolation/)

- [Server In-game Protocol Design and Optimization (Valve)](https://developer.valvesoftware.com/wiki/Latency_Compensating_Methods_in_Client/Server_In-game_Protocol_Design_and_Optimization)

- [Source Multiplayer Networking](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking)

## Building and Running

### Windows (Visual Studio)

The easiest way to build is to use [vcpkg](https://vcpkg.io/en/index.html) and install the dependencies through this.

```bash
vcpkg install sfml
vcpkg install imgui
vcpkg integrate install
```

Then open the project in Visual Studio and build.

### Linux

Install conan

```sh
python3 -m pip install conan==1.57
```

To build, at the root of the project:

```sh
sh scripts/build.sh install
```

The `install` argument is only needed for the first time compilation as this is what grabs the libraries from Conan

To run, at the root of the project:

```sh
sh scripts/run.sh
```

To build and run in release mode, simply add the `release` suffix:

```sh
sh scripts/build.sh release
sh scripts/run.sh release
```
