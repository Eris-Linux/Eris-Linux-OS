/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#ifndef NETWORK_API_WINDOW
#define NETWORK_API_WINDOW

#include <QCheckBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>


class NetworkApiWindow : public QWidget {
	public:
		explicit NetworkApiWindow(QWidget *parent = nullptr);

	private:
		QListWidget *netIfList;

		QLabel      *netIfStatusLabel;
		QPushButton *netIfStatusBtn;

		QCheckBox   *netIfAtBootChk;
		QCheckBox   *netIfIpv6Chk;
		QCheckBox   *netIfDhcpChk;

		QPushButton *netIfAddrBtn;
		QPushButton *netIfMaskBtn;
		QPushButton *netIfGatewayBtn;
		QPushButton *netIfDnsBtn;

		void network_interface_selected(void);
		void status_button_clicked(void);

		void addr_button_clicked(void);
		void mask_button_clicked(void);
		void gateway_button_clicked(void);
		void dns_button_clicked(void);

		void do_refresh(void);
		void do_update_config(void);
		void fill_netif_list(void);
		void display_dns_address(void);
		void display_netif_status(const char *netif);
		void display_netif_config(const char *netif, int up);

		bool ipv6;
		bool atboot;
		bool dhcp;
		char ip_address[64];
		char ip_netmask[64];
		char ip_gateway[64];

		char ip_dns[64];

		bool no_update;
};

#endif
