/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#ifndef SYSTEM_API_WINDOW
#define SYSTEM_API_WINDOW

#include <QLabel>
#include <QWidget>

class SystemApiWindow : public QWidget {
	public:
		explicit SystemApiWindow(QWidget *parent = nullptr);

	private:
		QLabel *systemTypeLabel;
		QLabel *systemModelLabel;
		QLabel *systemVersionLabel;
		QLabel *machineUuidLabel;
		QLabel *containersLabel;

		QLabel *rebootLabel;
		QLabel *shutdownLabel;

		void  display_system_labels(void);
};

#endif
