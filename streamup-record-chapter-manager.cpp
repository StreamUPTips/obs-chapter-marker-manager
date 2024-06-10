#include "streamup-record-chapter-manager.hpp"
#include "chapter-marker-dock.hpp"
#include "version.h"
#include <obs.h>
#include <obs-data.h>
#include <obs-encoder.h>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QMainWindow>
#include <QDockWidget>

#define QT_UTF8(str) QString::fromUtf8(str)
#define QT_TO_UTF8(str) str.toUtf8().constData()

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Andilippi");
OBS_MODULE_USE_DEFAULT_LOCALE("streamup-record-chapter-manager", "en-US")

//--------------------STRUCTS & GLOBALS--------------------

static bool add_chapter_marker(const char *chapter_name)
{
	if (!obs_frontend_recording_active()) {
		blog(LOG_WARNING,
		     "[StreamUP Record Chapter Manager] Recording is not active.");
		return false;
	}

	bool result = obs_frontend_recording_add_chapter(chapter_name);
	if (!result) {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Failed to add chapter marker. Ensure the output supports chapter markers.");
	} else {
		blog(LOG_INFO,
		     "[StreamUP Record Chapter Manager] Added chapter marker: %s",
		     chapter_name ? chapter_name : "Unnamed Chapter");
	}

	return result;
}

static void LoadChapterMarkerDock()
{
	const auto main_window =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);

	auto *dock_widget = new ChapterMarkerDock(main_window);

	const QString title =
		QString::fromUtf8(obs_module_text("StreamUP Chapter Marker"));
	const auto name = "ChapterMarkerDock";

#if LIBOBS_API_VER >= MAKE_SEMANTIC_VERSION(30, 0, 0)
	obs_frontend_add_dock_by_id(name, title.toUtf8().constData(),
				    dock_widget);
#else
	auto dock = new QDockWidget(main_window);
	dock->setObjectName(name);
	dock->setWindowTitle(title);
	dock->setWidget(dock_widget);
	dock->setFeatures(QDockWidget::DockWidgetMovable |
			  QDockWidget::DockWidgetFloatable);
	dock->setFloating(true);
	dock->hide();
	obs_frontend_add_dock(dock);
#endif

	obs_frontend_pop_ui_translation();
}

bool obs_module_load()
{
	blog(LOG_INFO, "[StreamUP Record Chapter Manager] loaded version %s",
	     PROJECT_VERSION);

	// Register a hotkey to add chapter markers
	obs_hotkey_id hotkey_id = obs_hotkey_register_frontend(
		"add_chapter_marker", "Add Chapter Marker",
		[](void *data, obs_hotkey_id id, obs_hotkey_t *hotkey,
		   bool pressed) {
			if (pressed) {
				add_chapter_marker("Unnamed Chapter");
			}
		},
		nullptr);

	// Create the dock widget
	LoadChapterMarkerDock();

	return true;
}

void obs_module_post_load(void) {}

void obs_module_unload()
{

}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("StreamUPRecordChapterManager");
}
