# AqaraHub

This is an open-source Zigbee hub for Xiaomi Aqara devices, as pictured [here](https://des.gbtcdn.com/uploads/pdm-desc-pic/Electronic/image/2017/04/25/20170425155840_15186.jpg). It aims to be a replacement to the Xiaomi Gateway that does not require communication to outside servers, and uses a saner communication option (e.g. MQTT).

This project is spefically aimed at Xiaomi Aqara devices, and there are currently no plans to support other Zigbee devices with this project.

## Get in touch!
I've been writing this thing on my own, and it appears to solve my use-case fairly well, but I'd love to get feedback from others!

Some of the things I'd like to know:

* Is anyone else even interested in a project like this?
* Were others able to compile it ?
* Has anyone actually got it to run ?
* Is anyone missing specific functionality ?

So instead of only following or starring this project, just drop me a message at fw@hardijzer.nl too :)

## Getting Started

At this point, reporting attributes received from the Xiaomi devices to MQTT appears to be working quite well. If this is all that you require, I encourage you to give it a shot.
Support for sending things back, like turning the Smart Plug on or off, is still on the to-do list.

### Libraries and tools used

This project uses a lot of C++14 features, so obviously a compiler supporting these is required. GCC 5 or later, Clang 3.4 or later, or Microsoft Visual Studio 2017 should fit the bill.

On top of that it makes heavy use of the [Boost C++ Libraries](http://www.boost.org/), the [Adobe Software Technology Lab Concurrency Libraries](https://github.com/stlab/libraries/) (hereafter "STLab-libraries"), Takatoshi Kondo's excellent [mqtt\_cpp library](https://github.com/redboltz/mqtt_cpp), and [The Art of C++ / JSON](https://github.com/taocpp/json) libraries.

All dependencies except for Boost can be pulled in as git submodules:
```
git submodule update --init --recursive
```
There is no need to compile these libraries, as they are all header-only.

The Boost libraries should be available on most Linux distributions, likely named either ```boost-devel```, ```boost-libs```, or just ```boost```.

### Compiling using CMake
On my machine, I can compile using the following commands:
```
git submodule update --init --recursive
mkdir build
cd build
cmake ..
make
```
Afterwards a binary named ```AqaraHub``` should have appeared in the build folder.

## Deployment

### Prerequisites
To run AqaraHub, several things are needed:

- A CC2531 Zigbee USB dongle, like one of [these](https://www.aliexpress.com/wholesale?SearchText=CC2531+USB+Dongle)
- A programmer to flash the CC2531, like one of [these](https://www.aliexpress.com/wholesale?SearchText=CC2531+Programmer)
- A cable from the standard double-row 2.54-spaced connector to the teeny-tiny connector on the CC2531, like one of [these](https://www.aliexpress.com/wholesale?SearchText=CC2531+Cable)
- A functioning MQTT Server
- One or more Xiaomi Aqara devices

### Flashing the Zigbee dongle
The Zigbee dongle should be running the "Pro-Secure\_LinkKeyJoin" firmware, available [here](https://github.com/mtornblad/zstack-1.2.2a.44539/blob/master/CC2531/CC2531ZNP-Pro-Secure_LinkKeyJoin.hex). You can use CC-Tool from [here](https://sourceforge.net/projects/cctool/files/) or [here](https://github.com/dashesy/cc-tool) to flash it to the dongle.

I've succesfully flashed my device using the following steps:
```
git clone https://github.com/dashesy/cc-tool.git
cd cc-tool
./configure
make
wget https://raw.githubusercontent.com/mtornblad/zstack-1.2.2a.44539/master/CC2531/CC2531ZNP-Pro-Secure_LinkKeyJoin.hex
```
Next connect the programmer to the dongle. Note that there is a very small "1" on one side of the plug on the dongle, and a "10" on the other side. The cable should be plugged in to have the red wire on the side of the "1". Then connect the USB dongle to the computer, and finally plug in the programmer to the computer. I'm not entirely sure why, but any other order does not appear to work for me.
Finally, instruct cc-tool to flash the firmware:
```
sudo ./cc-tool -e -w CC2531ZNP-Pro-Secure_LinkKeyJoin.hex
```
Note that I'm using sudo as otherwise cc-tool is unable to use libusb, for some reason. This is not the safest decision. If anyone knows the proper way to give this executable access to libusb, please let me know!

Finally disconnect both the dongle and the programmer from your computer, disconnect the debugging cable, and plug the dongle back in the computer. Once the green light turns off, The dongle should be ready to be used by AqaraHub.

### Running AqaraHub
Running AqaraHub is relatively simple. Simply instruct it to which serial port the USB dongle is using, the MQTT server to connect to, and the topic under which to publish all received information:
```
./AqaraHub --port /dev/ttyACM0 --mqtt mqtt://ArchServer/ --topic AqaraHub
```

## Contributing
Any and all help would be greatly appreciated. Feel free to make pull requests or add issues through [Github](https://github.com/Frans-Willem/AqaraHub).

Code formatting wise, I try to stick to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). Don't feel obligated to make pull requests perfect, most of the formatting can be solved with clang-format, and we can always clean it up together...

## Authors
AqaraHub is written by Frans-Willem Hardijzer. See my [Github profile](https://github.com/Frans-Willem) for contact options.

## License
AqaraHub is licensed under the GNU General Public License, version v3.0. See LICENSE-gpl-3.0.txt or the [online version](https://www.gnu.org/licenses/gpl-3.0.txt) for more information.

## Acknowledgments
I'd like to thank the [zigbee-shepherd](https://github.com/zigbeer/zigbee-shepherd) project both as inspiration as well as being a very good example on ZNP programming. It's debug output has helped me immensely in actually getting started programming the ZNP dongle.
