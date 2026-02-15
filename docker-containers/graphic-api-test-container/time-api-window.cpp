/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <liberis.h>
#include "virtual-keyboard.h"
#include "time-api-window.h"



class TimeZonesWindow : public QWidget {
	public:
		TimeZonesWindow(QWidget *parent) : QWidget(parent)
		{
			QVBoxLayout *layout = new QVBoxLayout(this);

			QLabel *titleLabel = new QLabel("Available Time Zones", this);
			titleLabel->setAlignment(Qt::AlignCenter);
			titleLabel->setObjectName("TitleLabel");
			layout->addWidget(titleLabel);

			QTextEdit *textEdit = new QTextEdit;
			char buffer[65536];
			eris_list_time_zones(buffer, 65535);
			buffer[65535] = '\0';

			textEdit->setPlainText(buffer);
			textEdit->setLineWrapMode(QTextEdit::WidgetWidth);
			textEdit->setReadOnly(true);
			textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
			textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
			layout->addWidget(textEdit);

			QPushButton *closeButton  = new QPushButton("Close", this);
			connect(closeButton, &QPushButton::clicked, this, &TimeZonesWindow::close);
			layout->addWidget(closeButton);
		}

};



TimeApiWindow::TimeApiWindow(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QLabel *titleLabel = new QLabel("Time API", this);
	titleLabel->setAlignment(Qt::AlignCenter);
	titleLabel->setObjectName("TitleLabel");
	mainLayout->addWidget(titleLabel);

	mainLayout->addStretch(1);

	QGridLayout *grid = new QGridLayout(this);
	QPushButton *commandButton;
	int row = 0;

	ntpEnabledLabel = new QLabel("");
	ntpEnabledLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	grid->addWidget(ntpEnabledLabel, row, 0);

	commandButton = new QPushButton("Configure NTP status", this);
	grid->addWidget(commandButton, row, 1);
	connect(commandButton, &QPushButton::clicked, this, [this]() {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "NTP", "Do you want to enable NTP service?", QMessageBox::Yes|QMessageBox::No);
		if (reply == QMessageBox::Yes) {
			eris_set_ntp_enable("yes");
		} else {
			eris_set_ntp_enable("no");
		}
		display_ntp_status();
	});

	row ++;


	ntpServerLabel = new QLabel("");
	ntpServerLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	grid->addWidget(ntpServerLabel, row, 0);

	ntpServerButton = new QPushButton("Configure NTP server", this);
	grid->addWidget(ntpServerButton, row, 1);
	connect(ntpServerButton, &QPushButton::clicked, this, [this]() {

		char defaultValue[1024];
		eris_get_ntp_server(defaultValue, 1023);
		defaultValue[1023] = '\0';

		bool ok;
		QString text = VirtualKeyboardDialog::getText(
			VirtualKeyboardDialog::Uri,
			"NTP server",
			"Enter the address of the NTP server:",
			defaultValue);

		if (! text.isEmpty()) {
			if (eris_set_ntp_server(text.toLatin1()) == 0) {
			}
		}
		display_ntp_server();
	});

	row ++;


	timeZoneLabel = new QLabel("");
	timeZoneLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	grid->addWidget(timeZoneLabel, row, 0);

	commandButton = new QPushButton("Configure time zone", this);
	grid->addWidget(commandButton, row, 1);
	connect(commandButton, &QPushButton::clicked, this, [this]() {

		char defaultValue[1024];
		eris_get_time_zone(defaultValue, 1023);
		defaultValue[1023] = '\0';

		bool ok;
		QString text = VirtualKeyboardDialog::getText(
			VirtualKeyboardDialog::Uri,
			"Time Zone",
			"Enter the time zone:",
			defaultValue);

		if (! text.isEmpty()) {
			if (eris_set_time_zone(text.toLatin1()) == 0) {
			}
		}
		display_time_zone();
	});

	row ++;


	localTimeLabel = new QLabel("");
	localTimeLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	localTimeLabel->setWordWrap(true);
	grid->addWidget(localTimeLabel, row, 0);


	commandButton = new QPushButton("List of time zones", this);
	grid->addWidget(commandButton, row, 1);
	connect(commandButton, &QPushButton::clicked, this, [this]() {
		TimeZonesWindow *win = new TimeZonesWindow(this);
		QSize screenSize = QGuiApplication::primaryScreen()->size();
		win->resize(screenSize);
		win->show();
	});

	row++;

	systemTimeLabel = new QLabel("");
	systemTimeLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	systemTimeLabel->setWordWrap(true);
	grid->addWidget(systemTimeLabel, row, 0);


	commandButton = new QPushButton("Configure system time", this);
	grid->addWidget(commandButton, row, 1);
	connect(commandButton, &QPushButton::clicked, this, [this]() {

		bool ok;
		QString text = VirtualKeyboardDialog::getText(
			VirtualKeyboardDialog::Uri,
			"System Time",
			"Enter the system time: (format YYYY/MM/DD hh:mm:ss)",
			"");

		if (! text.isEmpty()) {
			if (eris_set_system_time(text.toLatin1()) == 0) {
				this->refresh_time_labels();
			}
		}

	});

	refreshTimer = new QTimer(this);
	connect(refreshTimer, &QTimer::timeout, this, [this]() {
		this->refresh_time_labels();
	});

	refreshTimer->start(1000);

	mainLayout->addLayout(grid);

	mainLayout->addStretch(1);

	QPushButton *closeButton  = new QPushButton("Close", this);
	connect(closeButton, &QPushButton::clicked, this, [this]() {
		refreshTimer->stop();
		close();
	});
	mainLayout->addWidget(closeButton);

	display_ntp_status();
	display_time_zone();
	refresh_time_labels();
}



void TimeApiWindow::display_ntp_status(void)
{
	char buffer[1024];
	eris_get_ntp_enable(buffer, 1023);
	buffer[1023] = '\0';

	if (strcasecmp(buffer, "yes") == 0) {
		ntpEnabledLabel->setText(QString("NTP: enabled"));
		ntpServerButton->setEnabled(true);
		display_ntp_server();
	} else {
		ntpEnabledLabel->setText(QString("NTP: disabled"));
		ntpServerLabel->setText(QString(""));
		ntpServerButton->setEnabled(false);
	}
}



void TimeApiWindow::display_ntp_server(void)
{
	char buffer[1024];
	eris_get_ntp_server(buffer, 1023);
	buffer[1023] = '\0';

	ntpServerLabel->setText("NTP server: " + QString(buffer));
}



void TimeApiWindow::display_time_zone(void)
{
	char buffer[1024];
	eris_get_time_zone(buffer, 1023);
	buffer[1023] = '\0';

	timeZoneLabel->setText("Time zone: " + QString(buffer));
}



void TimeApiWindow::refresh_time_labels(void)
{
	char buffer[1024];

	eris_get_local_time(buffer, 1023);
	buffer[1023] = '\0';
	localTimeLabel->setText("Local time: " + QString(buffer));


	eris_get_system_time(buffer, 1023);
	buffer[1023] = '\0';
	systemTimeLabel->setText("System time: " + QString(buffer));
}

