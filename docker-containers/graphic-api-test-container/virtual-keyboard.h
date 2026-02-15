/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#ifndef VIRTUALKEYBOARDDIALOG_H
#define VIRTUALKEYBOARDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QString>
#include <QWidget>


class VirtualKeyboardDialog : public QDialog {
	public:

		enum KeyboardType {
			Numeric,           // [0-9] keys only
			NumericDecimal,    // [0-9] and decimal point
			Alphabetic,        // ABCDEF... layout (+ digits)
			Qwerty,            // Keyboard layout used in most of the world.
			Qwertz,            // Used in Germany, Belgium, Switzerland...
			Azerty,            // Used in France and french speaking countries.
			Uri                // Alpha + digits + . ~ - _ : / ? # @ ! $ & + , ; = %
		};

		explicit VirtualKeyboardDialog(KeyboardType type, QString title, QString prompt, QString defaultStr, QWidget *parent);

		QString getValue() const {
			return input->text();
		}

		static QString getText(
			KeyboardType type,
			QString title,
			QString prompt,
			QString defaultStr = "",
			QWidget *parent = nullptr);

	private:

		QLineEdit *input;
};

#endif
