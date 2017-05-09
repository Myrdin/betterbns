# betterbns [![Build status](https://ci.appveyor.com/api/projects/status/nyym43amonl87f3c?svg=true)](https://ci.appveyor.com/project/zeffy/betterbns)

### [Click here to get the latest build!][1]

---

This project is based off of [my steam_api.dll fork](https://github.com/zeffy/bnsboost-steam_api.dll) of [BNSBoost]. The entire codebase has been revamped, and it has almost no common code with BNSBoost at this point. However, without BNSBoost it is likely I would have never made betterbns, so many thanks to [@Xyene](https://github.com/Xyene).

While this project is called `betterbns`, nothing in the code relies on it being used with Blade & Soul, and could technically be used with other game launchers, as long as they import `version.dll`, use `CreateFileW`, `CreateProcessW`, and are vulnerable to IAT manipulation.

Not endorsed by NCSoft in any way, shape, or form; Blade & Soul, etc. are all trademarks of NCSoft. 

**Using this is almost certainly against the code of conduct, terms of use, and EULA for any online game.**

## Features

* Lets you bypass file verification, so you can use modified `.upks`, `config.dat`, `xml.dat`, or whatever else
* Lets you specify custom command line arguments to start the main game with
* Can immediately close the launcher after the main game starts

There is no editor included for modifying your Blade & Soul game files, use [BNSBoost] or [BnsDatTool] for that.

## Installation

These instructions are specifically for Blade & Soul, but the general idea should be the same for other games, too.

- [Grab the latest build from AppVeyor][1]
- Unzip my `version.dll` and `init.ini` into your NCLauncher directory
- That's all!

Your anti-virus might complain about `version.dll`, as it hooks some system functions (see below). If this happens you'll need to whitelist it before proceeding. If you're jumpy about security (understably so; using binaries from some guy on the internet is not smart), you can always recompile it yourself from source.

## Usage

### Using modified game files

Before you modify any file, navigate to its directory first, and create a subdirectory called `_original`, then make a copy the file(s) you wish to modify in `_original`. Then, you can use whatever game file editor you wish to edit the files (not the ones in `_original`!) however you wish. `betterbns` will make the game *launcher* read from the `_original` folder, while the game *client* will read from the regular folder.

This tricks the launcher's file integrity checking into thinking that your game files are unmodified, and allow the game to start without attempting to repair them.

### How to set up `init.ini` for custom command line arguments

The release zip contains a pre-made `init.ini` for Blade & Soul, but can be made to work with other games as well. `init.ini` just has to be in the working directory of whatever process loads `version.dll`.

The `init.ini` file is processed top-down, and first checks for the full path of the executable being started, and then just the process name. So in the example below, the second entry would take precedence over the first one because it uses a full path name.

Example `init.ini` for Blade & Soul:

```ini
; this applies to both 32- and 64-bit
[Client.exe]
ExitParentProcess=1 ; change this to 0 if you want NC Launcher to stay open after the game starts

; this one applies only to 64-bit, and will take precedence over the first
; entry because it uses a fully qualified path name
[C:\Program Files (x86)\NCSOFT\BnS\bin64\Client.exe]
ExtraArgs=-NOTEXTURESTREAMING -USEALLAVAILABLECORES -UNATTENDED
ExitParentProcess=1
```

### Blade & Soul specific

#### Enabling the DPS meter

[See BnS Academy's tutorial for this][2].

Create a folder in the `...\BnS\contents\Local\NCWEST\data` directory called `_original`, and copy the `*.dat` files into it, then use your tool of choice to edit the files as you normally would. **Do not modify files in the `_original` directory, or they will get repaired by the launcher!**

#### How can I suppress the "Ambiguous package name: Using '... .upk'" error message?

You can use the `-UNATTENDED` flag to suppress the message box errors. However, depending on where your mods are located in the load order hierarchy, there could still be issues such as the game loading the `.upk` files in the `_original` folder instead of your modded ones. Also having a lot of extra `.upk` files can make loading take much longer.

To solve either of these issues, go to the offending `_original` folder and rename all the `*.upk` files to `*.upk.ignore`. You can do this with `cmd` by running `ren *.upk *.upk.ignore` (you can revert this by running `ren *.ignore *.`). The `CreateFileW` hook will still detect them, but because they no longer have the `.upk` extension, Blade & Soul won't try to load them. This works with any other file type as well.

## How it works

The code itself is pretty short and easy to follow, but in general:

* My `version.dll` gets loaded by the launcher automatically, at which point it modifies the launcher's [IAT][3]
  * Hooks calls to a few Windows APIs (see the source)
  * Relevant calls to `CreateFileW` are redirected to the `_original` directory
  * `CreateProcessW` hook lets you add custom command line args to a started process (see above)
  * `LoadLibraryW` hook applies these hooks to any libraries loaded after `version.dll`

The code never touches the main game client, only its launcher.

## Reporting an issue

You may report an issue, but I give no garuntee of providing support, as I made this for my own use and work on it in my free time.

## Compilation

You can compile the solution with the newest version of Visual Studio.

[1]: https://ci.appveyor.com/project/betterbns/build/artifacts
[2]: https://www.bns.academy/english/damage-meter/
[3]: https://en.wikipedia.org/wiki/Portable_Executable#Import_Table
[BNSBoost]: https://github.com/Xyene/BNSBoost
[BnsDatTool]: http://www.bladeandsouldojo.com/forums/topic/184834-dat-files-packerunpacker/