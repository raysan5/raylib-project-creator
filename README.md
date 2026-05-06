# `rpc` - raylib project creator

![raylib project creator](screenshots/rpc_v200_shot01.png)

### [USE `rpc` TOOL ONLINE!](https://raysan5.itch.io/raylib-project-creator)

## What can I do with `rpc`?

Welcome to `rpc`! The new tool to help you setup a professional raylib project structure!

When starting with raylib is common to create a simple .c code file in some directory and jump directly into coding, the `raylib Windows Installer` package already provides a pre-configured environment to do it that way; the compiler, the Npp editor and raylib library are already pre-configured to do so.

But as soon as a project starts growing, usually more complex project structures are required. Usually a build system for multiple code files, external libraries, project assets/resources, configuration files, executable icons and even some automatized CI/CD system. Configuring all those aspects usually requires a lot of time.

`rpc` is a visual tool to simplify that process and automatically create and pre-configure some common build systems for a project, just providing an input source code file to start with.

_**DISCLAIMER: Note that a project could be really complex in terms of source files, external dependencies, resources and build configurations. `rpc` does not pretend to support all possible options but the minimum ones to quickly generate a working project structure to be directly uploaded to GitHub and start working on a bigger project.**_

## Features

 - Support `.rpc` **project configuration** interchange file format
    - Shared with homonymous tool: `rpb` - raylib project builder
 - Select startup project from some **game/tool templates**
 - Select **custom C code files** to generate project
 - Select **project assets** to be included in the project
 - Code scanning for **assets usage validation**
 - Generate **build systems**: scripts, Makefile, VSCode, VS2022
 - Generate complete GitHub project, ready to upload
 - Generate preconfigured **GitHub Actions**, ready to run
 - WEB: Download generated template as a .zip file
 - Command-line support for **automated project generation**
 - **Completely portable (no-dependencies)**

## Basic Usage

Open the tool, drag & drop your C code files and Asset files. Configure project properties, like repo name, project internal name, company and more. Selct desired build systems to be generated and press Generate Project Structure button.

`rpc` dektop version comes with command-line support for automated project(s) generation. You can check all available options with the following command:

 > rpc.exe --help

### Build Systems

`rpc` generates the following project build systems:

 - **Makefile** [`src/Makefile`]: The default and recommended raylib build system, preconfigured for multiple platforms (Windows, Linux, FreeBSD, macOS, WebAssembly).
 - **Scripts** [`projects/scripts/build.bat`]: Command-line build scripts, for Windows `CMD` (`build.bat`)
    WARNING: Script generated is currently Windows only (`build.bat`) and it calls the `src/Makefile` build system. A possible improvement for the future is calling directly the required process. Also, supporting some other shell would be nice.
 - **VS2022** [`projects/VS2022/*`]: Visual Studio 2022 complete solution (.sln). The generated solution contains raylib sources to be build along the project, in case it needs to be debugged or customized for the project needs. It also includes multiple build configurations and preconfigured output paths for better organization. All build happens to `projects/VS2022/build` directory.
 - **VSCode** [`projects/VSCode/*`]: Visual Studio Code preconfigured tasks and settings for the project._-CHECK LIMITATIONS BELOW-_
    WARNING: VSCode project requires the compiler and tools available in the system path; it also calls the `src/Makefile` build system. It would be nice to make it more self-contained.
 - **GitHub Actions** [`.github/workflows/*`]: GitHub Actions workflows preconfigured to compile the project on multiple platforms (Windows, Linux, macOS, WebAssembly). Windows workflow use `projects/VS2022` for building while Linux/macOS/WebAssembly use `src/Makefile` for project building. Note that those workflows are setup to automatically run and upload generated files to on new GitHub project `Release` creation. They can also be run manually.
 
## Generated project structure

The generated structure follows the template structure and contrains the following elements:

```
project-repo-name/
├── src/                // Project source code files
│   ├── resources       // Project assets
│   ├── project_name.c
│   └── Makefile        // Project multi-platform Makefile building (default)
├── raylib/             // [OPTIONAL] raylib library sources
├── projects/           // Project build systems, preconfigured
│   ├── VS2022/         // Build system: Visual Studio 2022
│   ├── VSCode/         // Build system: VSCode
│   └── scripts/        // Build system: scripts (.bat, .sh)
├── imagery/            // Project imagery: icons, banners, logo, splash...
├── .github/
│   └── workflows/      // GitHub Actions: Windows, Linux, macOS
├── .gitignore          // Project files to ignore, preconfigured for build systems
├── README.md           // Project main documentation (from template)
├── LICENSE             // Project source code license (MIT by default)
├── EULA.txt            // [OPTIONAL] Project End-User-Agreement
└── project_name.rpc    // Project configuration file, useful for [rpb] tool
```

## Project Configuration file

`rpc` supports loading, saving (and generation) of project configuration files: `.rpc`.

`.rpc` configuration files are shared betten `rpc` and `rpb` tools. `rpc` generates a base configuration file on project generation with provided properties and `rpb` can use that config file information to build the project for multiple platforms.

`.rpc` is a text and open file format, following a `.ini` style, and can be freely edited with any text editor.

`rpc` project generation uses a [pre-defined `.rpc` file as a base template](src/template/project_name.rpc) to further update required values. 

## Keyboard/Mouse Shortcuts

 - `F1` - Show Help window
 - `F2` - Show About window
 - `F3` - Show Sponsor window

**File Options**
 - `LCTRL + N` - New project file (.rpc)
 - `LCTRL + O` - Open project file (.rpc)
 - `LCTRL + S` - Save project file (.rpc)
 - `LCTRL + A` - Add source/asset files to project
 - `LCTRL + LSHIFT + A` - Add source/asset directory
 - `LCTRL + E` - Export project

## Command-line

```
USAGE:
    > rpc [--help] --i <src_dir>,<assets_dir> --rpc <project_config.rpc>
          [--pn <project_name>] [--rn <repo-name>]
          [--cn <commercial_name>] [--pv <version>]
          [--desc <project_description>] [--dev <developer_name>]
          [--devurl <developer_webpage>] [--devmail <developer_email>]
          [--out <output_path>]

OPTIONS:
    -h, --help                          : Show tool version and command line usage help

    -i, --input <path_dir>,<file01.xxx>,<file02.xxx>
                                        : Define inputs, directory or files(s), comma separated
    -rpc <config_file.rpc>              : Define raylib project configuration file
    -pn, --project-name <project_name>  : Define project internal name
    -rn, --repo-name <repository_name>  : Define project repository name
    -cn, --commercial-name <commercial_name>  : Define project commercial name
    -pv, --project-version <version>    : Define project version
    --desc <project_description>        : Define project description, use "Project Description"
    --dev <developer_name>              : Define developer name
    --devurl <developer_webpage>        : Define developer webpage
    --devmail <developer_email>         : Define developer email
    -o, --out <output_path>             : Define output path for project generation

EXAMPLES:
    > rpc -i src_dir -rpc my_project_config.rpc -pn cool_game -rn cool-game-repo -cn "Cool Game" -pv 1.0
        Generates project <cool_game> in output directory <cool-game-repo>
```

## Technologies

This tool has been created using the following open-source technologies:

 - [raylib](https://github.com/raysan5/raylib) - A simple and easy-to-use library to enjoy videogames programming
 - [raygui](https://github.com/raysan5/raygui) - A simple and easy-to-use immediate-mode-gui library
 - [rini](https://github.com/raysan5/rini) - A simple and easy-to-use config init files reader and writer
 - [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/) - File dialogs for desktop platforms

## Handmade Software

`rpc` is handmade software, it has been carefully developed using the C programming language, with great attention put on each line of code written.
This approach usually results in highly optimized and efficient code, but it is also more time-consuming and require a higher level of technical skills.
The result is great performance and small memory footprint.

`rpc` is self-contained in a portable package of about **3 MB**.

## Issues & Feedback

Issues and feedback can be reported at https://github.com/raysan5/raylib-project-creator.

For additional support, **priority issues review or tool customization** requests, please contact `ray[at]raylib.com`

## License

This project sources are licensed under an unmodified zlib/libpng license, which is an OSI-certified, BSD-like license that allows static linking with closed source software. Check [LICENSE](LICENSE) for further details.

*Copyright (c) 2024-2026 Ramon Santamaria ([@raysan5](https://github.com/raysan5))*
