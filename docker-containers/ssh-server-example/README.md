# Container Example for Eris Linux - SSH server

## Description

This project is an example of a container that can be used on the Eris Linux
environment.


It is an example of an SSH server installation.


Two accounts are provided by default:

- `root` with the password `root`
- `eris` with the password `linux`


*IMPORTANT*: do not use these passwords on production systems!

## License

This container example is licensed under the MIT license.


## Author

Christophe BLAESS 2026


## Installation

Prepare the container using the  `create-container`  script provided with
the Eris Linux containers package.

Connect to your account on the [Eris Linux Device Manager](https://www.eris-linux.net).

Go to `My containers` tab and click on `Upload a container` button to upload
your container. You may enter a password if you want to crypt the container
before it is stored on Eris Linux server.

After container upload, click on "Setup..." button. then fill some fields.

The `Compatible board` field with the type of board on which you'll use the
container,

The `Exported Ports` with `<external>:22/tcp`, with `<external>` being the
external port on which the container will be reachable.

Go to `My devices` tab, select the group of devices on which you want to
install the container. On the upper right table, click on the rightmost
button of one of the rows. In the list, select your container and click `Ok`.

After a few minutes, when the concerned device will have contacted the
Device Manager, downloaded and installed the container, use:

```
$ ssh  - p <external>  eris@<ip address>
```

With `<ip address>` being the address on wich the device is reachable on
the local subnet.

For more information, see Eris Linux documentation at [www.eris-linux.net](https://www-eris-linux.net).

