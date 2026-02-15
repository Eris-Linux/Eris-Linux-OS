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

#include <liberis.h>

#include <iostream>

#include "sbom-api-window.h"
#include "license-window.h"


SbomApiWindow::SbomApiWindow(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QLabel *titleLabel = new QLabel("S-BOM API", this);
	titleLabel->setAlignment(Qt::AlignCenter);
	titleLabel->setObjectName("TitleLabel");
	mainLayout->addWidget(titleLabel);

	mainLayout->addStretch(1);

	QGridLayout *grid = new QGridLayout(this);
	int row = 0;

	QLabel *label = new QLabel(
		"Installed packages:"
	);
	label->setAlignment(Qt::AlignLeft);
	grid->addWidget(label, row, 0, 1, 2);

	row++;

	packagesList = new QListWidget(this);
	connect(packagesList,&QListWidget::itemSelectionChanged, [&](){
		package_selected();
	});
	grid->addWidget(packagesList, row, 0, 1, 4);

	row++;

	packageLabel = new QLabel(
		""
	);
	packageLabel->setAlignment(Qt::AlignLeft);
	grid->addWidget(packageLabel, row, 0, 1, 2);

	row++;

	label = new QLabel(
		"Present licenses:"
	);
	label->setAlignment(Qt::AlignLeft);
	grid->addWidget(label, row, 0, 1, 2);

	row++;

	licensesList = new QListWidget(this);
	grid->addWidget(licensesList, row, 0, 1, 4);

	row++;

	QPushButton *btn  = new QPushButton("Display text", this);
	connect(btn, &QPushButton::clicked, this, [this]() {
		display_license();
	});
	grid->addWidget(btn, row, 1, 1, 2);

	row++;

	mainLayout->addLayout(grid);
	mainLayout->addStretch(1);

	QPushButton *closeButton  = new QPushButton("Close", this);
	connect(closeButton, &QPushButton::clicked, this, [this]() {
		close();
	});
	mainLayout->addWidget(closeButton);

	setLayout(mainLayout);

	fill_packages_list();
	fill_licenses_list();
}



void SbomApiWindow::fill_packages_list(void)
{
	char list[8192];

	eris_get_list_of_packages(list, 8192);
	list[8191] = '\0';

	char *pack;
	char *rest = list;

	while ((pack = strtok_r(rest, " ", &rest)) != NULL) {
		packagesList->addItem(QString(pack));
	}

	if (packagesList->count() == 0)
		return;

	packagesList->setCurrentRow(0);
}



void SbomApiWindow::package_selected(void)
{
	char version[32];
	char licenses[1024];

	eris_get_package_version(packagesList->currentItem()->text().toLatin1().constData(), version, 32);
	eris_get_package_licenses(packagesList->currentItem()->text().toLatin1().constData(), licenses, 256);

	char line[2048];

	snprintf(line, 2048, "%s  version: %s\nLicenses: %s\n",
		packagesList->currentItem()->text().toLatin1().constData(), version, licenses);

	packageLabel->setText(line);

}



#define MAX_LICENSE_SIZE (512 * 1024)

void SbomApiWindow::display_license(void)
{
	if (licensesList->selectedItems().isEmpty())
		return;

	LicenseWindow *win;
	char *content;
	
	content = (char *)malloc(MAX_LICENSE_SIZE);
	if (content != NULL) {
		eris_get_license_text(licensesList->currentItem()->text().toLatin1().constData(), content, MAX_LICENSE_SIZE);
		win = new LicenseWindow(licensesList->currentItem()->text().toLatin1().constData(), content, this);
		free(content);
	} else {
		win = new LicenseWindow(licensesList->currentItem()->text().toLatin1().constData(), "Not enough memory to display the license.", this);
	}
	QSize screenSize = QGuiApplication::primaryScreen()->size();
	win->resize(screenSize);
	win->show();
}




void SbomApiWindow::fill_licenses_list(void)
{
	char list[8192];

	eris_get_list_of_licenses(list, 8192);
	list[8191] = '\0';

	char *lic;
	char *rest = list;

	while ((lic = strtok_r(rest, " ", &rest)) != NULL) {
		licensesList->addItem(QString(lic));
 	}

 	if (licensesList->count() == 0)
 		return;

	licensesList->setCurrentRow(0);
}
