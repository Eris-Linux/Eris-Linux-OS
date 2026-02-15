/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <iostream>

#include <arpa/inet.h>

#include <liberis.h>

#include "network-api-window.h"
#include "virtual-keyboard.h"

static int check_ip_address_string(const char *string, int ip_v6);


NetworkApiWindow::NetworkApiWindow(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QLabel *titleLabel = new QLabel("Network API", this);
	titleLabel->setAlignment(Qt::AlignCenter);
	titleLabel->setObjectName("TitleLabel");
	mainLayout->addWidget(titleLabel);

	mainLayout->addStretch(1);

	QGridLayout *grid = new QGridLayout(this);
	int row = 0;

	QLabel *label = new QLabel(
		"Select the network interface in the following list:\n"
	);
	label->setAlignment(Qt::AlignLeft);
	grid->addWidget(label, row, 0, 1, 2);

	row++;

	netIfList = new QListWidget(this);
	connect(netIfList,&QListWidget::itemSelectionChanged, [&](){
		network_interface_selected();
	});
	grid->addWidget(netIfList, row, 0, 1, 4);

	row++;

	netIfStatusLabel = new QLabel("");
	netIfStatusLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	grid->addWidget(netIfStatusLabel, row, 0, 1, 2);

	netIfStatusBtn = new QPushButton("Enable / disable", this);
	connect(netIfStatusBtn, &QPushButton::clicked, this, [this]() {
		status_button_clicked();
	});
	grid->addWidget(netIfStatusBtn, row, 2, 1, 2);

	row++;

	netIfAtBootChk = new QCheckBox("Enabled at boot");
	connect(netIfAtBootChk, &QCheckBox::checkStateChanged, this, [this]() {
		atboot = netIfAtBootChk->isChecked();
		do_update_config();
	});
	grid->addWidget(netIfAtBootChk, row, 0, 1, 2);

	netIfAddrBtn = new QPushButton("Address", this);
	connect(netIfAddrBtn, &QPushButton::clicked, this, [this]() {
		addr_button_clicked();
	});
	grid->addWidget(netIfAddrBtn, row, 2, 1, 2);

	row++;

	netIfIpv6Chk = new QCheckBox("Use Ipv6 address");
	connect(netIfIpv6Chk, &QCheckBox::checkStateChanged, this, [this]() {
		ipv6 = netIfIpv6Chk->isChecked();
		do_update_config();
	});
	grid->addWidget(netIfIpv6Chk, row, 0, 1, 2);

	netIfMaskBtn = new QPushButton("Mask", this);
	connect(netIfMaskBtn, &QPushButton::clicked, this, [this]() {
		mask_button_clicked();
	});
	grid->addWidget(netIfMaskBtn, row, 2, 1, 2);

	row++;

	netIfDhcpChk = new QCheckBox("Use DHCP");
	connect(netIfDhcpChk, &QCheckBox::checkStateChanged, this, [this]() {
		dhcp = netIfDhcpChk->isChecked();
		do_update_config();
		no_update = true;
		do_refresh();
		no_update = false;
	});
	grid->addWidget(netIfDhcpChk, row, 0, 1, 2);

	netIfGatewayBtn = new QPushButton("Change", this);
	connect(netIfGatewayBtn, &QPushButton::clicked, this, [this]() {
		gateway_button_clicked();
	});
	grid->addWidget(netIfGatewayBtn, row, 2, 1, 2);

	row++;

	QPushButton *btn = new QPushButton("Refresh interface status", this);
	connect(btn, &QPushButton::clicked, this, [this]() {
		do_refresh();
	});
	grid->addWidget(btn, row, 1, 1, 2);

	row++;

	netIfDnsBtn = new QPushButton("Change", this);
	connect(netIfDnsBtn, &QPushButton::clicked, this, [this]() {
		dns_button_clicked();
	});
	grid->addWidget(netIfDnsBtn, row, 1, 1, 2);

	row++;

	row++;

	mainLayout->addLayout(grid);
	mainLayout->addStretch(1);

	QPushButton *closeButton  = new QPushButton("Close", this);
	connect(closeButton, &QPushButton::clicked, this, &NetworkApiWindow::close);
	mainLayout->addWidget(closeButton);

	fill_netif_list();
	display_dns_address();
}



void NetworkApiWindow::network_interface_selected(void)
{
	// std::cerr << __FUNCTION__ << std::endl;

	no_update = true;

	do_refresh();

	no_update = false;
}


void NetworkApiWindow::status_button_clicked(void)
{
	// std::cerr << __FUNCTION__ << std::endl;

	if (strcmp(netIfStatusBtn->text().toLatin1(), "Disable") == 0)
		eris_set_network_interface_status(netIfList->currentItem()->text().toLatin1(), "down");
	else
		eris_set_network_interface_status(netIfList->currentItem()->text().toLatin1(), "up");

	display_netif_status(netIfList->currentItem()->text().toLatin1());
}



void NetworkApiWindow::addr_button_clicked(void)
{
	QString text = VirtualKeyboardDialog::getText(
		VirtualKeyboardDialog::NumericDecimal,
		"IP address",
		"Enter the IP address of this device:",
		ip_address);

	if (text.isEmpty())
		return;

	if (! check_ip_address_string(text.toLatin1(), ipv6))
		return;

	strncpy(ip_address, text.toLatin1(), 64);
	ip_address[63] = '\0';
	do_update_config();

	no_update = true;
	do_refresh();
	no_update = false;
}



void NetworkApiWindow::mask_button_clicked(void)
{
	QString text = VirtualKeyboardDialog::getText(
		VirtualKeyboardDialog::NumericDecimal,
		"Subnet mask",
		"Enter the IP mask of the subnet:",
		ip_netmask);

	if (text.isEmpty())
		return;

	if (! check_ip_address_string(text.toLatin1(), ipv6))
		return;

	strncpy(ip_netmask, text.toLatin1(), 64);
	ip_netmask[63] = '\0';
	do_update_config();

	no_update = true;
	do_refresh();
	no_update = false;
}



void NetworkApiWindow::gateway_button_clicked(void)
{
	QString text = VirtualKeyboardDialog::getText(
		VirtualKeyboardDialog::NumericDecimal,
		"Gateway address",
		"Enter the IP address of the gateway:",
		ip_gateway);

	if (text.isEmpty())
		return;

	if (! check_ip_address_string(text.toLatin1(), ipv6))
		return;

	strncpy(ip_gateway, text.toLatin1(), 64);
	ip_gateway[63] = '\0';
	do_update_config();

	no_update = true;
	do_refresh();
	no_update = false;
}



void NetworkApiWindow::dns_button_clicked(void)
{
	QString text = VirtualKeyboardDialog::getText(
		VirtualKeyboardDialog::NumericDecimal,
		"DNS address",
		"Enter the IP address of the name server:",
		ip_dns);

	if (text.isEmpty())
		return;

	if (! check_ip_address_string(text.toLatin1(), ipv6))
		return;

	eris_set_nameserver_address(text.toLatin1());

	display_dns_address();
}



void NetworkApiWindow::do_refresh(void)
{
	// std::cerr << __FUNCTION__ << std::endl;

	if (netIfList->currentRow() < 0)
		return;

	const char *netif = netIfList->currentItem()->text().toLatin1().constData();

	char buffer[1024];
	eris_get_network_interface_config(netif, buffer, 1023);
	buffer[1023] = '\0';

	int i = 0;

	while ((buffer[i] != ' ') && (buffer[i] != '\0'))
		i++;
	if (buffer[i] == '\0')
		return;
	buffer[i++] = '\0';

	char *ptr = &(buffer[i]);
	while ((buffer[i] != ' ') && (buffer[i] != '\0'))
		i++;
	if (buffer[i] == '\0')
		return;
	buffer[i++] = '\0';

	ipv6 = (strcmp(ptr, "ipv6") == 0);

	ptr = &(buffer[i]);
	while ((buffer[i] != ' ') && (buffer[i] != '\0'))
		i++;
	if (buffer[i] == '\0')
		return;
	buffer[i++] = '\0';

	atboot = (strcmp(ptr, "atboot") == 0);

	ptr = &(buffer[i]);
	while ((buffer[i] != ' ') && (buffer[i] != '\n') && (buffer[i] != '\0'))
		i++;
	buffer[i++] = '\0';

	dhcp = (strcmp(ptr, "dhcp") == 0);

	if (! dhcp) {
		ptr = &(buffer[i]);
		while ((buffer[i] != ' ') && (buffer[i] != '\0'))
			i++;
		if (buffer[i] == '\0')
			return;
		buffer[i++] = '\0';
		strncpy(ip_address, ptr, 63);
		ip_address[63] = '\0';

		ptr = &(buffer[i]);
		while ((buffer[i] != ' ') && (buffer[i] != '\0'))
			i++;
		if (buffer[i] == '\0')
			return;
		buffer[i++] = '\0';
		strncpy(ip_netmask, ptr, 63);
		ip_netmask[63] = '\0';

		ptr = &(buffer[i]);
		while ((buffer[i] != ' ') && (buffer[i] != '\0'))
			i++;
		if (buffer[i] == '\0')
			return;
		buffer[i++] = '\0';
		strncpy(ip_gateway, ptr, 63);
		ip_gateway[63] = '\0';

	} else {
		strcpy(ip_address, "0.0.0.0");
		strcpy(ip_netmask, "0.0.0.0");
		strcpy(ip_gateway, "0.0.0.0");
	}

	display_netif_status(netif);
}



void NetworkApiWindow::do_update_config(void)
{
	// std::cerr << __FUNCTION__ << std::endl;

	if (no_update)
		return;

	std::cerr << __FUNCTION__ << "update " << std::endl;
	std::cerr << __FUNCTION__ << "atboot =  " << atboot << std::endl;

	const char *netif = netIfList->currentItem()->text().toLatin1().constData();

	eris_set_network_interface_config(
		netif,
		atboot ? "atboot" : "notatboot",
		ipv6 ? "ipv6" : "ipv4",
		dhcp ? "dhcp" : "static",
		ip_address,
		ip_netmask,
		ip_gateway);

}



void NetworkApiWindow::fill_netif_list(void)
{
	// std::cerr << __FUNCTION__ << std::endl;

	char list[1024];

	eris_get_list_of_network_interfaces(list, 1023);
	list[1023] = '\0';

	char *netif;
	char *rest = list;

	while ((netif = strtok_r(rest, " ", &rest)) != NULL) {
		netIfList->addItem(QString(netif));
 	}

 	if (netIfList->count() == 0)
 		return;

 	netIfList->setCurrentRow(0);
}



void NetworkApiWindow::display_dns_address(void)
{
	// std::cerr << __FUNCTION__ << std::endl;

	char address[128];

	if (eris_get_nameserver_address(address, 128) == 0)
		netIfDnsBtn->setText(QString("DNS address: ") + QString::fromUtf8(address));
}



void NetworkApiWindow::display_netif_status(const char *netif)
{
	// std::cerr << __FUNCTION__ << std::endl;

	char buffer[1024];

	eris_get_network_interface_status(netif, buffer, 1023);
	buffer[1023] = '\0';

	if (strncasecmp(buffer, "up", 2) == 0) {
		netIfStatusLabel->setText("Interface UP");
		netIfStatusBtn->setText("Disable");
		display_netif_config(netif, 1);
	} else {
		netIfStatusLabel->setText("Interface DOWN");
		netIfStatusBtn->setText("Enable");
		display_netif_config(netif, 0);
	}
}



void NetworkApiWindow::display_netif_config(const char *netif, int up)
{
	// std::cerr << __FUNCTION__ << std::endl;

	netIfIpv6Chk->setChecked(ipv6);
	netIfAtBootChk->setChecked(atboot);
	netIfDhcpChk->setChecked(dhcp);

	if (! up) {
		netIfAddrBtn->setText("");
		netIfAddrBtn->setEnabled(false);
		netIfMaskBtn->setText("");
		netIfMaskBtn->setEnabled(false);
		netIfGatewayBtn->setText("");
		netIfGatewayBtn->setEnabled(false);
		return;
	}

	if (dhcp) {
		netIfAddrBtn->setText("");
		netIfAddrBtn->setEnabled(false);
		netIfMaskBtn->setText("");
		netIfMaskBtn->setEnabled(false);
		netIfGatewayBtn->setText("");
		netIfGatewayBtn->setEnabled(false);
		return;
	}
	netIfAddrBtn->setText(QString("IP address: ") + QString::fromUtf8(ip_address));
	netIfAddrBtn->setEnabled(true);

	netIfMaskBtn->setText(QString("Subnet mask: ") + QString::fromUtf8(ip_netmask));
	netIfMaskBtn->setEnabled(true);

	netIfGatewayBtn->setText(QString("IP of the gateway: ") + QString::fromUtf8(ip_gateway));
	netIfGatewayBtn->setEnabled(true);
}



static int check_ip_address_string(const char *string, int ip_v6)
{
	if (ip_v6) {
		struct in6_addr addr;
		return (inet_pton(AF_INET6, string, &addr) == 1);
	}
	struct in_addr  addr;
	return (inet_pton(AF_INET, string, &addr) == 1);
}
