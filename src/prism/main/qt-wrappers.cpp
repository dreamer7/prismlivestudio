/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "qt-wrappers.hpp"
#include "pls-app.hpp"

#include <graphics/graphics.h>
#include <util/threading.h>
#include <QWidget>
#include <QLayout>
#include <QDataStream>
#include <QKeyEvent>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <QX11Info>
#endif

static inline void PLSErrorBoxva(QWidget *parent, const char *msg, va_list args)
{
	char full_message[4096];
	vsnprintf(full_message, 4095, msg, args);

	PLSAlertView::critical(parent, "Error", full_message);
}

void PLSErrorBox(QWidget *parent, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	PLSErrorBoxva(parent, msg, args);
	va_end(args);
}

PLSAlertView::Button PLSMessageBox::question(QWidget *parent, const QString &title, const QString &text, PLSAlertView::Buttons buttons, PLSAlertView::Button defaultButton)
{
	QMap<PLSAlertView::Button, QString> buttonMap;
#define translate_button(x)                                           \
	if (buttons & PLSAlertView::Button::x) {                      \
		buttonMap.insert(PLSAlertView::Button::x, QTStr(#x)); \
	}
	translate_button(Open);
	translate_button(Save);
	translate_button(Cancel);
	translate_button(Close);
	translate_button(Discard);
	translate_button(Apply);
	translate_button(Reset);
	translate_button(Yes);
	translate_button(No);
	translate_button(No);
	translate_button(Abort);
	translate_button(Retry);
	translate_button(Ignore);
#undef translate_button
	if (buttons & PLSAlertView::Button::Ok) {
		buttonMap.insert(PLSAlertView::Button::Ok, QTStr("OK"));
	}
	return PLSAlertView::question(parent, title, text, buttonMap, defaultButton);
}

PLSAlertView::Button PLSMessageBox::question(QWidget *parent, const QString &title, const QString &textTitle, const QString &textContent,
					     PLSAlertView::Buttons buttons /*= PLSAlertView::Buttons(PLSAlertView::Button::Yes | PLSAlertView::Button::No)*/,
					     PLSAlertView::Button defaultButton /*= PLSAlertView::Button::NoButton*/)
{
	return PLSAlertView::question(title, textTitle, textContent, parent, buttons, defaultButton);
}

void PLSMessageBox::information(QWidget *parent, const QString &title, const QString &text)
{
	PLSAlertView::information(parent, title, text, {{PLSAlertView::Button::Ok, QTStr("OK")}}, PLSAlertView::Button::Ok);
}

void PLSMessageBox::warning(QWidget *parent, const QString &title, const QString &text, bool enableRichText)
{
	PLSAlertView alertView(parent, PLSAlertView::Warning, title, text, QString(), {{PLSAlertView::Button::Ok, QTStr("OK")}}, PLSAlertView::Button::Ok);
	if (enableRichText) {
		alertView.setTextFormat(Qt::RichText);
	}
	alertView.exec();
}

void PLSMessageBox::critical(QWidget *parent, const QString &title, const QString &text)
{
	PLSAlertView::critical(parent, title, text, {{PLSAlertView::Button::Ok, QTStr("OK")}}, PLSAlertView::Button::Ok);
}

void QTToGSWindow(WId windowId, gs_window &gswindow)
{
#ifdef _WIN32
	gswindow.hwnd = (HWND)windowId;
#elif __APPLE__
	gswindow.view = (id)windowId;
#else
	gswindow.id = windowId;
	gswindow.display = QX11Info::display();
#endif
}

uint32_t TranslateQtKeyboardEventModifiers(Qt::KeyboardModifiers mods)
{
	int obsModifiers = INTERACT_NONE;

	if (mods.testFlag(Qt::ShiftModifier))
		obsModifiers |= INTERACT_SHIFT_KEY;
	if (mods.testFlag(Qt::AltModifier))
		obsModifiers |= INTERACT_ALT_KEY;
#ifdef __APPLE__
	// Mac: Meta = Control, Control = Command
	if (mods.testFlag(Qt::ControlModifier))
		obsModifiers |= INTERACT_COMMAND_KEY;
	if (mods.testFlag(Qt::MetaModifier))
		obsModifiers |= INTERACT_CONTROL_KEY;
#else
	// Handle windows key? Can a browser even trap that key?
	if (mods.testFlag(Qt::ControlModifier))
		obsModifiers |= INTERACT_CONTROL_KEY;
	if (mods.testFlag(Qt::MetaModifier))
		obsModifiers |= INTERACT_COMMAND_KEY;

#endif

	return obsModifiers;
}

QDataStream &operator<<(QDataStream &out, const std::vector<std::shared_ptr<OBSSignal>> &)
{
	return out;
}

QDataStream &operator>>(QDataStream &in, std::vector<std::shared_ptr<OBSSignal>> &)
{
	return in;
}

QDataStream &operator<<(QDataStream &out, const OBSScene &scene)
{
	return out << QString(obs_source_get_name(obs_scene_get_source(scene)));
}

QDataStream &operator>>(QDataStream &in, OBSScene &scene)
{
	QString sceneName;

	in >> sceneName;

	obs_source_t *source = obs_get_source_by_name(QT_TO_UTF8(sceneName));
	scene = obs_scene_from_source(source);

	return in;
}

QDataStream &operator<<(QDataStream &out, const OBSSceneItem &si)
{
	obs_scene_t *scene = obs_sceneitem_get_scene(si);
	obs_source_t *source = obs_sceneitem_get_source(si);
	return out << QString(obs_source_get_name(obs_scene_get_source(scene))) << QString(obs_source_get_name(source));
}

QDataStream &operator>>(QDataStream &in, OBSSceneItem &si)
{
	QString sceneName;
	QString sourceName;

	in >> sceneName >> sourceName;

	obs_source_t *sceneSource = obs_get_source_by_name(QT_TO_UTF8(sceneName));

	obs_scene_t *scene = obs_scene_from_source(sceneSource);
	si = obs_scene_find_source(scene, QT_TO_UTF8(sourceName));

	obs_source_release(sceneSource);

	return in;
}

void DeleteLayout(QLayout *layout)
{
	if (!layout)
		return;

	for (;;) {
		QLayoutItem *item = layout->takeAt(0);
		if (!item)
			break;

		QLayout *subLayout = item->layout();
		if (subLayout) {
			DeleteLayout(subLayout);
		} else {
			delete item->widget();
			delete item;
		}
	}

	delete layout;
}

class QuickThread : public QThread {
public:
	explicit inline QuickThread(std::function<void()> func_) : func(func_) {}

private:
	virtual void run() override { func(); }

	std::function<void()> func;
};

QThread *CreateQThread(std::function<void()> func)
{
	return new QuickThread(func);
}

volatile long insideEventLoop = 0;

void ExecuteFuncSafeBlock(std::function<void()> func)
{
	QEventLoop eventLoop;

	auto wait = [&]() {
		func();
		QMetaObject::invokeMethod(&eventLoop, "quit", Qt::QueuedConnection);
	};

	os_atomic_inc_long(&insideEventLoop);
	QScopedPointer<QThread> thread(CreateQThread(wait));
	thread->start();
	eventLoop.exec();
	thread->wait();
	os_atomic_dec_long(&insideEventLoop);
}

void ExecuteFuncSafeBlockMsgBox(std::function<void()> func, const QString &title, const QString &text)
{
	PLSAlertView alertView(nullptr, PLSAlertView::Icon::NoIcon, title, text, QString(), PLSAlertView::Button::NoButton);
	alertView.setHasCloseButton(false);

	auto wait = [&]() {
		func();
		QMetaObject::invokeMethod(&alertView, "accept", Qt::QueuedConnection);
	};

	os_atomic_inc_long(&insideEventLoop);
	QScopedPointer<QThread> thread(CreateQThread(wait));
	thread->start();
	alertView.exec();
	thread->wait();
	os_atomic_dec_long(&insideEventLoop);
}

static bool enable_message_boxes = false;

void EnableThreadedMessageBoxes(bool enable)
{
	enable_message_boxes = enable;
}

void ExecThreadedWithoutBlocking(std::function<void()> func, const QString &title, const QString &text)
{
	if (!enable_message_boxes)
		ExecuteFuncSafeBlock(func);
	else
		ExecuteFuncSafeBlockMsgBox(func, title, text);
}

bool LineEditCanceled(QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = reinterpret_cast<QKeyEvent *>(event);
		return keyEvent->key() == Qt::Key_Escape;
	}

	return false;
}

bool LineEditChanged(QEvent *event)
{
	if (event->type() == QEvent::KeyRelease) {
		QKeyEvent *keyEvent = reinterpret_cast<QKeyEvent *>(event);

		switch (keyEvent->key()) {
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
		case Qt::Key_Enter:
		case Qt::Key_Return:
			return true;
		}
	} else if (event->type() == QEvent::FocusOut) {
		return true;
	}

	return false;
}
