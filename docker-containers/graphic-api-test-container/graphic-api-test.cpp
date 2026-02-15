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
#include <QVector>
#include <QWidget>

#include <liberis.h>

#include "virtual-keyboard.h"

#include "gpio-api-window.h"
#include "network-api-window.h"
#include "sbom-api-window.h"
#include "system-api-window.h"
#include "time-api-window.h"
#include "update-api-window.h"


class MainMenu : public QWidget {

	public:
		MainMenu(QWidget *parent = nullptr) : QWidget(parent) {

			QVBoxLayout *mainLayout = new QVBoxLayout(this);
			mainLayout->setContentsMargins(20, 20, 20, 20);
			mainLayout->setSpacing(20);

			QHBoxLayout *titleLayout = new QHBoxLayout();

			QLabel *logo = new QLabel();
			QPixmap pix("/logo.png");
			if (!pix.isNull()) {
				logo->setPixmap(pix.scaledToWidth(300, Qt::SmoothTransformation));
				logo->setAlignment(Qt::AlignCenter);
				titleLayout->addWidget(logo, 0, Qt::AlignCenter);
			}

			QLabel *title = new QLabel("Eris Linux API test", this);
			title->setAlignment(Qt::AlignCenter);
			title->setObjectName("TitleLabel");
			title->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
			titleLayout->addWidget(title, 0, Qt::AlignCenter);

			mainLayout->addLayout(titleLayout);

			mainLayout->addStretch(2);

			QHBoxLayout *buttonHLayout = new QHBoxLayout();
			buttonHLayout->setSpacing(30);

			QVBoxLayout *buttonVLayout = new QVBoxLayout();
			buttonVLayout->setSpacing(30);

			QPushButton *btn;

			btn = new QPushButton("System Identification", this);
			connect(btn, &QPushButton::clicked, this, [this]() { systemApiOpen(); });
			buttonVLayout->addWidget(btn);

			btn = new QPushButton("System & Containers Update", this);
			connect(btn, &QPushButton::clicked, this, [this]() { updateApiOpen(); });
			buttonVLayout->addWidget(btn);

			btn = new QPushButton("Time Setup", this);
			connect(btn, &QPushButton::clicked, this, [this]() { timeApiOpen(); });
			buttonVLayout->addWidget(btn);

			btn = new QPushButton("Watchdog (not implemented yet)", this);
			connect(btn, &QPushButton::clicked, this, [this]() { wdogApiOpen(); });
			buttonVLayout->addWidget(btn);

			btn = new QPushButton("Software Bill of Materials", this);
			connect(btn, &QPushButton::clicked, this, [this]() { sbomApiOpen(); });
			buttonVLayout->addWidget(btn);

			buttonHLayout->addLayout(buttonVLayout);

			buttonVLayout = new QVBoxLayout();
			buttonVLayout->setSpacing(30);

			btn = new QPushButton("Network Interfaces", this);
			connect(btn, &QPushButton::clicked, this, [this]() { networkApiOpen(); });
			buttonVLayout->addWidget(btn);

			btn = new QPushButton("General Puposes I/O", this);
			connect(btn, &QPushButton::clicked, this, [this]() { gpiosApiOpen(); });
			buttonVLayout->addWidget(btn);

			btn = new QPushButton("Display features (not implemented yet)", this);
			connect(btn, &QPushButton::clicked, this, [this]() { displayApiOpen(); });
			buttonVLayout->addWidget(btn);

			btn = new QPushButton("Audio features (not implemented yet)", this);
			connect(btn, &QPushButton::clicked, this, [this]() { audioApiOpen(); });
			buttonVLayout->addWidget(btn);

			buttonHLayout->addLayout(buttonVLayout);

			mainLayout->addLayout(buttonHLayout);
		}

	private:
		void audioApiOpen()
		{
		}

		void displayApiOpen()
		{
		}

		void gpiosApiOpen()
		{
			GpioApiWindow *win = new GpioApiWindow();
			QSize screenSize = QGuiApplication::primaryScreen()->size();
			win->resize(screenSize);
			win->show();
		}

		void networkApiOpen()
		{
			NetworkApiWindow *win = new NetworkApiWindow();
			QSize screenSize = QGuiApplication::primaryScreen()->size();
			win->resize(screenSize);
			win->show();
		}

		void sbomApiOpen()
		{
			SbomApiWindow *win = new SbomApiWindow();
			QSize screenSize = QGuiApplication::primaryScreen()->size();
			win->resize(screenSize);
			win->show();
		}

		void systemApiOpen()
		{
			SystemApiWindow *win = new SystemApiWindow();
			QSize screenSize = QGuiApplication::primaryScreen()->size();
			win->resize(screenSize);
			win->show();
		}

		void timeApiOpen()
		{
			TimeApiWindow *win = new TimeApiWindow();
			QSize screenSize = QGuiApplication::primaryScreen()->size();
			win->resize(screenSize);
			win->show();
		}

		void updateApiOpen()
		{
			UpdateApiWindow *win = new UpdateApiWindow();
			QSize screenSize = QGuiApplication::primaryScreen()->size();
			win->resize(screenSize);
			win->show();
		}

		void wdogApiOpen()
		{
		}
};


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	QFont font = app.font();
	font.setPointSize(font.pointSize() * 3);
	app.setFont(font);

	QFile file("/api-test.qss");
	if (file.open(QFile::ReadOnly | QFile::Text)) {
		QTextStream in(&file);
		QString style = in.readAll();
		app.setStyleSheet(style);
	} else {
		qWarning("Unable to load /api-test.qss");
	}

	MainMenu menu;

	QSize screenSize = QGuiApplication::primaryScreen()->size();
	menu.resize(screenSize);
	menu.show();

	return app.exec();
}
