/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#ifndef LICENSEWINDOW_H
#define LICENSEWINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QString>
#include <QWidget>


class LicenseWindow : public QDialog {
	public:

		explicit LicenseWindow(QString title, QString content, QWidget *parent);

	private:

		QLineEdit *input;
};

#endif
