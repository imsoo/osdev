### Overview

A hobbyist operating system created from scratch. This project has targeted to build an operating system for the x86-64 multicore processor.

* Features of this project:

> * OS Kernel : Bootstrapping, Memory management, Process management and scheduling, GUI System ...
> * ***Toy*** TCP/IP Stack : Ethernet(E1000), ARP, IP, ICMP, UDP, TCP, DNS, DHCP
> * Simple Application : Toy web browser (almost like viewer), Simple telnet client ...


* Example of result

| Operating System | Toy web browser |
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/81499912-a62a4c00-9309-11ea-9fd3-881693de3481.gif" width="100%" height="35%">|<img src="https://user-images.githubusercontent.com/11255376/81499906-a0cd0180-9309-11ea-8234-b77155ab61a1.gif" width="100%" height="35%">|
---
### OS Kernel

Kernel based on [MINT64OS Project](https://github.com/kkamagui/mint64os)

Features of MINT64 OS project:

>* 64bit Multicore support
>* Memory protection and management with paging
>* Multi-level priority-based scheduler and process management
>* Simple filesystem based on FAT
>* Peripheral devices support (keyboard, mouse, and serial port)
>* System call and libraries for user applications
>* Application support with ELF format
>* GUI system
>* ~~Boot from USB device, and so~~ (not support in this project)
---
### Toy TCP/IP stack

My goal is to understand inter network communication. So I'm just focused on easy to implement. I derived help from the following documents. But does not implement all of the features and not perfect. I only tested the following scenario.

* **Design decision**
> * All of the frames are passed between processing layers in one buffer when passing a layer boundary. 
> * All of the frames are queued each module. 
> * Each protocol module is a thread and they have a own frame queue. 


| TCP/IP Stack architecture | Frame (when TCP received segment from IP module)  |
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/81503993-cf56d680-9321-11ea-9556-de08f33b46e1.jpg" width="90%" height="15%">|<img src="https://user-images.githubusercontent.com/11255376/81503990-cd8d1300-9321-11ea-8e7b-a0b5f996e91c.jpg" width="90%" height="25%">|

* **Scenario**

|***Scenario Number***| ***Source Host*** | ***<------*** | ***Protocol*** | ***------>*** | ***Destination Host*** |
|:--:|:--:|:--:|:--:|:--:|:--:|
|Scenario 1| Host | <------ | ***ARP***  Request / Reply | ------> | Gateway |
|Scenario 2| Host | <------ | ***DHCP*** Configuration | ------> | Gateway |
|Scenario 3| Host | <------ | ***DNS*** Request / Response | ------> | DNS Server |
|Scenario 4| Host | <------ | ***TCP*** Connection & ***HTTP*** 1.1 | ------> | Web Server |
|Scenario 5| Host | <------ | ***TCP*** Connection & ***Telent*** | ------> | Telnet Server |

* **Documents**

>* Network Stack - [OS Dev Wiki](https://wiki.osdev.org/Networking)
>* ARP - [RFC 826 - An Ethernet Address Resolution Protocol](https://tools.ietf.org/html/rfc826)
>* IP - [RFC 791 - INTERNET PROTOCOL](https://tools.ietf.org/html/rfc791)
>* ICMP - [RFC 792 - INTERNET CONTROL MESSAGE PROTOCOL](https://tools.ietf.org/html/rfc792)
>* UDP - [RFC 768 - User Datagram Protocol](https://tools.ietf.org/html/rfc768)
>* TCP - [RFC 793 - TRANSMISSION CONTROL PROTOCOL](https://tools.ietf.org/html/rfc793)
>* DNS - [RFC 1035 - DOMAIN NAMES - IMPLEMENTATION AND SPECIFICATION](https://tools.ietf.org/html/rfc1035)
>* DHCP - [RFC 2131 - Dynamic Host Configuration Protocol](https://tools.ietf.org/html/rfc2131)
---
### Toy web browser

Toy web browser based on [Robinson-C](https://github.com/wernsey/robinson-c). Robinson-C is C implementation of Matt Brubeck's [Robinson](https://github.com/mbrubeck/robinson) HTML engine. 

| Toy browser engine pipeline  | Output of [test HTML](https://github.com/imsoo/osdev/blob/master/osdev/03.Application/04.Browser/test2.html) |
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/81500925-28b60a00-9310-11ea-8a0b-9c8fe2e95087.JPG" width="100%" height="25%">|<img src="https://user-images.githubusercontent.com/11255376/81500691-d4f6f100-930e-11ea-9a60-76a2a9db8e35.gif" width="100%" height="35%">|
