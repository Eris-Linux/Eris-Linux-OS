/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#ifndef SBOM_API_WINDOW
#define SBOM_API_WINDOW

#include <QButtonGroup>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QTimer>
#include <QWidget>


class SbomApiWindow : public QWidget {
	public:
		explicit SbomApiWindow(QWidget *parent = nullptr);

	private:
		QListWidget  *packagesList;
		QLabel       *packageLabel;
		QListWidget  *licensesList;

		void package_selected(void);
		void display_license(void);
		void fill_packages_list(void);
		void fill_licenses_list(void);

};

#endif
