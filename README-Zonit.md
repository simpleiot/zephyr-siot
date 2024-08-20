# Zonit Zephyr Projects

This is a Zephyr Workspace
[T2 topology](https://docs.zephyrproject.org/latest/develop/west/workspaces.html#t2-star-topology-application-is-the-manifest-repository)
that sets up to build the Zonit Zephyr applications.

## Setup

- `mkdir zephyr-zonit`
- `cd zephyr-work`
- `west init -m git@gitea.zonit.com:Zonit-Dev/zephyr-zonit.git`
- `west update`
- `cd zonit`
- `. envsetup.sh` (notice leading `.`)
- `siot_setup`

## Build

see main [README.md](/README.md)
