# ps5-payload-websrv
This is a simple web server that can be executed on a Playstation 5 that has
been jailbroken via the [BD-J][bdj] or the [webkit][webkit] entry points.
It can be used to launch homebrew via the Webkit browser embedded with the PS5,
or remotely from your PC, phone, etc.

## Quick-start
To deploy ps5-payload-websrv, first launch the [ps5-payload-elfldr][elfldr],
then load the payload and connect to the web server on port 8080:

```console
john@localhost:~$ export PS5_HOST=ps5
john@localhost:~$ wget -q -O - https://github.com/ps5-payload-dev/websrv/releases/download/v0.1/Payload.zip | gunzip -c -d | nc -q0 $PS5_HOST 9021
```

## Available services
Examples:
- http://ps5:8080/launch.html - Launch PKGs
- http://ps5:8080/homebrew.html - Launch Homebrew
- http://ps5:8080/fs/ - Browser the filesystem (html)
- http://ps5:8080/fs/?fmt=json - Browser the filesystem (json)
- http://ps5:8080/fs/system_ex/app/NPXS40028/redis.conf - Download a file

Installing Homebrew
The web server will search for homebrew in /data/homebrew, and makes a couple
of assumtions on the filestructure. More specifically, suppose you have a
homebrew called MyHomebrew, the loader assumes that the following files exist:
- /data/homebrew/MyHomebrew/eboot.elf - Payload to run, compiled with [ps5-payload-sdk][sdk].
- /data/homebrew/MyHomebrew/sce_sys/icon0.png - An icon to be rendered at /homebrew.html.

You can also add your own html page that is activated when the icon is clicked
instead of loading eboot.elf:
- /data/homebrew/MyHomebrew/sce_sys/index.html

This is useful when the payload accepts different command line options, e.g.,
an emulator that expects arguments for loading roms. For an example, see:
- https://github.com/ps5-payload-dev/websrv/tree/master/homebrew/LakeSnes.7z

## Building
Assuming you have the [ps5-payload-sdk][sdk] installed on a Debian-flavored
operating system, the payload can be compiled using the following commands:
```console
john@localhost:ps5-payload-dev/websrv$ export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
john@localhost:ps5-payload-dev/websrv$ ./libmicrohttpd.sh # build and install libmicrohttpd to $PS5_PAYLOAD_SDK
john@localhost:ps5-payload-dev/websrv$ make
```

## Reporting Bugs
If you encounter problems with ps5-payload-websrv, please [file a github issue][issues].
If you plan on sending pull requests which affect more than a few lines of code,
please file an issue before you start to work on you changes. This will allow us
to discuss the solution properly before you commit time and effort.

## License
ps5-payload-websrv is licensed under the GPLv3+.

[bdj]: https://github.com/john-tornblom/bdj-sdk
[sdk]: https://github.com/ps5-payload-dev/sdk
[webkit]: https://github.com/Cryptogenic/PS5-IPV6-Kernel-Exploit
[issues]: https://github.com/ps5-payload-dev/websrv/issues/new
[elfldr]: https://github.com/ps5-payload-dev/elfldr
