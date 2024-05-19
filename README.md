# ps5-payload-websrv
This is a simple web server that can be executed on a Playstation 5 that has
been jailbroken via the [BD-J][bdj] or the [webkit][webkit] entry points.

## Quick-start
To deploy ps5-payload-websrv, first launch the [ps5-payload-elfldr][elfldr], then
load the payload and connect to the web server on port 8080:

```console
john@localhost:~$ export PS5_HOST=ps5
john@localhost:~$ wget -q -O - https://github.com/ps5-payload-dev/websrv/releases/download/v0.1/Payload.zip | gunzip -c -d | nc -q0 $PS5_HOST 9021
john@localhost:~ $wget -q -O - http://$PS5_HOST:8080/index.html
```

## Available services
TODO

Examples:
- http://ps5:8080/fs/ - Browser the filesystem (html)
- http://ps5:8080/fs/?fmt=json - Browser the filesystem (json)
- http://ps5:8080/fs/system_ex/app/NPXS40028/redis.conf - Download a file
- http://ps5:8080/launch?titleId=PPSA01325 Launch ASTROs Playroom
- http://ps5:8080/hbldr?path=/data/diasurgical/devilution/devilutionx - Launch /data/diasurgical/devilution/devilutionx
- http://ps5:8080/hbldr?path=/data/LakeSnes/lakesnes.elf&args=/data/LakeSnes/roms/punch.smc - Launch lakesnes.elf with the rom punch.smc

## Building
TODO

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
