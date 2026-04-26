# SPL fix kpm
This kpm override ro.vendor.build.security_patch (vendor SPL) to fix Play Integirity on locked Lenovo Y700 gen4 (TB322FC).

On March 2026, the users of Lenovo Y700 gen4 lost strong integrity even with locked bootloader. It was caused by older security patch level (SPL) on the stock ROM. Google demands an SPL within the past 12 months for strong integrity. The SPL consists of three values: boot, vendor and system. All of them must be within past a year. Lenovo only updates system SPL, so Y700 gen4 has lost strong integrity because it has passed roughly a year since the release of the device.

This kpatch module spoofs vendor SPL to the latest value to pass strong integrity. It replaces vendor SPL with "2026-04-05".

As of April 2026, the SPL values of latest version are the followings:

- Device: TB322FC_CN
- Version: ZUXOS_1.5.10.117_260212_PRC
- Vendor SPL: 2025-02-05
- Boot SPL: 2025-02-05
- System SPL: 2026-01-05

This kpm doesn't fix boot SPL. You need to replace prop in VBMeta structure of boot partition to fix boot SPL. Check for --boot-spl option of [testkey-signer](https://github.com/polygraphene/testkey-signer).

This kpm was developed for users having root access with locked bootloader using Lenovo testkey vulnerability.

# Why kpm instead of magisk module?
The spoofing must happen before the launch of keymint service. The initialization of magisk module is too late because the keymint service launches before decryption of /data partition. The kpm can trigger spoofing before it.

Boot sequence: `Boot kernel` -> `Initialize kpms` -> `Start init` -> `Load SPL from /vendor/build.prop` -> `Start keymint` -> `Decrypt /data` -> ...

# How to install?
For APatch users:
Just embed this kpm on your kernel.

For KernelSU (and forks) or Magisk user:
Use [KPatch-Next-Module](https://github.com/KernelSU-Next/KPatch-Next-Module).

# Note
The decryption key of `/data` is bound to SPL. If you boot the device with newer (spoofed) SPL once, you cannot decrypt `/data` with older SPL and you must wipe data from recovery. Be careful of uninstalling this module.

# License
GPL-2.0

# Build
Clone [KernelPatch](https://github.com/bmax121/KernelPatch/) and this repo in the same directory and run `make ANDROID_NDK=(ndk dir)`.
```
git clone https://github.com/bmax121/KernelPatch/
git clone (this repo)
cd (this repo)
make ANDROID_NDK=(ndk dir)

```

# Acknowledgements
This project is a fork of hosts_redirect.
- [hosts_redirect](https://github.com/lzghzr/APatch_kpm/blob/main/hosts_redirect/)
- [hosts_file_redirect](https://github.com/AndroidPatch/kpm/tree/main/src/hosts_file_redirect)
- [KernelPatch](https://github.com/bmax121/KernelPatch/)
