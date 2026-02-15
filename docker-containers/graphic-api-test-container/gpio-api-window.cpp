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

#include "errno.h"
#include <iostream>

#include <liberis.h>

#include "gpio-api-window.h"


GpioApiWindow::GpioApiWindow(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QLabel *titleLabel = new QLabel("GPIO API", this);
	titleLabel->setAlignment(Qt::AlignCenter);
	titleLabel->setObjectName("TitleLabel");
	mainLayout->addWidget(titleLabel);

	mainLayout->addStretch(1);

	QGridLayout *grid = new QGridLayout(this);
	int row = 0;

	QLabel *label = new QLabel(
		"Select the GPIO line to work with:"
	);
	label->setAlignment(Qt::AlignLeft);
	grid->addWidget(label, row, 0, 1, 2);

	row++;

	gpioLineList = new QListWidget(this);
	connect(gpioLineList,&QListWidget::itemSelectionChanged, [&](){
		gpio_line_selected();
	});
	grid->addWidget(gpioLineList, row, 0, 1, 4);

	row++;

	label = new QLabel(
		"\nSelect the direction of this GPIO line:"
	);
	label->setAlignment(Qt::AlignLeft);
	grid->addWidget(label, row, 0, 1, 2);

	row++;

	directionButtonGroup  = new QButtonGroup(this);
	directionNoneButton    = new QRadioButton("Not reserved");
	directionInputButton  = new QRadioButton("Input");
	directionOutputButton = new QRadioButton("Output");
	directionButtonGroup->addButton(directionNoneButton,   0);
	directionButtonGroup->addButton(directionInputButton,  1);
	directionButtonGroup->addButton(directionOutputButton, 2);
	grid->addWidget(directionNoneButton, row, 1, 1, 2);
	grid->addWidget(directionInputButton, row, 2, 1, 2);
	grid->addWidget(directionOutputButton, row, 3, 1, 2);

	connect(directionButtonGroup, &QButtonGroup::idToggled, this, [this](int id, bool checked) {
		if (checked)
			 direction_selected(id);
	});

	row++;

	valueButtonLabel = new QLabel(
		"Select the value to output:"
	);
	valueButtonLabel->setAlignment(Qt::AlignLeft);
	grid->addWidget(valueButtonLabel, row, 0, 1, 2);

	row++;

	valueButtonGroup = new QButtonGroup(this);
	value0Button = new QRadioButton("0");
	value1Button = new QRadioButton("1");
	valueButtonGroup->addButton(value0Button, 0);
	valueButtonGroup->addButton(value1Button, 1);
	grid->addWidget(value0Button, row, 1, 1, 1);
	grid->addWidget(value1Button, row, 2, 1, 1);

	value0Button->hide();
	value1Button->hide();

	connect(valueButtonGroup, &QButtonGroup::idToggled, this, [this](int id, bool checked) {
		if (checked)
			 value_selected(id);
	});

	valueLabel = new QLabel("0/1");
	valueLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	grid->addWidget(valueLabel, row, 1, 1, 2);
	valueLabel->hide();

	row++;

	mainLayout->addLayout(grid);
	mainLayout->addStretch(1);

	QPushButton *closeButton  = new QPushButton("Close", this);
	connect(closeButton, &QPushButton::clicked, this, [this]() {
		refreshTimer->stop();
		if (current_name != NULL) {
			eris_release_gpio(current_name);
			free(current_name);
			current_name = NULL;
		}
		close();
	});
	mainLayout->addWidget(closeButton);

	refreshTimer = new QTimer(this);
	connect(refreshTimer, &QTimer::timeout, this, [this]() {
		this->display_input_value();
	});
	refreshTimer->start(300);

	current_name = NULL;
	fill_gpio_line_list();
	no_update = false;
}



void GpioApiWindow::gpio_line_selected(void)
{
	if (current_name != NULL) {
		eris_release_gpio(current_name);
		free(current_name);
		current_name = NULL;
	}

	int direction = 0;
	no_update = true;
	directionButtonGroup->button(direction)->setChecked(true);
	no_update = false;
}


void GpioApiWindow::direction_selected(int direction)
{
	if ((direction == 2) && (! no_update)) {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Output", "Are you sure you want to turn this GPIO to output?\n(This may be dangerous for your board)" , QMessageBox::Yes|QMessageBox::No);
		if (reply != QMessageBox::Yes) {
			directionButtonGroup->button(0)->setChecked(true);
			return;
		}
	}
	if (current_name != NULL) {
		free(current_name);
		current_name = NULL;
	}

	if (direction == 0) {
		value0Button->hide();
		value1Button->hide();
		valueLabel->hide();
	} else if (direction == 1) {
		valueButtonLabel->setText("Value read on input:");
		value0Button->hide();
		value1Button->hide();
		valueLabel->show();
		display_input_value();

	} if (direction == 2) {
		current_name = (char *)malloc(gpioLineList->currentItem()->text().length() + 1);
		if (current_name != NULL)
			strcpy(current_name, gpioLineList->currentItem()->text().toLatin1().constData());
		valueButtonLabel->setText("Select the value to output:");
		value0Button->show();
		value1Button->show();
		valueLabel->hide();
		valueButtonGroup->button(0)->setChecked(true);
	}

	if (direction == 2) {
		if (eris_request_gpio_for_output(current_name, 0) != 0) {
			free(current_name);
			current_name = NULL;
		}
	}
}


void GpioApiWindow::value_selected(int value)
{
	if ((! no_update) && (current_name != NULL)) {
		eris_write_gpio_value(current_name, value);
	}
}


void GpioApiWindow::display_input_value(void)
{
	if (! valueLabel->isVisible())
		return;

	if (eris_request_gpio_for_input(gpioLineList->currentItem()->text().toLatin1().constData()) == 0) {
		int val = eris_read_gpio_value(gpioLineList->currentItem()->text().toLatin1().constData());
		if (val == 1)
			valueLabel->setText("1");
		else if (val == 0)
			valueLabel->setText("0");
		else  {
			valueLabel->setText("?");
		}
		eris_release_gpio(gpioLineList->currentItem()->text().toLatin1().constData());
	} else {
			valueLabel->setText("???");
	}
}


void GpioApiWindow::fill_gpio_line_list(void)
{
	char list[8192];

	eris_get_list_of_gpio(list, 8192);
	list[8191] = '\0';

	char *gpio;
	char *rest = list;

	while ((gpio = strtok_r(rest, " ", &rest)) != NULL) {
		gpioLineList->addItem(QString(gpio));
 	}

 	if (gpioLineList->count() == 0)
 		return;

	gpioLineList->setCurrentRow(0);
}
