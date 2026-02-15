/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#ifndef UPDATE_API_WINDOW
#define UPDATE_API_WINDOW

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>


class UpdateApiWindow : public QWidget {
	public:
		explicit UpdateApiWindow(QWidget *parent = nullptr);

	private:

		QLabel      *systemUpdateStatusLabel;
		QLabel      *rebootNeededLabel;
		QPushButton *rebootNeededBtn;
		QLabel      *contactPeriodLabel;
		QPushButton *contactPeriodBtn;
		QPushButton *contactNowBtn;
		QCheckBox   *automaticRebootChk;
		QCheckBox   *immediateContainerUpdateChk;
		QTimer      *refreshTimer;

		void reboot_needed_button_clicked(void);
		void contact_period_button_clicked(void);
		void automatic_reboot_check_changed(void);
		void immediate_container_update_check_changed(void);
		void restore_factory_presets(void);
		void update_values(void);

		bool noupdate;
};


#endif
