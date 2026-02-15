/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#ifndef WATCHDOG_API_WINDOW
#define WATCHDOG_API_WINDOW

#include <QLabel>
#include <QWidget>


class WatchdogApiWindow : public QWidget {
	public:
		explicit WatchdogApiWindow(QWidget *parent = nullptr);

	private:
		QLabel *watchdogGetDelayLabel;
		QLabel *watchdogSetDelayLabel;
		QLabel *watchdogFeedLabel;
		QLabel *disableWatchdogLabel;
};

#endif
