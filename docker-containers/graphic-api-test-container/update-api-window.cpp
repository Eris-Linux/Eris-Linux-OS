/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#include <QApplication>
#include <QFile>
#include <QGridLayout>
#include <QGuiApplication>
#include <QMessageBox>
#include <QScreen>
#include <QVBoxLayout>
#include <QWidget>

#include <iostream>

#include <arpa/inet.h>

#include <liberis.h>

#include "update-api-window.h"
#include "virtual-keyboard.h"



UpdateApiWindow::UpdateApiWindow(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QLabel *titleLabel = new QLabel("System Update API", this);
	titleLabel->setAlignment(Qt::AlignCenter);
	titleLabel->setObjectName("TitleLabel");
	mainLayout->addWidget(titleLabel);

	mainLayout->addStretch(1);

	QGridLayout *grid = new QGridLayout(this);
	int row = 0;

	systemUpdateStatusLabel = new QLabel(
		"System update status: (unknown)\n"
	);
	systemUpdateStatusLabel->setAlignment(Qt::AlignCenter);
	grid->addWidget(systemUpdateStatusLabel, row, 0, 1, 2);

	row++;

	rebootNeededLabel = new QLabel("");
	rebootNeededLabel->setAlignment(Qt::AlignCenter);
	grid->addWidget(rebootNeededLabel, row, 0, 1, 2);

	rebootNeededBtn = new QPushButton("Ask for a reboot", this);
	connect(rebootNeededBtn, &QPushButton::clicked, this, [this]() {
		reboot_needed_button_clicked();
	});
	grid->addWidget(rebootNeededBtn, row, 2, 1, 2);

	row++;

	contactPeriodLabel = new QLabel("");
	contactPeriodLabel->setAlignment(Qt::AlignCenter);
	grid->addWidget(contactPeriodLabel, row, 0, 1, 2);

	contactPeriodBtn = new QPushButton("Modify", this);
	connect(contactPeriodBtn, &QPushButton::clicked, this, [this]() {
		contact_period_button_clicked();
	});
	grid->addWidget(contactPeriodBtn, row, 2, 1, 2);

	row++;

	contactNowBtn = new QPushButton("Contact server now", this);
	connect(contactNowBtn, &QPushButton::clicked, this, [this]() {
		eris_contact_server();
	});
	grid->addWidget(contactNowBtn, row, 1, 1, 2);

	row++;

	automaticRebootChk = new QCheckBox("Automatic reboot after update");
	connect(automaticRebootChk, &QCheckBox::checkStateChanged, this, [this]() {
		automatic_reboot_check_changed();
	});
	grid->addWidget(automaticRebootChk, row, 1, 1, 2);

	row++;


	immediateContainerUpdateChk = new QCheckBox("Update container immediately when available");
	connect(immediateContainerUpdateChk, &QCheckBox::checkStateChanged, this, [this]() {
		immediate_container_update_check_changed();
	});
	grid->addWidget(immediateContainerUpdateChk, row, 1, 1, 2);

	row++;


	QPushButton *btn = new QPushButton("Restore factory presets", this);
	connect(btn, &QPushButton::clicked, this, [this]() {
		restore_factory_presets();
	});
	grid->addWidget(btn, row, 1, 1, 2);

	row++;

	row++;

	mainLayout->addLayout(grid);
	mainLayout->addStretch(1);

	QPushButton *closeButton  = new QPushButton("Close", this);
	connect(closeButton, &QPushButton::clicked, this, [this]() {
		refreshTimer->stop();
		close();
	});
	mainLayout->addWidget(closeButton);

	update_values();

	refreshTimer = new QTimer(this);
	connect(refreshTimer, &QTimer::timeout, this, [this]() {
		update_values();
	});
	refreshTimer->start(300);

}


void UpdateApiWindow::reboot_needed_button_clicked(void)
{
	eris_set_reboot_needed_flag(! eris_get_reboot_needed_flag());
}


void UpdateApiWindow::contact_period_button_clicked(void)
{
	int period = eris_get_server_contact_period();
	if (period == -1 )
		period = 300;

	char value[32];
	snprintf(value, 32, "%d", period);
	value[31] = '\0';

	QString text = VirtualKeyboardDialog::getText(
		VirtualKeyboardDialog::Numeric,
		"Contact Period",
		"Enter the period in seconds between contacts with the server:",
		value);

	if (text.isEmpty())
		return;

	if ((sscanf(text.toLatin1(), "%d", &period) == 1)
	 && (period >= 0)
	 && (period <= 86400))
		eris_set_server_contact_period(period);
}


void UpdateApiWindow::automatic_reboot_check_changed(void)
{
	if (! noupdate) {
		eris_set_automatic_reboot_flag(automaticRebootChk->isChecked() ? 1 : 0);
	}
}


void UpdateApiWindow::immediate_container_update_check_changed(void)
{
	if (! noupdate) {
		eris_set_container_update_policy(immediateContainerUpdateChk->isChecked() ? 1 : 0);
	}
}


void UpdateApiWindow::restore_factory_presets(void)
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Reboot", "Are you sure you want to restore the factory presets of the system?", QMessageBox::Yes|QMessageBox::No);
	if (reply == QMessageBox::Yes) {	
		eris_restore_factory_preset();
	}
}


void UpdateApiWindow::update_values(void)
{

	noupdate = true;

	int val = eris_get_system_update_status();

	switch (val) {
	case 1:
		systemUpdateStatusLabel->setText("System Ok - No update pending.");
		break;
	case 2:
		systemUpdateStatusLabel->setText("System update install in progress.");
		break;
	case 3:
		systemUpdateStatusLabel->setText("System update install Ok. Waiting for reboot.");
		break;
	case 4:
		systemUpdateStatusLabel->setText("System update install failed.");
		break;
	case 5:
		systemUpdateStatusLabel->setText("System reboot in progress.");
		break;
	default:
		systemUpdateStatusLabel->setText("System update status unknown (?).");
		break;
	}

	if (eris_get_reboot_needed_flag()) {
		rebootNeededLabel->setText("A reboot has been requested by the update system,\nthe Web device manager or a container.");
		rebootNeededBtn->setText("Refuse the reboot");
	} else {
		rebootNeededLabel->setText("No reboot is programmed");
		rebootNeededBtn->setText("Ask for a reboot");
	}

	val = eris_get_server_contact_period();
	if (val > 0) {
		char buffer[128];
		snprintf(buffer, 128, "Period between contacts with the server: %d sec.", val);
		buffer[127] = '\0';
		contactPeriodLabel->setText(buffer);
	} else if (val == 0) {
		contactPeriodLabel->setText("No periodic contact with the server");
	}

	automaticRebootChk->setChecked(eris_get_automatic_reboot_flag());
	immediateContainerUpdateChk->setChecked(eris_get_container_update_policy());

	noupdate = false;
}

