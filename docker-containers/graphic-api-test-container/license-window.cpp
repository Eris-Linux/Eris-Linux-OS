/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QPlainTextEdit>
#include <QPushButton>

#include "license-window.h"


LicenseWindow::LicenseWindow(QString title, QString content, QWidget *parent): QDialog(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QLabel *titleLabel = new QLabel(title, this);
	titleLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(titleLabel);

	QPlainTextEdit *textEdit = new QPlainTextEdit(this);
	textEdit->setReadOnly(true);
	textEdit->setPlainText(content);
	mainLayout->addWidget(textEdit);

	QPushButton *closeButton = new QPushButton("Close", this);
	connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
	mainLayout->addWidget(closeButton);

	setLayout(mainLayout);
}