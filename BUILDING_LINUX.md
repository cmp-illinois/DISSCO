# Building DISSCO on Linux: quick commands

**Only want to use DISSCO?** Download the released AppImage from
[DOWNLOAD.md](DOWNLOAD.md#linux) instead.

This page lists only the commands. The
[detailed Linux guide](BUILDING_LINUX_DETAILED.md) explains each step, covers
Ubuntu 22.04 and 24.04, and lists fixes for common errors.

Run the commands in a Terminal window. To paste into Terminal, use
**Ctrl+Shift+V** (plain Ctrl+V does not work there).

## Which Linux do you have?

```sh
cat /etc/os-release
```

| If it shows | Then |
| --- | --- |
| Debian 13 (`trixie`) or LMDE 7, or Ubuntu 26.04 or newer | Follow the steps below. |
| Ubuntu 22.04 or 24.04, Linux Mint 21 or 22, or Pop!_OS | The Qt that comes with these systems is too old for DISSCO, which needs Qt 6.8. Follow the [Ubuntu 22.04 and 24.04 steps](BUILDING_LINUX_DETAILED.md#ubuntu-2204-and-2404-download-qt) instead. |
| Something else | See [Other Linux systems](BUILDING_LINUX_DETAILED.md#other-linux-systems). |

## 1. Install the packages (once)

On a lab or shared computer, the administrator does this step. To check
whether it has been done, run:

```sh
dpkg -s git build-essential cmake qt6-base-dev libsndfile1-dev > /dev/null && echo ready
```

If it prints `ready`, go to step 2. Otherwise, run these two lines on your own
computer, or ask the administrator to run them on a lab computer:

```sh
sudo apt update
sudo apt install -y git build-essential cmake qt6-base-dev libsndfile1-dev lilypond
```

`sudo` asks for your password; nothing appears while you type it. On your own
Debian computer, if it says you are `not in the sudoers file` or
`sudo: command not found`, run this line instead and type the root password you
chose when installing Debian:

```sh
su -l -c 'apt update && apt install -y git build-essential cmake qt6-base-dev libsndfile1-dev lilypond'
```

`lilypond` is only needed for score and PDF output. None of the steps below
need `sudo`.

## 2. Download DISSCO

```sh
cd
git clone https://github.com/cmp-illinois/DISSCO.git
cd DISSCO
```

If Git says `destination path 'DISSCO' already exists`, you downloaded DISSCO
before. Run `cd ~/DISSCO` and `git pull --ff-only` instead, then continue with
step 3.

## 3. Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
```

The build takes several minutes. It has worked when one of the last lines says
`[100%] Built target`.

## 4. Open LASSIE

```sh
./build/LASSIE/lassie
```

Next time, start it with `~/DISSCO/build/LASSIE/lassie`. LASSIE runs the `cmod`
engine from the `build` folder, so do not delete that folder or move the
`DISSCO` folder. If you did, run `rm -rf build` in the `DISSCO` folder and
repeat step 3.

## Update to the latest version

Close LASSIE, run these two commands, then repeat step 3:

```sh
cd ~/DISSCO
git pull --ff-only
```

If you built DISSCO with the Ubuntu 22.04 and 24.04 steps, follow
[Updating after new fixes](BUILDING_LINUX_DETAILED.md#updating-after-new-fixes)
instead.

## Something went wrong?

Look up the error message in the detailed guide's
[Troubleshooting](BUILDING_LINUX_DETAILED.md#troubleshooting) section.
