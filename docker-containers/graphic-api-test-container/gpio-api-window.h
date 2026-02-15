/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#ifndef GPIO_API_WINDOW
#define GPIO_API_WINDOW

#include <QButtonGroup>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QTimer>
#include <QWidget>


class GpioApiWindow : public QWidget {
	public:
		explicit GpioApiWindow(QWidget *parent = nullptr);

	private:
		QListWidget *gpioLineList;

		QButtonGroup *directionButtonGroup;
		QRadioButton *directionNoneButton;
		QRadioButton *directionInputButton;
		QRadioButton *directionOutputButton;
		QLabel       *valueButtonLabel;
		QButtonGroup *valueButtonGroup;
		QRadioButton *value0Button;
		QRadioButton *value1Button;
		QLabel       *valueLabel;
		QTimer       *refreshTimer;

		void gpio_line_selected(void);
		void direction_selected(int direction);
		void value_selected(int value);
		void display_input_value(void);
		void fill_gpio_line_list(void);

		bool no_update;

		char *current_name;
};

#endif
