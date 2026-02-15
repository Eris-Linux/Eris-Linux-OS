/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#ifndef TIME_API_WINDOW
#define TIME_API_WINDOW

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>


class TimeApiWindow : public QWidget {
	public:
		explicit TimeApiWindow(QWidget *parent = nullptr);

	private:
		QLabel      *ntpEnabledLabel;
		QLabel      *ntpServerLabel;
		QPushButton *ntpServerButton;
		QLabel      *timeZoneLabel;
		QLabel      *localTimeLabel;
		QLabel      *systemTimeLabel;
		QTimer      *refreshTimer;

		void display_ntp_status(void);
		void display_ntp_server(void);
		void display_time_zone(void);
		void refresh_time_labels(void);
};

#endif
