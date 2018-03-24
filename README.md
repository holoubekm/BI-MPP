# BI-MPP Course
## Metody připojování periferií
### CTU / ČVUT FIT

During the course we were given different assigments mainly concerning peripheries connectivity. Some lectures were done in the Windows environment (mainly USB) while others in Linux (kernel drivers).

### Lecture 1
This time we were programming a single 8bit chip. Classic start with a blinking LED and lots of fun.

### Lecture 2
This lecture was about communication with a mainboard builting clock module via PCI and direct CMOS registers.

### Lecture 3
Program read the first sector of SATA disk using a prehistoric communication via registers. The program needs VENDOR and DEVICE IDs.

### Lecture 4
This is libusb client software communicating with a mouse reading it's raw data. The program needs VENDOR and DEVICE IDs.

### Lecture 5
This program uses a libusb module to read config and partition structure of an USB flash disk. The program needs VENDOR and DEVICE IDs.

### Lecture 6
This program is an USB device software driving it's communication with a PC.

### Lecture 7
Just a continuation of the previous lecture

### Lecture 8
A continuation of two previous lectures, adding a client library allowing PC to communicate with the device and vice-versa.

### Lecture 9
A simple Linux kernel driver just to try the whole process of it's creation.

### Lecture 11
A program reading raw data from the camera. Algorithm decodes data and tries to detect red object in front of the camera.
