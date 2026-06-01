# Container Example for Eris Linux - API web test

## Description

This project is an example of a container that can be used on the Eris Linux
environment.


It's an example of an embedded HTTP server and an HMI to communicate with the
system using the Eris API.


## License

This container example is licensed under the MIT license.


## Author

Christophe BLAESS 2026


## Installation

Prepare the container using the  `create-container`  script provided with
the Eris Linux containers package.

Connect to your account on the [Eris Linux Device Manager](https://www.eris-linux.net).

Go to `My containers` tab and click on the `Upload a container` button to
upload your container. You may enter a password if you want to encrypt the
container before it is stored on Eris Linux server.

After container upload, click on "Setup..." button. Then fill in some fields.

The `Compatible board` field with the type of board on which you'll use the
container,

Fill The `Exported Ports` with `80:80/tcp`.

Go to `My devices` tab, select the group of devices on which you want to
install the container. On the upper right table, click on the rightmost
button of one of the rows. In the list, select your container and click `Ok`.

After a few minutes, when the concerned device will have contacted the
Device Manager, downloaded and installed the container, fire a web browser
and connect to the IP address of the device.

For more information, see Eris Linux documentation at
[www.eris-linux.net](https://www.eris-linux.net).

