#  archinfo

Returns a list of running processes and the architecture they are running under.

Apple M1/Apple Silicon/arm64 macOS can run x86_64 programs via Rosetta and most M1 systems currently (~March 2021) very likely run a mix of x86_64 and arm64 processes.

Activity Monitor can show the architecture, but command line tools such as `ps` and `top` do not due to Apple hiding the details of the proper `sysctl()` incantations necessary to get this info.

Patrick Wardle reverse engineered Activity Monitor— <https://www.patreon.com/posts/45121749> — and I slapped that hack together with some code from Sydney San Martin — https://gist.github.com/s4y/1173880/9ea0ed9b8a55c23f10ecb67ce288e09f08d9d1e5 — into a nascent, bare bones command line utility `archinfo`.

It will do more, soon, but for now it just returns a big JSON blog (that will work fine with `jq`) of running processes and their respective architectures.

Build from source or grab from the releases.

```
Rscript -e 'table(jsonlite::fromJSON(system("/usr/local/bin/archinfo", intern=TRUE))$arch)'
##
## arm64 x86_64
##   419     29
```
