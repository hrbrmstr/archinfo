#  archinfo

Returns a list of running processes and the architecture they are running under.

Apple M1/Apple Silicon/arm64 macOS can run x86_64 programs via Rosetta and most M1 systems currently (~March 2021) very likely run a mix of x86_64 and arm64 processes.

Activity Monitor can show the architecture, but command line tools such as `ps` and `top` do not due to Apple hiding the details of the proper `sysctl()` incantations necessary to get this info.

Patrick Wardle reverse-engineered Activity Monitor — <https://www.patreon.com/posts/45121749> — and I slapped that hack into a bare-bones command line utility `archinfo`.

It returns columnar output or JSON (via `--json`) — that will work nicely with `jq` — of running processes and their respective architectures.

Build from source or grab from the releases.

```bash
$ archinfo
...
   5949  arm64 /System/Library/Frameworks/AudioToolbox.framework/AudioComponentRegistrar
   5923  arm64 /System/Library/CoreServices/LocationMenu.app/Contents/MacOS/LocationMenu
   5901 x86_64 /Library/Application Support/Adobe/Adobe Desktop Common/IPCBox/AdobeIPCBroker.app/Contents/MacOS/AdobeIPCBroker
   5873  arm64 /Applications/Utilities/Adobe Creative Cloud Experience/CCXProcess/CCXProcess.app/Contents/MacOS/../libs/Adobe_CCXProcess.node
   5863  arm64 /bin/sleep
   5861 x86_64 /Applications/Tailscale.app/Contents/PlugIns/IPNExtension.appex/Contents/MacOS/IPNExtension
   5855 x86_64 /Applications/Elgato Control Center.app/Contents/MacOS/Elgato Control Center
   5852 x86_64 /Applications/Tailscale.app/Contents/MacOS/Tailscale
   5849  arm64 /System/Library/CoreServices/TextInputSwitcher.app/Contents/MacOS/TextInputSwitcher
...
```

```bash
$ archinfo --pid $(pgrep keyboardservicesd)
  60298 x86_64 /usr/libexec/keyboardservicesd

$ archinfo --json --pid $(pgrep keyboardservicesd)
{"pid":60298,"arch":"x86_64","name":"/usr/libexec/keyboardservicesd"}
```

```
archinfo --json | jq -r --slurp 'group_by(.arch)[] | { arch: .[0].arch, count: length } | [ .arch, .count ] | @tsv'
## arm64   382
## x86_64  12
```
