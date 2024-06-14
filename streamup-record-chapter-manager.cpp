#include "streamup-record-chapter-manager.hpp"
#include "annotation-dock.hpp"
#include "chapter-marker-dock.hpp"
#include "version.h"
#include <obs.h>
#include <obs-data.h>
#include <obs-encoder.h>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QMainWindow>
#include <QDockWidget>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>
#include <util/platform.h>

#define QT_UTF8(str) QString::fromUtf8(str)
#define QT_TO_UTF8(str) str.toUtf8().constData()

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Andilippi");
OBS_MODULE_USE_DEFAULT_LOCALE("streamup-record-chapter-manager", "en-US")

static ChapterMarkerDock *chapterMarkerDock = nullptr;

static void LoadChapterMarkerDock()
{
	const auto main_window =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);

	if (!chapterMarkerDock) {
		chapterMarkerDock = new ChapterMarkerDock(main_window);

		const QString title = QString::fromUtf8(
			obs_module_text("StreamUP Chapter Marker"));
		const auto name = "ChapterMarkerDock";

#if LIBOBS_API_VER >= MAKE_SEMANTIC_VERSION(30, 0, 0)
		obs_frontend_add_dock_by_id(name, QT_TO_UTF8(title),
					    chapterMarkerDock);
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
}

static void OnStartRecording()
{
	if (!chapterMarkerDock ||
	    !chapterMarkerDock->exportChaptersToFileEnabled) {
		return;
	}

	chapterMarkerDock->createExportFiles();

	QString timestamp = chapterMarkerDock->getCurrentRecordingTime();
	chapterMarkerDock->addChapterMarker("Start", "Recording");
}

static void FrontEndEventHandler(enum obs_frontend_event event, void *)
{
	switch (event) {
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		if (chapterMarkerDock) {
			chapterMarkerDock->updateCurrentChapterLabel("Start");
			OnStartRecording();
		}
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		if (chapterMarkerDock) {
			chapterMarkerDock->showFeedbackMessage(
				"Recording is not active. Chapter marker not added.",
				true);
			;
			chapterMarkerDock->showFeedbackMessage("Recording finished.",
							 false);
		}
		break;
	default:
		break;
	}
}

//--------------------HOTKEY HANDLERS--------------------
obs_hotkey_id addDefaultChapterMarkerHotkey = OBS_INVALID_HOTKEY_ID;

static void SaveLoadHotkeys(obs_data_t *save_data, bool saving, void *)
{
	if (saving) {
		obs_data_array_t *hotkey_save_array =
			obs_hotkey_save(addDefaultChapterMarkerHotkey);
		obs_data_set_array(save_data, "addDefaultChapterMarkerHotkey",
				   hotkey_save_array);
		obs_data_array_release(hotkey_save_array);
	} else {
		obs_data_array_t *hotkey_save_array = obs_data_get_array(
			save_data, "addDefaultChapterMarkerHotkey");
		obs_hotkey_load(addDefaultChapterMarkerHotkey,
				hotkey_save_array);
		obs_data_array_release(hotkey_save_array);
	}
}

void AddDefaultChapterMarkerHotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey,
		       bool pressed)
{
	if (!pressed)
		return;
	if (!obs_frontend_recording_active()) {
		chapterMarkerDock->showFeedbackMessage(
			"Recording is not active. Chapter marker not added.",
			true);
		chapterMarkerDock->showFeedbackMessage(
			"Recording is not active. Chapter marker not added.",
			true);
		return;
	}

	QString chapterName = chapterMarkerDock->defaultChapterName + " " +
			      QString::number(chapterMarkerDock->chapterCount);
	chapterMarkerDock->addChapterMarker(chapterName, "Hotkey");
	blog(LOG_INFO, "[StreamUP Record Chapter Manager] chapterCount: %d",
	     chapterMarkerDock->chapterCount);

	chapterMarkerDock->chapterCount++; // Increment the chapter count
}

//--------------------MENU HELPERS--------------------
obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving)
{
	char *configPath = obs_module_config_path("configs.json");
	obs_data_t *data = nullptr;

	if (saving) {
		if (obs_data_save_json(save_data, configPath)) {
			blog(LOG_INFO,
			     "[StreamUP Record Chapter Manager] Settings saved to %s",
			     configPath);
		} else {
			blog(LOG_WARNING,
			     "[StreamUP Record Chapter Manager] Failed to save settings to file.");
		}
	} else {
		data = obs_data_create_from_json_file(configPath);

		if (!data) {
			blog(LOG_INFO,
			     "[StreamUP Record Chapter Manager] Settings not found. Creating default settings...");

			char *dirPath = obs_module_config_path("");
			os_mkdirs(dirPath);
			bfree(dirPath);

			data = obs_data_create();
			obs_data_set_string(data, "default_chapter_name",
					    "Chapter");
			obs_data_set_bool(data, "export_chapters_enabled",
					  false);

			if (obs_data_save_json(data, configPath)) {
				blog(LOG_INFO,
				     "[StreamUP Record Chapter Manager] Default settings saved to %s",
				     configPath);
			} else {
				blog(LOG_WARNING,
				     "[StreamUP Record Chapter Manager] Failed to save default settings to file.");
			}

			obs_data_release(data);
			data = obs_data_create_from_json_file(configPath);
		} else {
			blog(LOG_INFO,
			     "[StreamUP Record Chapter Manager] Settings loaded successfully from %s",
			     configPath);
		}
	}

	bfree(configPath);
	return data;
}

//--------------------STARTUP COMMANDS--------------------
static void RegisterHotkeys()
{
	addDefaultChapterMarkerHotkey = obs_hotkey_register_frontend(
		"addDefaultChapterMarker",
		obs_module_text("AddDefaultChapterMarker"), AddDefaultChapterMarkerHotkey,
		chapterMarkerDock);
}

bool obs_module_load()
{
	blog(LOG_INFO, "[StreamUP Record Chapter Manager] loaded version %s",
	     PROJECT_VERSION);

	RegisterHotkeys();
	obs_frontend_add_save_callback(SaveLoadHotkeys, nullptr);
	obs_frontend_add_event_callback(FrontEndEventHandler, nullptr);

	LoadChapterMarkerDock();

	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);

	if (settings) {
		chapterMarkerDock->applySettings(settings);
		obs_data_release(settings);
	}


	return true;
}

void obs_module_post_load(void)
{
	chapterMarkerDock->onAnnotationClicked(true);
}

//--------------------EXIT COMMANDS--------------------
void obs_module_unload()
{
	obs_frontend_remove_event_callback(FrontEndEventHandler, nullptr);

	obs_frontend_remove_save_callback(SaveLoadHotkeys, nullptr);
	obs_hotkey_unregister(addDefaultChapterMarkerHotkey);
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("StreamUPRecordChapterManager");
}
