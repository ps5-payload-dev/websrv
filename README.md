# ps5-payload-websrv
This is a simple web server that can be executed on a Playstation 5 that has
been jailbroken via the [BD-J][bdj] or the [webkit][webkit] entry points.
It can be used to launch homebrew via the Webkit browser embedded with the PS5,
or remotely from your PC, phone, etc.

## Quick-start
To deploy ps5-payload-websrv, first make sure [ps5-payload-elfldr][elfldr] is
running, then load the payload as follows:

```console
john@localhost:~$ export PS5_HOST=ps5
john@localhost:~$ wget -q -O - https://github.com/ps5-payload-dev/websrv/releases/latest/download/Payload.zip | gunzip -c -d | nc -q0 $PS5_HOST 9021
```

To interact with ps5-payload-websrv, either install the [Launcher PKG][launcher]
on your PS5, or point your browser to one of the available services exemplified
below.

## Available services
Examples:
- http://ps5:8080/index.html - Launch Homebrew
- http://ps5:8080/elfldr - Launch ELF Payloads
- http://ps5:8080/fs/ - Browser the local filesystem (html)
- http://ps5:8080/fs/?fmt=json - Browser the local filesystem (json)
- http://ps5:8080/fs/system_ex/app/NPXS40028/redis.conf - Download a local file
- http://ps5:8080/mdns - List mDNS services discovered by websrv (json)
- http://ps5:8080/smb?addr=192.168.1.1 - List shares on a remote SMB host (json)
- http://ps5:8080/smb/share?addr=192.168.1.1 - List files and folders shared by a remote SMB host (json)
- http://ps5:8080/smb/share/file?addr=192.168.1.1 - Download a remote SMB file via websrv

## Installing Homebrew
The web server will search for homebrew in /data/homebrew, /mnt/usb%d/homebrew, /mnt/ext%d/homebrew,
and makes a couple of assumtions on the filestructure. More specifically, suppose you have a
homebrew called MyHomebrew, the loader assumes that the following files exist:
- /data/homebrew/MyHomebrew/eboot.elf - Payload to run, compiled with [ps5-payload-sdk][sdk].
- /data/homebrew/MyHomebrew/sce_sys/icon0.png - An icon to be rendered at /index.html.

You can also add your own custom UI extensions by specifying a javascript file named:
- /data/homebrew/MyHomebrew/homebrew.js

This is useful when the payload accepts different command line options, e.g.,
an emulator that expects arguments for loading roms. For an example on
available capabillitiles, see:
- https://github.com/ps5-payload-dev/websrv/tree/master/homebrew/demo

For real-world homebrew, checkout:
- https://github.com/ps5-payload-dev/websrv/releases
- https://github.com/cy33hc/ps5-ezremote-client

## Building
Assuming you have the [packbrew][packbrew] SDK installed on a Debian-flavored
operating system, the payload can be compiled using the following commands:
```console
john@localhost:ps5-payload-dev/websrv$ export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
john@localhost:ps5-payload-dev/websrv$ make
```

## Known Issues
- Homebrew sometimes crashes when there is already a previous homebrew running.
- The kernel can panic if you go into restmode and back again while a homebrew is running.

## Reporting Bugs
If you encounter problems with ps5-payload-websrv, please [file a github issue][issues].
If you plan on sending pull requests which affect more than a few lines of code,
please file an issue before you start to work on you changes. This will allow us
to discuss the solution properly before you commit time and effort.

## License
ps5-payload-websrv uses [xterm.js][xterm.js] in its web interface, which is licenced 
under the MIT licence. 
Unless otherwhise explicitly specified inside induvidual files, The rest is the source 
code included with ps5-payload-websrv are licensed under the GPLv3+.


[bdj]: https://github.com/john-tornblom/bdj-sdk
[sdk]: https://github.com/ps5-payload-dev/sdk
[packbrew]: https://github.com/ps5-payload-dev/pacbrew-repo
[webkit]: https://github.com/Cryptogenic/PS5-IPV6-Kernel-Exploit
[issues]: https://github.com/ps5-payload-dev/websrv/issues/new
[elfldr]: https://github.com/ps5-payload-dev/elfldr
[xterm.js]: https://github.com/xtermjs/xterm.js
[launcher]: https://github.com/ps5-payload-dev/websrv/raw/refs/heads/master/homebrew/IV9999-FAKE00000_00-HOMEBREWLOADER01.pkg
