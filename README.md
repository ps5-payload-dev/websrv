# ps5-payload-websrv
This is a simple web server that can be executed on a Playstation 5 that has
been jailbroken via the [BD-J][bdj] or the [webkit][webkit] entry points.

## Quick-start
TODO

## Available services
TODO

Examples:
- http://ps5:8080/fs/ - Browser the filesystem (html)
- http://ps5:8080/fs/?fmt=json - Browser the filesystem (json)
- http://ps5:8080/fs/system_ex/app/NPXS40028/redis.conf - Download a file
- http://ps5:8080/launch?titleId=PPSA01325 Launch ASTROs Playroom
- http://ps5:8080/hbldr?path=/data/diasurgical/devilution/devilutionx - Launch /data/diasurgical/devilution/devilutionx

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