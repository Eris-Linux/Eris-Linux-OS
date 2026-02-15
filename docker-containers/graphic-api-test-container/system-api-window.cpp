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
#include "system-api-window.h"


SystemApiWindow::SystemApiWindow(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QLabel *titleLabel = new QLabel("System API", this);
	titleLabel->setAlignment(Qt::AlignCenter);
	titleLabel->setObjectName("TitleLabel");
	mainLayout->addWidget(titleLabel);

	mainLayout->addStretch(1);

	QGridLayout *grid = new QGridLayout(this);
	QPushButton *commandButton;
	int row = 0;


	systemTypeLabel = new QLabel("");
	systemTypeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	systemTypeLabel->setWordWrap(true);
	grid->addWidget(systemTypeLabel, row, 0);

	row++;


	systemModelLabel = new QLabel("");
	systemModelLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	systemModelLabel->setWordWrap(true);
	grid->addWidget(systemModelLabel, row, 0);

	row++;


	systemVersionLabel = new QLabel("");
	systemVersionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	systemVersionLabel->setWordWrap(true);
	grid->addWidget(systemVersionLabel, row, 0);

	row++;


	machineUuidLabel = new QLabel("");
	machineUuidLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	machineUuidLabel->setWordWrap(true);
	grid->addWidget(machineUuidLabel, row, 0);


	row++;

	containersLabel = new QLabel("");
	containersLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	containersLabel->setWordWrap(true);
	grid->addWidget(containersLabel, row, 0, 1, 2);


	row++;
/*
	commandButton = new QPushButton("System restart (reboot)", this);
	grid->addWidget(commandButton, row, 0);
	connect(commandButton, &QPushButton::clicked, this, [this]() {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Reboot", "Are you sure you want to reboot?", QMessageBox::Yes|QMessageBox::No);
		if (reply == QMessageBox::Yes) {
			if (eris_reboot() != 0) {
				QMessageBox::question(this, "Error", "Unable to restart the system.", QMessageBox::Ok);
			}
		}
	});

	commandButton = new QPushButton("System halt (shutdown)", this);
	grid->addWidget(commandButton, row, 1);
	connect(commandButton, &QPushButton::clicked, this, [this]() {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Reboot", "Are you sure you want to shutdown?", QMessageBox::Yes|QMessageBox::No);
		if (reply == QMessageBox::Yes) {
			shutdownLabel->setText(QString(""));
			if (eris_shutdown() != 0) {
				QMessageBox::question(this, "Error", "Unable to halt the system.", QMessageBox::Ok);
			}
		}
	});

	row++;
*/

	mainLayout->addLayout(grid);

	mainLayout->addStretch(1);

	QPushButton *closeButton  = new QPushButton("Close", this);
	connect(closeButton, &QPushButton::clicked, this, &SystemApiWindow::close);
	mainLayout->addWidget(closeButton);

	display_system_labels();
}


void SystemApiWindow::display_system_labels(void)
{
	char buffer[1024];

	if (eris_get_system_type(buffer, 1023) == 0) {
		buffer[1023] = '\0';
		systemTypeLabel->setText("System type: " + QString(buffer));
	} else {
		systemTypeLabel->setText("System type: ???");
	}

	if (eris_get_system_model(buffer, 1023) == 0) {
		buffer[1023] = '\0';
		systemModelLabel->setText("System model: " + QString(buffer));
	} else {
		systemModelLabel->setText("System model: ???");
	}

	if (eris_get_system_version(buffer, 1023) == 0) {
		buffer[1023] = '\0';
		systemVersionLabel->setText("System version: " + QString(buffer));
	} else {
		systemVersionLabel->setText("System version: ???");
	}

	if (eris_get_system_uuid(buffer, 1023) == 0) {
		buffer[1023] = '\0';
		machineUuidLabel->setText("Machine UUID: " + QString(buffer));
	} else {
		machineUuidLabel->setText("Machine UUID: ???");
	}

	snprintf(buffer, 1024, "Containers:\n");
	for (int slot = 0; slot < eris_get_number_of_slots(); slot++) {
		if (! eris_get_container_presence(slot))
			continue;
		char name[1024];
		eris_get_container_name(slot, name, 1023);
		name[1023] = '\0';

		char version[1024];
		eris_get_container_version(slot, version, 1023);
		version[1023] = '\0';
		size_t start = strlen(buffer);
		snprintf(&(buffer[start]), 1023 - start, "   %d: %s %s\n", slot, name, version);
	}
	containersLabel->setText(buffer);
}

