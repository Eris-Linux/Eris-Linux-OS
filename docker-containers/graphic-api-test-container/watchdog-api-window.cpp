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
#include <QVBoxLayout>
#include <QWidget>

#include <liberis.h>
#include "virtual-keyboard.h"
#include "watchdog-api-window.h"


WatchdogApiWindow::WatchdogApiWindow(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout(this);
	QPushButton *commandButton;

	int row = 0;

	commandButton = new QPushButton("eris_get_watchdog_delay()", this);
	grid->addWidget(commandButton, row, 0);
	connect(commandButton, &QPushButton::clicked, this, [this]() {
		char buffer[1024];
		snprintf(buffer, 1023, "%d", eris_get_watchdog_delay());
		buffer[1023] = '\0';
		watchdogGetDelayLabel->setText(QString(buffer));
	});

	watchdogGetDelayLabel = new QLabel("");
	watchdogGetDelayLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	watchdogGetDelayLabel->setWordWrap(true);
	grid->addWidget(watchdogGetDelayLabel, row, 1);

	row++;


	commandButton = new QPushButton("eris_set_watchdog_delay()", this);
	grid->addWidget(commandButton, row, 0);
	connect(commandButton, &QPushButton::clicked, this, [this]() {
		watchdogSetDelayLabel->setText("");
		disableWatchdogLabel->setText("");

		char defaultValue[1024];
		snprintf(defaultValue, 1023, "%d", eris_get_watchdog_delay());
		defaultValue[1023] = '\0';

		bool ok;
		QString text = VirtualKeyboardDialog::getText(
			VirtualKeyboardDialog::NumericDecimal,
			"Watchdog Delay",
			"Enter the watchdog delay in seconds:",
			defaultValue);

		if (! text.isEmpty()) {
			int delay = text.toInt(&ok);
			if (ok) {
				if (eris_set_watchdog_delay(delay) == 0) {
					watchdogSetDelayLabel->setText(QString("Watchdog set."));
				}
			}
		}
	});

	watchdogSetDelayLabel = new QLabel("");
	watchdogSetDelayLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	watchdogSetDelayLabel->setWordWrap(true);
	grid->addWidget(watchdogSetDelayLabel, row, 1);

	row++;


	commandButton = new QPushButton("eris_feed_watchdog()", this);
	grid->addWidget(commandButton, row, 0);
	connect(commandButton, &QPushButton::clicked, this, [this]() {
		watchdogSetDelayLabel->setText("");
		watchdogFeedLabel->setText(QString(""));
		if (eris_feed_watchdog() == 0)
			watchdogFeedLabel->setText(QString("Dog fed!"));
		disableWatchdogLabel->setText("");
	});

	watchdogFeedLabel = new QLabel("");
	watchdogFeedLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	watchdogFeedLabel->setWordWrap(true);
	grid->addWidget(watchdogFeedLabel, row, 1);

	row++;
	

	commandButton = new QPushButton("eris_disable_watchdog()", this);
	grid->addWidget(commandButton, row, 0);
	connect(commandButton, &QPushButton::clicked, this, [this]() {
		watchdogSetDelayLabel->setText("");
		watchdogFeedLabel->setText("");
		disableWatchdogLabel->setText(QString("Watchdog disabled"));
	});

	disableWatchdogLabel = new QLabel("");
	disableWatchdogLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	disableWatchdogLabel->setWordWrap(true);
	grid->addWidget(disableWatchdogLabel, row, 1);

	row++;


	mainLayout->addLayout(grid);

	QPushButton *closeButton  = new QPushButton("Close", this);
	connect(closeButton, &QPushButton::clicked, this, &WatchdogApiWindow::close);
	mainLayout->addWidget(closeButton);
}
