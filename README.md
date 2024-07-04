# SFML Template

## Building and Running

### Windows (Visual Studio)

The easiest way to build is to use [vcpkg](https://vcpkg.io/en/index.html) and install SFML through this.

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
