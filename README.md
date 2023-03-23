# Opentok C to Python OpenCV Inter-process Communication via Named pipe
A sample where frames from an Opentok C process is sent to a Python OpenCV process via a named Pipe (FIFO)

You will need a valid [Vonage Video API](https://tokbox.com/developer/)
account to build this app. (Note that OpenTok is now the Vonage Video API.)

### Solution Diagram
![Solution Diagram](https://github.com/nexmo-se/opentok_c_to_py/blob/main/OpentokCtoPy.png)

## Setting up your environment

### OpenTok SDK

Building this sample application requires having a local installation of the
OpenTok Linux SDK.

#### On Debian-based Linuxes

The OpenTok Linux SDK for x86_64 is available as a Debian
package. For Debian we support Debian 9 (Strech) and 10 (Buster). We maintain
our own Debian repository on packagecloud. For Debian 10, follow these steps
to install the packages from our repository.

* Add packagecloud repository:

```bash
curl -s https://packagecloud.io/install/repositories/tokbox/debian/script.deb.sh | sudo bash
```

* Install the OpenTok Linux SDK packages.

```bash
sudo apt install libopentok-dev
```

#### On non-Debian-based Linuxes

Download the OpenTok SDK from [https://tokbox.com/developer/sdks/linux/] (https://tokbox.com/developer/sdks/linux/)
and extract it and set the `LIBOPENTOK_PATH` environment variable to point to the path where you extracted the SDK.
For example:

```bash
wget https://tokbox.com/downloads/libopentok_linux_llvm_x86_64-2.24.3
tar xvf libopentok_linux_llvm_x86_64-2.24.3
export LIBOPENTOK_PATH=<path_to_SDK>
```

## Other dependencies

Before building the sample application you will need to install the following dependencies

### On Debian-based Linuxes

```bash
sudo apt install build-essential cmake clang libc++-dev libc++abi-dev \
    pkg-config libasound2 libpulse-dev libsdl2-dev
```

### On Fedora

```bash
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install SDL2-devel clang pkg-config libcxx-devel libcxxabi-devel cmake
```

## Building and running the C app

Once you have installed the dependencies, you can build the sample application.
Since it's good practice to create a build folder, let's go ahead and create it
in the project directory:


Copy the [config-sample.h](onfig-sample.h) file as `config.h` at
`opentok_c_to_py/`:

```bash
$ cp config-sample.h config.h
```

Edit the `config.h` file and add your OpenTok API key,
an OpenTok session ID, and token for the floor and translator sessions. For test purposes,
you can obtain a session ID and token from the project page in your
[Vonage Video API](https://tokbox.com/developer/) account. However,
in a production application, you will need to dynamically obtain the session
ID and token from a web service that uses one of
the [Vonage Video API server SDKs](https://tokbox.com/developer/sdks/server/).

Next, create the building bits using `cmake`:

```bash
$ cd build
$ CC=clang CXX=clang++ cmake ..
```

Note we are using `clang/clang++` compilers.

Use `make` to build the code:

```bash
$ make
```

When the `opentok-c-to-fifo` binary is built, run it:

```bash
$ ./opentok-c-to-fifo
```

You can use the [OpenTok Playground](https://tokbox.com/developer/tools/playground/)
to connect to the OpenTok session in a web browser. This application will only subscribe and get video frames

You can end the sample application by typing Control + C in the console.

## What does this project do

The C code gets the opentok video frames and uses `ffmpeg` to write the frames to a named pipe (/tmp/vonage_frame_buffer.fifo)

The Python code (`opentok_c_to_py/test_opencv.py`) uses OpenCV to read the named pipe (/tmp/vonage_frame_buffer.fifo) and write the frames to `output_video.avi`

You should run both code simultaneously. When either of the process closes, the pipe will close and you have to restart both.
