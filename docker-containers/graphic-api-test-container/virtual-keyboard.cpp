/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#include <QDialog>
#include <QFile>
#include <QGuiApplication>
#include <QLabel>
#include <QLayout>
#include <QScreen>

#include "virtual-keyboard.h"


VirtualKeyboardDialog::VirtualKeyboardDialog(KeyboardType type, QString title, QString prompt, QString defaultStr, QWidget *parent): QDialog(parent), input(new QLineEdit(this))
{
	QFile f("/virtual-keyboard.qss");
	if (f.open(QFile::ReadOnly)) {
		setStyleSheet(f.readAll());
	}

	this->setObjectName("VirtualKeyboardDialog");

	QSize screenSize = QGuiApplication::primaryScreen()->size();
	int w = screenSize.width() * 0.9;
	int h = screenSize.height() * 0.9;

	QGridLayout *grid = new QGridLayout(this);

	QFrame *frame = new QFrame(this);
	frame->setObjectName("VirtualKeyboardFrame");
	frame->setFrameShape(QFrame::Box);

	frame->setMinimumSize(w, h);
	frame->setMaximumSize(w, h);
	frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	grid->addWidget(frame, 1,1);

	setLayout(grid);

	QVBoxLayout *layout = new QVBoxLayout(frame);

	QLabel *titleLabel = new QLabel(title, this);
	titleLabel->setAlignment(Qt::AlignCenter);
	titleLabel->setObjectName("TitleLabel");
	layout->addWidget(titleLabel);

	QLabel *promptLabel = new QLabel(prompt, this);
	promptLabel->setAlignment(Qt::AlignLeft);
	promptLabel->setObjectName("PromptLabel");
	layout->addWidget(promptLabel);

	input->setText(defaultStr);
	input->setReadOnly(true);
	layout->addWidget(input);

	grid = new QGridLayout(this);

	QPushButton *btn;

	int nb_cols;

	if ((type == VirtualKeyboardDialog::NumericDecimal)
	 || (type == VirtualKeyboardDialog::Numeric)) {

		int num = 1;
		for (int row = 0; row < 3; ++row) {
			for (int col = 0; col < 3; ++col) {
				btn = new QPushButton(QString::number(num));
				connect(btn, &QPushButton::clicked, [this, num]() {
					input->setText(input->text() + QString::number(num));
				});
				grid->addWidget(btn, row, col);
				num++;
			}
		}
		btn = new QPushButton(QString("<"));
		connect(btn, &QPushButton::clicked, [this, num]() {
			if (! input->text().isEmpty()) {
				QString str = input->text();
				str.chop(1);
				input->setText(str);
			}
		});
		grid->addWidget(btn, 3, 0);

		btn = new QPushButton(QString::number(0));
		connect(btn, &QPushButton::clicked, [this, num]() {
			input->setText(input->text() + QString::number(0));
		});
		grid->addWidget(btn, 3, 1);

		if (type == VirtualKeyboardDialog::NumericDecimal) {
			btn = new QPushButton(QString("."));
			connect(btn, &QPushButton::clicked, [this, num]() {
				input->setText(input->text() + QString("."));
			});
			grid->addWidget(btn, 3, 2);
		}
		btn = new QPushButton("OK", this);
		btn->setObjectName("OkButton");
		connect(btn, &QPushButton::clicked, this, &QDialog::accept);
		grid->addWidget(btn, 4, 0, 1, 2);

		btn = new QPushButton("Cancel", this);
		btn->setObjectName("CancelButton");
		connect(btn, &QPushButton::clicked, this, &QDialog::reject);
		grid->addWidget(btn, 4, 2);

		layout->addLayout(grid);

		return;

	}



	if (type == VirtualKeyboardDialog::Qwerty) {
		char key[5][12] = {
			{ '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+' },
			{ '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=' },
			{ 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}' },
			{ 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', ';', '"' },
			{ 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', ',', '.', '/' }
		};

		nb_cols = 12;

		for (int row = 0; row < 5; ++row) {
			for (int col = 0; col < nb_cols; ++col) {
				char c = key[row][col];
				btn = new QPushButton(QString(c));
				connect(btn, &QPushButton::clicked, [this, c]() {
					input->setText(input->text() + QString(c));
				});
				grid->addWidget(btn, row, col);
			}
		}
	} else if (type == VirtualKeyboardDialog::Azerty) {
		char key[5][12] = {
			{ '&', '"', '#', '{', '(', '-', '|', '_', '^', '@', ')', '}' },
			{ '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '+', '=' },
			{ 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '%', '}' },
			{ 'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '*', '!' },
			{ 'W', 'X', 'C', 'V', 'B', 'N', ',', '?', ';', '.', ':', '/' }
		};

		nb_cols = 12; 

		for (int row = 0; row < 5; ++row) {
			for (int col = 0; col < nb_cols; ++col) {
				char c = key[row][col];
				btn = new QPushButton(QString(c));
				connect(btn, &QPushButton::clicked, [this, c]() {
					input->setText(input->text() + QString(c));
				});
				grid->addWidget(btn, row, col);
			}
		}
	} else if (type == VirtualKeyboardDialog::Uri) {
		char key[5][11] = {
			{ '!', '@', '#', '$', '%', '|', '&', '*', '_', '+', '~' },
			{ '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-' },
			{ 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '=' },
			{ 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '?', '/' },
			{ 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '.', ',', ';', ':' }
		};

		nb_cols = 11;

		for (int row = 0; row < 5; ++row) {
			for (int col = 0; col < nb_cols; ++col) {
				char c = key[row][col];
				btn = new QPushButton(QString(c));
				connect(btn, &QPushButton::clicked, [this, c]() {
					input->setText(input->text() + QString(c));
				});
				grid->addWidget(btn, row, col);
			}
		}
	}

	btn = new QPushButton(QString("<"));
	connect(btn, &QPushButton::clicked, [this]() {
		if (! input->text().isEmpty()) {
			QString str = input->text();
			str.chop(1);
			input->setText(str);
		}
	});
	grid->addWidget(btn, 5, 0, 1, 2);

	btn = new QPushButton(QString(' '));
	connect(btn, &QPushButton::clicked, [this]() {
		input->setText(input->text() + QString(' '));
	});
	grid->addWidget(btn, 5, 2, 1, nb_cols - 6);

	btn = new QPushButton("OK", this);
	btn->setObjectName("OkButton");
	connect(btn, &QPushButton::clicked, this, &QDialog::accept);
	grid->addWidget(btn, 5, nb_cols - 4, 1, 2);

	btn = new QPushButton("Cancel", this);
	btn->setObjectName("CancelButton");
	connect(btn, &QPushButton::clicked, this, &QDialog::reject);
	grid->addWidget(btn, 5, nb_cols - 2, 1, 2);


	layout->addLayout(grid);
}



QString VirtualKeyboardDialog::getText(KeyboardType type, QString title, QString prompt, QString defaultStr, QWidget *parent)
{
	VirtualKeyboardDialog dlg(type, title, prompt, defaultStr, parent);

	if (dlg.exec() == QDialog::Accepted)
		return dlg.getValue();

	return {};
}
